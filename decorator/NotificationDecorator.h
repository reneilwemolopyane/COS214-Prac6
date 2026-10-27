#pragma once
#include <functional>
#include <string>
#include "WorkItemDecorator.h"

/** @brief Tells interested parties when the item starts, completes or is cancelled. */
class NotificationDecorator : public WorkItemDecorator {
public:
    typedef std::function<void(const std::string&)> Sink;

    /** @param w wrapped component
     *  @param s receives each message (SMS/email adapter, test stub, ...) */
    NotificationDecorator(WorkflowComponent* w, Sink s);

    void start();
    void complete();
    void cancel();
    WorkflowComponent* clone() const;
private:
    void tell(const std::string& event);
    Sink sink;
};