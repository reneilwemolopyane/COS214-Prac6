cmake_minimum_required(VERSION 3.10)
project(TIVIDY CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Folder that CONTAINS tividy/ (temporary copy of the engine headers)
set(TIVIDY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/scratch)

# Any .cpp files from the engine that your code needs to link against.
set(TIVIDY_ENGINE_SOURCES
    # scratch/tividy/composite_prototype/WorkflowComponent.cpp
    # scratch/tividy/mediator/Mediator.cpp
)

set(CHAIN_SOURCES
    chain_of_responsibility/ApprovalHandler.cpp
    chain_of_responsibility/LevelApprover.cpp
)

set(DECORATOR_SOURCES
    decorator/WorkItemDecorator.cpp
    decorator/ValidationDecorator.cpp
    decorator/PriorityDecorator.cpp
    decorator/SecurityDecorator.cpp
    decorator/AuditDecorator.cpp
    decorator/NotificationDecorator.cpp
    decorator/DeadlineDecorator.cpp
)

enable_testing()

add_executable(tividy_tests
    tests/test_main.cpp
    tests/test_approval.cpp
    tests/test_decorators.cpp
    ${CHAIN_SOURCES}
    ${DECORATOR_SOURCES}
    ${TIVIDY_ENGINE_SOURCES}
)

target_include_directories(tividy_tests PRIVATE
    tests                       # StubItem.h
    third_party                 # "doctest/doctest.h"
    chain_of_responsibility     # "Approval.h"
    decorator                   # "Decorators.h"
    ${TIVIDY_INCLUDE_DIR}       # "tividy/..."
)

add_test(NAME tividy_tests COMMAND tividy_tests)