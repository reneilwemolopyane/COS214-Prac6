#include "doctest.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/scenarios/MotorClaimWorkflowDirector.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {
void collectItems(const WorkflowComponent* component, std::vector<const WorkItem*>& items) {
    const Stage* stage = dynamic_cast<const Stage*>(component);
    if (stage) {
        for (std::size_t i = 0; i < stage->childCount(); ++i)
            collectItems(stage->childAt(i), items);
        return;
    }
    const WorkItem* item = dynamic_cast<const WorkItem*>(component);
    if (item) items.push_back(item);
}
}

TEST_CASE("motor claim director builds a validated repair-and-settlement workflow") {
    ValidatingWorkflowBuilder builder;
    MotorClaimWorkflowDirector director;
    director.construct(builder);
    WorkflowDefinition* definition = builder.getResult();

    REQUIRE(definition != nullptr);
    CHECK(definition->getId() == "motor-claim");
    CHECK(definition->getVersion() == 1);
    CHECK(definition->isValidated());

    std::vector<const WorkItem*> items;
    collectItems(definition->getRoot(), items);
    CHECK(items.size() == 16);

    std::set<std::string> ids;
    for (std::size_t i = 0; i < items.size(); ++i) ids.insert(items[i]->getId());
    CHECK(ids.count("Lodge claim") == 1);
    CHECK(ids.count("Screen claim for fraud") == 1);
    CHECK(ids.count("Inspect completed repairs") == 1);
    CHECK(ids.count("Release settlement payment") == 1);

    delete definition;
}

TEST_CASE("motor claim definition joins parallel checks before decision and payment follows inspection") {
    ValidatingWorkflowBuilder builder;
    MotorClaimWorkflowDirector director;
    director.construct(builder);
    WorkflowDefinition* definition = builder.getResult();
    const std::vector<std::pair<std::string, std::string> >& deps = definition->getDependencies();

    const auto hasDependency = [&deps](const std::string& from, const std::string& to) {
        for (std::size_t i = 0; i < deps.size(); ++i)
            if (deps[i].first == from && deps[i].second == to) return true;
        return false;
    };

    CHECK(hasDependency("Collect claim documents", "Verify police report"));
    CHECK(hasDependency("Collect claim documents", "Assess vehicle damage"));
    CHECK(hasDependency("Collect claim documents", "Obtain repair estimate"));
    CHECK(hasDependency("Collect claim documents", "Screen claim for fraud"));
    CHECK(hasDependency("Screen claim for fraud", "Review claim and decide"));
    CHECK(hasDependency("Inspect completed repairs", "Release settlement payment"));

    CHECK(definition->getRules().at("approval.handlerLimitZAR") == "20000");
    CHECK(definition->getRules().at("documents.missingEscalationDays") == "7");
    CHECK(definition->getRules().at("payment.requiresPassedFinalInspection") == "true");
    delete definition;
}
