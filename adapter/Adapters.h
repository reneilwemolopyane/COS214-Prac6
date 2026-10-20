#pragma once
/** @file Adapters.h  Generic object Adapters. (ADAPTER OWNER)
 *  Engine code never changes per scenario: a new scenario only constructs these in main.cpp with its own
 *  endpoints/translators, or supplies a FunctionAdapter around any callable. */
#include <functional>
#include "tividy/adapter/ExternalConnector.h"
#include "tividy/adapter/Adaptees.h"

/** Wraps ANY callable external system. Scenario passes the translation lambda from main.cpp. */
class FunctionAdapter : public ExternalConnector {
public:
    typedef std::function<ConnectorResult(const ConnectorRequest&)> Handler;
    FunctionAdapter(const std::string& name, Handler h) : n(name), handler(h) {}
    std::string name() const { return n; }
    ConnectorResult execute(const ConnectorRequest&);   // TODO
private: std::string n; Handler handler;
};
/** operation -> REST verb/endpoint mapping is configuration (DataMap), not code. */
class RestApiAdapter : public ExternalConnector {
public:
    RestApiAdapter(const std::string& name, RestClientAPI* api, const DataMap& endpointConfig)
        : n(name), adaptee(api), endpoints(endpointConfig) {}
    std::string name() const { return n; }
    ConnectorResult execute(const ConnectorRequest& request);   // TODO
private: std::string n; RestClientAPI* adaptee; DataMap endpoints;
};
class MessagingAdapter : public ExternalConnector {
public:
    MessagingAdapter(const std::string& name, MessagingAPI* api) : n(name), adaptee(api) {}
    std::string name() const { return n; }
    ConnectorResult execute(const ConnectorRequest& request);   // TODO: "send" -> adaptee->send
private: std::string n; MessagingAPI* adaptee;
};
class PaymentAdapter : public ExternalConnector {
public:
    PaymentAdapter(const std::string& name, PaymentAPI* api) : n(name), adaptee(api) {}
    std::string name() const { return n; }
    ConnectorResult execute(const ConnectorRequest& request);   // TODO: "pay"/"reverse"
private: std::string n; PaymentAPI* adaptee;
};
