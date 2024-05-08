#pragma once

#include <co_ecs/detail/allocator/temp_allocator.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>

namespace co_ecs {

/// @brief Parallelize func over elements in range
/// @tparam R Range type
/// @param range Range to apply func to
/// @param func Function
template<typename R>
void parallel_for(R&& range, auto&& func) {
    auto& thread_pool = thread_pool::get();
    auto num_workers = thread_pool.num_workers();
    auto work_size = std::ranges::distance(range);
    auto batch_size = work_size / num_workers;

    if (batch_size < 1) {
        // fast path for small range
        std::ranges::for_each(range, func);
    } else {
        // alias a subrange type that is our batch of work
        using batch_t = decltype(std::ranges::subrange(range.begin(), range.end()));

        std::vector<batch_t, detail::temp_allocator<batch_t>> batches;

        // prepare batches
        {
            batches.reserve(num_workers);
            auto b = range.begin();
            auto e = b;
            for (auto i = 0; i < num_workers; i++) {
                auto e = (i < num_workers - 1) ? std::next(b, batch_size) : range.end();
                batches.emplace_back(std::ranges::subrange(b, e));
                b = e;
            }
        }

        // submit batches
        {
            task_t* parent = nullptr;
            for (auto& batch : batches) {
                auto* task = thread_pool.submit([&batch, &func]() { std::ranges::for_each(batch, func); }, parent);
                if (!parent) {
                    parent = task;
                }
            }
            thread_pool.wait(parent);
        }
    }
}

} // namespace co_ecs
