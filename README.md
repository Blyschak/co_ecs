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
  GIT_REPOSITORY https://github.com/Blyschak/cobalt-ecs
  GIT_TAG main)
FetchContent_MakeAvailable(co_ecs)

# link to your executable
target_link_libraries(my-engine co_ecs)
```

### Code example

[Example](examples/hello-registry/main.cpp)

```c++
#include <co_ecs/co_ecs.hpp>

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

## Components

Component is any C++ type that satisfies the following:
  - It is a non-union class type, meaning any ```struct``` or ```class```
  - It is ```noexcept``` move constructible
  - It is ```noexcept``` move assignable

The ```co_ecs::entity``` ID structure is handled the same way components are internally. It would be invalid to assign it as a component to an entity, so when attempting to do the following:

```c++
registry.create<co_ecs::entity, position>({}, { 3, 4 });
```

MSVC will generate the following error:

```
error C2672: 'co_ecs::registry::create': no matching overloaded function found
```

## Empty component type

## Safety

```co_ecs``` tries to provide a safe API to work with. In example, creating an entity and specifying same component type more than once is ambiguous and causes undefined behavior, so the following snippet will fail to compile:

```c++
registry.create<position, position>({ 1, 2 }, { 3, 4 });
```

GCC gives the following error:

```
main.cpp:7:27:   required from here
type_traits.hpp:83:19: error: static assertion failed: Types must be unique within parameter pack
   83 |     static_assert(!std::is_same<T1, T2>::value, "Types must be unique within parameter pack");
```

With MSVC, the error looks like this:

```
error C2338: static_assert failed: 'Types must be unique within parameter pack'
```

```co_ecs::entity``` internally is stored as if it were a component

Another example - querying ```co_ecs::entity```

### Pitfalls

Components are referenced internally with IDs. The component ID is generated statically and the resulting ID is compiler implementation-defined. There should be no logic which expects a concrete ID value.
Given how the ID generation works there's one limitation that restricts placing components inside implementation files (```*.cpp```) or private headers.

Consider the following two components with the same name defined in two separate compilation units in a global namespace:

*foo.cpp*:

```c++
struct my_component {
    int value;
};
```

*bar.cpp*:

```c++
struct my_component {
    void* data;
};
```

There is a high chance the compiler will generate the same ID for two different structures which will lead to undefined behavior. Thus, it is recommended to place all components in public header files or even in a single header file to avoid name collisions.

### Usage with shared objects/DLLs
