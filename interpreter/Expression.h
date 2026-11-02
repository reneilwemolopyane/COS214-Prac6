#pragma once
/**
 * @file Expression.h
 * @brief Interpreter pattern: a tiny rule language for routing decisions.
 *
 * A workflow definition attaches a *guard* to a work item, for example
 * @code
 *   outcome('Review claim and decide') == REPAIR
 *   fundBalance >= $requestedAmount and bursaryType == need
 * @endcode
 * The text is parsed once into a tree of Expression objects (the abstract syntax tree).
 * At run time the InstanceCoordinator evaluates the tree against the running
 * instance's data, so routing is data, not code: a new rule never edits the engine.
 *
 * Roles: AbstractExpression = Expression; TerminalExpression = ComparisonExpression;
 * NonterminalExpressions = AndExpression, OrExpression, NotExpression;
 * Context = EvaluationContext; Client = ExpressionParser.
 *
 * Grammar:
 *   or         := and ('or' and)*
 *   and        := not ('and' not)*
 *   not        := 'not' not | primary
 *   primary    := '(' or ')' | comparison
 *   comparison := operand ('=='|'!='|'<'|'<='|'>'|'>=') value
 *   operand    := word | 'quoted' | outcome('Work item name')
 *   value      := word | 'quoted' | number | $word   ($word is looked up in the context)
 */
#include <string>

/** @brief Context role: resolves keys such as "claimAmount" or "outcome:Assess damage" to text. */
class EvaluationContext {
public:
    virtual ~EvaluationContext() {}
    /** @return the value stored under @p key, or "" if unknown. */
    virtual std::string lookup(const std::string& key) const = 0;
};

/** @brief AbstractExpression: a boolean rule that can be evaluated and printed. */
class Expression {
public:
    virtual ~Expression() {}
    virtual bool evaluate(const EvaluationContext& context) const = 0;
    /** @brief Deep copy (expressions are copied with the definition into each instance). */
    virtual Expression* clone() const = 0;
    /** @brief Readable form, used in the audit trail ("branch not taken: ..."). */
    virtual std::string toString() const = 0;
};

/** @brief TerminalExpression: compares one context value with a literal or another context value. */
class ComparisonExpression : public Expression {
public:
    enum Op { EQ, NE, LT, LE, GT, GE };
    /** @param key context key on the left; @param rhs literal, or a context key if @p rhsIsReference */
    ComparisonExpression(const std::string& key, Op op, const std::string& rhs, bool rhsIsReference);
    bool evaluate(const EvaluationContext& context) const;
    Expression* clone() const;
    std::string toString() const;
private:
    std::string key; Op op; std::string rhs; bool rhsIsReference;
};

/** @brief NonterminalExpression: both operands must hold. */
class AndExpression : public Expression {
public:
    AndExpression(Expression* l, Expression* r);   ///< takes ownership
    ~AndExpression();
    bool evaluate(const EvaluationContext& c) const;
    Expression* clone() const;
    std::string toString() const;
private:
    Expression* left; Expression* right;
    AndExpression(const AndExpression&); AndExpression& operator=(const AndExpression&);
};

/** @brief NonterminalExpression: at least one operand must hold. */
class OrExpression : public Expression {
public:
    OrExpression(Expression* l, Expression* r);    ///< takes ownership
    ~OrExpression();
    bool evaluate(const EvaluationContext& c) const;
    Expression* clone() const;
    std::string toString() const;
private:
    Expression* left; Expression* right;
    OrExpression(const OrExpression&); OrExpression& operator=(const OrExpression&);
};

/** @brief NonterminalExpression: negation. */
class NotExpression : public Expression {
public:
    explicit NotExpression(Expression* e);         ///< takes ownership
    ~NotExpression();
    bool evaluate(const EvaluationContext& c) const;
    Expression* clone() const;
    std::string toString() const;
private:
    Expression* inner;
    NotExpression(const NotExpression&); NotExpression& operator=(const NotExpression&);
};

/** @brief Client role: turns rule text into an expression tree. */
class ExpressionParser {
public:
    /** @return a new expression the caller owns.
     *  @throws std::invalid_argument with a readable message on a syntax error. */
    static Expression* parse(const std::string& text);
};
