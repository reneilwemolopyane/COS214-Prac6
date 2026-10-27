#include "tividy/adapter/PaymentAPI.h"
#include <sstream>

std::string PaymentAPI::submit(long, const std::string&) {
    if (down) return "";
    std::ostringstream o; o << "TXN-" << ++counter;
    txns[o.str()] = false;
    return o.str();
}
bool PaymentAPI::reverse(const std::string& txn) {
    if (down) return false;
    std::map<std::string, bool>::iterator it = txns.find(txn);
    if (it == txns.end() || it->second) return false;   // unknown or already reversed
    it->second = true;
    return true;
}
