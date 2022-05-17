#pragma once

#include <cobalt/asl/handle.hpp>
#include <vector>

namespace cobalt::asl {

/// @brief Handle pool that creates and recycles reusable H handles
///
/// @tparam H Handle type
template<typename H>
class handle_pool {
public:
    /// @brief Create new handle
    ///
    /// @return Handle
    [[nodiscard]] constexpr H create() {
        if (!_free_ids.empty()) {
            auto id = _free_ids.back();
            _free_ids.pop_back();
            return H(id, _generations[id]);
        }
        auto handle = H(_next_id++);
        _generations.emplace_back();
        return handle;
    };

    /// @brief Check if handle is still alive
    ///
    /// @param handle Handle to check
    /// @return True if handle is alive
    [[nodiscard]] constexpr bool alive(H handle) const noexcept {
        return _generations[handle.id()] == handle.generation();
    }

    /// @brief Recycle the handle, handle will be reused in next create()
    ///
    /// @param handle Handle to recycle
    void constexpr recycle(const H& handle) {
        if (!alive(handle)) {
            return;
        }
        _generations[handle.id()]++;
        _free_ids.push_back(handle.id());
    }

private:
    typename H::id_t _next_id{};
    std::vector<typename H::generation_t> _generations;
    std::vector<typename H::id_t> _free_ids;
};

} // namespace cobalt::asl