#pragma once

#include <cstddef>

#define bench_prefix(name)                                                    \
    constexpr std::size_t operator"" _##name(unsigned long long int number) { \
        return number;                                                        \
    }

bench_prefix(components);
bench_prefix(bytes_each);
bench_prefix(entities);
