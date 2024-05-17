<!-- @cond TURN_OFF_DOXYGEN -->

<!-- omit in toc -->
# co_ecs

<img src="https://github.com/Blyschak/co_ecs/blob/assets/logo.png?raw=true" alt="logo" width="15%"/>

![C++](https://img.shields.io/badge/STD-C++20-blue)
[![Doxygen](https://img.shields.io/badge/Documentation-Doxygen-blue)](https://blyschak.github.io/co_ecs/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](http://unlicense.org/)
![Build badge](https://github.com/Blyschak/co_ecs/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/gh/Blyschak/co_ecs/branch/main/graph/badge.svg?token=BZ8Z6TXN55)](https://codecov.io/gh/Blyschak/cobalt-ecs)
![LoC](https://raw.githubusercontent.com/Blyschak/co_ecs/badges/badge.svg)

co_ecs is a header-only library implementing an Entity Component System.

<!-- omit in toc -->
## API Documentation

[Doxygen](https://blyschak.github.io/co_ecs/)

<!-- omit in toc -->
## Table of Contents

- [Build](#build)
  - [Build and run tests](#build-and-run-tests)
  - [Build and run benchmarks](#build-and-run-benchmarks)
  - [Build documentation](#build-documentation)
  - [Build examples](#build-examples)
- [Usage](#usage)
  - [System wide installation](#system-wide-installation)
  - [CMake Integration](#cmake-integration)
  - [Code example](#code-example)
- [Components](#components)
- [Views](#views)
- [Safety](#safety)
- [Pitfalls](#pitfalls)
- [Usage Across Binary Boundaries](#usage-across-binary-boundaries)
- [License](#license)

<!-- @endcond TURN_OFF_DOXYGEN -->


## Build

Compilation requires compiler with C++ 20 support.

Tested compilers:
  - GCC 11.2
  - Clang 16
  - MSVC 14.32

Before building targets create a ```build/``` directory in the root of the source tree and ```cd``` into it:

```
mkdir build/ && cd build/
```

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

Coverage reports might be misleading when building in release mode, so make sure you build in debug mode or explicitly pass ```-DCMAKE_BUILD_TYPE=Debug```.

### Build and run benchmarks

```
# Build with optimizations
cmake .. -DCO_ECS_ENABLE_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
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

### System wide installation

Inside your build directory execute the following to install ```co_ecs```:

```
make install
```

This ```make``` target will install ```co_ecs``` headers into your system include directory.

### CMake Integration

Using ```FetchConent```:

```cmake
include(FetchContent)

FetchContent_Declare(
  co_ecs
  GIT_REPOSITORY https://github.com/Blyschak/co_ecs
  GIT_TAG main)
FetchContent_MakeAvailable(co_ecs)

# link to your executable
target_link_libraries(my-engine co_ecs)
```

### Code example

[Example](examples/hello-registry/main.cpp)

```cpp
#include <co_ecs/co_ecs.hpp>

#include <iostream>

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

void print_positions(const co_ecs::registry& registry) {
    registry.each(
        [](const position& position) { std::cout << "position {" << position.x << ", " << position.y << "}\n"; });
}

int main() {
    co_ecs::registry registry;

    for (int i = 0; i < 100; i++) {
        registry.create<position, velocity>({ i * 1.f, i * 1.5f }, { i * .3f, -i * 5.f });
    }

    for (auto [position, velocity] : registry.view<position&, const velocity&>().each()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    print_positions(registry);

    return 0;
}
```

## Components

Component is any C++ type that satisfies the following:
  - It is a non-union class type, meaning any ```struct``` or ```class```
  - It is ```noexcept``` move constructible
  - It is ```noexcept``` move assignable

## Views

Views are used to iterate over entities in the registry with a specified set of components attached:

```cpp
for (auto [position, velocity] : registry.view<position&, const velocity&>().each()) {
    // ...
}
```

You don't need to worry about caching a view object returned by `registry::view()` because it is very inexpensive to create. The actual work of matching archetypes is done only when you start iterating over it using `view::each()`.

Another way to iterate over relevant entities is by using `registry::each()`, which takes an invocable as a parameter that accepts component reference types:

```cpp
registry.each([](position& pos, const velocity& vel){
    // ...
});
```

`co_ecs` aims to provide a const-correct API. For example, a view with `const` references can only be created from a `const` registry reference:

```cpp
void foo(const co_ecs::registry& registry) {
    registry.view<const position&, const velocity&>(); // OK
    registry.view<position&, const velocity&>();       // Error
}
```


Views are used to iterate over entities in the registry with a specified set of components attached:

```cpp
for (auto [position, velocity] : registry.view<position&, const velocity&>().each()) {
    // ...
}
```

You don't need to worry about caching a view object returned by `registry::view()` because it is very inexpensive to create. The actual work of matching archetypes is done only when you start iterating over it using `view::each()`.

Another way to iterate over relevant entities is by using `registry::each()`, which takes an invocable as a parameter that accepts component reference types:

```cpp
registry.each([](position& pos, const velocity& vel){
    // ...
});
```

This kind of iteration might be even faster and better optimized by the compiler. The function can operate on a chunk that yields two tuples of pointers to the actual data, whereas the `each()` variant returns an iterator over iterators to the actual data, which is more challenging for the compiler to optimize. Refer to the benchmarks to see the actual performance difference. We look forward to compilers improving their optimization of `<ranges>` machinery to make the performance of these two variants match.

`co_ecs` aims to provide a const-correct API. For example, a view with `const` references can only be created from a `const` registry reference:

```cpp
void foo(const co_ecs::registry& registry) {
    registry.view<const position&, const velocity&>(); // OK
    registry.view<position&, const velocity&>();       // Error
}
```

## Safety

`co_ecs` aims to provide a safe API. For example, creating an entity and specifying the same component type more than once is ambiguous and causes undefined behavior. The following snippet will fail to compile:

```cpp
registry.create<position, position>({ 1, 2 }, { 3, 4 });
```

The compiler will generate a static assertion error:

```
Types must be unique within parameter pack
```

Another example is when mistakenly trying to assign a component of type `co_ecs::entity`. The `co_ecs::entity` structure is handled internally in the same way as components, but it is invalid to assign it as a component to an entity. When attempting the following:

```cpp
registry.create<co_ecs::entity, position>({}, { 3, 4 });
```

The compiler will raise the following static assertion error:

```
Cannot give a mutable pointer/reference to the entity
```

This implies that you cannot have write access to the memory chunk where `co_ecs::entity` objects are stored.

The same error message appears when trying to create a `co_ecs::view` with read-write access to `co_ecs::entity`:

```cpp
auto view = registry.view<co_ecs::entity&, position&>(); // Error
```

However, querying for `const co_ecs::entity&` is valid and the correct method to fetch entity IDs together with their components in the view:

```cpp
auto view = registry.view<const co_ecs::entity&, position&>(); // OK
```

## Pitfalls

Components are referenced internally by IDs. The component ID is generated statically, and the resulting ID is compiler implementation-defined. There should be no logic that relies on a specific ID value. Due to how ID generation works, there is a limitation that restricts placing components inside implementation files (`*.cpp`) or private headers.

Consider the following two components with the same name defined in two separate compilation units in the global namespace:

*foo.cpp*:

```cpp
struct my_component {
    int value;
};
```

*bar.cpp*:

```cpp
struct my_component {
    void* data;
};
```

There is a high chance the compiler will generate the same ID for these two different structures, leading to undefined behavior. Therefore, it is recommended to place all components in public header files, or even in a single header file, to avoid name collisions.

## Usage Across Binary Boundaries

`co_ecs` can be used across binary boundaries. For example, you can create an engine shared library with core components and then use the registry inside another shared library or executable that links to the engine. The core shared library should be compiled with `CO_ECS_HOST` defined, and the others must define `CO_ECS_CLIENT`.

*engine.h*:
```cpp
#include <co_ecs/co_ecs.hpp>

struct transform {
    float translation[3];
    float rotation[3];
    float scale[3];
};
```

*engine.cpp*:
```cpp
#define CO_ECS_HOST

#include "engine.h"
```

*client.cpp*:
```cpp
#define CO_ECS_CLIENT

#include "engine.h"
```

## License

co_ecs is distributed under the MIT license.