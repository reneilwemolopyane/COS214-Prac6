// Interpreter pattern: rule text -> expression tree -> boolean, plus the approval router (Chain of Responsibility).
#include "doctest.h"
#include <map>
#include <stdexcept>
#include "tividy/interpreter/Expression.h"
#include "tividy/chain_of_responsibility/ApprovalRouter.h"

namespace {
struct MapContext : EvaluationContext {
    std::map<std::string, std::string> v;
    std::string lookup(const std::string& k) const { std::map<std::string, std::string>::const_iterator i = v.find(k); return i == v.end() ? "" : i->second; }
};
bool eval(const std::string& rule, const MapContext& c) { Expression* e = ExpressionParser::parse(rule); bool r = e->evaluate(c); delete e; return r; }
}

TEST_CASE("Interpreter: comparisons treat numbers as numbers and text as text") {
    MapContext c; c.v["amount"] = "9"; c.v["type"] = "merit"; c.v["limit"] = "10";
    CHECK(eval("amount < 10", c));            // numeric, not "9" < "10" as text
    CHECK(eval("amount < $limit", c));
    CHECK(eval("type == merit", c));
    CHECK(eval("type != need", c));
    CHECK_FALSE(eval("amount >= 10", c));
}
TEST_CASE("Interpreter: and / or / not / parentheses and outcome()") {
    MapContext c; c.v["a"] = "1"; c.v["b"] = "2"; c.v["outcome:Review claim and decide"] = "REPAIR";
    CHECK(eval("a == 1 and b == 2", c));
    CHECK(eval("a == 9 or b == 2", c));
    CHECK(eval("not a == 9", c));
    CHECK(eval("(a == 9 or a == 1) and b == 2", c));
    CHECK(eval("outcome('Review claim and decide') == REPAIR", c));
    CHECK_FALSE(eval("outcome('Review claim and decide') == WRITE_OFF", c));
    CHECK(eval("missing == ''", c));          // unknown keys read as empty text
}
TEST_CASE("Interpreter: syntax errors are reported, not crashed on") {
    CHECK_THROWS_AS(ExpressionParser::parse(""), std::invalid_argument);
    CHECK_THROWS_AS(ExpressionParser::parse("a ="), std::invalid_argument);
    CHECK_THROWS_AS(ExpressionParser::parse("a == 1 and"), std::invalid_argument);
    CHECK_THROWS_AS(ExpressionParser::parse("(a == 1"), std::invalid_argument);
    CHECK_THROWS_AS(ExpressionParser::parse("a == 'open"), std::invalid_argument);
}
TEST_CASE("Interpreter: clone and toString round-trip") {
    Expression* e = ExpressionParser::parse("a == 1 and not b > 3");
    Expression* copy = e->clone();
    MapContext c; c.v["a"] = "1"; c.v["b"] = "2";
    CHECK(copy->evaluate(c));
    CHECK(copy->toString() == e->toString());
    delete e; delete copy;
}
TEST_CASE("ApprovalRouter: level by amount, climb on timeout, top of chain") {
    ApprovalRouter* r = ApprovalRouter::parse("Handler=20000;Manager=100000;Senior=*");
    std::string note;
    CHECK(r->route(15000, -1, note) == 0);
    CHECK(r->route(20000, -1, note) == 0);    // limit is inclusive
    CHECK(r->route(20001, -1, note) == 1);
    CHECK(r->route(5e9, -1, note) == 2);
    CHECK(r->route(15000, 0, note) == 1);     // handler timed out: next level, fresh deadline
    CHECK(note.find("Handler escalated") != std::string::npos);
    CHECK(r->route(15000, 2, note) == -1);    // nobody above the top
    delete r;
}
TEST_CASE("ApprovalRouter: bad configuration is rejected") {
    CHECK_THROWS_AS(ApprovalRouter::parse(""), std::invalid_argument);
    CHECK_THROWS_AS(ApprovalRouter::parse("Handler"), std::invalid_argument);
    CHECK_THROWS_AS(ApprovalRouter::parse("Handler=lots"), std::invalid_argument);
}
