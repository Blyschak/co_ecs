#pragma once

#include <algorithm>

namespace cobalt::asl {

template<typename R>
R set_difference(const R& r1, const R& r2) {
    R result;
    std::set_difference(r1.begin(), r1.end(), r2.begin(), r2.end(), std::inserter(result, result.begin()));
    return result;
}

template<typename R>
R set_union(const R& r1, const R& r2) {
    R result;
    std::set_union(r1.begin(), r1.end(), r2.begin(), r2.end(), std::inserter(result, result.begin()));
    return result;
}


} // namespace cobalt::asl