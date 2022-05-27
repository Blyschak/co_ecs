#pragma once

#include <cobalt/asl/platform.hpp>

#ifdef COBALT_COMPILER_MSVC
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace cobalt::asl {

namespace {

/// @brief Allocate aligned storage of the specified size
///
/// @param alignment Alignment
/// @param size Size of the storage
/// @return void* Pointer to the allocated memory
void* aligned_alloc(size_t alignment, size_t size) {
    // MSVC compiler does not support std::aligned_alloc, thus we're using MSVC specific __aligned_malloc
#ifdef COBALT_COMPILER_MSVC
    return _aligned_malloc(alignment, size);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

/// @brief Free memory allocated with aligned_alloc
///
/// @param ptr Pointer to the allocated memory
void aligned_free(void* ptr) {
    // MSVC compiler does not support std::aligned_alloc, thus we're using MSVC specific __aligned_free
#ifdef COBALT_COMPILER_MSVC
    return _aligned_free(ptr);
#else
    return std::free(ptr);
#endif
}

} // namespace

} // namespace cobalt::asl