#include "artery/application/RadioManager.h"
#include "artery/application/RadioModule.h"
#include "artery/application/ItsRadioModule.h"
#include "inet/common/ModuleAccess.h"

#include "artery/netw/GeoNetPacketWrapper.h"
#include "artery/netw/GeoNetRequest.h"
#include "artery/messages/GeoNetPacket_m.h"
#include <vanetza/common/serialization_buffer.hpp>
#include <vanetza/geonet/indication_context.hpp>
#include <vanetza/common/its_aid.hpp>

namespace artery
{


Define_Module(RadioManager)

RadioManager::RadioManager()
{
}

void RadioManager::initialize()
{
	initializeRadioManager();
}

void RadioManager::initializeRadioManager()
{
}

void RadioManager::registerRadioModule(RadioModule* radioModule)
{
	Enter_Method_Silent();

	mRadioModules.push_back(radioModule);
	mAccessInterfaces.push_back(radioModule);
	mChannelMap[radioModule->getChannel()] = radioModule;
}

AccessInterfaceList RadioManager::getInterfaces()
{
	return mAccessInterfaces;
}

RadioModule* RadioManager::getRadioModule(vanetza::Channel channel) const
{
	return mChannelMap.at(channel);
}

vanetza::dcc::Scheduler& RadioManager::getDccScheduler(vanetza::Channel channel) const
{
	auto radioModule = getRadioModule(channel);
	auto itsRadioModule = static_cast<ItsRadioModule*>(radioModule);
	return itsRadioModule->getDccScheduler();
}

} // namespace artery
