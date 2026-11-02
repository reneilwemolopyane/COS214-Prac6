#pragma once
#include <vector>
#include <cstddef>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/Common.h"

/**
 * @brief Composite group that executes child components sequentially or in parallel.
 *
 * A stage owns its children. Sequential stages activate children in order and
 * stop advancing when a child is unfinished or unsuccessful.
 */
class Stage : public WorkflowComponent {
public:
    /** @brief Serializable runtime fields needed to restore stage execution. */
    struct RuntimeSnapshot {
        bool activated;
        bool cancelledFlag;
        bool finishedNotified;
        std::size_t cursor;
        bool cursorActivated;
    };

    /** @return A snapshot of this stage's runtime execution state. */
    RuntimeSnapshot captureRuntimeSnapshot() const;
    /**
     * @brief Restores runtime state from a snapshot.
     * @param snapshot Previously captured state.
     * @throws std::invalid_argument if the snapshot cursor exceeds the child count.
     */
    void restoreRuntimeSnapshot(const RuntimeSnapshot& snapshot);

    /**
     * @brief Creates a stage with a name and execution mode.
     * @param name Display name for the stage.
     * @param m Child execution mode; defaults to SEQUENTIAL.
     */
    explicit Stage(const std::string& name = "", ExecutionMode m = SEQUENTIAL);
    /** @brief Destroys the stage and its owned child components. */
    ~Stage() override;

    /** @brief Copying is disabled because a stage owns its children. */
    Stage(const Stage&) = delete;
    /** @brief Copy assignment is disabled because a stage owns its children. */
    Stage& operator=(const Stage&) = delete;

    /**
     * @brief Adds a child and transfers ownership to this stage.
     * @throws std::invalid_argument if the child is null or already has a parent.
     * @throws std::logic_error if this stage is active or cancelled.
     */
    void add(WorkflowComponent* c);
    /**
     * @brief Removes and destroys a direct child.
     * @throws std::logic_error if this stage is active or cancelled.
     * @throws std::invalid_argument if the component is not a direct child.
     */
    void remove(WorkflowComponent* c);
    /** @return The number of direct child components. */
    std::size_t childCount() const;
    /**
     * @brief Returns a direct child by index.
     * @throws std::out_of_range if index is outside the child list.
     */
    WorkflowComponent* childAt(std::size_t index) const;
    /** @return This stage's execution mode. */
    ExecutionMode getMode() const { return mode; }
    /** @brief Sets the execution mode, typically before the stage is cloned. */
    void setMode(ExecutionMode m) { mode = m; }

    /** @brief Starts execution by activating this stage. */
    void start() override;
    /** @brief Activates this stage and begins coordinating its children. */
    void activate() override;
    /** @brief Throws because a stage completes automatically when its children finish. */
    void complete() override;
    /** @brief Cancels unfinished children and then this stage. */
    void cancel() override;
    /** @return Whether the stage finished and all children completed successfully. */
    bool isComplete() const override;
    /** @return Whether the stage and all children have reached terminal states. */
    bool isFinished() const override;
    /** @return Whether any child component failed. */
    bool isFailed() const override;
    /** @brief Called by a child when its execution state changes to finished. */
    void childFinished(WorkflowComponent& child) override;
    /** @brief Creates a deep copy of this stage and its children. */
    WorkflowComponent* clone() const override;
    /** @return The stage name. */
    std::string getName() const override;
    /** @brief Visits this stage and recursively visits its children. */
    void accept(WorkflowVisitor& v) override;
    /** @brief Recursively collects the work items in this stage. */
    void collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& out) override;
    /** @brief Replaces a matching descendant with a wrapper component. */
    bool replaceChild(WorkflowComponent* oldChild, WorkflowComponent* wrapper) override;
    /** @return This stage. */
    Stage* asStage() override { return this; }
    /** @return This stage as a const pointer. */
    const Stage* asStage() const override { return this; }
    /** @brief Adds ordering constraints contributed by this stage and its descendants. */
    void collectOrdering(std::vector<std::pair<std::string, std::string> >& out) override;

protected:
    std::string name;
    ExecutionMode mode;
    std::vector<WorkflowComponent*> children;
    
    bool activated;
    bool cancelledFlag;
    bool finishedNotified;
    bool busy;
    std::size_t cursor;
    bool cursorActivated;

    void advance();
    void finish();
    void cloneChildrenInto(Stage& target) const;
    
    struct BusyGuard {
        bool& busy;
        BusyGuard(bool& b) : busy(b) { busy = true; }
        ~BusyGuard() { busy = false; }
    };
};

class SubProcess : public Stage {
public:
    /** @brief Creates a named nested stage with the requested execution mode. */
    explicit SubProcess(const std::string& n = "", ExecutionMode m = SEQUENTIAL);
    /** @brief Creates a deep copy as a SubProcess. */
    WorkflowComponent* clone() const override;
};