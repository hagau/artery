#ifndef MCOSTRATEGY_H
#define MCOSTRATEGY_H

#include <omnetpp/csimplemodule.h>
#include <vanetza/common/channel.hpp>
#include <vanetza/geonet/data_request.hpp>

#include <vector>

namespace artery
{
namespace application
{

class McoStrategy : public omnetpp::cSimpleModule
{
public:
    virtual vanetza::Channel choose(vanetza::geonet::DataRequest&) = 0;
    virtual vanetza::Channel choose(vanetza::ItsAid) = 0;
};

} // namespace application
} // namespace artery

#endif /* MCOSTRATEGY_H */

