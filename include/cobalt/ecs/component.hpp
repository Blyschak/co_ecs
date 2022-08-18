#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <functional>
#include <numeric>
#include <stdexcept>

#include <cobalt/asl/dynamic_bitset.hpp>
#include <cobalt/asl/family.hpp>
#include <cobalt/asl/type_traits.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/type_meta.hpp>

namespace cobalt::ecs {

/// @brief Type for component ID
using component_id = std::uint32_t;

/// @brief Invalid component ID
constexpr auto invalid_component_id = std::numeric_limits<component_id>::max();

/// @brief Type for family used to generated component IDs.
using component_family = asl::family<struct _component_family_t, component_id>;

/// @brief Component concept. The component must be a struct/class that can be move constructed and move
/// assignable
///
/// @tparam T Component type
template<typename T>
concept component =
    std::is_class_v<T> && std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

/// @brief Component reference concept. It should be a reference or const reference to C, where C satisfies component
/// concept
///
/// @tparam T Component reference type
template<typename T>
concept component_reference = std::is_reference_v<T> && component<std::remove_cvref_t<T>>;

/// @brief Decay component; converts component_reference to component by removing cv-qualifiers and reference.
///
/// @tparam T Type
template<component_reference T>
using decay_component_t = std::decay_t<T>;

/// @brief Struct to determine const-ness of component reference type
///
/// @tparam T component reference type
template<component_reference T>
struct const_component_reference {
    constexpr static bool value = std::is_const_v<std::remove_reference_t<T>>;
};

/// @brief Returns true for const component references
///
/// @tparam T component_reference type
template<component_reference T>
constexpr bool const_component_reference_v = const_component_reference<T>::value;

/// @brief Struct to determine mutability of component reference type
///
/// @tparam T component reference type
template<component_reference T>
struct mutable_component_reference {
    constexpr static bool value = !std::is_const_v<std::remove_reference_t<T>>;
};

/// @brief Returns true for non-const component references
///
/// @tparam T component_reference type
template<component_reference T>
constexpr bool mutable_component_reference_v = mutable_component_reference<T>::value;

/// @brief Struct to determine whether all component references are const
///
/// @tparam Args Component references
template<component_reference... Args>
struct const_component_references {
    constexpr static bool value = std::conjunction_v<const_component_reference<Args>...>;
};

/// @brief Returns true when all Args are const references
///
/// @tparam Args Component references
template<component_reference... Args>
constexpr bool const_component_references_v = const_component_references<Args...>::value;

/// @brief Component metadata. Stores an ID, size, alignment, destructor, etc.
struct component_meta {
public:
    /// @brief Constructs component_meta for type T
    ///
    /// @tparam T Component type
    /// @return component_meta Component metadata
    template<component T>
    static component_meta of() noexcept {
        return component_meta{
            component_family::id<T>,
            type_meta::of<T>(),
        };
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
    const type_meta* type;
};

/// @brief Component set holds a set of component IDs
class component_set {
public:
    using storage_type = asl::dynamic_bitset<>;

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
        insert(component_family::id<T>);
    }

    /// @brief Erase component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void erase() {
        erase(component_family::id<T>);
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    [[nodiscard]] bool contains() const {
        return contains(component_family::id<T>);
    }

    /// @brief Inserts component into the set
    ///
    /// @param id Component ID
    void insert(component_id id) {
        _component_bitset.set(id);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id id) {
        _component_bitset.set(id, false);
    }

    /// @brief Check if component is present in the set
    ///
    /// @param id Component ID
    /// @return true When component ID is present
    /// @return false When component ID is not present
    [[nodiscard]] bool contains(component_id id) const {
        return _component_bitset.test(id);
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    bool operator==(const component_set& rhs) const noexcept {
        return _component_bitset == rhs._component_bitset;
    }

private:
    friend class component_set_hasher;
    storage_type _component_bitset{};
};

/// @brief Component set hasher
class component_set_hasher {
public:
    /// @brief Hash component set
    ///
    /// @param set Component set
    /// @return std::size_t Hash value
    std::size_t operator()(const component_set& set) const {
        return std::hash<typename component_set::storage_type>()(set._component_bitset);
    }
};

/// @brief Component set holds a set of components metadata
class component_meta_set {
public:
    using size_type = std::size_t;
    using storage_type = std::vector<component_meta>;
    using value_type = typename storage_type::value_type;
    using const_iterator = typename storage_type::const_iterator;

    /// @brief Construct component set from given component types
    ///
    /// @tparam Args Components type parameter pack
    /// @return component_meta_set Component set
    template<component... Args>
    static component_meta_set create() {
        component_meta_set s;
        s._components.reserve(sizeof...(Args));
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
        erase(component_family::id<T>);
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    [[nodiscard]] bool contains() const {
        return contains(component_family::id<T>);
    }

    /// @brief Inserts component into the set
    ///
    /// @param meta Component meta
    void insert(const value_type& meta) {
        if (contains(meta.id)) {
            return;
        }
        _bitset.insert(meta.id);
        _components.emplace_back(meta);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id id) {
        if (!contains(id)) {
            return;
        }
        _bitset.erase(id);
        std::erase_if(_components, [id](const auto& meta) { return meta.id == id; });
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

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    bool operator==(const component_meta_set& rhs) const noexcept {
        return _bitset == rhs._bitset;
    }

    /// @brief Return a bitset of components
    ///
    /// @return const component_set& Component bitset
    [[nodiscard]] const component_set& ids() const noexcept {
        return _bitset;
    }

private:
    component_set _bitset;
    storage_type _components;
};

} // namespace cobalt::ecs