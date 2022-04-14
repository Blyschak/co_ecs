#pragma once

#include <cobalt/asl/algorithm.hpp>
#include <vector>

namespace cobalt::asl {

/// @brief node_base is a base class for all nodes in our graph. It includes
/// strong links to child nodes, those nodes which are "owned" by an instance of
/// node_base and weak links, or edges, to nodes wich are used for search
/// optimization when one needs a node with a bit of the key added or removed.
/// The graph consists of nodes. If we follow the graph from the root node using
/// strong child relation pointer recursivelly we will find that this graph is
/// sort of a prefix tree. The only difference is that nodes have edges for
/// search, insertion and erasure optimizations. This class uses CRTP idiom in
/// order to return pointers to nodes of user's derived class type.
///
/// @tparam T Key type
/// @tparam derived User's derived type
template<typename T, typename derived>
class node_base {
public:
    using key_type = T;
    using node_type = derived;
    using value_type = typename key_type::value_type;

    /// @brief Edge represents a link to another node
    struct edge {
        value_type value;
        node_type* node;
    };

    /// @brief default constructor
    constexpr node_base() noexcept(std::is_nothrow_constructible_v<key_type>) = default;

    /// @brief Construct node_base with a key
    ///
    /// @param key Key to construct node with
    constexpr node_base(const key_type& key) noexcept(std::is_nothrow_constructible_v<key_type>) : _key(key) {
    }

    /// @brief Deleted copy constructor
    ///
    /// @param rhs Other node_base instance
    constexpr node_base(const node_base& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Other node_base instance
    constexpr node_base& operator=(const node_base& rhs) = delete;

    /// @brief Default move constructor
    ///
    /// @param rhs Other node_base instance
    constexpr node_base(node_base&& rhs) noexcept(std::is_nothrow_move_constructible_v<key_type>) = default;

    /// @brief Default move assignment operator
    ///
    /// @param rhs Other node_base instance
    constexpr node_base& operator=(node_base&& rhs) noexcept(std::is_nothrow_move_assignable_v<key_type>) = default;

    /// @brief Returns key
    ///
    /// @return const key_type& Key associated with this node
    [[nodiscard]] constexpr const key_type& key() const {
        return _key;
    }

private:
    // This class deserves friendship with graph since
    // it requires access to modify links of the node while
    // it is inaccessible from derived class.
    template<typename K, typename A>
    friend class graph;

    /// @brief Return pointer to derived class
    ///
    /// @return node_type* Pointer to derived class
    [[nodiscard]] constexpr node_type* self() noexcept {
        return static_cast<derived*>(this); // safe since we use CRTP
    }

    /// @brief Return pointer to derived class
    ///
    /// @return const node_type* Pointer to derived class
    [[nodiscard]] constexpr const node_type* self() const noexcept {
        return static_cast<const derived*>(this); // safe since we use CRTP
    }

    /// @brief Find a right edge corresponding to passed value and return a
    /// pointer to its node
    ///
    /// @param value Value
    /// @return node_type* Found node, nullptr is returned when not found
    [[nodiscard]] constexpr node_type* right(const value_type& value) noexcept {
        auto it = std::find_if(_right.begin(), _right.end(), [value](const auto& edge) { return edge.value == value; });
        if (it == _right.end()) {
            return nullptr;
        }
        return it->node;
    }

    /// @brief Find a left edge corresponding to passed value and return a
    /// pointer to its node
    ///
    /// @param value Value
    /// @return node_type* Found node, nullptr is returned when not found
    [[nodiscard]] constexpr node_type* left(const value_type& value) noexcept {
        auto it = std::find_if(_left.begin(), _left.end(), [value](const auto& edge) { return edge.value == value; });
        if (it == _left.end()) {
            return nullptr;
        }
        return it->node;
    }

    /// @brief Add child, an owned node, to this node
    ///
    /// @param node Pointer to the node to add to this node as a child
    constexpr void add_child(node_type* node) {
        _childs.emplace_back(node);
    }

    /// @brief Remove child from this node
    ///
    /// @param node Pointer to the node to remove from this node child list
    constexpr void remove_child(const node_type* node) {
        auto iter = std::find(_childs.begin(), _childs.end(), node);
        std::swap(*iter, _childs.back());
        _childs.pop_back();
    }

    /// @brief Link this node with the passed node. This method estanblishes
    /// weak relationship between these nodes. This node will have a right edge
    /// added to the passed node while the passed node will have left edge added
    /// pointing to this node
    ///
    /// @param value Value
    /// @param node Node to estanblish relationship with
    constexpr void link(const value_type& value, node_type* node) {
        _right.emplace_back(value, node);
        node->_left.emplace_back(value, self());
    }

    /// @brief Unlink this node from the passed node. This method will remove
    /// the weak relationship between nodes.
    ///
    /// @param value Value
    /// @param node Node to remove relationships with
    constexpr void unlink(const value_type& value, node_type* node) {
        auto predicate = [value](const auto& edge) { return edge.value == value; };
        auto it = std::find_if(_right.begin(), _right.end(), predicate);
        if (it != _right.end()) {
            std::swap(*it, _right.back());
            _right.pop_back();
        }
        it = std::find_if(node->_left.begin(), node->_left.end(), predicate);
        if (it != node->_left.end()) {
            std::swap(*it, node->_left.back());
            node->_left.pop_back();
        }
    }

    /// @brief Executes func for every right edge node
    ///
    /// @param func Callable to execute
    constexpr void for_each_right_node(auto func) const noexcept(std::is_nothrow_invocable_v<decltype(func)>) {
        for (const auto& edge : _right) {
            func(edge.value, edge.node);
        }
    }

    /// @brief Executes func for every left edge node
    ///
    /// @param func Callable to execute
    constexpr void for_each_left_node(auto func) const noexcept(std::is_nothrow_invocable_v<decltype(func)>) {
        for (const auto& edge : _left) {
            func(edge.value, edge.node);
        }
    }

    /// @brief Executes func for every child node
    ///
    /// @param func Callable to execute
    constexpr void for_each_child(auto func) const noexcept(std::is_nothrow_invocable_v<decltype(func)>) {
        for (const auto& node : _childs) {
            func(node);
        }
    }

    /// @brief Visit nodes starting from current node recursivelly in
    /// depth-first order. If the callable visitor returns false the iteration
    /// is stopped at this node
    ///
    /// @param visitor Callable to execute
    constexpr void visit(auto visitor) noexcept(std::is_nothrow_invocable_v<decltype(visitor)>) {
        if (!visitor(*self())) {
            return;
        }
        for (auto& node : _childs) {
            node->visit(visitor);
        }
    }

    key_type _key;
    std::vector<node_type*> _childs;
    std::vector<edge> _right;
    std::vector<edge> _left;
};

/// @brief Node default implementation.
///
/// @tparam T Key type
template<typename T>
class node : public node_base<T, node<T>> {
public:
    /// @brief Construct node from key
    ///
    /// @param key Key to construct node from
    constexpr node(T key) : node_base<T, node<T>>(key) {
    }
};

/// @brief Graph is a data structure that is organized in a way like a prefix
/// tree is, however it's nodes have additional "weak" pointers for insertion
/// and erasure optimizations.
///
/// @tparam N Node type, a derivative from node_base
/// @tparam A Allocator type for nodes
template<typename N, typename A = std::allocator<N>>
class graph {
public:
    using node_type = N;
    using allocator_type = A;
    using allocator_traits = typename std::allocator_traits<allocator_type>;
    using key_type = typename N::key_type;
    using value_type = typename N::value_type;
    using nodes_vector = std::vector<node_type*>;
    using iterator = typename nodes_vector::iterator;
    using const_iterator = typename nodes_vector::const_iterator;

