#ifndef MCOSTATICSTRATEGY_H
#define MCOSTATICSTRATEGY_H

#include "artery/application/McoStrategy.h"
#include <vanetza/common/channel.hpp>
#include <vanetza/common/its_aid.hpp>

#include <vector>

namespace artery
{
namespace application
{

class McoStaticStrategy : public McoStrategy
{
public:
	void initialize() override;
	vanetza::Channel choose(vanetza::geonet::DataRequest&) override;
	vanetza::Channel choose(vanetza::ItsAid) override;

private:
	using ChannelMap = std::map<vanetza::ItsAid, vanetza::Channel>;

	void parseChannelMap(const omnetpp::cXMLElement*);

	ChannelMap mChannelMap;
};

} // namespace application
} // namespace artery

#endif /* MCOSTATICSTRATEGY_H */

