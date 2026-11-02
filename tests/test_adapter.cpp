//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../adapter/ConnectorResult.h"
#include "../adapter/ExternalConnector.h"
#include "../adapter/ConnectorRegistry.h"
#include "../adapter/PaymentAdapter.h"
#include "../adapter/MessageAdapter.h"
#include "../adapter/SchedulingAdapter.h"
#include "../adapter/DocumentStorageAdapter.h"
#include "../adapter/RecordLookupAdapter.h"
static ConnectorRequest req(const std::string& op) { ConnectorRequest r; r.operation = op; return r; }

TEST_CASE("Registry finds a registered connector and returns null for unknown names") {
    PaymentAPI api; 
    PaymentAdapter pay("pay", &api); 
    ConnectorRegistry reg;
    
    reg.add("payments", &pay);
    
    ExternalConnector* found = reg.find("payments");
    ExternalConnector* missing = reg.find("missing");
    
    CHECK(found == &pay);
    CHECK(missing == nullptr); // or == 0
}


TEST_CASE("PaymentAdapter pays and returns a reference") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest r = req("pay"); r.params["amount"] = "50000"; r.params["account"] = "ACC1";
    ConnectorResult res = p.execute(r);
    CHECK(res.success);
    CHECK(res.data["reference"] != "");
}

TEST_CASE("PaymentAdapter marks gateway downtime as retryable") {
    PaymentAPI api; api.setDown(true); PaymentAdapter p("pay", &api);
    ConnectorRequest r = req("pay"); r.params["amount"] = "100"; r.params["account"] = "A";
    ConnectorResult res = p.execute(r);
    CHECK(!res.success);
    CHECK(res.retryable);
}

TEST_CASE("PaymentAdapter rejects invalid input permanently") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest r = req("pay"); r.params["amount"] = "abc"; r.params["account"] = "A";
    ConnectorResult res = p.execute(r);
    CHECK(!res.success);
    CHECK(!res.retryable);
    CHECK(!p.execute(req("nonsense")).success);
}

TEST_CASE("PaymentAdapter reverses a real payment exactly once") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest pay = req("pay"); pay.params["amount"] = "500"; pay.params["account"] = "A";
    ConnectorRequest r = req("reverse"); r.params["reference"] = p.execute(pay).data["reference"];
    CHECK(p.execute(r).success);
    ConnectorResult again = p.execute(r);
    CHECK(!again.success);
    CHECK(!again.retryable);
}

TEST_CASE("MessagingAdapter sends, and flags an outage as retryable") {
    MessagingAPI api; MessagingAdapter m("msg", &api);
    ConnectorRequest r = req("send"); r.params["to"] = "0821234567"; r.params["text"] = "hi";
    CHECK(m.execute(r).success);
    api.setDown(true);
    ConnectorResult res = m.execute(r);
    CHECK(!res.success);
    CHECK(res.retryable);
}


