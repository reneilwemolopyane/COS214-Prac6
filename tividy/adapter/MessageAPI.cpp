#include "tividy/adapter/MessageAPI.h"

bool MessagingAPI::send(const std::string&, const std::string&, bool) { return !down; }
