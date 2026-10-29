cmake_minimum_required(VERSION 3.10)

project(TividyUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

set(DOCTEST_DIR ${CMAKE_SOURCE_DIR}/third_party/doctest)

# ===== Facade / Builder unit tests (isolated with fakes) =====
add_executable(FacadeBuilderUnitTests
    tests/test_main.cpp
    tests/test_builders.cpp
    tests/test_definition.cpp
    tests/test_director.cpp
    tests/test_facade.cpp
    tests/test_repository.cpp

    tividy/builder/WorkflowBuilder.cpp
    tividy/builder/WorkflowDefinition.cpp
    tividy/builder/DraftWorkflowBuilder.cpp
    tividy/builder/ValidatingWorkflowBuilder.cpp
    tividy/builder/WorkflowDirector.cpp
    tividy/facade/DefinitionRepository.cpp
    tividy/facade/WorkflowFacade.cpp
)

target_include_directories(FacadeBuilderUnitTests BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/fakes
)

target_include_directories(FacadeBuilderUnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME FacadeBuilderUnitTests COMMAND FacadeBuilderUnitTests)

# ===== State + Composite tests =====
add_executable(StateCompositeTests
    tests/StateCompositeTests.cpp
    tividy/composite_prototype/Stage.cpp
    tividy/composite_prototype/WorkItem.cpp
    tividy/state/WorkItemState.cpp
    tividy/state/States.cpp
    tividy/core/Participant.cpp
)

target_include_directories(StateCompositeTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tividy
    ${DOCTEST_DIR}
)

add_test(NAME StateCompositeTests COMMAND StateCompositeTests)

# ===== Chain of Responsibility + Decorator tests =====
set(CD_TESTS ${CMAKE_SOURCE_DIR}/tests/chain_decorator)

add_executable(ChainDecoratorTests
    tests/chain_decorator/test_main.cpp
    tests/chain_decorator/test_approval.cpp
    tests/chain_decorator/test_decorators.cpp
    tividy/chain_of_responsibility/ApprovalHandler.cpp
    tividy/chain_of_responsibility/LevelApprover.cpp
    tividy/decorator/WorkItemDecorator.cpp
    tividy/decorator/ValidationDecorator.cpp
    tividy/decorator/PriorityDecorator.cpp
    tividy/decorator/SecurityDecorator.cpp
    tividy/decorator/AuditDecorator.cpp
    tividy/decorator/NotificationDecorator.cpp
    tividy/decorator/DeadlineDecorator.cpp
    tividy/mediator/WorkflowMediator.cpp
)

target_include_directories(ChainDecoratorTests PRIVATE
    ${CD_TESTS}
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/tividy/chain_of_responsibility
    ${CMAKE_SOURCE_DIR}/tividy/decorator
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME ChainDecoratorTests COMMAND ChainDecoratorTests)

# ===== Integration tests: Facade + Builder + Prototype + Memento =====
add_executable(IntegrationTests
    IntegrationTests/main.cpp
    IntegrationTests/FacadeBuilderPrototypeMementoTests.cpp

    tividy/builder/WorkflowBuilder.cpp
    tividy/builder/WorkflowDefinition.cpp
    tividy/builder/DraftWorkflowBuilder.cpp
    tividy/builder/ValidatingWorkflowBuilder.cpp
    tividy/builder/WorkflowDirector.cpp
    tividy/facade/DefinitionRepository.cpp
    tividy/facade/WorkflowFacade.cpp
    tividy/command/Command.cpp
    tividy/command/CommandProcessor.cpp
    tividy/command/WorkflowEngine.cpp
    tividy/core/WorkflowInstance.cpp
    tividy/core/Participant.cpp
    tividy/composite_prototype/Stage.cpp
    tividy/composite_prototype/WorkItem.cpp
    tividy/state/WorkItemState.cpp
    tividy/state/States.cpp
    tividy/mediator/WorkflowMediator.cpp
    tividy/mediator/InstanceCoordinator.cpp
    tividy/strategy/AssignmentStrategy.cpp
    tividy/strategy/WorkAssigner.cpp
    tividy/observer/ObserverPattern.cpp
    tividy/observer/Observers.cpp
    tividy/memento/Memento.cpp
    tividy/memento/ProcessDataStore.cpp
)

target_include_directories(IntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${DOCTEST_DIR}
)

add_test(NAME IntegrationTests COMMAND IntegrationTests)

# ===== Pattern integration tests (Composite + Decorator + Chain + Visitor) =====
add_executable(PatternIntegrationTests
    tests/chain_decorator/test_main.cpp          # reuses the doctest main
    tests/integration/test_pattern_integration.cpp

    # Composite + State
    tividy/composite_prototype/Stage.cpp
    tividy/composite_prototype/WorkItem.cpp
    tividy/state/WorkItemState.cpp
    tividy/state/States.cpp
    tividy/core/Participant.cpp
    tividy/mediator/WorkflowMediator.cpp

    # Chain of Responsibility
    tividy/chain_of_responsibility/ApprovalHandler.cpp
    tividy/chain_of_responsibility/LevelApprover.cpp

    # Decorator
    tividy/decorator/WorkItemDecorator.cpp
    tividy/decorator/ValidationDecorator.cpp
    tividy/decorator/PriorityDecorator.cpp
    tividy/decorator/SecurityDecorator.cpp
    tividy/decorator/AuditDecorator.cpp
    tividy/decorator/NotificationDecorator.cpp
    tividy/decorator/DeadlineDecorator.cpp
    tividy/decorator/ApprovalDecorator.cpp

    # Visitor
    tividy/visitor/BottleneckVisitor.cpp
    tividy/memento/ProcessDataStore.cpp
    tividy/memento/Memento.cpp
)

target_include_directories(PatternIntegrationTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tividy
    ${CMAKE_SOURCE_DIR}/third_party
    ${DOCTEST_DIR}
)

add_test(NAME PatternIntegrationTests COMMAND PatternIntegrationTests)

# ===== Command + Observer tests =====
add_subdirectory(tests/command_observer)

# ===== Mediator + Strategy + State tests =====
add_executable(MediatorStrategyStateTests
    tests/test_mediator_strategy_state.cpp
    tividy/mediator/InstanceCoordinator.cpp
    tividy/mediator/WorkflowMediator.cpp
    tividy/strategy/AssignmentStrategy.cpp
    tividy/strategy/WorkAssigner.cpp
    tividy/composite_prototype/WorkItem.cpp
    tividy/composite_prototype/Stage.cpp
    tividy/state/WorkItemState.cpp
    tividy/state/States.cpp
    tividy/core/Participant.cpp
    tividy/observer/ObserverPattern.cpp 


)
target_include_directories(MediatorStrategyStateTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tividy
    ${DOCTEST_DIR}
)
add_test(NAME MediatorStrategyStateTests COMMAND MediatorStrategyStateTests)