#include "tividy/interpreter/Expression.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {

bool toNumber(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = 0;
    out = std::strtod(s.c_str(), &end);
    return end && *end == '\0';
}

enum TokKind { T_WORD, T_STRING, T_REF, T_OP, T_LP, T_RP, T_END };

struct Token { TokKind kind; std::string text; };

std::vector<Token> tokenize(const std::string& in) {
    std::vector<Token> out;
    std::size_t i = 0;
    while (i < in.size()) {
        char c = in[i];
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        Token t;
        if (c == '(') { t.kind = T_LP; t.text = "("; ++i; }
        else if (c == ')') { t.kind = T_RP; t.text = ")"; ++i; }
        else if (c == '\'' || c == '"') {
            std::size_t j = in.find(c, i + 1);
            if (j == std::string::npos) throw std::invalid_argument("unterminated quote in rule: " + in);
            t.kind = T_STRING; t.text = in.substr(i + 1, j - i - 1); i = j + 1;
        } else if (c == '=' || c == '!' || c == '<' || c == '>') {
            std::string op(1, c);
            if (i + 1 < in.size() && in[i + 1] == '=') { op += '='; ++i; }
            ++i;
            if (op == "=" || op == "!") throw std::invalid_argument("unknown operator '" + op + "' in rule: " + in);
            t.kind = T_OP; t.text = op;
        } else {
            bool ref = (c == '$');
            if (ref) ++i;
            std::size_t j = i;
            while (j < in.size() && !std::isspace(static_cast<unsigned char>(in[j])) &&
                   in[j] != '(' && in[j] != ')' && in[j] != '=' && in[j] != '!' &&
                   in[j] != '<' && in[j] != '>' && in[j] != '\'' && in[j] != '"') ++j;
            if (j == i) throw std::invalid_argument("unexpected character in rule: " + in);
            t.kind = ref ? T_REF : T_WORD; t.text = in.substr(i, j - i); i = j;
        }
        out.push_back(t);
    }
    Token end; end.kind = T_END; out.push_back(end);
    return out;
}

std::string lower(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i) s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    return s;
}

class Parser {
public:
    explicit Parser(const std::string& text) : src(text), toks(tokenize(text)), pos(0) {}
    Expression* run() {
        Expression* e = parseOr();
        if (toks[pos].kind != T_END) { delete e; fail("unexpected '" + toks[pos].text + "'"); }
        return e;
    }
private:
    std::string src; std::vector<Token> toks; std::size_t pos;
    void fail(const std::string& why) { throw std::invalid_argument("bad rule \"" + src + "\": " + why); }
    bool isWord(const char* w) const { return toks[pos].kind == T_WORD && lower(toks[pos].text) == w; }

    Expression* parseOr() {
        Expression* left = parseAnd();
        while (isWord("or")) { ++pos; Expression* r = 0;
            try { r = parseAnd(); } catch (...) { delete left; throw; }
            left = new OrExpression(left, r); }
        return left;
    }
    Expression* parseAnd() {
        Expression* left = parseNot();
        while (isWord("and")) { ++pos; Expression* r = 0;
            try { r = parseNot(); } catch (...) { delete left; throw; }
            left = new AndExpression(left, r); }
        return left;
    }
    Expression* parseNot() {
        if (isWord("not")) { ++pos; return new NotExpression(parseNot()); }
        return parsePrimary();
    }
    Expression* parsePrimary() {
        if (toks[pos].kind == T_LP) {
            ++pos; Expression* e = parseOr();
            if (toks[pos].kind != T_RP) { delete e; fail("missing ')'"); }
            ++pos; return e;
        }
        return parseComparison();
    }
    std::string parseKey() {
        const Token& t = toks[pos];
        if (t.kind == T_WORD && lower(t.text) == "outcome" && toks[pos + 1].kind == T_LP) {
            pos += 2;
            if (toks[pos].kind != T_STRING) fail("outcome(...) needs a quoted work-item name");
            std::string name = toks[pos++].text;
            if (toks[pos].kind != T_RP) fail("missing ')' after outcome(...)");
            ++pos;
            return "outcome:" + name;
        }
        if (t.kind == T_WORD || t.kind == T_STRING) { ++pos; return t.text; }
        fail("expected a value name, found '" + t.text + "'");
        return "";
    }
    Expression* parseComparison() {
        std::string key = parseKey();
        if (toks[pos].kind != T_OP) fail("expected a comparison operator after '" + key + "'");
        std::string o = toks[pos++].text;
        ComparisonExpression::Op op = ComparisonExpression::EQ;
        if (o == "==") op = ComparisonExpression::EQ; else if (o == "!=") op = ComparisonExpression::NE;
        else if (o == "<") op = ComparisonExpression::LT; else if (o == "<=") op = ComparisonExpression::LE;
        else if (o == ">") op = ComparisonExpression::GT; else op = ComparisonExpression::GE;
        const Token& v = toks[pos];
        if (v.kind != T_WORD && v.kind != T_STRING && v.kind != T_REF) fail("expected a value after '" + o + "'");
        ++pos;
        return new ComparisonExpression(key, op, v.text, v.kind == T_REF);
    }
};

} // namespace

