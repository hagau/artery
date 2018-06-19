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

#ifndef ARTERY_MIDDLEWARE_H_
#define ARTERY_MIDDLEWARE_H_

#include "artery/application/Facilities.h"
#include "artery/application/LocalDynamicMap.h"
#include "artery/application/McoStrategy.h"
#include "artery/application/RadioIndicationInterface.h"
#include "artery/application/RadioManager.h"
#include "artery/application/RadioModule.h"
#include "artery/application/Timer.h"
#include "artery/utility/Identity.h"
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>
#include <omnetpp/simtime.h>
#include <vanetza/access/data_request.hpp>
#include <vanetza/access/interface.hpp>
#include <vanetza/btp/data_interface.hpp>
#include <vanetza/btp/port_dispatcher.hpp>
#include <vanetza/common/clock.hpp>
#include <vanetza/common/channel.hpp>
#include <vanetza/common/its_aid.hpp>
#include <vanetza/common/stored_position_provider.hpp>
#include <vanetza/common/runtime.hpp>
#include <vanetza/dcc/flow_control.hpp>
#include <vanetza/dcc/scheduler.hpp>
#include <vanetza/dcc/state_machine.hpp>
#include <vanetza/geonet/packet.hpp>
#include <vanetza/geonet/router.hpp>
#include <vanetza/security/backend.hpp>
#include <vanetza/security/certificate_cache.hpp>
#include <vanetza/security/certificate_provider.hpp>
#include <vanetza/security/certificate_validator.hpp>
#include <vanetza/security/security_entity.hpp>
#include <vanetza/security/sign_header_policy.hpp>
#include <map>
#include <memory>

// forward declarations
class ItsG5BaseService;
class RadioDriverBase;

namespace traci { class VehicleController; }

namespace artery
{

/**
 * Middleware providing a runtime context for services.
 */
class Middleware :
	public omnetpp::cSimpleModule, public omnetpp::cListener,
	public vanetza::btp::RequestInterface, public RadioIndicationInterface
{
	public:
		typedef uint16_t port_type;

		Middleware();
		void request(const vanetza::btp::DataRequestB&, std::unique_ptr<vanetza::DownPacket>) override;
		Facilities& getFacilities() { return mFacilities; }
		port_type getPortNumber(const ItsG5BaseService*) const;
		const Identity& getIdentity() const { return mIdentity; }

		// radio input
		void indicate(vanetza::Channel, omnetpp::cMessage*);

	protected:
		void initialize(int stage) override;
		int numInitStages() const override;
		void finish() override;
		void handleMessage(omnetpp::cMessage* msg) override;
		virtual void handleSelfMsg(omnetpp::cMessage* msg);
		void receiveSignal(cComponent*, omnetpp::simsignal_t, double, cObject*) override;
		virtual void initializeIdentity(Identity&);
		virtual void initializeManagementInformationBase(vanetza::geonet::MIB&);
		virtual void update();
		omnetpp::cModule* findHost();
		const vanetza::Runtime& getRuntime() const { return mRuntime; }
		vanetza::geonet::Router& getRouter() const { ASSERT(mGeoRouter); return *mGeoRouter; }

		vanetza::StoredPositionProvider mPositionProvider;
		vanetza::geonet::StationType mGnStationType;

	private:
		void updateServices();
		void initializeMiddleware();

		void initializeRadioModules();
		void initializeNetworking();
		void initializeGeoNetworking();

		void initializeServices();
		void initializeSecurity();
		void scheduleRuntime();
		omnetpp::SimTime convertSimTime(vanetza::Clock::time_point tp) const;

		RadioManager* mRadioManager;

		application::McoStrategy* mMcoStrategy;

		Timer mTimer;
		Identity mIdentity;
		artery::LocalDynamicMap mLocalDynamicMap;
		vanetza::Runtime mRuntime;

		std::unique_ptr<vanetza::security::Backend> mSecurityBackend;
		std::unique_ptr<vanetza::security::CertificateProvider> mSecurityCertificates;
		std::unique_ptr<vanetza::security::CertificateValidator> mSecurityCertificateValidator;
		std::unique_ptr<vanetza::security::CertificateCache> mSecurityCertificateCache;
		std::unique_ptr<vanetza::security::SignHeaderPolicy> mSecuritySignHeaderPolicy;
		std::unique_ptr<vanetza::security::SecurityEntity> mSecurityEntity;
		vanetza::geonet::MIB mGeoMib;
		std::unique_ptr<vanetza::geonet::Router> mGeoRouter;
		vanetza::btp::PortDispatcher mBtpPortDispatcher;
		omnetpp::SimTime mUpdateInterval;
		omnetpp::cMessage* mUpdateMessage;
		omnetpp::cMessage* mUpdateRuntimeMessage;
		Facilities mFacilities;
		std::map<ItsG5BaseService*, port_type> mServices;
};

} // namespace artery

#endif
