#include "tividy/adapter/AdapterHelpers.h"
#include "tividy/adapter/SchedulingAdapter.h"

// Checks that the calendar is reachable, then hands the request to the right method.
ConnectorResult SchedulingAdapter::execute(const ConnectorRequest& request) {
    if (adaptee == 0) return failed(n, false, "no scheduling client");
    if (!adaptee->available()) return failed(n, true, "scheduling service unavailable");

    if (request.operation == "findSlot") return findSlot(request);
    if (request.operation == "book") return bookSlot(request);
    if (request.operation == "cancel") return cancelBooking(request);
    return failed(n, false, "unknown operation '" + request.operation + "'");
}

// "findSlot": returns the first free slot for a resource on a date.
ConnectorResult SchedulingAdapter::findSlot(const ConnectorRequest& request) {
    std::string resource = param(request.params, "resource");
    std::string date = param(request.params, "date");
    if (resource.empty() || date.empty()) return failed(n, false, "missing resource or date");

    std::vector<std::string> slots = adaptee->freeSlots(resource, date);
    if (slots.empty()) return failed(n, false, "no free slot");

    ConnectorResult result(true, false, "slot found");
    result.data["slot"] = slots.front();
    return result;
}

// "book": reserves a slot and returns a booking reference.
ConnectorResult SchedulingAdapter::bookSlot(const ConnectorRequest& request) {
    std::string resource = param(request.params, "resource");
    std::string date = param(request.params, "date");
    std::string slot = param(request.params, "slot");
    if (resource.empty() || date.empty() || slot.empty())
        return failed(n, false, "missing resource, date or slot");

    std::string reference = adaptee->book(resource, date, slot);
    if (reference.empty()) return failed(n, false, "slot unavailable");

    ConnectorResult result(true, false, "booked");
    result.data["bookingRef"] = reference;
    return result;
}

// "cancel": removes a booking.
ConnectorResult SchedulingAdapter::cancelBooking(const ConnectorRequest& request) {
    std::string reference = param(request.params, "bookingRef");
    if (reference.empty()) return failed(n, false, "missing bookingRef");

    if (!adaptee->cancel(reference)) return failed(n, false, "booking not found");
    return ConnectorResult(true, false, "cancelled");
}
