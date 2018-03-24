#ifndef RADIOMANAGER_H_
#define RADIOMANAGER_H_

#include "artery/application/RadioModule.h"
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>
#include <vanetza/common/channel.hpp>
#include <vanetza/common/runtime.hpp>
#include <vanetza/geonet/router.hpp>
#include <vanetza/geonet/packet.hpp>
#include <vanetza/security/security_entity.hpp>

namespace artery
{


class RadioManager : public omnetpp::cSimpleModule
{
	public:
		RadioManager();

		vanetza::geonet::Router::AccessInterfaceList getInterfaces();

		RadioModule* getRadioModule(vanetza::Channel) const;
		void registerRadioModule(RadioModule*);
		vanetza::dcc::Scheduler& getDccScheduler(vanetza::Channel) const;

	protected:
		void initialize();

	private:
		void initializeRadioManager();
		void initializeRadioModules();

		vanetza::Runtime* mRuntime;

		std::vector<RadioModule*> mRadioModules;
		std::map<vanetza::Channel, RadioModule*> mChannelMap;

		vanetza::geonet::Router::AccessInterfaceList mAccessInterfaces;
};


} // namespace artery

#endif
