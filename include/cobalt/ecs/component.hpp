#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <cobalt/asl/family.hpp>
#include <cobalt/asl/type_traits.hpp>
#include <cobalt/ecs/entity.hpp>

namespace cobalt::ecs {

/// @brief Type for component ID
using component_id = std::uint32_t;

/// @brief Invalid component ID
constexpr auto invalid_component_id = std::numeric_limits<component_id>::max();

/// @brief Type for family used to generated component IDs.
using component_family = cobalt::asl::family<struct _component_family_t, component_id>;

/// @brief Component concept. The component must be a struct/class that can be default constructed and safely move
/// assignable
///
/// @tparam T Component type
template<typename T>
concept component =
    std::is_class_v<T> && std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>;

/// @brief Component reference concept. It should be a reference or const reference to C, where C satisfies component
/// concept
///
/// @tparam T Component reference type
template<typename T>
concept component_reference = std::is_reference_v<T> && component<std::remove_cvref_t<T>>;

/// @brief Component or reference. This concept is satisfied either by a reference to component or component itself
///
/// @tparam T Type
template<typename T>
concept component_or_reference = component<T> || component_reference<T>;

/// @brief Decay component; converts component_or_reference to component by removing cv-qualifiers and reference.
///
/// @tparam T Type
template<component_or_reference T>
using decay_component_t = std::remove_cvref_t<T>;

/// @brief Returns true for non-const component references
///
/// @tparam T Type
/// @return true When the component reference is mutable
/// @return false When the component reference is immutable
template<component_or_reference T>
constexpr bool is_mutable_component_reference() noexcept {
    return !std::is_const_v<std::remove_reference_t<T>>;
}

/// @brief Returns true for const component references
///
/// @tparam T Type
/// @return true When the component reference is immutable
/// @return false When the component reference is mutable
template<component_or_reference T>
constexpr bool is_immutable_component_reference() noexcept {
    return std::is_const_v<std::remove_reference_t<T>>;
}

/// @brief Returns component ID for given T type
///
/// @tparam T Component type
/// @return decltype(auto) Component ID
template<component T>
decltype(auto) get_component_id() {
    return component_family::id<decay_component_t<T>>;
}

/// @brief Component metadata. Stores an ID, size, alignment, destructor, etc.
struct component_meta {
public:
    /// @brief Constructs component_meta for type T
    ///
    /// @tparam T Component type
    /// @return const component_meta& Component metadata
    template<component T>
    static const component_meta of() noexcept {
        return component_meta{ get_component_id<T>(),
            sizeof(T),
            alignof(T),
            [](void* ptr, void* rhs) { std::construct_at(static_cast<T*>(ptr), std::move(*static_cast<T*>(rhs))); },
            [](void* lhs, void* rhs) { *static_cast<T*>(lhs) = std::move(*static_cast<T*>(rhs)); },
            [](void* ptr) { static_cast<T*>(ptr)->~T(); } };
    }

    /// @brief Spaceship operator
    ///
    /// @param rhs Right hand side
    /// @return auto Result of comparison
    constexpr auto operator<=>(const component_meta& rhs) const noexcept {
        return id <=> rhs.id;
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    constexpr bool operator==(const component_meta& rhs) const noexcept {
        return id == rhs.id;
    }

    component_id id;
    std::size_t size;
    std::size_t align;
    std::function<void(void*, void*)> move_ctor;
    std::function<void(void*, void*)> move_assign;
    std::function<void(void*)> dtor;
};

/// @brief Component set holds a set of component IDs
class component_set {
public:
    using storage_type = std::uint64_t;

    /// @brief Default constructor
    component_set() = default;

    component_set(const component_set& rhs) = default;
    component_set(component_set&& rhs) = default;
    component_set& operator=(const component_set& rhs) = default;
    component_set& operator=(component_set&& rhs) = default;

    /// @brief Construct component set from given component types
    ///
    /// @tparam Args Components type parameter pack
    /// @return component_set Component set
    template<component... Args>
    static component_set create() {
        component_set s;
        (..., s.insert<Args>());
        return s;
    }

    /// @brief Insert component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void insert() {
        insert(get_component_id<T>());
    }

    /// @brief Erase component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void erase() {
        erase(get_component_id<T>());
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    bool contains() const {
        return contains(get_component_id<T>());
    }

