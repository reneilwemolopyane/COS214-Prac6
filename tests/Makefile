CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I .
SRC = command/Command.cpp command/CommandProcessor.cpp command/WorkflowEngine.cpp \
      observer/ObserverPattern.cpp observer/Observers.cpp \
      core/Participant.cpp core/WorkflowInstance.cpp
TESTS = tests/test_main.cpp tests/test_command.cpp tests/test_observer.cpp tests/test_workflow_engine.cpp

unit_tests: $(TESTS) $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: unit_tests
	./unit_tests

valgrind: unit_tests
	valgrind --leak-check=full --error-exitcode=1 ./unit_tests

clean:
	rm -f unit_tests