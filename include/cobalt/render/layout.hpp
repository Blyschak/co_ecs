#pragma once

#include <cobalt/render/types.hpp>

#include <cstdint>
#include <vector>

namespace cobalt::renderer {

struct vertex_element {
    vertex_format format{};
    std::size_t size{};
    std::uint8_t count{};
    bool normalized{};

    [[nodiscard]] decltype(auto) offset() const noexcept {
        return _offset;
    }

private:
    std::size_t _offset{};

    friend class vertex_layout;
};

class vertex_layout {
public:
    using iterator = std::vector<vertex_element>::iterator;
    using const_iterator = std::vector<vertex_element>::const_iterator;

    template<typename T>
    explicit vertex_layout(T elements) : elements(elements) {
        std::size_t offset = 0;
        for (const auto& element : elements) {
            element._offset = offset;
            offset += element.size;
            _stride += element.size;
        }
    }

    vertex_layout(const vertex_layout&) = default;
    vertex_layout& operator=(const vertex_layout&) = default;
    vertex_layout(vertex_layout&&) = default;
    vertex_layout& operator=(vertex_layout&&) = default;

    ~vertex_layout() = default;

    iterator begin() {
        return elements.begin();
    }
    iterator end() {
        return elements.end();
    }
    const_iterator begin() const {
        return elements.begin();
    }
    const_iterator end() const {
        return elements.end();
    }

    [[nodiscard]] decltype(auto) size() const noexcept {
        return elements.size();
    }
    [[nodiscard]] decltype(auto) stride() const noexcept {
        return _stride;
    }

private:
    std::vector<vertex_element> elements{};
    std::size_t _stride{};
};

} // namespace cobalt::renderer