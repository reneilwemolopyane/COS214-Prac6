cmake_minimum_required(VERSION 3.10)

project(TividyUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

set(DOCTEST_DIR ${CMAKE_SOURCE_DIR}/third_party/doctest)

# ===== Facade / Builder tests =====
add_executable(WorkflowBuilderTests
    Facade_BuilderTests/main.cpp
    Facade_BuilderTests/WorkflowDefinitionTests.cpp
    Facade_BuilderTests/DraftWorkflowBuilderTests.cpp
    Facade_BuilderTests/ValidatingWorkflowBuilderTests.cpp
    Facade_BuilderTests/WorkflowDirectorTests.cpp
    Facade_BuilderTests/DefinitionRepositoryTests.cpp
    Facade_BuilderTests/WorkflowFacadeTests.cpp
    Facade_BuilderTests/WorkflowEngineTests.cpp

    tividy/builder/WorkflowBuilder.cpp
    tividy/builder/WorkflowDefinition.cpp
    tividy/builder/DraftWorkflowBuilder.cpp
    tividy/builder/ValidatingWorkflowBuilder.cpp
    tividy/builder/WorkflowDirector.cpp
    tividy/facade/DefinitionRepository.cpp
    tividy/facade/WorkflowFacade.cpp
    tividy/command/WorkflowEngine.cpp
    tividy/core/WorkflowInstance.cpp
)

target_include_directories(WorkflowBuilderTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Facade_BuilderTests
    ${DOCTEST_DIR}
)

add_test(NAME WorkflowBuilderTests COMMAND WorkflowBuilderTests)

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

# ===== Command + Observer tests =====
add_subdirectory(tests/command_observer)
