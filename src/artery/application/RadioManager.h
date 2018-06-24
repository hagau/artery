#ifndef RADIOMANAGER_H_
#define RADIOMANAGER_H_

#include "artery/application/RadioModule.h"
#include "artery/utility/Channel.h"
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>
#include <vanetza/common/runtime.hpp>
#include <vanetza/geonet/router.hpp>
#include <vanetza/geonet/packet.hpp>
#include <vanetza/security/security_entity.hpp>

namespace artery
{


using AccessInterface = std::tuple<Channel, RadioModule*>;
using AccessInterfaceList = std::vector<RadioModule*>;

class RadioManager : public omnetpp::cSimpleModule
{
	public:
		RadioManager();

		AccessInterfaceList getInterfaces();

		RadioModule* getRadioModule(vanetza::Channel) const;
		void registerRadioModule(RadioModule*);
		vanetza::dcc::Scheduler& getDccScheduler(Channel) const;

	protected:
		void initialize();

	private:
		void initializeRadioManager();
		void initializeRadioModules();

		vanetza::Runtime* mRuntime;

		std::vector<RadioModule*> mRadioModules;
		std::map<vanetza::Channel, RadioModule*> mChannelMap;

		AccessInterfaceList mAccessInterfaces;
};


} // namespace artery

#endif
