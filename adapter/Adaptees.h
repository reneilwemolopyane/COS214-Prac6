#pragma once
/** @file Adaptees.h  GENERIC technology-level external interfaces (incompatible with ExternalConnector).
 *  Real or test-stub implementations are supplied/configured by the scenario in main.cpp. (ADAPTER OWNER) */
#include <string>
#include <map>

/** Typical HTTP/REST client (verbs + endpoint). */
class RestClientAPI {
public:
    virtual ~RestClientAPI() {}
    virtual int get(const std::string& endpoint, const std::string& query, std::string& body);    // returns HTTP code
    virtual int post(const std::string& endpoint, const std::string& payload, std::string& body);
    virtual int del(const std::string& endpoint, std::string& body);
};
/** Typical SMS/email-style messaging service. */
class MessagingAPI {
public:
    virtual ~MessagingAPI() {}
    virtual bool send(const std::string& to, const std::string& text, bool urgent);
};
/** Typical payment/bank service (amounts in cents, reversible). */
class PaymentAPI {
public:
    virtual ~PaymentAPI() {}
    virtual std::string submit(long amountCents, const std::string& account);   // txn id, "" on failure
    virtual bool reverse(const std::string& txnId);
};
