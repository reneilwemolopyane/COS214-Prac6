#include "tividy/adapter/Adaptees.h"
// STUBS: TODO simulated external systems (test doubles).
int RestClientAPI::get(const std::string&, const std::string&, std::string&) { return 0; }
int RestClientAPI::post(const std::string&, const std::string&, std::string&) { return 0; }
int RestClientAPI::del(const std::string&, std::string&) { return 0; }
bool MessagingAPI::send(const std::string&, const std::string&, bool) { return false; }
std::string PaymentAPI::submit(long, const std::string&) { return ""; }
bool PaymentAPI::reverse(const std::string&) { return false; }
