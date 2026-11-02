#ifndef MESSAGEAPI_H
#define MESSAGEAPI_H

#include <string>

/** @brief Typical SMS/email-style messaging service (ADAPTEE), simulated. setDown(true) simulates an outage. */
class MessagingAPI {
public:
    MessagingAPI() : down(false) {}
    virtual ~MessagingAPI() {}
    /** @return true if the message was accepted for delivery, false if the service is down. */
    virtual bool send(const std::string& to, const std::string& text, bool urgent);
    /** @return true if the service is up. */
    bool available() const { return !down; }
    /** @brief Simulates an outage (true) or recovery (false). */
    void setDown(bool d) { down = d; }
protected:
    bool down;
};

#endif
