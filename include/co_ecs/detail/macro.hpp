#pragma once

namespace co_ecs::detail {
#if defined __clang__
#define CO_ECS_PRETTY_FUNCTION __PRETTY_FUNCTION__
constexpr auto PrettyFunctionPrefix = '=';
constexpr auto prettyFunctionSuffix = ']';
#elif defined __GNUC__
#define CO_ECS_PRETTY_FUNCTION __PRETTY_FUNCTION__
constexpr auto PrettyFunctionPrefix = '=';
constexpr auto prettyFunctionSuffix = ';';
#elif defined _MSC_VER
#define CO_ECS_PRETTY_FUNCTION __FUNCSIG__
constexpr auto PrettyFunctionPrefix = '<';
constexpr auto prettyFunctionSuffix = '>';
#endif
} // namespace co_ecs::detail

#ifndef CO_ECS_EXPORT
#if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#define CO_ECS_EXPORT __declspec(dllexport)
#define CO_ECS_IMPORT __declspec(dllimport)
#define CO_ECS_HIDDEN
#elif defined __GNUC__ && __GNUC__ >= 4
#define CO_ECS_EXPORT __attribute__((visibility("default")))
#define CO_ECS_IMPORT __attribute__((visibility("default")))
#define CO_ECS_HIDDEN __attribute__((visibility("hidden")))
#else // Unsupported compiler
#define CO_ECS_EXPORT
#define CO_ECS_IMPORT
#define CO_ECS_HIDDEN
#endif
#endif

#ifndef CO_ECS_API
#if defined CO_ECS_HOST
#define CO_ECS_API CO_ECS_EXPORT
#elif defined CO_ECS_CLIENT
#define CO_ECS_API CO_ECS_IMPORT
#else // No API
#define CO_ECS_API
#endif
#endif