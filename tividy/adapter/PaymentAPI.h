#ifndef PAYMENTAPI_H
#define PAYMENTAPI_H

#include <map>
#include <string>

/** @brief Typical payment/bank service (ADAPTEE), simulated in memory.
 *  Amounts are in cents. Every payment can be reversed once. setDown(true) simulates an outage. */
class PaymentAPI {
public:
    PaymentAPI() : down(false), counter(0) {}
    virtual ~PaymentAPI() {}
    /** @return a transaction id, or "" if the gateway is down. */
    virtual std::string submit(long amountCents, const std::string& account);
    /** @return true only if txnId is a known payment that has not been reversed yet and the gateway is up. */
    virtual bool reverse(const std::string& txnId);
    /** @return true if the gateway is up. */
    bool available() const { return !down; }
    /** @return true if txnId is a known payment (reversed or not). */
    bool knows(const std::string& txnId) const { return txns.count(txnId) > 0; }
    /** @brief Simulates an outage (true) or recovery (false). */
    void setDown(bool d) { down = d; }
protected:
    bool down;
    int counter;
    std::map<std::string, bool> txns;   ///< txnId -> reversed?
};

#endif
