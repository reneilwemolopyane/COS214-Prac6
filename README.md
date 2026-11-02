# TIVIDY Workflow Engine

TIVIDY is a modular C++ workflow engine with an interactive text interface
(TUI), sample insurance and education workflows, and a web server. The project
demonstrates 17 design patterns: Adapter, Builder, Chain of Responsibility,
Command, Composite, Decorator, Factory Method, Facade, Interpreter, Mediator,
Memento, Observer, Prototype, State, Strategy, Template Method, and Visitor.

## Requirements

- C++11-compatible compiler
- CMake 3.10 or newer
- Make or another CMake-supported build tool

## Build and run the TUI

From the repository root:

```sh
cmake -S . -B build
cmake --build build --target tividy_tui
./build/tividy_tui
```

Follow the on-screen prompts to select a workflow and interact with its work
items. To disable terminal colors, run:

```sh
./build/tividy_tui --no-color
```

## Run the tests

Build all targets, then run the test suite:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run with Docker

If Docker is installed, build and launch the TUI in a container:

```sh
docker build -t tividy .
docker run --rm -it tividy
```

## Generate the API documentation

Install [Doxygen](https://www.doxygen.nl/) and run this from the repository
root:

```sh
doxygen Doxyfile
```

The generated HTML documentation is written to `docs/api/`.

## Project layout

- `core/`, `composite_prototype/`, `state/`, and other pattern directories
  contain the reusable engine.
- `scenarios/` contains the sample insurance and education workflows.
- `tui/` contains the interactive text interface.
- `api/` contains the web server and JSON API.
- `tests/` and `Facade_BuilderTests/` contain automated tests.
- `docs/` contains diagrams and Doxygen documentation.
