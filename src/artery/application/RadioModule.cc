#include "artery/application/RadioModule.h"
#include "artery/messages/GeoNetPacket_m.h"
#include "artery/netw/GeoNetRequest.h"
#include "inet/common/ModuleAccess.h"


namespace artery
{

Define_Module(RadioModule)

RadioModule::RadioModule()
{
}

RadioModule::~RadioModule()
{
}

void RadioModule::initialize()
{
}

void RadioModule::initializeModule()
{
	mRadioDriverIn = gate("radioDriverIn");
	mRadioDriverOut = gate("radioDriverOut");

	mRadioDriver = inet::getModuleFromPar<RadioDriverBase>(par("radioDriverModule"), inet::getContainingNode(this));

	// implicit conversion to vanetza::Channel
	mChannel = par("radioChannel").longValue();
}

void RadioModule::request(const vanetza::dcc::DataRequest& req,
                          std::unique_ptr<vanetza::ChunkPacket> payload)
{
}

void RadioModule::initialize(vanetza::Runtime* runtime, RadioIndicationInterface* indicationInterface)
{
	mRuntime = runtime;
	mIndicationInterface = indicationInterface;

	initializeModule();
}

void RadioModule::handleMessage(omnetpp::cMessage* msg)
{
	if (msg->isSelfMessage()) {
		throw cRuntimeError("Unknown self message");
	} else {
		if (msg->getArrivalGate() == mRadioDriverIn) {
			indicate(msg);
			return;
		}
		throw cRuntimeError("Unknown message");
	}
}

void RadioModule::indicate(omnetpp::cMessage* msg)
{
	mIndicationInterface->indicate(mChannel, msg);
}

vanetza::Channel RadioModule::getChannel()
{
	return mChannel;
}

vanetza::MacAddress RadioModule::getMacAddress()
{
	assert(mRadioDriver);
	return mRadioDriver->getMacAddress();
}

} // namespace artery
