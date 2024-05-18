#pragma once

#include <cstddef>

#define bench_suffix(name)                                                    \
    constexpr std::size_t operator"" _##name(unsigned long long int number) { \
        return number;                                                        \
    }

bench_suffix(components);
bench_suffix(bytes_each);
bench_suffix(entities);
