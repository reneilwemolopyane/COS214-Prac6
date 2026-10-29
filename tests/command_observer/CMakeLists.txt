cmake_minimum_required(VERSION 3.10)
project(TIVIDY_CommandObserver_Tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(DUMMY ${CMAKE_CURRENT_SOURCE_DIR}/dummy)
set(UNIT ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)

set(ENGINE_SOURCES
    ${REPO_ROOT}/tividy/command/Command.cpp
    ${REPO_ROOT}/tividy/command/CommandProcessor.cpp
    ${REPO_ROOT}/tividy/command/WorkflowEngine.cpp
    ${REPO_ROOT}/tividy/observer/ObserverPattern.cpp
    ${REPO_ROOT}/tividy/observer/Observers.cpp
    ${REPO_ROOT}/tividy/core/WorkflowInstance.cpp
)

set(DUMMY_SOURCES
    ${DUMMY}/Participant.cpp
)

set(TEST_SOURCES
    ${UNIT}/test_main.cpp
    ${UNIT}/test_command.cpp
    ${UNIT}/test_observer.cpp
    ${UNIT}/test_workflow_engine.cpp
)

enable_testing()

add_executable(command_observer_tests ${TEST_SOURCES} ${ENGINE_SOURCES} ${DUMMY_SOURCES})

target_include_directories(command_observer_tests PRIVATE
    ${DUMMY}
    ${UNIT}
    ${REPO_ROOT}
)

if(NOT MSVC)
    target_compile_options(command_observer_tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME command_observer_tests COMMAND command_observer_tests)