    /// @brief Construct graph with allocator
    ///
    /// @param allocator Allocator to use for nodes
    constexpr graph(const allocator_type& allocator = allocator_type{}) :
        _alloc(allocator), _root(allocator_traits::allocate(_alloc, 1)) {
        std::uninitialized_default_construct_n(_root, 1);
    }

    /// @brief Deleted copy constructor.
    /// TODO: It is possible to copy, but requires some special handling.
    ///
    /// @param rhs Other graph
    constexpr graph(const graph& rhs) = delete;

    /// @brief Deleted copy assignment operator.
    /// TODO: It is possible to copy, but requires some special handling.
    ///
    /// @param rhs Other graph
    constexpr graph& operator=(const graph& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Other graph
    constexpr graph(graph&& rhs) noexcept {
        swap(std::move(rhs));
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Other graph
    constexpr graph& operator=(graph&& rhs) noexcept {
        swap(std::move(rhs));
        return *this;
    }

    /// @brief Swap graphs
    ///
    /// @param rhs Another graph
    constexpr void swap(graph&& rhs) noexcept {
        std::swap(_alloc, rhs._alloc);
        std::swap(_root, rhs._root);
    }

    /// @brief Destructor of the graph
    constexpr ~graph() {
        if (_root) {
            destroy(_root);
        }
    }

    /// @brief Find node in the graph by key.
    ///
    /// @param key Key to search node for
    /// @return const node_type* Found node
    [[nodiscard]] constexpr const node_type* find(const key_type& key) const noexcept {
        auto* node = _root;
        for (const auto& value : key) {
            if (auto* right = node->right(value); right) {
                node = right;
            } else {
                return nullptr;
            }
        }
        return node;
    }

    /// @brief Find node in the graph by key.
    ///
    /// @param key Key to search node for
    /// @return node_type* Found node
    [[nodiscard]] constexpr node_type* find(const key_type& key) noexcept {
        auto* node = _root;
        for (const auto& value : key) {
            if (auto* right = node->right(value); right) {
                node = right;
            } else {
                return nullptr;
            }
        }
        return node;
    }

    /// @brief Insert new node in the graph constructed with key. If the node
    /// already exists it is returned.
    ///
    /// @param key Key
    /// @return std::pair<node_type*, bool> A pair of boolean telling whether
    /// the node was inserted or it was already found and a pointer to the
    /// desired node
    constexpr std::pair<node_type*, bool> emplace(const key_type& key) {
        auto* node = _root;
        bool inserted{ false };
        for (const auto& value : key) {
            if (auto* right = node->right(value); right) {
                node = right;
            } else {
                auto temp = node->key();
                temp.insert(value);
                auto* new_node = allocator_traits::allocate(_alloc, 1);
                allocator_traits::construct(_alloc, new_node, temp);
                node->add_child(new_node);
                node->link(value, new_node);
                node = new_node;
                _nodes.emplace_back(new_node);
                inserted = true;
            }
        }
        return std::make_pair(node, inserted);
    }

    /// @brief Insert node to the right from the given node adding value to the
    /// given node's key. If the node already exists it is returned.
    ///
    /// @param key Key
    /// @return std::pair<node_type*, bool> A pair of boolean telling whether
    /// the node was inserted or it was already found and a pointer to the
    /// desired node
    constexpr std::pair<node_type*, bool> emplace_right(node_type* node, const value_type& value) {
        if (auto* right = node->right(value); right) {
            return std::make_pair(right, false);
        }
        auto temp = node->key();
        temp.insert(value);
        auto [inserted_node, inserted] = emplace(temp);
        node->link(value, inserted_node);
        _nodes.emplace_back(inserted_node);
        return std::make_pair(inserted_node, inserted);
    }

    /// @brief Insert node to the left from the given node removing value from
    /// the given node's key. If the node already exists it is returned.
    ///
    /// @param key Key
    /// @return std::pair<node_type*, bool> A pair of boolean telling whether
    /// the node was inserted or it was already found and a pointer to the
    /// desired node
    constexpr std::pair<node_type*, bool> emplace_left(node_type* node, const value_type& value) {
        if (auto* left = node->left(value); left) {
            return std::make_pair(left, false);
        }
        auto temp = node->key();
        temp.insert(value);
        auto [inserted_node, inserted] = emplace(temp);
        inserted_node->link(value, node);
        _nodes.emplace_back(inserted_node);
        return std::make_pair(inserted_node, inserted);
    }

    /// @brief Erase the node with given key from the graph. If no node exists
    /// with the given key, nullptr is returned.
    ///
    /// @param key Key
    /// @return std::pair<node_type*, bool> A pair of boolean telling whether
    /// the node was erased or it was not present in the graph and a pointer to
    /// the parent node
    constexpr std::pair<node_type*, bool> erase(const key_type& key) {
        auto* parent = _root;
        auto* node = parent;
        auto first = key.begin();
        auto last = key.end();
        for (; first != last; first++) {
            if (auto* right = node->right(*first); right) {
                parent = node;
                node = right;
            } else {
                return std::make_pair(node, false);
            }
        }
        auto value = *--first;
        parent->unlink(value, node);
        parent->remove_child(node);
        std::erase(_nodes, node);
        destroy(node);
        return std::make_pair(parent, true);
    }

    /// @brief Visit all nodes starting from the root recursivelly in
    /// depth-first order
    ///
    /// @param visitor Visitor callable
    constexpr void visit(auto visitor) noexcept(std::is_nothrow_invocable_v<decltype(visitor)>) {
        _root->visit(visitor);
    }

    /// @brief Return iterator to beginning
    ///
    /// @return iterator Iterator
    [[nodiscard]] iterator begin() noexcept {
        return _nodes.begin();
    }

    /// @brief Return iterator to the end
    ///
    /// @return iterator Iterator
    [[nodiscard]] iterator end() noexcept {
        return _nodes.end();
    }

    /// @brief Return const iterator to beginning
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator begin() const noexcept {
        return _nodes.begin();
    }

    /// @brief Return const iterator to the end
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator end() const noexcept {
        return _nodes.end();
    }

    /// @brief Return const iterator to beginning
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    /// @brief Return const iterator to the end
    ///
    /// @return const_iterator Iterator
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }


private:
    constexpr void destroy(node_type* node) {
        node->for_each_right_node([node](const auto& value, const auto& right) { node->unlink(value, right); });
        node->for_each_left_node([node](const auto& value, const auto& left) { left->unlink(value, node); });
        node->for_each_child([this](const auto& node) { destroy(node); });
        allocator_traits::destroy(_alloc, node);
        allocator_traits::deallocate(_alloc, node, 1);
    }

    allocator_type _alloc;
    node_type* _root{};
    nodes_vector _nodes;
};

} // namespace cobalt::asl