    /// @brief Inserts component into the set
    ///
    /// @param id Component ID
    void insert(component_id id) {
        if (contains(id)) {
            return;
        }
        _component_bitset |= (1 << id);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id id) {
        if (!contains(id)) {
            return;
        }
        _component_bitset &= ~(1 << id);
    }

    /// @brief Check if component is present in the set
    ///
    /// @param id Component ID
    /// @return true When component ID is present
    /// @return false When component ID is not present
    [[nodiscard]] bool contains(component_id id) const {
        if (id >= 64) {
            throw std::invalid_argument("id should be < 64");
        }
        return _component_bitset & (1 << id);
    }

    /// @brief Spaceship operator
    ///
    /// @param rhs Right hand side
    /// @return auto Result of comparison
    auto operator<=>(const component_set& rhs) const noexcept {
        return _component_bitset <=> rhs._component_bitset;
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    bool operator==(const component_set& rhs) const noexcept {
        return _component_bitset == rhs._component_bitset;
    }

    operator bool() const {
        return _component_bitset > 0;
    }

    component_set operator&(const component_set& rhs) const noexcept {
        component_set s;
        s._component_bitset = _component_bitset & rhs._component_bitset;
        return s;
    }

private:
    friend class component_set_hasher;
    storage_type _component_bitset{};
};

/// @brief Component set holds a set of components metadata
class component_meta_set {
public:
    using size_type = std::size_t;
    using value_type = typename std::vector<component_meta>::value_type;
    using const_iterator = typename std::vector<component_meta>::const_iterator;

    /// @brief Default constructor
    component_meta_set() = default;

    component_meta_set(const component_meta_set& rhs) = default;
    component_meta_set(component_meta_set&& rhs) = default;
    component_meta_set& operator=(const component_meta_set& rhs) = default;
    component_meta_set& operator=(component_meta_set&& rhs) = default;

    /// @brief Construct component set from given component types
    ///
    /// @tparam Args Components type parameter pack
    /// @return component_meta_set Component set
    template<component... Args>
    static component_meta_set create() {
        component_meta_set s;
        (..., s.insert<Args>());
        return s;
    }

    /// @brief Insert component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void insert() {
        insert(component_meta::of<T>());
    }

    /// @brief Erase component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void erase() {
        erase(get_component_id<T>());
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    bool contains() const {
        return contains(get_component_id<T>());
    }

    /// @brief Inserts component into the set
    ///
    /// @param meta Component meta
    void insert(const value_type& meta) {
        if (contains(meta.id)) {
            return;
        }
        _bitset.insert(meta.id);
        auto iter = std::lower_bound(_components.begin(), _components.end(), meta);
        _components.emplace(iter, meta);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id id) {
        if (!contains(id)) {
            return;
        }
        _bitset.erase(id);
        auto iter = std::lower_bound(
            _components.begin(), _components.end(), id, [](const auto& meta, auto id) { return meta.id < id; });
        _components.erase(iter);
    }

    /// @brief Check if component is present in the set
    ///
    /// @param id Component ID
    /// @return true When component ID is present
    /// @return false When component ID is not present
    [[nodiscard]] bool contains(component_id id) const {
        return _bitset.contains(id);
    }

    /// @brief Returns how many components in the set
    ///
    /// @return size_type Number of components in the set
    [[nodiscard]] size_type size() const noexcept {
        return _components.size();
    }

    /// @brief Return const iterator to beginning of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator begin() const noexcept {
        return _components.begin();
    }

    /// @brief Return const iterator to the end of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator end() const noexcept {
        return _components.end();
    }

    /// @brief Return const iterator to beginning of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    /// @brief Return const iterator to the end of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    /// @brief Spaceship operator
    ///
    /// @param rhs Right hand side
    /// @return auto Result of comparison
    auto operator<=>(const component_meta_set& rhs) const noexcept {
        return _bitset <=> rhs._bitset;
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    bool operator==(const component_meta_set& rhs) const noexcept {
        return _bitset == rhs._bitset;
    }

    const component_set& bitset() const noexcept {
        return _bitset;
    }

private:
    component_set _bitset;
    std::vector<component_meta> _components;
};

class component_set_hasher {
public:
    std::size_t operator()(const component_set& set) const {
        return std::hash<std::uint64_t>()(set._component_bitset);
    }
};

} // namespace cobalt::ecs