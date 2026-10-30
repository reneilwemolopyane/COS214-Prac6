.RECIPEPREFIX := >
CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -g
CPPFLAGS += -I. -Itividy -Ithird_party/doctest -MMD -MP

BUILD := build
EXE   := $(if $(filter Windows_NT,$(OS)),.exe,)

# Engine = every .cpp under tividy/, except a GUI folder and any main.cpp
ENGINE_SRC := $(filter-out tividy/gui/% %/main.cpp, \
                $(wildcard tividy/*.cpp tividy/*/*.cpp tividy/*/*/*.cpp))
ENGINE_OBJ := $(ENGINE_SRC:%.cpp=$(BUILD)/%.o)
ENGINE_LIB := $(BUILD)/libtividy.a

# Single-file test programs: tests/<name>.cpp, each one defines its own main (doctest)
TEST_SRC := tests/test_mediator_strategy_state.cpp tests/StateCompositeTests.cpp tests/test_builders.cpp tests/test_definition.cpp tests/test_director.cpp tests/test_facade.cpp tests/test_repository.cpp tests/VisitorMementoTests.cpp
TEST_BIN := $(TEST_SRC:tests/%.cpp=$(BUILD)/bin/%$(EXE))

.PHONY: all test clean
all: $(TEST_BIN)

test: all
> @for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done

$(BUILD)/%.o: %.cpp
> @mkdir -p $(dir $@)
> $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(ENGINE_LIB): $(ENGINE_OBJ)
> @rm -f $@
> ar rcs $@ $^

$(BUILD)/bin/%$(EXE): tests/%.cpp $(ENGINE_LIB)
> @mkdir -p $(dir $@)
> $(CXX) $(CXXFLAGS) $(CPPFLAGS) $< $(ENGINE_LIB) -o $@

clean:
> rm -rf $(BUILD)

-include $(ENGINE_OBJ:.o=.d)
demo: main.cpp $(ENGINE_LIB)
> @mkdir -p $(BUILD)/bin
> $(CXX) $(CXXFLAGS) $(CPPFLAGS) main.cpp $(ENGINE_LIB) -o $(BUILD)/bin/demo$(EXE)
> ./$(BUILD)/bin/demo$(EXE)
