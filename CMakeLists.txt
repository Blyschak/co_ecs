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

option(CO_ECS_ENABLE_DOCS "Enable documentation build" OFF)
if(CO_ECS_ENABLE_DOCS)
  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
endif()

option(CO_ECS_ENABLE_EXAMPLES "Enable examples build" OFF)
if(CO_ECS_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()

option(CO_ECS_ENABLE_TESTING "Enable tests" OFF)
if(CO_ECS_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

option(CO_ECS_ENABLE_BENCHMARKS "Enable benchmakrs" OFF)
if(CO_ECS_ENABLE_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
