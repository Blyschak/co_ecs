#pragma once

#include <cassert>
#include <cstdint>

#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/ecs/component.hpp>
#include <cobalt/ecs/entity.hpp>

namespace cobalt::ecs {

template<component T>
class typed_chunk;

class chunk {
public:
    static constexpr std::size_t chunk_bytes = 16 * 1024; // 16 KB

    struct chunk_info {
        void* begin{};
        void* end{};
        component_meta meta{};
    };

    chunk() : chunk(component_meta_set::create()) {
    }

    chunk(const component_meta_set& set) {
        _buffer = reinterpret_cast<char*>(std::aligned_alloc(4, chunk_bytes));
        auto netto_size = calculate_netto_element_size(_buffer, set);
        auto remaining_space = chunk_bytes - netto_size;
        auto remaining_elements_count = remaining_space / calculate_brutto_element_size(set);
        _max_size = remaining_elements_count + 1;

        char* ptr = _buffer;
        for (const auto& meta : set) {
            chunk_info cmeta;
            cmeta.begin = ptr;
            cmeta.end = ptr + (reinterpret_cast<std::size_t>(ptr) % meta.align) + _max_size * meta.size;
            ptr = reinterpret_cast<char*>(cmeta.end);
            cmeta.meta = meta;
            _info.emplace(meta.id, cmeta);
        }
        // make space for entity
        chunk_info cmeta;
        const component_meta meta = component_meta::of<entity>();
        cmeta.begin = ptr;
        cmeta.end = ptr + (reinterpret_cast<std::size_t>(ptr) % meta.align) + _max_size * meta.size;
        ptr = reinterpret_cast<char*>(cmeta.end);
        cmeta.meta = meta;
        _info.emplace(meta.id, cmeta);
        assert(ptr <= _buffer + chunk_bytes);
    }

    chunk(const chunk& rhs) = delete;
    chunk& operator=(const chunk& rhs) = delete;

    chunk(chunk&& rhs) {
        _buffer = rhs._buffer;
        _size = rhs._size;
        _max_size = rhs._max_size;
        _info = rhs._info;

        rhs._buffer = nullptr;
    }

    chunk& operator=(chunk&& rhs) {
        _buffer = rhs._buffer;
        _size = rhs._size;
        _max_size = rhs._max_size;
        _info = rhs._info;

        rhs._buffer = nullptr;
        return *this;
    }

    ~chunk() {
        if (!_buffer) {
            return;
        }
        for (const auto& [id, meta] : _info) {
            for (std::size_t i = 0; i < _size; i++) {
                meta.meta.dtor(reinterpret_cast<char*>(meta.begin) + i);
            }
        }
        std::free(_buffer);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        assert(!full());
        (..., std::construct_at(ptr<Args>(size()), args));
        _size++;
    }

    void pop_back() {
        assert(!empty());
        _size--;
        for (const auto& [id, meta] : _info) {
            meta.meta.dtor(reinterpret_cast<char*>(meta.begin) + _size * meta.meta.size);
        }
    }

    entity swap_end(std::size_t index, chunk& other_chunk) {
        assert(index < _size);
        if (size() == 1 || index == _size - 1) {
            pop_back();
            return entity::invalid();
        }
        assert(!other_chunk.empty());
        std::size_t other_chunk_index = other_chunk._size - 1;
        entity ent = other_chunk.at<entity>(other_chunk_index);
        for (const auto& [id, meta] : _info) {
            meta.meta.move_assign(reinterpret_cast<char*>(meta.begin) + index * meta.meta.size,
                reinterpret_cast<char*>(other_chunk._info[id].begin) + other_chunk_index * meta.meta.size);
        }
        other_chunk.pop_back();
        return ent;
    }

    std::size_t move(std::size_t index, chunk& other_chunk) {
        assert(index < _size);
        assert(!other_chunk.full());
        std::size_t other_chunk_index = other_chunk._size;
        for (const auto& [id, meta] : _info) {
            if (!other_chunk._info.contains(id)) {
                continue;
            }
            meta.meta.move_ctor(
                reinterpret_cast<char*>(other_chunk._info.at(id).begin) + other_chunk_index * meta.meta.size,
                reinterpret_cast<char*>(meta.begin) + index * meta.meta.size);
        }
        other_chunk._size++;
        return other_chunk_index;
    }

    template<component T>
    T& at(std::size_t index) noexcept {
        assert(index < _size);
        return *(reinterpret_cast<T*>(_info.at(component_meta::of<T>().id).begin) + index);
    }

    template<component T>
    const T& at(std::size_t index) const noexcept {
        assert(index < _size);
        return *(reinterpret_cast<T*>(_info.at(component_meta::of<T>().id).begin) + index);
    }

    template<component T>
    T* ptr(std::size_t index) noexcept {
        auto meta = component_meta::of<T>();
        return (reinterpret_cast<T*>(_info.at(component_meta::of<T>().id).begin) + index);
    }

    template<component T>
    const T* ptr(std::size_t index) const noexcept {
        auto meta = component_meta::of<T>();
        return (reinterpret_cast<T*>(_info.at(component_meta::of<T>().id).begin) + index);
    }

    static std::size_t calculate_brutto_element_size(const component_meta_set& components_meta) noexcept {
        return std::accumulate(components_meta.begin(),
            components_meta.end(),
            component_meta::of<entity>().size,
            [](const auto& res, const auto& meta) { return res + meta.size; });
    }

    static std::size_t calculate_netto_element_size(const char* begin,
        const component_meta_set& components_meta) noexcept {
        auto end = begin;
        for (const auto& m : components_meta) {
            end += (reinterpret_cast<std::size_t>(begin) % m.align);
            end += m.size;
        }
        const auto meta = component_meta::of<entity>();
        end += (reinterpret_cast<std::size_t>(begin) % meta.align);
        end += meta.size;
        return end - begin;
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept {
        return _max_size;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return _size;
    }

    [[nodiscard]] constexpr bool full() const noexcept {
        return size() == max_size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return size() == 0;
    }

    template<component_or_reference T>
    [[nodiscard]] constexpr typed_chunk<decay_component_t<T>>& as_container_of() requires(
        is_mutable_component_reference<T>()) {
        return *reinterpret_cast<typed_chunk<decay_component_t<T>>*>(this);
    }

    template<component_or_reference T>
    [[nodiscard]] constexpr const typed_chunk<decay_component_t<T>>& as_container_of() const
        requires(is_immutable_component_reference<T>()) {
        return *reinterpret_cast<const typed_chunk<decay_component_t<T>>*>(this);
    }

private : char* _buffer;
    std::size_t _size{};
    std::size_t _max_size{};
    asl::sparse_map<component_id, chunk_info> _info;
};

template<component T>
class typed_chunk : public chunk {
public:
    [[nodiscard]] constexpr T* begin() noexcept {
        return ptr<T>(0);
    }

    [[nodiscard]] constexpr T* end() noexcept {
        return ptr<T>(size());
    }

    [[nodiscard]] constexpr const T* begin() const noexcept {
        return ptr<T>(0);
    }

    [[nodiscard]] constexpr const T* end() const noexcept {
        return ptr<T>(size());
    }
};

} // namespace cobalt::ecs