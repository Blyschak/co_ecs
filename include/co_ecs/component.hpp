#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <functional>
#include <numeric>
#include <stdexcept>

#include <co_ecs/detail/dynamic_bitset.hpp>
#include <co_ecs/detail/hash_map.hpp>
#include <co_ecs/detail/type_traits.hpp>
#include <co_ecs/type_meta.hpp>

namespace co_ecs {

namespace detail {

/// @brief Type registry is a container holding a mapping between type to its assigned ID.
///
/// @tparam Type it is parametrized over
/// @tparam _id_type Type used for IDs
template<typename = void, typename _id_type = std::uint64_t>
class type_registry {
public:
    using id_type = _id_type;

    /// @brief Get ID of for a type
    ///
    /// @param type_string Name of the type
    /// @return id_type ID assigned to a type
    CO_ECS_API static auto id(std::string_view type_string) -> id_type {
        auto [iter, inserted] = get_id_map().emplace(type_string, get_next_id());
        if (inserted) {
            get_next_id()++;
        }
        auto type_id = iter->second;
        return type_id;
    }

    /// @brief Get ID mapping
    ///
    /// @return hash_map<std::string_view, id_type>& Reference to a hash map: name -> ID
    CO_ECS_API static auto get_id_map() -> hash_map<std::string_view, id_type>& {
        static hash_map<std::string_view, id_type> id_map{};
        return id_map;
    }

    /// @brief Get next free ID
    ///
    /// @return id_type& Next ID value
    CO_ECS_API static auto get_next_id() -> id_type& {
        static id_type next_id{};
        return next_id;
    }
};

/// @brief Generate unique sequential IDs for types
///
/// @tparam Base type
/// @tparam _id_type Type for id
template<typename Base = void, typename _id_type = std::uint64_t>
struct type_id {
    using id_type = _id_type;

    template<typename T>
    inline static const id_type value = type_registry<Base, id_type>::id(type_name<T>());
};

} // namespace detail

/// @brief Type for component ID
using component_id_t = std::uint32_t;

/// @brief Invalid component ID
constexpr auto invalid_component_id = std::numeric_limits<component_id_t>::max();

/// @brief Type for family used to generated component IDs.
using component_id = detail::type_id<struct _component_family_t, component_id_t>;

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
    static auto of() noexcept -> component_meta {
        return component_meta{
            component_id::value<T>,
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
    constexpr auto operator==(const component_meta& rhs) const noexcept -> bool {
        return id == rhs.id;
    }

    component_id_t id;
    const type_meta* type;
};

/// @brief Component set holds a set of component IDs
class component_set {
public:
    using storage_type = detail::dynamic_bitset<>;

    /// @brief Construct component set from given component types
    ///
    /// @tparam Args Components type parameter pack
    /// @return component_set Component set
    template<component... Args>
    static auto create() -> component_set {
        component_set s;
        (..., s.insert<Args>());
        return s;
    }

    /// @brief Insert component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void insert() {
        insert(component_id::value<T>);
    }

    /// @brief Erase component of type T
    ///
    /// @tparam T Component type
    template<component T>
    void erase() {
        erase(component_id::value<T>);
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    [[nodiscard]] auto contains() const -> bool {
        return contains(component_id::value<T>);
    }

    /// @brief Inserts component into the set
    ///
    /// @param id Component ID
    void insert(component_id_t id) {
        _bitset.set(id);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id_t id) {
        _bitset.set(id, false);
    }

    /// @brief Check if component is present in the set
    ///
    /// @param id Component ID
    /// @return true When component ID is present
    /// @return false When component ID is not present
    [[nodiscard]] auto contains(component_id_t id) const -> bool {
        return _bitset.test(id);
    }

    void clear() noexcept {
        _bitset.clear();
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    auto operator==(const component_set& rhs) const noexcept -> bool {
        return _bitset == rhs._bitset;
    }

private:
    friend class component_set_hasher;
    storage_type _bitset{};
};

/// @brief Component set hasher
class component_set_hasher {
public:
    /// @brief Hash component set
    ///
    /// @param set Component set
    /// @return std::size_t Hash value
    auto operator()(const component_set& set) const -> std::size_t {
        return std::hash<typename component_set::storage_type>()(set._bitset);
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
    static auto create() -> component_meta_set {
        component_meta_set s;
        s._components_meta.reserve(sizeof...(Args));
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
        erase(component_id::value<T>);
    }

    /// @brief Check if component of type T is present in the set
    ///
    /// @tparam T Component type
    /// @return true When component type T is present
    /// @return false When component type T is not present
    template<component T>
    [[nodiscard]] auto contains() const -> bool {
        return contains(component_id::value<T>);
    }

    /// @brief Inserts component into the set
    ///
    /// @param meta Component meta
    void insert(const value_type& meta) {
        if (contains(meta.id)) {
            return;
        }
        _component_set.insert(meta.id);
        _components_meta.emplace_back(meta);
    }

    /// @brief Erases component from the set
    ///
    /// @param id Component ID
    void erase(component_id_t id) {
        if (!contains(id)) {
            return;
        }
        _component_set.erase(id);
        std::erase_if(_components_meta, [id](const auto& meta) { return meta.id == id; });
    }

    /// @brief Check if component is present in the set
    ///
    /// @param id Component ID
    /// @return true When component ID is present
    /// @return false When component ID is not present
    [[nodiscard]] auto contains(component_id_t id) const -> bool {
        return _component_set.contains(id);
    }

    /// @brief Returns how many components in the set
    ///
    /// @return size_type Number of components in the set
    [[nodiscard]] auto size() const noexcept -> size_type {
        return _components_meta.size();
    }

    /// @brief Return const iterator to beginning of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] auto begin() const noexcept -> const_iterator {
        return _components_meta.begin();
    }

    /// @brief Return const iterator to the end of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] auto end() const noexcept -> const_iterator {
        return _components_meta.end();
    }

    /// @brief Return const iterator to beginning of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
        return begin();
    }

    /// @brief Return const iterator to the end of the set
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] auto cend() const noexcept -> const_iterator {
        return end();
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side
    /// @return true If equal
    /// @return false If not equal
    auto operator==(const component_meta_set& rhs) const noexcept -> bool {
        return _component_set == rhs._component_set;
    }

    /// @brief Return a bitset of components
    ///
    /// @return const component_set& Component bitset
    [[nodiscard]] auto ids() const noexcept -> const component_set& {
        return _component_set;
    }

private:
    component_set _component_set;
    storage_type _components_meta;
};

} // namespace co_ecs