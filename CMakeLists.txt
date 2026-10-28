cmake_minimum_required(VERSION 3.20)

project(TividyUnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# ===== Shared: make "tividy/<module>/X.h" resolve =====
# Engine headers are included as "tividy/<module>/X.h" but the repo folders are flat
# (and the repo isn't named "tividy"), so mirror them into build/include/tividy/.
set(SHIM ${CMAKE_BINARY_DIR}/include)
foreach(mod core mediator state strategy command observer composite_prototype
            visitor adapter builder facade decorator chain_of_responsibility)
    file(GLOB mod_headers ${CMAKE_SOURCE_DIR}/${mod}/*.h)
    if(mod_headers)
        file(COPY ${mod_headers} DESTINATION ${SHIM}/tividy/${mod})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${mod_headers})
    endif()
endforeach()

# ===== Facade / Builder tests (dev) =====
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
    "${SHIM}"
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
# ===== Chain of Responsibility + Decorator tests (doctest, C++11) =====
set(CD_TESTS ${CMAKE_SOURCE_DIR}/tests/chain_decorator)

# Visitor isn't on dev yet: use the tracked stub until it is.
set(STUB_DIR "")
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/visitor/Visitor.h)
    message(STATUS "visitor/Visitor.h not on this branch - using test stub")
    set(STUB_DIR ${CD_TESTS}/stub)
endif()

set(MEDIATOR_SRC "")
if(EXISTS ${CMAKE_SOURCE_DIR}/mediator/WorkflowMediator.cpp)
    set(MEDIATOR_SRC ${CMAKE_SOURCE_DIR}/mediator/WorkflowMediator.cpp)
endif()

add_executable(ChainDecoratorTests
    tests/chain_decorator/test_main.cpp
    tests/chain_decorator/test_approval.cpp
    tests/chain_decorator/test_decorators.cpp
    chain_of_responsibility/ApprovalHandler.cpp
    chain_of_responsibility/LevelApprover.cpp
    decorator/WorkItemDecorator.cpp
    decorator/ValidationDecorator.cpp
    decorator/PriorityDecorator.cpp
    decorator/SecurityDecorator.cpp
    decorator/AuditDecorator.cpp
    decorator/NotificationDecorator.cpp
    decorator/DeadlineDecorator.cpp
    ${MEDIATOR_SRC}
)

set_target_properties(ChainDecoratorTests PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

target_include_directories(ChainDecoratorTests PRIVATE
    ${CD_TESTS}                          # StubItem.h
    third_party                          # "doctest/doctest.h"
    chain_of_responsibility
    decorator
    ${SHIM}                              # "tividy/..."
    ${STUB_DIR}
    # engine headers that use bare includes like "WorkflowEvent.h"
    core mediator state composite_prototype
)

add_test(NAME ChainDecoratorTests COMMAND ChainDecoratorTests)
