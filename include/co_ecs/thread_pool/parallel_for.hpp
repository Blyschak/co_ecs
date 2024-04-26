#pragma once

#include <co_ecs/detail/allocator/temp_allocator.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>

namespace co_ecs {

/// @brief Parallelize func over elements in range
/// @param range Range to apply func to
/// @param func Function
void parallel_for(auto&& range, auto&& func) {
    auto& tp = thread_pool::get();
    auto size = std::ranges::distance(range);
    auto num_workers = tp.num_workers();

    // Calculate the chunk size, each chunk passed to a worker
    auto chunk_size = size / num_workers;

    auto begin = range.begin();
    auto end = range.end();

    // Prepare an array of pairs [start, end] from the range each worker will deal with
    struct chunk_data {
        decltype(begin) start;
        decltype(end) stop;
    };

    // Using thread pool worker stack allocator to avoid new/delete.
    std::vector<chunk_data, detail::temp_allocator<chunk_data>> chunks;
    chunks.resize(num_workers);

    auto chunk_start = range.begin();
    task_t* parent = nullptr; // An anchor task we will wait for

    for (auto i = 0; i < num_workers; i++) {
        auto chunk_stop = std::next(chunk_start, chunk_size);

        if (i == num_workers - 1) {
            chunk_stop = end;
        }

        chunks[i].start = chunk_start;
        chunks[i].stop = chunk_stop;

        auto chunk = chunks.begin() + i;

        auto* task = tp.allocate([chunk, &func]() { std::for_each(chunk->start, chunk->stop, func); }, parent);

        if (!parent) {
            parent = task;
        }

        tp.submit(task);

        chunk_start = chunk_stop;
    }

    tp.wait(parent);
}

} // namespace co_ecs
