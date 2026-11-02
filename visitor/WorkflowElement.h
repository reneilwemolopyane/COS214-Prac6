#ifndef WORKFLOWELEMENT_H
#define WORKFLOWELEMENT_H

class WorkflowVisitor;

/**
 * @brief Interface for an object that can be visited by a workflow visitor.
 */
class WorkflowElement {
public:
    /** @brief Virtual destructor for polymorphic workflow elements. */
    virtual ~WorkflowElement() {}

    /**
     * @brief Dispatches this element to the appropriate visitor operation.
     * @param v Visitor that processes this element.
     */
    virtual void accept(WorkflowVisitor& v) = 0;
};

#endif