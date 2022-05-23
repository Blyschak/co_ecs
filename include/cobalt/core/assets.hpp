#pragma once

#include <cobalt/asl/handle.hpp>
#include <cobalt/asl/handle_pool.hpp>
#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/core/pointer.hpp>

namespace cobalt {

/// @brief Asset ID type
///
/// @tparam T Asset type
template<typename T>
using asset_id = std::uint32_t;

/// @brief Asset generation ID
///
/// @tparam T Asset type
template<typename T>
using asset_generation_id = std::uint32_t;

/// @brief Asset handle type
///
/// @tparam T Asset type
template<typename T>
using asset_handle = cobalt::asl::handle<asset_id<T>, asset_generation_id<T>>;

/// @brief Asset pool
///
/// @tparam T Asset type
template<typename T>
using asset_pool = cobalt::asl::handle_pool<asset_handle<T>>;

/// @brief Asset traits
///
/// @tparam T Asset type
template<typename T>
struct asset_traits;

/// @brief Asset storage
///
/// @tparam T Asset type
/// @tparam asset_traits<T>::loader_type Asset loader type
template<typename T, typename asset_loader = typename asset_traits<T>::loader_type>
class asset_storage {
public:
    /// @brief Load an asset from a path
    ///
    /// @param path Asset path
    /// @return asset_handle<T> Handle is returned to the user
    asset_handle<T> from_path(const std::string& path) {
        auto asset = _loader.load_from_file(path);

        auto handle = _pool.create();
        _assets.emplace(handle.id(), std::move(asset));

        return handle;
    }

    /// @brief Get asset by handle
    ///
    /// @param handle Asset handle
    /// @return T* Asset pointer, nullptr if not ready or found
    T* get(asset_handle<T> handle) {
        return get_impl(*this, handle);
    }

    /// @brief Get asset by handle
    ///
    /// @param handle Asset handle
    /// @return const T* Asset pointer, nullptr if not ready or found
    const T* get(asset_handle<T> handle) const {
        return get_impl(*this, handle);
    }

private:
    static T* get_impl(auto&& self, asset_handle<T> handle) {
        if (!self._pool.alive(handle)) {
            return nullptr;
        }
        auto iter = self._assets.find(handle.id());
        if (iter == self._assets.end()) {
            return nullptr;
        }
        return iter->second.get();
    }

    asset_loader _loader;
    asset_pool<T> _pool;
    asl::sparse_map<asset_id<T>, owned<T>> _assets;
};

} // namespace cobalt