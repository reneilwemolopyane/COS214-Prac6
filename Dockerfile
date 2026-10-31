# Build and test TIVIDY in a clean container:  docker build -t tividy . && docker run --rm -it tividy
FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake make && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build && cmake --build build -j"$(nproc)" && cd build && ctest --output-on-failure

FROM ubuntu:24.04
COPY --from=build /src/build/tividy_tui /usr/local/bin/tividy_tui
CMD ["tividy_tui"]
