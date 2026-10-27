#ifndef MESSAGEADAPTER_H
#define MESSAGEADAPTER_H

#include <string>
#include "ExternalConnector.h"
#include "MessageAPI.h"


/** @brief Adapts a messaging service. Operation "send" with params "to", "text" and optional "urgent"="true".
 *  Retryable failure = the service is down. Everything else is a permanent failure. */
class MessagingAdapter : public ExternalConnector {
public:
    MessagingAdapter(const std::string& name, MessagingAPI* api) : n(name), adaptee(api) {}
    std::string name() const { return n; }
    /** @brief Translates a "send" request into MessagingAPI::send() and the reply into a ConnectorResult. */
    ConnectorResult execute(const ConnectorRequest& request);
private:
    std::string n; MessagingAPI* adaptee;
};

#endif
