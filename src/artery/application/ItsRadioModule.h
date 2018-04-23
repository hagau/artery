#ifndef ITSRADIOMODULE_H_
#define ITSRADIOMODULE_H_

#include "artery/application/RadioModule.h"
#include <omnetpp/ccomponent.h>
#include <omnetpp/clistener.h>
#include <omnetpp/csimplemodule.h>
#include <vanetza/access/interface.hpp>
#include <vanetza/access/data_request.hpp>
#include <vanetza/common/channel.hpp>
#include <vanetza/common/runtime.hpp>
#include <vanetza/dcc/flow_control.hpp>
#include <vanetza/dcc/scheduler.hpp>
#include <vanetza/dcc/state_machine.hpp>
#include <vanetza/geonet/packet.hpp>


namespace artery
{

class ChannelLoadListener : public omnetpp::cListener
{
	public:
		ChannelLoadListener(vanetza::dcc::StateMachine&);
	protected:
		void receiveSignal(omnetpp::cComponent*, omnetpp::simsignal_t, double, omnetpp::cObject*);
	private:
		vanetza::dcc::StateMachine& mDccFsm;
};

class ItsRadioModule :
	public RadioModule,
	public vanetza::access::Interface
{
	public:
		ItsRadioModule();
		virtual ~ItsRadioModule();

		void initialize();
		void initializeModule();

		void request(const vanetza::dcc::DataRequest&,
					 std::unique_ptr<vanetza::ChunkPacket>) override;

		void request(const vanetza::access::DataRequest&,
					 std::unique_ptr<vanetza::ChunkPacket>);

		vanetza::dcc::Scheduler& getDccScheduler() const;

	private:
		ChannelLoadListener mChannelLoadListener;

		vanetza::dcc::StateMachine mDccFsm;
		std::unique_ptr<vanetza::dcc::Scheduler> mDccScheduler;
		std::unique_ptr<vanetza::dcc::FlowControl> mDccControl;

};


} // namespace artery

#endif
