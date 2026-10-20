#include "tividy/adapter/Adapters.h"
// STUBS: TODO translate request -> adaptee call -> ConnectorResult (set retryable on temporary failures).
ConnectorResult FunctionAdapter::execute(const ConnectorRequest&)  { return ConnectorResult(); }
ConnectorResult RestApiAdapter::execute(const ConnectorRequest&)   { return ConnectorResult(); }
ConnectorResult MessagingAdapter::execute(const ConnectorRequest&) { return ConnectorResult(); }
ConnectorResult PaymentAdapter::execute(const ConnectorRequest&)   { return ConnectorResult(); }
