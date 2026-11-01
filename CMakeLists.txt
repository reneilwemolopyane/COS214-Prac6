cmake_minimum_required(VERSION 3.10)

project(Tividy LANGUAGES CXX)

# The project specification requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# ===== Shared: make "tividy/<module>/X.h" resolve =====
# Engine headers are included as "tividy/<module>/X.h" but the repo folders are flat
# (and the repo isn't named "tividy"), so mirror them into build/include/tividy/.
set(ENGINE_MODULES core mediator state strategy command observer composite_prototype
                   visitor adapter builder facade decorator chain_of_responsibility memento interpreter scenarios)
set(SHIM ${CMAKE_BINARY_DIR}/include)
set(ENGINE_SRC "")
foreach(mod ${ENGINE_MODULES})
    file(GLOB mod_headers ${CMAKE_SOURCE_DIR}/${mod}/*.h)
    if(mod_headers)
        file(COPY ${mod_headers} DESTINATION ${SHIM}/tividy/${mod})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${mod_headers})
    endif()
    # Industry-specific code lives in the scenarios library, NOT in the reusable engine.
    if(NOT mod STREQUAL "scenarios")
        file(GLOB mod_sources ${CMAKE_SOURCE_DIR}/${mod}/*.cpp)
        list(APPEND ENGINE_SRC ${mod_sources})
    endif()
endforeach()
file(GLOB SCENARIO_SRC ${CMAKE_SOURCE_DIR}/scenarios/*.cpp)

# ===== The reusable TIVIDY engine (every module) as one static library =====
add_library(tividy_engine STATIC ${ENGINE_SRC})
target_include_directories(tividy_engine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${SHIM})


# ===== Industry scenarios (insurance + education): configuration that USES the engine =====
add_library(tividy_scenarios STATIC ${SCENARIO_SRC})
target_include_directories(tividy_scenarios PUBLIC ${CMAKE_SOURCE_DIR} ${SHIM})
target_link_libraries(tividy_scenarios PUBLIC tividy_engine)

# --- Interactive text UI (the main way to run TIVIDY) ---
add_executable(tividy_tui tui/main.cpp)
target_link_libraries(tividy_tui tividy_scenarios)

# ===== Unit tests (doctest) =====
set(DOCTEST_DIR ${CMAKE_SOURCE_DIR}/third_party)

# --- Facade / Builder / Engine tests ---
file(GLOB FB_TESTS ${CMAKE_SOURCE_DIR}/Facade_BuilderTests/*.cpp)
add_executable(WorkflowBuilderTests ${FB_TESTS})
target_include_directories(WorkflowBuilderTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Facade_BuilderTests ${DOCTEST_DIR}/doctest)
target_link_libraries(WorkflowBuilderTests tividy_scenarios)
add_test(NAME WorkflowBuilderTests COMMAND WorkflowBuilderTests)

# --- State and Composite tests ---
add_executable(StateCompositeTests tests/StateCompositeTests.cpp)
target_include_directories(StateCompositeTests PRIVATE ${DOCTEST_DIR}/doctest)
target_link_libraries(StateCompositeTests tividy_engine)
add_test(NAME StateCompositeTests COMMAND StateCompositeTests)

# --- Chain of Responsibility + Decorator tests ---
set(CD_TESTS ${CMAKE_SOURCE_DIR}/tests/chain_decorator)
add_executable(ChainDecoratorTests
    ${CD_TESTS}/test_main.cpp
    ${CD_TESTS}/test_approval.cpp
    ${CD_TESTS}/test_decorators.cpp)
target_include_directories(ChainDecoratorTests PRIVATE
    ${CD_TESTS} ${DOCTEST_DIR}
    ${CMAKE_SOURCE_DIR}/chain_of_responsibility ${CMAKE_SOURCE_DIR}/decorator)
target_link_libraries(ChainDecoratorTests tividy_engine)
add_test(NAME ChainDecoratorTests COMMAND ChainDecoratorTests)

# --- Adapter tests (Adapter pattern: all five connectors) ---
add_executable(AdapterTests tests/test_adapter.cpp tests/test_main.cpp)
target_include_directories(AdapterTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(AdapterTests tividy_engine)
add_test(NAME AdapterTests COMMAND AdapterTests)

# --- Mediator, Strategy and Observer tests ---
add_executable(MediatorStrategyTests tests/MediatorStrategyTests.cpp tests/test_main.cpp)
target_include_directories(MediatorStrategyTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(MediatorStrategyTests tividy_engine)
add_test(NAME MediatorStrategyTests COMMAND MediatorStrategyTests)

# --- Visitor and Memento tests ---
# Uses the same doctest framework as the other test suites.
add_executable(VisitorMementoTests tests/VisitorMementoTests.cpp)
target_include_directories(VisitorMementoTests PRIVATE ${DOCTEST_DIR}/doctest)
target_link_libraries(VisitorMementoTests tividy_engine)
add_test(NAME VisitorMementoTests COMMAND VisitorMementoTests)

# --- Observer, concrete observers and engine wiring (end to end) ---
add_executable(ObserverEngineTests tests/ObserverEngineTests.cpp tests/test_main.cpp)
target_include_directories(ObserverEngineTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(ObserverEngineTests tividy_engine)
add_test(NAME ObserverEngineTests COMMAND ObserverEngineTests)

# --- Both industries end to end on the real engine (Interpreter, approval chain, decorators, adapters) ---
add_executable(ScenarioIntegrationTests tests/ScenarioIntegrationTests.cpp tests/test_main.cpp)
target_include_directories(ScenarioIntegrationTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(ScenarioIntegrationTests tividy_scenarios)
add_test(NAME ScenarioIntegrationTests COMMAND ScenarioIntegrationTests)

add_executable(InterpreterTests tests/InterpreterTests.cpp tests/test_main.cpp)
target_include_directories(InterpreterTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(InterpreterTests tividy_engine)
add_test(NAME InterpreterTests COMMAND InterpreterTests)

# --- The interactive TUI driven by a recorded keyboard session (both industries, exception paths) ---
add_test(NAME TuiSmoke_Insurance_Completes
         COMMAND sh -c "\"$<TARGET_FILE:tividy_tui>\" --no-color < \"${CMAKE_SOURCE_DIR}/tests/tui_smoke.txt\"")
set_tests_properties(TuiSmoke_Insurance_Completes PROPERTIES PASS_REGULAR_EXPRESSION "Insurance \\| motor-claim-1  \\[COMPLETED\\]")

add_test(NAME TuiSmoke_Education_Rejects
         COMMAND sh -c "\"$<TARGET_FILE:tividy_tui>\" --no-color < \"${CMAKE_SOURCE_DIR}/tests/tui_smoke.txt\"")
set_tests_properties(TuiSmoke_Education_Rejects PROPERTIES PASS_REGULAR_EXPRESSION "Education \\| bursary-application-1  \\[REJECTED\\]")

add_test(NAME TuiSmoke_Deadline_Escalates
         COMMAND sh -c "\"$<TARGET_FILE:tividy_tui>\" --no-color < \"${CMAKE_SOURCE_DIR}/tests/tui_smoke.txt\"")
set_tests_properties(TuiSmoke_Deadline_Escalates PROPERTIES PASS_REGULAR_EXPRESSION "1 item\\(s\\) escalated")

# ===== Tividy Web Server =====
add_executable(TividyServer
    api/main_server.cpp
    api/ApiServer.cpp
    api/JsonSerializer.cpp
)

target_include_directories(TividyServer PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(TividyServer PRIVATE 
    tividy_scenarios
    tividy_engine
    pthread
)
if(WIN32)
    target_link_libraries(TividyServer PRIVATE ws2_32 wsock32)
endif()