// ---------------- SchedulingAdapter ----------------
TEST_CASE("SchedulingAdapter finds the first free slot and books it") {
    SchedulingAPI api; SchedulingAdapter s("cal", &api);
    ConnectorRequest f = req("findSlot"); f.params["resource"] = "assessor-1"; f.params["date"] = "2026-11-02";
    ConnectorResult found = s.execute(f);
    REQUIRE(found.success);
    CHECK(found.data["slot"] == "09:00");

    ConnectorRequest b = req("book"); b.params = f.params; b.params["slot"] = found.data["slot"];
    ConnectorResult booked = s.execute(b);
    CHECK(booked.success);
    CHECK(booked.data["bookingRef"] != "");
}
TEST_CASE("SchedulingAdapter refuses a double booking permanently") {
    SchedulingAPI api; SchedulingAdapter s("cal", &api);
    ConnectorRequest b = req("book"); b.params["resource"] = "r"; b.params["date"] = "d"; b.params["slot"] = "09:00";
    CHECK(s.execute(b).success);
    ConnectorResult again = s.execute(b);
    CHECK(!again.success);
    CHECK(!again.retryable);
}
TEST_CASE("SchedulingAdapter frees the slot again after a cancel") {
    SchedulingAPI api; SchedulingAdapter s("cal", &api);
    ConnectorRequest b = req("book"); b.params["resource"] = "r"; b.params["date"] = "d"; b.params["slot"] = "09:00";
    std::string ref = s.execute(b).data["bookingRef"];
    ConnectorRequest c = req("cancel"); c.params["bookingRef"] = ref;
    CHECK(s.execute(c).success);
    CHECK(!s.execute(c).success);          // already cancelled
    CHECK(s.execute(b).success);           // slot is free again
}
TEST_CASE("SchedulingAdapter marks an outage retryable and bad input permanent") {
    SchedulingAPI api; SchedulingAdapter s("cal", &api);
    ConnectorRequest f = req("findSlot"); f.params["resource"] = "r"; f.params["date"] = "d";
    CHECK(!s.execute(req("findSlot")).success);       // missing params
    CHECK(!s.execute(req("nonsense")).success);       // unknown operation
    CHECK(!s.execute(req("nonsense")).retryable);
    api.setDown(true);
    ConnectorResult res = s.execute(f);
    CHECK(!res.success);
    CHECK(res.retryable);
}

// ---------------- DocumentStorageAdapter ----------------
TEST_CASE("DocumentStorageAdapter stores a document and fetches it back") {
    DocumentStoreAPI api; DocumentStorageAdapter d("docs", &api);
    ConnectorRequest st = req("store");
    st.params["name"] = "licence.pdf"; st.params["contentType"] = "application/pdf"; st.params["content"] = "data";
    ConnectorResult stored = d.execute(st);
    REQUIRE(stored.success);
    ConnectorRequest fe = req("fetch"); fe.params["documentId"] = stored.data["documentId"];
    ConnectorResult fetched = d.execute(fe);
    CHECK(fetched.success);
    CHECK(fetched.data["content"] == "data");
}
TEST_CASE("DocumentStorageAdapter rejects invalid uploads permanently") {
    DocumentStoreAPI api; DocumentStorageAdapter d("docs", &api);
    ConnectorRequest st = req("store"); st.params["content"] = "data";          // no name
    ConnectorResult res = d.execute(st);
    CHECK(!res.success);
    CHECK(!res.retryable);
    st.params["name"] = "a.txt"; st.params["content"] = "";                        // empty content
    CHECK(!d.execute(st).success);
}
TEST_CASE("DocumentStorageAdapter deletes a document and reports a missing one") {
    DocumentStoreAPI api; DocumentStorageAdapter d("docs", &api);
    ConnectorRequest st = req("store"); st.params["name"] = "a"; st.params["content"] = "x";
    std::string id = d.execute(st).data["documentId"];
    ConnectorRequest de = req("delete"); de.params["documentId"] = id;
    CHECK(d.execute(de).success);
    CHECK(!d.execute(de).success);
    ConnectorRequest fe = req("fetch"); fe.params["documentId"] = id;
    ConnectorResult res = d.execute(fe);
    CHECK(!res.success);
    CHECK(!res.retryable);
}
TEST_CASE("DocumentStorageAdapter marks an outage retryable and recovers") {
    DocumentStoreAPI api; DocumentStorageAdapter d("docs", &api);
    ConnectorRequest st = req("store"); st.params["name"] = "a"; st.params["content"] = "x";
    api.setDown(true);
    ConnectorResult res = d.execute(st);
    CHECK(!res.success);
    CHECK(res.retryable);
    api.setDown(false);
    CHECK(d.execute(st).success);
}

