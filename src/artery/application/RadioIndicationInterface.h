#ifndef RADIOINTERFACE_H_
#define RADIOINTERFACE_H_

#include <vanetza/common/channel.hpp>
#include <omnetpp/cmessage.h>

namespace artery
{

class RadioIndicationInterface
{
	public:
		virtual void indicate(vanetza::Channel, omnetpp::cMessage*) = 0;
};


} // namespace artery

#endif
