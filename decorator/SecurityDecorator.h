#pragma once
#include <functional>
#include <string>
#include "WorkItemDecorator.h"

/** @brief Only lets a participant holding the required role act on the item. */
class SecurityDecorator : public WorkItemDecorator {
public:
    /** Returns the role of whoever is acting right now (supplied by the engine/TUI). */
    typedef std::function<std::string()> ActorProvider;

    /** @param w        wrapped component
     *  @param required role needed to act on the item */
    SecurityDecorator(WorkflowComponent* w, const std::string& required);

    /** @param actor role of the participant acting right now */
    SecurityDecorator(WorkflowComponent* w, const std::string& required,
                      const std::string& actor);

    /** Sets a fixed role for whoever acts next. An empty role is always denied. */
    void setActor(const std::string& role);

    /** Asks this function who is acting, on every call. Takes priority over setActor(). */
    void setActorProvider(ActorProvider p);

    /** @throws std::runtime_error if the actor's role is not permitted */
    void start();
    void complete();
    void complete(const std::string& outcome) override;
    void assign(Participant* p) override;
    void cancel();
    WorkflowComponent* clone() const;
private:
    void check() const;
    std::string requiredRole, actorRole;
    ActorProvider provider;
};