// ---- ComparisonExpression ----
ComparisonExpression::ComparisonExpression(const std::string& k, Op o, const std::string& r, bool ref)
    : key(k), op(o), rhs(r), rhsIsReference(ref) {}

bool ComparisonExpression::evaluate(const EvaluationContext& c) const {
    std::string a = c.lookup(key);
    std::string b = rhsIsReference ? c.lookup(rhs) : rhs;
    double x = 0, y = 0;
    int cmp;
    if (toNumber(a, x) && toNumber(b, y)) cmp = (x < y) ? -1 : (x > y ? 1 : 0);   // numbers compare as numbers
    else cmp = a.compare(b);
    switch (op) {
        case EQ: return cmp == 0;
        case NE: return cmp != 0;
        case LT: return cmp < 0;
        case LE: return cmp <= 0;
        case GT: return cmp > 0;
        case GE: return cmp >= 0;
    }
    return false;
}
Expression* ComparisonExpression::clone() const { return new ComparisonExpression(key, op, rhs, rhsIsReference); }
std::string ComparisonExpression::toString() const {
    static const char* names[] = { "==", "!=", "<", "<=", ">", ">=" };
    return key + " " + names[op] + " " + (rhsIsReference ? "$" : "") + rhs;
}

// ---- And / Or / Not ----
AndExpression::AndExpression(Expression* l, Expression* r) : left(l), right(r) {}
AndExpression::~AndExpression() { delete left; delete right; }
bool AndExpression::evaluate(const EvaluationContext& c) const { return left->evaluate(c) && right->evaluate(c); }
Expression* AndExpression::clone() const { return new AndExpression(left->clone(), right->clone()); }
std::string AndExpression::toString() const { return "(" + left->toString() + " and " + right->toString() + ")"; }

OrExpression::OrExpression(Expression* l, Expression* r) : left(l), right(r) {}
OrExpression::~OrExpression() { delete left; delete right; }
bool OrExpression::evaluate(const EvaluationContext& c) const { return left->evaluate(c) || right->evaluate(c); }
Expression* OrExpression::clone() const { return new OrExpression(left->clone(), right->clone()); }
std::string OrExpression::toString() const { return "(" + left->toString() + " or " + right->toString() + ")"; }

NotExpression::NotExpression(Expression* e) : inner(e) {}
NotExpression::~NotExpression() { delete inner; }
bool NotExpression::evaluate(const EvaluationContext& c) const { return !inner->evaluate(c); }
Expression* NotExpression::clone() const { return new NotExpression(inner->clone()); }
std::string NotExpression::toString() const { return "not " + inner->toString(); }

Expression* ExpressionParser::parse(const std::string& text) {
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) throw std::invalid_argument("rule is empty");
    return Parser(text).run();
}
