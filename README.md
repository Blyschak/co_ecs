# CoECS

![C++](https://img.shields.io/badge/STD-C++20-blue)
[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](http://unlicense.org/)
![Build badge](https://github.com/Blyschak/co_ecs/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/gh/Blyschak/co_ecs/branch/main/graph/badge.svg?token=BZ8Z6TXN55)](https://codecov.io/gh/Blyschak/cobalt-ecs)
![LoC](https://raw.githubusercontent.com/Blyschak/co_ecs/badges/badge.svg)

```co_ecs``` is a header-only library implementing an Entity Component System.

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

```c++
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

Views are used to iterate over entities in the registry which have a given set of components attached:

```c++
for (auto [position, velocity] : registry.view<position&, const velocity&>().each()) {
    // ...
}
```

Note, you don't have to worry about caching a view object returned by a ```registry::view()``` as it is very cheap to create, only when start iterating over it using ```view::each()``` an actual work on matching archetypes is done.

There's another way to iterate over interesting entities by using ```registry::each()``` which takes in an invocable as a parameter accepting component reference types:

```c++
registry.each([](position& pos, const velocity& vel){
    // ...
});
```

Note that this kind of iteration might be even faster and better optimized by the compiler since the func can operate on a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks to see the actual difference. Looking forward for compilers to be better at optimizing ```<ranges>``` machinery for the above two variants to match in performance.

```co_ecs``` tries to provide const-correct API. For example, a view with ```const``` references only can be created with from a ```const``` registry reference:

```c++
void foo(const co_ecs::registry& registry) {
    registry.view<const position&, const velocity&>(); // OK
    registry.view<position&, const velocity&>();       // Error
}
```

## Safety

```co_ecs``` tries to provide a safe API to work with. In example, creating an entity and specifying same component type more than once is ambiguous and causes undefined behavior, so the following snippet will fail to compile:

```c++
registry.create<position, position>({ 1, 2 }, { 3, 4 });
```

compiler will generate a static assertion error:

```
Types must be unique within parameter pack
```

Another example is when trying to assign a component of type ```co_ecs::entity``` by mistake. The ```co_ecs::entity``` structure is handled the same way components are internally. It would be invalid to assign it as a component to an entity, so when attempting to do the following:

```c++
registry.create<co_ecs::entity, position>({}, { 3, 4 });
```

compiler will raise the following static assertion error:

```
Cannot give a mutable pointer/reference to the entity
```

implying that you cannot have a write access into chunk's memory where ```co_ecs::entity``` objects are stored.

The same error message appears when trying to create a ```co_ecs::view``` with read-write access to ```co_ecs::entity```:

```c++
auto view = registry.view<co_ecs::entity&, position&>(); // Error
```

On the other hand, trying to query for ```const co_ecs::entity&``` is valid and the right method to fetch entity IDs together with it's components in the view:

```c++
auto view = registry.view<const co_ecs::entity&, position&>(); // Ok
```



## Pitfalls

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

## Exception safety

TBD

## Usage across binary boundaries

```co_ecs``` can be used across boundaries. One may create an engine shared library with core components and then use the registry inside another shared library or executable that links to the engine.
The core shared library should be compiled with ```CO_ECS_HOST``` defined and others must define ```CO_ECS_CLIENT```:

*engine.h*:
```c++
#include <co_ecs/co_ecs.hpp>

struct transform {
    float translation[3];
    float rotation[3];
    float scale[3];
};
```

*engine.cpp*:
```c++
#define CO_ECS_HOST

#include "engine.h"
```

*client.cpp*:
```c++
#define CO_ECS_CLIENT

#include "engine.h"
```

## Performance

TBD