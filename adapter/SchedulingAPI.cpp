#include "SchedulingAPI.h"
#include <algorithm>
#include <sstream>

namespace {
const char* const kSlots[] = {"09:00", "11:00", "14:00"};   // every resource offers these slots each day
std::string key(const std::string& r, const std::string& d, const std::string& s) { return r + "|" + d + "|" + s; }
}

std::vector<std::string> SchedulingAPI::freeSlots(const std::string& resource, const std::string& date) {
    std::vector<std::string> free;
    if (down) return free;
    for (int i = 0; i < 3; ++i) {
        bool taken = false;
        for (std::map<std::string, std::string>::const_iterator it = bookings.begin(); it != bookings.end(); ++it)
            if (it->second == key(resource, date, kSlots[i])) taken = true;
        if (!taken) free.push_back(kSlots[i]);
    }
    return free;
}
std::string SchedulingAPI::book(const std::string& resource, const std::string& date, const std::string& slot) {
    if (down) return "";
    std::vector<std::string> free = freeSlots(resource, date);
    if (std::find(free.begin(), free.end(), slot) == free.end()) return "";   // taken or not a real slot
    std::ostringstream o; o << "BK-" << ++counter;
    bookings[o.str()] = key(resource, date, slot);
    return o.str();
}
bool SchedulingAPI::cancel(const std::string& bookingRef) {
    return !down && bookings.erase(bookingRef) > 0;
}
