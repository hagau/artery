#ifndef RADIOMODULE_H_
#define RADIOMODULE_H_

#include "artery/application/RadioIndicationInterface.h"
#include "artery/nic/RadioDriverBase.h"
#include "artery/utility/Channel.h"
#include <omnetpp/ccomponent.h>
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>
#include <vanetza/access/interface.hpp>
#include <vanetza/access/data_request.hpp>
#include <vanetza/common/runtime.hpp>
#include <vanetza/dcc/flow_control.hpp>
#include <vanetza/dcc/scheduler.hpp>
#include <vanetza/dcc/state_machine.hpp>
#include <vanetza/geonet/packet.hpp>


namespace artery
{

class RadioModule : public omnetpp::cSimpleModule,
	public vanetza::dcc::RequestInterface
{
	public:
		RadioModule();
		virtual ~RadioModule();

		// initialize everything that needs the runtime in here
		virtual void initializeModule();

		virtual void request(const vanetza::dcc::DataRequest&,
							 std::unique_ptr<vanetza::ChunkPacket>);

		virtual void initialize(vanetza::Runtime*, RadioIndicationInterface*);

		virtual Channel getChannel();
		virtual vanetza::MacAddress getMacAddress();

    protected:
		void initialize() override;
		void handleMessage(omnetpp::cMessage* msg) override;

		void indicate(omnetpp::cMessage*);

		vanetza::Runtime* mRuntime;

		RadioIndicationInterface* mIndicationInterface;

		RadioDriverBase* mRadioDriver;
		omnetpp::cGate* mRadioDriverIn;
		omnetpp::cGate* mRadioDriverOut;

		vanetza::Channel mChannel;
};


} // namespace artery

#endif
