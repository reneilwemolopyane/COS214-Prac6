#ifndef SCHEDULINGAPI_H
#define SCHEDULINGAPI_H

#include <map>
#include <set>
#include <string>
#include <vector>
/** @brief Typical calendar/booking service (ADAPTEE). Finds free slots, books and cancels appointments.
 *  Simulated in memory for tests; setDown(true) simulates an outage. */
class SchedulingAPI {
public:
    SchedulingAPI() : down(false), counter(0) {}
    virtual ~SchedulingAPI() {}
    /** @return the free slots for a resource on a date (empty if none or the service is down). */
    virtual std::vector<std::string> freeSlots(const std::string& resource, const std::string& date);
    /** @return a booking reference, or "" if the slot is taken, unknown or the service is down. */
    virtual std::string book(const std::string& resource, const std::string& date, const std::string& slot);
    virtual bool cancel(const std::string& bookingRef);
    bool available() const { return !down; }
    void setDown(bool d) { down = d; }
protected:
    bool down;
    int counter;
    std::map<std::string, std::string> bookings;   // bookingRef -> "resource|date|slot"
};

#endif
