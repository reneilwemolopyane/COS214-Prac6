#ifndef SCHEDULINGADAPTER_H
#define SCHEDULINGADAPTER_H
#include <string>
#include "ExternalConnector.h"
#include "SchedulingAPI.h"

/** @brief Generic ADAPTER for any calendar/booking service (any industry).
 *  Operations (request.operation):
 *   "findSlot"  params: resource, date                 -> data["slot"] (first free slot)
 *   "book"      params: resource, date, slot           -> data["bookingRef"]
 *   "cancel"    params: bookingRef
 *  Retryable failure = the service is down. Everything else is a permanent failure. */
class SchedulingAdapter : public ExternalConnector {
public:
    SchedulingAdapter(const std::string& name, SchedulingAPI* api) : n(name), adaptee(api) {}
    std::string name() const override{ return n; }
    /** @brief Translates a generic request into adaptee calls and the reply into a ConnectorResult. */
    ConnectorResult execute(const ConnectorRequest& request) override;
private:
    ConnectorResult findSlot(const ConnectorRequest& request);
    ConnectorResult bookSlot(const ConnectorRequest& request);
    ConnectorResult cancelBooking(const ConnectorRequest& request);

    std::string n; SchedulingAPI* adaptee;
};

#endif
