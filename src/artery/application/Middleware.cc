//
// Copyright (C) 2014 Raphael Riebl <raphael.riebl@thi.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "artery/application/Middleware.h"
#include "artery/application/RadioModule.h"
#include "artery/application/ItsG5PromiscuousService.h"
#include "artery/application/ItsG5Service.h"
#include "artery/messages/GeoNetPacket_m.h"
#include "artery/netw/GeoNetIndication.h"
#include "artery/netw/GeoNetRequest.h"
#include "artery/nic/RadioDriverBase.h"
#include "artery/utility/IdentityRegistry.h"
#include "artery/utility/FilterRules.h"
#include "inet/common/ModuleAccess.h"
#include <vanetza/btp/header.hpp>
#include <vanetza/btp/header_conversion.hpp>
#include <vanetza/btp/ports.hpp>
#include <vanetza/geonet/data_confirm.hpp>
#include <vanetza/geonet/packet.hpp>
#include <vanetza/geonet/position_vector.hpp>
#include <vanetza/security/naive_certificate_provider.hpp>
#include <vanetza/security/null_certificate_provider.hpp>
#include <vanetza/security/null_certificate_validator.hpp>
#include <vanetza/net/mac_address.hpp>
#include <algorithm>
#include <chrono>
#include <string>

namespace artery
{

Define_Module(Middleware)

Middleware::Middleware() : mLocalDynamicMap(mTimer)
{
}

void Middleware::request(const vanetza::btp::DataRequestB& req, std::unique_ptr<vanetza::DownPacket> payload)
{
	Enter_Method("request");
	using namespace vanetza;
	btp::HeaderB btp_header;
	btp_header.destination_port = req.destination_port;
	btp_header.destination_port_info = req.destination_port_info;
	payload->layer(OsiLayer::Transport) = btp_header;

	geonet::DataConfirm confirm;
	switch (req.gn.transport_type) {
		case geonet::TransportType::SHB: {
			geonet::ShbDataRequest request(mGeoMib);
			copy_request_parameters(req, request);
			auto channel = mMcoStrategy->choose(request);
			confirm = mGeoRouter.at(channel)->request(request, std::move(payload));
		}
			break;
		case geonet::TransportType::GBC: {
			geonet::GbcDataRequest request(mGeoMib);
			copy_request_parameters(req, request);
			auto channel = mMcoStrategy->choose(request);
			confirm = mGeoRouter.at(channel)->request(request, std::move(payload));
		}
			break;
		default:
			throw cRuntimeError("Unknown or unimplemented transport type");
			break;
	}

	if (confirm.rejected()) {
		throw cRuntimeError("GN-Data.request rejected");
	}

	scheduleRuntime();
}

int Middleware::numInitStages() const
{
	return 3;
}

void Middleware::initialize(int stage)
{
	switch (stage) {
		case 0:
			initializeMiddleware();
			break;
		case 1:
			initializeServices();
			break;
		case 2: {
				// start update cycle with random jitter to avoid unrealistic node synchronization
				const auto jitter = uniform(SimTime(0, SIMTIME_MS), mUpdateInterval);
				scheduleAt(simTime() + jitter + mUpdateInterval, mUpdateMessage);
				scheduleRuntime();
			}
			break;
		default:
			break;
	}
}

void Middleware::initializeRadioModules()
{
	auto numRadios = par("numRadios").longValue();
	auto parent = getParentModule();
	for (auto i = 0; i < numRadios; i++) {
		auto radioModule = static_cast<RadioModule*>(getParentModule()->getSubmodule("radioModule", i));
		radioModule->initialize(&mRuntime, this);
		mRadioManager->registerRadioModule(radioModule);
	}
}

void Middleware::initializeGeoNetworking()
{
	using vanetza::geonet::UpperProtocol;
	vanetza::geonet::Address gn_addr;
	mGnStationType = vanetza::geonet::StationType::UNKNOWN;
	gn_addr.is_manually_configured(true);
	gn_addr.station_type(mGnStationType);
	gn_addr.country_code(0);

	auto radioModule = mRadioManager->getRadioModule(vanetza::channel::CCH);
	gn_addr.mid(radioModule->getMacAddress());

	auto access_ifcs = mRadioManager->getInterfaces();

	for (auto ifc : access_ifcs) {
		GeoRouter geoRouter;
		geoRouter.reset(new vanetza::geonet::Router {mRuntime, mGeoMib});
		geoRouter->set_address(gn_addr);

		geoRouter->set_transport_handler(UpperProtocol::BTP_B, &mBtpPortDispatcher);
		geoRouter->set_security_entity(mSecurityEntity.get());

		geoRouter->set_access_interface(ifc);

		mGeoRouter.insert({ifc->getChannel(), std::move(geoRouter)});
	}
}

void Middleware::initializeNetworking()
{
	mRadioManager = static_cast<RadioManager*>(getParentModule()->getSubmodule("radioManager"));
	mFacilities.register_const(mRadioManager);

	initializeRadioModules();

	mMcoStrategy = static_cast<application::McoStrategy*>(getParentModule()->getSubmodule("mcoStrategy"));

	initializeGeoNetworking();
}

void Middleware::initializeMiddleware()
{
	mTimer.setTimebase(par("datetime"));

	mFacilities.register_const(&mTimer);
	mFacilities.register_mutable(&mLocalDynamicMap);

	mRuntime.reset(mTimer.getCurrentTime());

	mUpdateInterval = par("updateInterval").doubleValue();
	mUpdateMessage = new cMessage("middleware update");
	mUpdateRuntimeMessage = new cMessage("runtime update");
	initializeSecurity();

	initializeNetworking();

	initializeManagementInformationBase(mGeoMib);

	initializeIdentity(mIdentity);
	emit(artery::IdentityRegistry::updateSignal, &mIdentity);
}

void Middleware::initializeIdentity(Identity& id)
{
	id.geonet = mGeoRouter.at(channel::CCH)->get_local_position_vector().gn_addr;
}

void Middleware::initializeManagementInformationBase(vanetza::geonet::MIB& mib)
{
	mib.itsGnDefaultTrafficClass.tc_id(3); // send BEACONs with DP3
	mib.itsGnSecurity = par("vanetzaEnableSecurity").boolValue();
}

void Middleware::initializeSecurity()
{
	using namespace vanetza::security;
	const char* vanetzaCryptoBackend = par("vanetzaCryptoBackend");
	mSecurityBackend = create_backend(vanetzaCryptoBackend);
	if (!mSecurityBackend) {
		throw cRuntimeError("No security backend found with name \"%s\"", vanetzaCryptoBackend);
	}

	const std::string vanetzaCertificateProvider = par("vanetzaCertificateProvider");
	if (vanetzaCertificateProvider == "Null") {
		mSecurityCertificates.reset(new NullCertificateProvider());
	} else if (vanetzaCertificateProvider == "Naive") {
		mSecurityCertificates.reset(new NaiveCertificateProvider(mRuntime.now()));
	} else {
		throw cRuntimeError("No certificate provider available with name \"%s\"",
			vanetzaCertificateProvider.c_str());
	}

	const std::string vanetzaCertificateValidator = par("vanetzaCertificateValidator");
	if (vanetzaCertificateValidator == "Null") {
		mSecurityCertificateValidator.reset(new NullCertificateValidator());
	} else if (vanetzaCertificateValidator == "NullOk") {
		std::unique_ptr<NullCertificateValidator> validator { new NullCertificateValidator() };
		static const CertificateValidity ok;
		ASSERT(ok);
		validator->certificate_check_result(ok);
		mSecurityCertificateValidator = std::move(validator);
	} else {
		throw cRuntimeError("No certificate validator available with name \"%s\"",
			vanetzaCertificateValidator.c_str());
	}

	mSecurityCertificateCache.reset(new CertificateCache(mRuntime));
	mSecuritySignHeaderPolicy.reset(new DefaultSignHeaderPolicy(mRuntime.now(), mPositionProvider));

	SignService sign_service;
	const std::string vanetzaSecuritySignService = par("vanetzaSecuritySignService");
	if (vanetzaSecuritySignService == "straight") {
		sign_service = straight_sign_service(*mSecurityCertificates, *mSecurityBackend, *mSecuritySignHeaderPolicy);
	} else if (vanetzaSecuritySignService == "deferred") {
		sign_service = deferred_sign_service(*mSecurityCertificates, *mSecurityBackend, *mSecuritySignHeaderPolicy);
	} else if (vanetzaSecuritySignService == "dummy") {
		sign_service = dummy_sign_service(mRuntime, NullCertificateProvider::null_certificate());
	} else {
		throw cRuntimeError("No security sign service available with name \"%s\"",
			vanetzaSecuritySignService.c_str());
	}

	VerifyService verify_service;
	const std::string vanetzaSecurityVerifyService = par("vanetzaSecurityVerifyService");
	if (vanetzaSecurityVerifyService == "straight") {
		verify_service = straight_verify_service(mRuntime, *mSecurityCertificates, *mSecurityCertificateValidator,
			*mSecurityBackend, *mSecurityCertificateCache, *mSecuritySignHeaderPolicy, mPositionProvider);
	} else if (vanetzaSecurityVerifyService == "dummy") {
		verify_service = dummy_verify_service(VerificationReport::Success, CertificateValidity::valid());
	} else {
		throw cRuntimeError("No security verify service available with name \"%s\"",
			vanetzaSecurityVerifyService.c_str());
	}

	mSecurityEntity.reset(new SecurityEntity(sign_service, verify_service));
}

void Middleware::initializeServices()
{
	cXMLElement* config = par("services").xmlValue();
	for (cXMLElement* service_cfg : config->getChildrenByTagName("service")) {
		cModuleType* module_type = cModuleType::get(service_cfg->getAttribute("type"));
		const char* service_name = service_cfg->getAttribute("name") ?
				service_cfg->getAttribute("name") :
				service_cfg->getAttribute("type");

		cXMLElement* service_filters = service_cfg->getFirstChildWithTag("filters");
		bool service_applicable = true;
		if (service_filters) {
			artery::FilterRules rules(getRNG(0), mIdentity);
			service_applicable = rules.applyFilterConfig(*service_filters);
		}

		if (service_applicable) {
			cModule* module = module_type->createScheduleInit(service_name, this);
			ItsG5BaseService* service = dynamic_cast<ItsG5BaseService*>(module);

			if (service) {
				cXMLElement* listener = service_cfg->getFirstChildWithTag("listener");
				if (listener && listener->getAttribute("port")) {
					port_type port = boost::lexical_cast<port_type>(listener->getAttribute("port"));
					mServices.emplace(service, port);
					mBtpPortDispatcher.set_non_interactive_handler(vanetza::host_cast<port_type>(port), service);
				} else if (!service->requiresListener()) {
					mServices.emplace(service, 0);
				} else {
					auto promiscuous = dynamic_cast<ItsG5PromiscuousService*>(service);
					if (promiscuous != nullptr) {
						mServices.emplace(service, 0);
						mBtpPortDispatcher.add_promiscuous_hook(promiscuous);
					} else {
						throw cRuntimeError("No listener port defined for %s", service_name);
					}
				}
			} else {
				throw cRuntimeError("%s is not of type ItsG5BaseService", module_type->getFullName());
			}
		}
	}
}

void Middleware::finish()
{
	cancelAndDelete(mUpdateMessage);
	cancelAndDelete(mUpdateRuntimeMessage);
	emit(artery::IdentityRegistry::removeSignal, &mIdentity);
}

void Middleware::handleMessage(cMessage *msg)
{
	// Update clock before anything else is executed (which might read the clock)
	mRuntime.trigger(mTimer.getCurrentTime());

	if (msg->isSelfMessage()) {
		handleSelfMsg(msg);
	} else {
		throw cRuntimeError("Unknown message");
	}
}

void Middleware::handleSelfMsg(cMessage *msg)
{
	if (msg == mUpdateMessage) {
		update();
	} else if (msg == mUpdateRuntimeMessage) {
		// runtime is triggered each handleMessage invocation
	} else {
		throw cRuntimeError("Unknown self-message in ITS-G5");
	}
}

void Middleware::indicate(vanetza::Channel channel, cMessage *msg)
{
	Enter_Method_Silent();

	auto* packet = check_and_cast<GeoNetPacket*>(msg);
	auto& wrapper = packet->getPayload();
	auto* indication = check_and_cast<GeoNetIndication*>(packet->getControlInfo());
	mGeoRouter.at(channel)->indicate(wrapper.extract_up_packet(), indication->source, indication->destination);
	scheduleRuntime();
	delete msg;
}

cModule* Middleware::findHost()
{
	return inet::getContainingNode(this);
}

void Middleware::update()
{
	updateServices();
	scheduleAt(simTime() + mUpdateInterval, mUpdateMessage);
}

void Middleware::receiveSignal(cComponent* component, simsignal_t signal, double value, cObject* details)
{
}

void Middleware::scheduleRuntime()
{
	Enter_Method_Silent();
	cancelEvent(mUpdateRuntimeMessage);
	auto next_time_point = mRuntime.next();
	while (next_time_point < vanetza::Clock::time_point::max()) {
		if (next_time_point > mRuntime.now()) {
			scheduleAt(convertSimTime(next_time_point), mUpdateRuntimeMessage);
			break;
		} else {
			mRuntime.trigger(mRuntime.now());
			next_time_point = mRuntime.next();
		}
	}
}

void Middleware::updateServices()
{
	mLocalDynamicMap.dropExpired();
	for (auto& kv : mServices) {
		kv.first->trigger();
	}
}

Middleware::port_type Middleware::getPortNumber(const ItsG5BaseService* service) const
{
	port_type port = 0;
	auto it = mServices.find(const_cast<ItsG5BaseService*>(service));
	if (it != mServices.end()) {
		port = it->second;
	}
	return port;
}

SimTime Middleware::convertSimTime(vanetza::Clock::time_point tp) const
{
	using namespace std::chrono;
	const auto d = duration_cast<microseconds>(tp - mRuntime.now());
	return SimTime { simTime().inUnit(SIMTIME_US) + d.count(), SIMTIME_US };
}

void Middleware::updateGeoRouterPosition(vanetza::PositionFix position)
{
	for (auto& entry : mGeoRouter) {
		entry.second->update_position(position);
	}
}

} // namespace artery

