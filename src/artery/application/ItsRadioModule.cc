#include "artery/application/ItsRadioModule.h"
#include "artery/messages/GeoNetPacket_m.h"
#include "artery/netw/GeoNetRequest.h"
#include "inet/common/ModuleAccess.h"



namespace artery
{

Define_Module(ItsRadioModule)

ChannelLoadListener::ChannelLoadListener(vanetza::dcc::StateMachine& dccFsm) : mDccFsm(dccFsm)
{
}

void ChannelLoadListener::receiveSignal(cComponent* component, omnetpp::simsignal_t signal, double value, cObject* details)
{
	if (signal == RadioDriverBase::ChannelLoadSignal) {
		ASSERT(value >= 0.0 && value <= 1.0);
		unsigned busy_samples = std::round(12500.0 * value);
		vanetza::dcc::ChannelLoad cl { busy_samples, 12500 };
		mDccFsm.update(cl);
	}
}


ItsRadioModule::ItsRadioModule() : mChannelLoadListener(mDccFsm)
{
}

ItsRadioModule::~ItsRadioModule()
{
}

void ItsRadioModule::initialize()
{
	mRadioDriver->subscribe(RadioDriverBase::ChannelLoadSignal, &mChannelLoadListener);
}

void ItsRadioModule::initializeModule()
{
	artery::RadioModule::initializeModule();


	mDccScheduler.reset(new vanetza::dcc::Scheduler {mDccFsm, mRuntime->now()});

	mDccControl.reset(new vanetza::dcc::FlowControl {*mRuntime, *mDccScheduler.get(), *this});
	mDccControl->queue_length(par("vanetzaDccQueueLength"));
}

void ItsRadioModule::request(const vanetza::access::DataRequest& req,
                 std::unique_ptr<vanetza::ChunkPacket> payload)
{
	Enter_Method("request");

	if ((*payload)[vanetza::OsiLayer::Network].ptr() == nullptr) {
		throw omnetpp::cRuntimeError("Missing network layer payload in middleware request");
	}

	GeoNetPacket* net = new GeoNetPacket("GeoNet packet");
	net->setByteLength(payload->size());
	net->setPayload(GeoNetPacketWrapper(std::move(payload)));
	net->setControlInfo(new GeoNetRequest(req));

	send(net, mRadioDriverOut);
}

void ItsRadioModule::request(const vanetza::dcc::DataRequest& req,
                          std::unique_ptr<vanetza::ChunkPacket> payload)
{
	Enter_Method("request");

	mDccControl->request(req, std::move(payload));
}


vanetza::dcc::Scheduler& ItsRadioModule::getDccScheduler() const
{
	return *mDccScheduler;
}

} // namespace artery
