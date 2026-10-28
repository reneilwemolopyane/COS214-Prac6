
cmake_minimum_required(VERSION 3.20)

project(TividyUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(WorkflowBuilderTests
    Facade_BuilderTests/main.cpp
    Facade_BuilderTests/WorkflowDefinitionTests.cpp
    Facade_BuilderTests/DraftWorkflowBuilderTests.cpp
    Facade_BuilderTests/ValidatingWorkflowBuilderTests.cpp
    Facade_BuilderTests/WorkflowDirectorTests.cpp
    Facade_BuilderTests/DefinitionRepositoryTests.cpp
    Facade_BuilderTests/WorkflowFacadeTests.cpp
    Facade_BuilderTests/WorkflowEngineTests.cpp

    builder/WorkflowBuilder.cpp
    builder/WorkflowDefinition.cpp
    builder/DraftWorkflowBuilder.cpp
    builder/ValidatingWorkflowBuilder.cpp
    builder/WorkflowDirector.cpp
    facade/DefinitionRepository.cpp
    facade/WorkflowFacade.cpp
    command/WorkflowEngine.cpp
    core/WorkflowInstance.cpp
)

target_include_directories(WorkflowBuilderTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/Facade_BuilderTests"
)

add_test(NAME WorkflowBuilderTests COMMAND WorkflowBuilderTests)


# --- State and Composite Tests ---
add_executable(StateCompositeTests
    tests/StateCompositeTests.cpp
    composite_prototype/Stage.cpp
    composite_prototype/WorkItem.cpp
    state/WorkItemState.cpp
    state/States.cpp
    core/Participant.cpp
)

target_include_directories(StateCompositeTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/composite_prototype"
    "${CMAKE_CURRENT_SOURCE_DIR}/state"
    "${CMAKE_CURRENT_SOURCE_DIR}/visitor"
)

add_test(NAME StateCompositeTests COMMAND StateCompositeTests)