#include "artery/application/McoStaticStrategy.h"
#include <omnetpp/ccomponenttype.h>
#include <omnetpp/cxmlelement.h>
#include <boost/lexical_cast.hpp>

using namespace omnetpp;

namespace artery
{
namespace application
{


Define_Module(McoStaticStrategy)

void McoStaticStrategy::initialize()
{
	auto channelMap = par("aidChannelMap").xmlValue();
	parseChannelMap(channelMap);
}

void McoStaticStrategy::parseChannelMap(const omnetpp::cXMLElement* channelMap)
{
	auto throwError = [](const std::string& msg, const omnetpp::cXMLElement* node) {
		throw omnetpp::cRuntimeError("%s: %s", node->getSourceLocation(), msg.c_str());
	};

	mChannelMap.clear();

	for (omnetpp::cXMLElement* map : channelMap->getChildrenByTagName("map")) {
		auto aidAttribute = map->getAttribute("aid");
		if (!aidAttribute) {
			throwError("missing 'aid' attribute in 'map' tag", map);
		}
		vanetza::ItsAid aid = boost::lexical_cast<int>(aidAttribute);

		auto channelAttribute = map->getAttribute("channel");
		if (!channelAttribute) {
			throwError("missing 'channel' attribute in 'map' tag", map);
		}
		vanetza::Channel channel = boost::lexical_cast<int>(channelAttribute);

		mChannelMap.insert(std::make_pair(aid, channel));
	}
}

vanetza::Channel McoStaticStrategy::choose(vanetza::geonet::DataRequest& request)
{
	return choose(request.its_aid);
}

vanetza::Channel McoStaticStrategy::choose(vanetza::ItsAid aid)
{
	auto result = mChannelMap.find(aid);

	if (result == mChannelMap.end()) {
		throw cRuntimeError("Unknown AID");
	}
	return result->second;
}

} // namespace application
} // namespace artery