// ---------------- Registry with the new adapters ----------------
TEST_CASE("Registry holds the scheduling and document adapters by name") {
    SchedulingAPI cal; DocumentStoreAPI store;
    SchedulingAdapter s("calendar", &cal); DocumentStorageAdapter d("documents", &store);
    ConnectorRegistry reg;
    reg.add("scheduling", &s); reg.add("documents", &d);
    ExternalConnector* a = reg.find("scheduling");
    ExternalConnector* b = reg.find("documents");
    CHECK(a == &s);
    CHECK(b == &d);
    CHECK(a->name() == "calendar");
}

// ---------------- extra edge cases ----------------
TEST_CASE("PaymentAdapter marks a failed reversal during an outage as retryable") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest pay = req("pay"); pay.params["amount"] = "500"; pay.params["account"] = "A";
    ConnectorRequest r = req("reverse"); r.params["reference"] = p.execute(pay).data["reference"];
    api.setDown(true);
    ConnectorResult res = p.execute(r);
    CHECK(!res.success);
    CHECK(res.retryable);
    api.setDown(false);
    CHECK(p.execute(r).success);                      // works again once the gateway is back
}

TEST_CASE("PaymentAdapter rejects a reversal with no reference permanently") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorResult res = p.execute(req("reverse"));
    CHECK(!res.success);
    CHECK(!res.retryable);
}

TEST_CASE("MessagingAdapter rejects a message with no recipient permanently") {
    MessagingAPI api; MessagingAdapter m("msg", &api);
    ConnectorRequest r = req("send"); r.params["text"] = "hi";   // no "to"
    ConnectorResult res = m.execute(r);
    CHECK(!res.success);
    CHECK(!res.retryable);
    CHECK(!m.execute(req("nonsense")).success);
}

TEST_CASE("Registry replaces a connector registered under the same name") {
    PaymentAPI a1, a2;
    PaymentAdapter first("first", &a1), second("second", &a2);
    ConnectorRegistry reg;
    reg.add("payments", &first);
    reg.add("payments", &second);
    ExternalConnector* found = reg.find("payments");
    CHECK(found == &second);
    CHECK(found->name() == "second");
}

// ---------------- added: null adaptee, unknown ids, polymorphism, registry execute ----------------
TEST_CASE("Every adapter fails permanently when it has no adaptee") {
    PaymentAdapter p("p", 0); MessagingAdapter m("m", 0); SchedulingAdapter s("s", 0); DocumentStorageAdapter d("d", 0);
    ExternalConnector* all[4] = { &p, &m, &s, &d };
    for (int i = 0; i < 4; ++i) {
        ConnectorResult res = all[i]->execute(req("anything"));
        CHECK(!res.success);
        CHECK(!res.retryable);
    }
}
TEST_CASE("PaymentAdapter refuses to reverse an unknown reference permanently") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest r = req("reverse"); r.params["reference"] = "TXN-999";
    ConnectorResult res = p.execute(r);
    CHECK(!res.success);
    CHECK(!res.retryable);
}
TEST_CASE("PaymentAdapter gives each payment its own reference") {
    PaymentAPI api; PaymentAdapter p("pay", &api);
    ConnectorRequest r = req("pay"); r.params["amount"] = "100"; r.params["account"] = "A";
    CHECK(p.execute(r).data["reference"] != p.execute(r).data["reference"]);
}
TEST_CASE("MessagingAdapter rejects empty text and reports an outage as retryable") {
    MessagingAPI api; MessagingAdapter m("msg", &api);
    ConnectorRequest r = req("send"); r.params["to"] = "082";
    CHECK(!m.execute(r).success);                     // no text
    CHECK(!m.execute(r).retryable);
    r.params["text"] = "hi"; api.setDown(true);
    CHECK(m.execute(r).retryable);
    api.setDown(false);
    CHECK(m.execute(r).success);
}
TEST_CASE("Error messages name the connector that failed") {
    PaymentAPI api; PaymentAdapter p("bank-gateway", &api);
    ConnectorResult res = p.execute(req("nonsense"));
    CHECK(res.message.find("bank-gateway") != std::string::npos);
}
TEST_CASE("Adapters behave polymorphically through ExternalConnector") {
    PaymentAPI api; MessagingAPI mapi; PaymentAdapter p("pay", &api); MessagingAdapter m("msg", &mapi);
    ExternalConnector* c1 = &p; ExternalConnector* c2 = &m;
    CHECK(c1->name() == "pay");
    CHECK(c2->name() == "msg");
    ConnectorRequest send = req("send"); send.params["to"] = "1"; send.params["text"] = "x";
    CHECK(c2->execute(send).success);
    CHECK(!c1->execute(send).success);                // payment does not understand "send"
}
TEST_CASE("Registry execute reports an unknown connector as a failure, not a crash") {
    ConnectorRegistry reg;
    ConnectorResult res = reg.execute("nope", req("pay"));
    CHECK(!res.success);
    CHECK(!res.retryable);
    CHECK(res.message.find("nope") != std::string::npos);
}
TEST_CASE("Registry execute reaches the right adapter") {
    MessagingAPI api; MessagingAdapter m("msg", &api); ConnectorRegistry reg; reg.add("sms", &m);
    ConnectorRequest r = req("send"); r.params["to"] = "1"; r.params["text"] = "x";
    CHECK(reg.execute("sms", r).success);
}
TEST_CASE("Two adapters of the same kind are isolated from each other") {
    SchedulingAPI a, b; SchedulingAdapter sa("a", &a), sb("b", &b);
    ConnectorRequest bk = req("book"); bk.params["resource"] = "r"; bk.params["date"] = "d"; bk.params["slot"] = "09:00";
    CHECK(sa.execute(bk).success);
    CHECK(sb.execute(bk).success);                    // different calendar, so not a double booking
}

