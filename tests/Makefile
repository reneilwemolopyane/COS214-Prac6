CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I .
SRC = tividy/command/Command.cpp tividy/command/CommandProcessor.cpp tividy/command/WorkflowEngine.cpp \
      tividy/observer/ObserverPattern.cpp tividy/observer/Observers.cpp \
      tividy/core/Participant.cpp
TESTS = tests/test_main.cpp tests/test_command.cpp tests/test_observer.cpp tests/test_workflow_engine.cpp

unit_tests: $(TESTS) $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: unit_tests
	./unit_tests

valgrind: unit_tests
	valgrind --leak-check=full --error-exitcode=1 ./unit_tests

clean:
	rm -f unit_tests