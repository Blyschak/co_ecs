cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  cobalt
  VERSION 0.9.0
  DESCRIPTION "Cobalt ECS"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)

add_library(co_ecs INTERFACE)
target_include_directories(co_ecs INTERFACE include/)

find_package(Doxygen)

option(ENABLE_DOCS "Enable documentation build" ON)
if(ENABLE_DOCS)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
endif()

option(ENABLE_EXAMPLES "Enable examples build" ON)
if(ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()

option(ENABLE_TESTING "Enable tests" ON)
if(ENABLE_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

option(ENABLE_BENCHMARKS "Enable benchmakrs" ON)
if(ENABLE_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