// RecordLookupAdapter tests (doctest). main() lives in tests/test_main.cpp.


static ConnectorRequest lreq(const std::string& op, const std::string& registry = "", const std::string& key = "") {
    ConnectorRequest r; r.operation = op;
    if (!registry.empty()) r.params["registry"] = registry;
    if (!key.empty()) r.params["key"] = key;
    return r;
}

TEST_CASE("RecordLookupAdapter verifies an active record as valid") {
    RecordLookupAPI api; api.addRecord("policies", "POL-1", "comprehensive");
    RecordLookupAdapter a("lookup", &api);
    ConnectorResult res = a.execute(lreq("verify", "policies", "POL-1"));
    CHECK(res.success);
    CHECK(res.data["valid"] == "true");
    CHECK(res.data["value"] == "comprehensive");
}
TEST_CASE("RecordLookupAdapter reports an inactive record as not valid, without failing") {
    RecordLookupAPI api; api.addRecord("policies", "POL-2", "comprehensive", false);   // e.g. lapsed
    RecordLookupAdapter a("lookup", &api);
    ConnectorResult res = a.execute(lreq("verify", "policies", "POL-2"));
    CHECK(res.success);                       // the check itself worked
    CHECK(res.data["valid"] == "false");
}
TEST_CASE("RecordLookupAdapter reports an unknown record as not valid, without failing") {
    RecordLookupAPI api; RecordLookupAdapter a("lookup", &api);
    ConnectorResult res = a.execute(lreq("verify", "policies", "NOPE"));
    CHECK(res.success);
    CHECK(res.data["valid"] == "false");
    CHECK(res.data.count("value") == 0);
}
TEST_CASE("RecordLookupAdapter looks up a stored value and its active flag") {
    RecordLookupAPI api; api.addRecord("results", "STU-9", "78%", true);
    RecordLookupAdapter a("lookup", &api);
    ConnectorResult res = a.execute(lreq("lookup", "results", "STU-9"));
    CHECK(res.success);
    CHECK(res.data["value"] == "78%");
    CHECK(res.data["active"] == "true");
}
TEST_CASE("RecordLookupAdapter fails permanently when a looked-up record does not exist") {
    RecordLookupAPI api; RecordLookupAdapter a("lookup", &api);
    ConnectorResult res = a.execute(lreq("lookup", "results", "STU-404"));
    CHECK(!res.success);
    CHECK(!res.retryable);
}
TEST_CASE("RecordLookupAdapter keeps registries separate") {
    RecordLookupAPI api; api.addRecord("policies", "X1", "policy-data"); RecordLookupAdapter a("lookup", &api);
    CHECK(a.execute(lreq("verify", "policies", "X1")).data["valid"] == "true");
    CHECK(a.execute(lreq("verify", "results", "X1")).data["valid"] == "false");   // same key, other registry
}
TEST_CASE("RecordLookupAdapter rejects missing parameters and unknown operations permanently") {
    RecordLookupAPI api; RecordLookupAdapter a("lookup", &api);
    ConnectorResult noKey = a.execute(lreq("verify", "policies"));
    CHECK(!noKey.success);
    CHECK(!noKey.retryable);
    CHECK(!a.execute(lreq("verify", "", "K")).success);
    ConnectorResult bad = a.execute(lreq("nonsense", "policies", "K"));
    CHECK(!bad.success);
    CHECK(!bad.retryable);
}
TEST_CASE("RecordLookupAdapter marks an outage retryable and recovers") {
    RecordLookupAPI api; api.addRecord("policies", "POL-1", "v"); RecordLookupAdapter a("lookup", &api);
    api.setDown(true);
    ConnectorResult res = a.execute(lreq("verify", "policies", "POL-1"));
    CHECK(!res.success);
    CHECK(res.retryable);
    api.setDown(false);
    CHECK(a.execute(lreq("verify", "policies", "POL-1")).success);
}
TEST_CASE("RecordLookupAdapter reports an invalid request as permanent even during an outage") {
    RecordLookupAPI api; RecordLookupAdapter a("lookup", &api);
    api.setDown(true);
    ConnectorResult res = a.execute(lreq("verify"));       // no registry, no key
    CHECK(!res.success);
    CHECK(!res.retryable);
}
TEST_CASE("RecordLookupAdapter without an adaptee fails permanently") {
    RecordLookupAdapter a("lookup", 0);
    ConnectorResult res = a.execute(lreq("verify", "r", "k"));
    CHECK(!res.success);
    CHECK(!res.retryable);
}
TEST_CASE("RecordLookupAdapter error messages name the connector") {
    RecordLookupAPI api; RecordLookupAdapter a("police-check", &api);
    CHECK(a.execute(lreq("nonsense", "r", "k")).message.find("police-check") != std::string::npos);
}
TEST_CASE("One adapter class serves two industries through the registry") {
    RecordLookupAPI policyAdmin, schoolRecords;
    policyAdmin.addRecord("policies", "POL-1", "active cover");
    schoolRecords.addRecord("results", "STU-9", "78%");
    RecordLookupAdapter policy("policy-admin", &policyAdmin), results("results-check", &schoolRecords);
    ConnectorRegistry reg;
    reg.add("policy-admin", &policy); reg.add("results-check", &results);

    CHECK(reg.execute("policy-admin", lreq("verify", "policies", "POL-1")).data["valid"] == "true");
    CHECK(reg.execute("results-check", lreq("lookup", "results", "STU-9")).data["value"] == "78%");
    CHECK(reg.execute("results-check", lreq("verify", "policies", "POL-1")).data["valid"] == "false");   // isolated
}
TEST_CASE("RecordLookupAdapter behaves polymorphically next to another adapter") {
    RecordLookupAPI api; PaymentAPI bank; api.addRecord("r", "k", "v");
    RecordLookupAdapter l("lookup", &api); PaymentAdapter p("pay", &bank);
    ExternalConnector* all[2] = { &l, &p };
    CHECK(all[0]->execute(lreq("verify", "r", "k")).success);
    CHECK(!all[1]->execute(lreq("verify", "r", "k")).success);   // payment does not understand "verify"
}