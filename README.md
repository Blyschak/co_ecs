# Cobalt ECS

![C++](https://img.shields.io/badge/STD-C++20-blue)
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)
![Build badge](https://github.com/Blyschak/cobalt-ecs/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/gh/Blyschak/cobalt-ecs/branch/main/graph/badge.svg?token=BZ8Z6TXN55)](https://codecov.io/gh/Blyschak/cobalt-ecs)
![LoC](https://raw.githubusercontent.com/Blyschak/cobalt-ecs/badges/badge.svg)

```co_ecs``` is a header-only library implementing an Entity Component System.

## Build

Compilation requires compiler with C++ 20 support.

Tested compilers:
  - GCC 11.2
  - MSVC 14.32

Before building targets create a ```build/``` and cd into it directory where all build artifacts will be stored.

### Build and run tests

```
cmake .. -DCO_ECS_ENABLE_TESTING=ON
make -j$(nproc)
make test
```

Generate coverage report:

```
cmake .. -DCO_ECS_ENABLE_TESTING=ON -DCO_ECS_CODE_COVERAGE=ON
make -j$(nproc)
make test
```

### Build and run benchmarks

```
cmake .. -DCO_ECS_ENABLE_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release # Build with optimizations
make -j$(nproc)
benchmarks/benchmarks
```

### Build documentation

```
cmake .. -DCO_ECS_ENABLE_DOCS=ON
make docs
# documentation generated: docs/html/index.html
```

### Build examples

```
cmake .. -DCO_ECS_ENABLE_EXAMPLES=ON
make -j$(nproc)
```

## Usage

### CMake Integration

Using ```FetchConent```:

```cmake
include(FetchContent)

FetchContent_Declare(
  co_ecs
  GIT_REPOSITORY https://github.com/Blyschak/cobalt-ecs
  GIT_TAG main)
FetchContent_MakeAvailable(co_ecs)

# link to your executable
target_link_libraries(my-engine co_ecs)
```

### Code example

[Example](examples/hello-registry/main.cpp)

```c++
#include <co_ecs/registry.hpp>
#include <co_ecs/view.hpp>

#include <iostream>

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

int main() {
    co_ecs::registry registry;

    for (int i = 0; i < 100; i++) {
        registry.create<position, velocity>({ i * 1.f, i * 1.5f }, { i * .3f, -i * 5.f });
    }

    for (auto [position, velocity] : co_ecs::view<position&, const velocity&>(registry).each()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    for (auto [position] : co_ecs::view<const position&>(registry).each()) {
        std::cout << "position {" << position.x << ", " << position.y << "}\n";
    }

    return 0;
}
```
