#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <functional>
#include <algorithm>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare
{
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:
    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept = default;
    };

    using node_allocator_type = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<btree_node>;

    pp_allocator<value_type> _allocator;
    node_allocator_type _node_alloc;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    btree_node* allocate_node();
    void deallocate_node(btree_node* node);
    void destroy_subtree(btree_node* node) noexcept;
    btree_node* copy_subtree(const btree_node* src);

    bool insert_non_full(btree_node* node, const tree_data_type& data);
    bool insert_non_full(btree_node* node, tree_data_type&& data);
    void split_child(btree_node* parent, size_t child_idx);

    bool remove_key(btree_node* node, const tkey& key);
    void remove_from_node(btree_node* node, size_t idx);
    void merge(btree_node* node, size_t idx);
    void borrow_from_prev(btree_node* node, size_t idx);
    void borrow_from_next(btree_node* node, size_t idx);

    std::pair<btree_node*, size_t> find_internal(const tkey& key) const;
    btree_node* get_leftmost_leaf(btree_node* root) const;
    btree_node* get_rightmost_leaf(btree_node* root) const;

public:
    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());
    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());

    B_tree(const B_tree& other);
    B_tree(B_tree&& other) noexcept;
    B_tree& operator=(const B_tree& other);
    B_tree& operator=(B_tree&& other) noexcept;
    ~B_tree() noexcept;

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node*, size_t>> _path;
        size_t _index;
    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;
        explicit btree_iterator(const std::stack<std::pair<btree_node*, size_t>>& path = {}, size_t index = 0);
    };

    class btree_const_iterator final
    {
        std::stack<std::pair<const btree_node*, size_t>> _path;
        size_t _index;
    public:
        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;
        explicit btree_const_iterator(const std::stack<std::pair<const btree_node*, size_t>>& path = {}, size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node*, size_t>> _path;
        size_t _index;
    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;
        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node*, size_t>>& path = {}, size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<const btree_node*, size_t>> _path;
        size_t _index;
    public:
        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        self& operator++();
        self operator++(int);
        self& operator--();
        self operator--(int);
        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;
        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;
        explicit btree_const_reverse_iterator(const std::stack<std::pair<const btree_node*, size_t>>& path = {}, size_t index = 0);
    };

    tvalue& at(const tkey& key);
    const tvalue& at(const tkey& key) const;
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    btree_iterator begin();
    btree_iterator end();
    btree_const_iterator begin() const;
    btree_const_iterator end() const;
    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;
    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();
    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;
    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    size_t size() const noexcept;
    bool empty() const noexcept;
    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;
    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;
    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;
    bool contains(const tkey& key) const;

    void clear() noexcept;
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);
    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);
    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);
    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);
    btree_iterator erase(const tkey& key);
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const {
    return static_cast<const compare&>(*this)(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const {
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type>
B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept {
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::allocate_node() {
    btree_node* node = _node_alloc.allocate(1);
    try {
        std::construct_at(node);
    } catch (...) {
        _node_alloc.deallocate(node, 1);
        throw;
    }
    return node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::deallocate_node(btree_node* node) {
    std::destroy_at(node);
    _node_alloc.deallocate(node, 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::destroy_subtree(btree_node* node) noexcept {
    if (!node) return;
    for (auto* child : node->_pointers) {
        destroy_subtree(child);
    }
    deallocate_node(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::copy_subtree(const btree_node* src) {
    if (!src) return nullptr;
    btree_node* node = allocate_node();
    try {
        node->_keys = src->_keys;
        node->_pointers.reserve(src->_pointers.size());
        for (auto* child : src->_pointers) {
            node->_pointers.push_back(copy_subtree(child));
        }
    } catch (...) {
        destroy_subtree(node);
        throw;
    }
    return node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::get_leftmost_leaf(btree_node* root) const {
    if (!root) return nullptr;
    btree_node* cur = root;
    while (!cur->_pointers.empty()) {
        cur = cur->_pointers.front();
    }
    return cur;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::get_rightmost_leaf(btree_node* root) const {
    if (!root) return nullptr;
    btree_node* cur = root;
    while (!cur->_pointers.empty()) {
        cur = cur->_pointers.back();
    }
    return cur;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const compare& cmp, pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _node_alloc(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(pp_allocator<value_type> alloc, const compare& comp)
    : compare(comp), _allocator(alloc), _node_alloc(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc)
    : B_tree(cmp, alloc) {
    for (auto it = begin; it != end; ++it) {
        insert(*it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(std::initializer_list<std::pair<tkey, tvalue>> data,
                                         const compare& cmp, pp_allocator<value_type> alloc)
    : B_tree(cmp, alloc) {
    for (const auto& p : data) {
        insert(p);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
    : compare(other), _allocator(other._allocator), _node_alloc(other._node_alloc), _size(other._size) {
    _root = copy_subtree(other._root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)),
      _node_alloc(std::move(other._node_alloc)), _root(std::exchange(other._root, nullptr)),
      _size(std::exchange(other._size, 0)) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other) {
    if (this != &other) {
        clear();
        static_cast<compare&>(*this) = other;
        _allocator = other._allocator;
        _node_alloc = other._node_alloc;
        _root = copy_subtree(other._root);
        _size = other._size;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept {
    if (this != &other) {
        clear();
        static_cast<compare&>(*this) = std::move(other);
        _allocator = std::move(other._allocator);
        _node_alloc = std::move(other._node_alloc);
        _root = std::exchange(other._root, nullptr);
        _size = std::exchange(other._size, 0);
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept {
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept {
    destroy_subtree(_root);
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_child(btree_node* parent, size_t child_idx) {
    btree_node* child = parent->_pointers[child_idx];
    btree_node* new_child = allocate_node();
    
    size_t mid = t;
    
    tree_data_type median = std::move(child->_keys[mid]);
    
    new_child->_keys.assign(std::make_move_iterator(child->_keys.begin() + mid + 1),
                            std::make_move_iterator(child->_keys.end()));
    child->_keys.erase(child->_keys.begin() + mid, child->_keys.end());
    
    if (!child->_pointers.empty()) {
        new_child->_pointers.assign(std::make_move_iterator(child->_pointers.begin() + mid + 1),
                                    std::make_move_iterator(child->_pointers.end()));
        child->_pointers.erase(child->_pointers.begin() + mid + 1, child->_pointers.end());
    }
    
    parent->_keys.insert(parent->_keys.begin() + child_idx, std::move(median));
    parent->_pointers.insert(parent->_pointers.begin() + child_idx + 1, new_child);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::insert_non_full(btree_node* node, const tree_data_type& data) {
    auto it = std::lower_bound(node->_keys.begin(), node->_keys.end(), data,
                               [this](const tree_data_type& a, const tree_data_type& b) {
                                   return compare_pairs(a, b);
                               });
    size_t idx = it - node->_keys.begin();
    
    if (it != node->_keys.end() && !compare_pairs(data, *it) && !compare_pairs(*it, data)) {
        return false;
    }
    
    if (node->_pointers.empty()) {
        node->_keys.insert(it, data);
        return true;
    } else {
        btree_node* child = node->_pointers[idx];
        if (child->_keys.size() == maximum_keys_in_node) {
            split_child(node, idx);
            if (compare_pairs(node->_keys[idx], data)) {
                child = node->_pointers[idx + 1];
            } else {
                child = node->_pointers[idx];
            }
        }
        return insert_non_full(child, data);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::insert_non_full(btree_node* node, tree_data_type&& data) {
    auto it = std::lower_bound(node->_keys.begin(), node->_keys.end(), data,
                               [this](const tree_data_type& a, const tree_data_type& b) {
                                   return compare_pairs(a, b);
                               });
    size_t idx = it - node->_keys.begin();
    
    if (it != node->_keys.end() && !compare_pairs(data, *it) && !compare_pairs(*it, data)) {
        return false;
    }
    
    if (node->_pointers.empty()) {
        node->_keys.insert(it, std::move(data));
        return true;
    } else {
        btree_node* child = node->_pointers[idx];
        if (child->_keys.size() == maximum_keys_in_node) {
            split_child(node, idx);
            if (compare_pairs(node->_keys[idx], data)) {
                child = node->_pointers[idx + 1];
            } else {
                child = node->_pointers[idx];
            }
        }
        return insert_non_full(child, std::move(data));
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::remove_from_node(btree_node* node, size_t idx) {
    if (node->_pointers.empty()) {
        node->_keys.erase(node->_keys.begin() + idx);
    } else {
        tree_data_type old_key = std::move(node->_keys[idx]);
        btree_node* left_child = node->_pointers[idx];
        btree_node* right_child = node->_pointers[idx + 1];
        
        if (left_child->_keys.size() >= t) {
            btree_node* cur = left_child;
            while (!cur->_pointers.empty()) {
                cur = cur->_pointers.back();
            }
            node->_keys[idx] = cur->_keys.back();
            remove_key(left_child, node->_keys[idx].first);
        } else if (right_child->_keys.size() >= t) {
            btree_node* cur = right_child;
            while (!cur->_pointers.empty()) {
                cur = cur->_pointers.front();
            }
            node->_keys[idx] = cur->_keys.front();
            remove_key(right_child, node->_keys[idx].first);
        } else {
            merge(node, idx);
            remove_key(left_child, old_key.first);
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::merge(btree_node* node, size_t idx) {
    btree_node* left = node->_pointers[idx];
    btree_node* right = node->_pointers[idx + 1];
    
    left->_keys.push_back(std::move(node->_keys[idx]));
    node->_keys.erase(node->_keys.begin() + idx);
    node->_pointers.erase(node->_pointers.begin() + idx + 1);
    
    left->_keys.insert(left->_keys.end(),
                       std::make_move_iterator(right->_keys.begin()),
                       std::make_move_iterator(right->_keys.end()));
    left->_pointers.insert(left->_pointers.end(),
                           std::make_move_iterator(right->_pointers.begin()),
                           std::make_move_iterator(right->_pointers.end()));
    
    deallocate_node(right);
    
    if (node == _root && node->_keys.empty()) {
        _root = left;
        deallocate_node(node);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_prev(btree_node* node, size_t idx) {
    btree_node* child = node->_pointers[idx];
    btree_node* left_sibling = node->_pointers[idx - 1];
    
    child->_keys.insert(child->_keys.begin(), std::move(node->_keys[idx - 1]));
    if (!child->_pointers.empty()) {
        child->_pointers.insert(child->_pointers.begin(), left_sibling->_pointers.back());
        left_sibling->_pointers.pop_back();
    }
    node->_keys[idx - 1] = std::move(left_sibling->_keys.back());
    left_sibling->_keys.pop_back();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::borrow_from_next(btree_node* node, size_t idx) {
    btree_node* child = node->_pointers[idx];
    btree_node* right_sibling = node->_pointers[idx + 1];
    
    child->_keys.push_back(std::move(node->_keys[idx]));
    if (!child->_pointers.empty()) {
        child->_pointers.push_back(right_sibling->_pointers.front());
        right_sibling->_pointers.erase(right_sibling->_pointers.begin());
    }
    node->_keys[idx] = std::move(right_sibling->_keys.front());
    right_sibling->_keys.erase(right_sibling->_keys.begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::remove_key(btree_node* node, const tkey& key) {
    auto it = std::lower_bound(node->_keys.begin(), node->_keys.end(), key,
                               [this](const tree_data_type& a, const tkey& b) {
                                   return compare_keys(a.first, b);
                               });
    size_t idx = it - node->_keys.begin();
    bool found = (it != node->_keys.end() && !compare_keys(key, it->first));
    
    if (found) {
        if (node->_pointers.empty()) {
            node->_keys.erase(it);
            return true;
        } else {
            if (node->_pointers[idx]->_keys.size() < t) {
                if (idx > 0 && node->_pointers[idx - 1]->_keys.size() >= t) {
                    borrow_from_prev(node, idx);
                } else if (idx + 1 < node->_pointers.size() && node->_pointers[idx + 1]->_keys.size() >= t) {
                    borrow_from_next(node, idx);
                } else {
                    if (idx > 0) {
                        merge(node, idx - 1);
                        idx--;
                    } else {
                        merge(node, idx);
                    }
                }
            }
            remove_key(node->_pointers[idx], key);
        }
    } else {
        if (node->_pointers.empty()) {
            return false;
        }
        if (node->_pointers[idx]->_keys.size() < t) {
            if (idx > 0 && node->_pointers[idx - 1]->_keys.size() >= t) {
                borrow_from_prev(node, idx);
            } else if (idx + 1 < node->_pointers.size() && node->_pointers[idx + 1]->_keys.size() >= t) {
                borrow_from_next(node, idx);
            } else {
                if (idx > 0) {
                    merge(node, idx - 1);
                    idx--;
                } else {
                    merge(node, idx);
                }
            }
        }
        return remove_key(node->_pointers[idx], key);
    }
    return found;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node*, size_t>
B_tree<tkey, tvalue, compare, t>::find_internal(const tkey& key) const {
    btree_node* cur = _root;
    while (cur) {
        auto it = std::lower_bound(cur->_keys.begin(), cur->_keys.end(), key,
                                   [this](const tree_data_type& a, const tkey& b) {
                                       return compare_keys(a.first, b);
                                   });
        size_t idx = it - cur->_keys.begin();
        if (it != cur->_keys.end() && !compare_keys(key, it->first) && !compare_keys(it->first, key)) {
            return {cur, idx};
        }
        if (cur->_pointers.empty()) {
            break;
        }
        cur = cur->_pointers[idx];
    }
    return {nullptr, 0};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(const std::stack<std::pair<btree_node*, size_t>>& path, size_t index)
    : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept {
    btree_node* node = _path.top().first;
    return reinterpret_cast<reference>(node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++() {
    if (_path.empty()) return *this;
    btree_node* node = _path.top().first;
    size_t idx = _path.top().second;

    if (!node->_pointers.empty() && idx < node->_pointers.size()) {
        node = node->_pointers[idx + 1];
        ++_path.top().second;
        _path.push({node, 0});
        while (!node->_pointers.empty()) {
            node = node->_pointers[0];
            _path.push({node, 0});
        }
    } else {
        if (idx + 1 < node->_keys.size()) {
            ++_path.top().second;
        } else {
            _path.pop();
            while (!_path.empty() && _path.top().second == _path.top().first->_keys.size()) {
                _path.pop();
            }
        }
    }

    _index = _path.empty() ? 0 : _path.top().second;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int) {
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--() {
    if (_path.empty()) return *this;
    btree_node* node = _path.top().first;
    size_t idx = _path.top().second;

    if (!node->_pointers.empty() && idx > 0) {
        node = node->_pointers[idx];
        --_path.top().second;
        _path.push({node, node->_keys.size()});
        while (!node->_pointers.empty()) {
            node = node->_pointers.back();
            _path.push({node, node->_keys.size()});
        }
    } else {
        if (idx > 0) {
            --_path.top().second;
        } else {
            _path.pop();
            while (!_path.empty() && _path.top().second == 0) {
                _path.pop();
            }
            if (!_path.empty()) {
                --_path.top().second;
            }
        }
    }

    _index = _path.empty() ? 0 : _path.top().second;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int) {
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept {
    if (_path.empty() && other._path.empty()) return true;
    if (_path.empty() || other._path.empty()) return false;
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept {
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept {
    if (_path.empty()) return 0;
    return _path.top().first->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept {
    if (_path.empty()) return false;
    return _path.top().first->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(const btree_iterator& it) noexcept
    : _index(it._index) {
    std::stack<std::pair<btree_node*, size_t>> tmp = it._path;
    std::stack<std::pair<const btree_node*, size_t>> reversed;
    while (!tmp.empty()) {
        reversed.push({tmp.top().first, tmp.top().second});
        tmp.pop();
    }
    while (!reversed.empty()) {
        _path.push(reversed.top());
        reversed.pop();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(const std::stack<std::pair<const btree_node*, size_t>>& path, size_t index)
    : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept {
    const btree_node* node = _path.top().first;
    return reinterpret_cast<reference>(node->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++() {
    if (_path.empty()) return *this;
    const btree_node* node = _path.top().first;
    size_t idx = _path.top().second;

    if (!node->_pointers.empty() && idx < node->_pointers.size()) {
        node = node->_pointers[idx + 1];
        ++_path.top().second;
        _path.push({node, 0});
        while (!node->_pointers.empty()) {
            node = node->_pointers[0];
            _path.push({node, 0});
        }
    } else {
        if (idx + 1 < node->_keys.size()) {
            ++_path.top().second;
        } else {
            _path.pop();
            while (!_path.empty() && _path.top().second == _path.top().first->_keys.size()) {
                _path.pop();
            }
        }
    }

    _index = _path.empty() ? 0 : _path.top().second;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int) {
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--() {
    if (_path.empty()) return *this;
    const btree_node* node = _path.top().first;
    size_t idx = _path.top().second;

    if (!node->_pointers.empty() && idx > 0) {
        node = node->_pointers[idx];
        --_path.top().second;
        _path.push({node, node->_keys.size()});
        while (!node->_pointers.empty()) {
            node = node->_pointers.back();
            _path.push({node, node->_keys.size()});
        }
    } else {
        if (idx > 0) {
            --_path.top().second;
        } else {
            _path.pop();
            while (!_path.empty() && _path.top().second == 0) {
                _path.pop();
            }
            if (!_path.empty()) {
                --_path.top().second;
            }
        }
    }

    _index = _path.empty() ? 0 : _path.top().second;
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int) {
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept {
    if (_path.empty() && other._path.empty()) return true;
    if (_path.empty() || other._path.empty()) return false;
    return _path.top().first == other._path.top().first && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept {
    return _path.empty() ? 0 : _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept {
    if (_path.empty()) return 0;
    return _path.top().first->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept {
    if (_path.empty()) return false;
    return _path.top().first->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(const btree_iterator& it) noexcept
    : _path(it._path), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator btree_iterator() const noexcept {
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept {
    btree_iterator it(_path, _index);
    --it;
    return *it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++() {
    btree_iterator it(_path, _index);
    --it;
    *this = btree_reverse_iterator(it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int) {
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--() {
    btree_iterator it(_path, _index);
    ++it;
    *this = btree_reverse_iterator(it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int) {
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept {
    return btree_iterator(_path, _index) == btree_iterator(other._path, other._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept {
    return btree_iterator(_path, _index).depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept {
    return btree_iterator(_path, _index).current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept {
    return btree_iterator(_path, _index).is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept {
    return btree_iterator(_path, _index).index();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(const std::stack<std::pair<btree_node*, size_t>>& path, size_t index)
    : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept
    : _path(it._path), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator btree_const_iterator() const noexcept {
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept {
    btree_const_iterator it(_path, _index);
    --it;
    return *it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept {
    return &(operator*());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++() {
    btree_const_iterator it(_path, _index);
    --it;
    *this = btree_const_reverse_iterator(it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int) {
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::self&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--() {
    btree_const_iterator it(_path, _index);
    ++it;
    *this = btree_const_reverse_iterator(it);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::self
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int) {
    self tmp = *this;
    --(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept {
    return btree_const_iterator(_path, _index) == btree_const_iterator(other._path, other._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept {
    return btree_const_iterator(_path, _index).depth();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept {
    return btree_const_iterator(_path, _index).current_node_keys_count();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept {
    return btree_const_iterator(_path, _index).is_terminate_node();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept {
    return btree_const_iterator(_path, _index).index();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(const std::stack<std::pair<const btree_node*, size_t>>& path, size_t index)
    : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) {
    auto it = find(key);
    if (it == end()) throw std::out_of_range("B_tree::at: key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const {
    auto it = find(key);
    if (it == end()) throw std::out_of_range("B_tree::at: key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key) {
    auto it = find(key);
    if (it == end()) {
        auto [new_it, inserted] = emplace(key, tvalue{});
        return new_it->second;
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key) {
    auto it = find(key);
    if (it == end()) {
        auto [new_it, inserted] = emplace(std::move(key), tvalue{});
        return new_it->second;
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::begin() {
    if (!_root) return end();
    std::stack<std::pair<btree_node*, size_t>> path;
    btree_node* cur = _root;
    while (!cur->_pointers.empty()) {
        path.push({cur, 0});
        cur = cur->_pointers.front();
    }
    path.push({cur, 0});
    return btree_iterator(path, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::end() {
    return btree_iterator({}, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::begin() const {
    return const_cast<B_tree*>(this)->begin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::end() const {
    return btree_const_iterator({}, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::cbegin() const {
    return begin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::cend() const {
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rbegin() {
    return btree_reverse_iterator(--end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rend() {
    return btree_reverse_iterator(begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rbegin() const {
    return btree_const_reverse_iterator(--cend());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::rend() const {
    return btree_const_reverse_iterator(cbegin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::crbegin() const {
    return rbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::crend() const {
    return rend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept {
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept {
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::find(const tkey& key) {
    auto [node, idx] = find_internal(key);
    if (!node) return end();
    std::stack<std::pair<btree_node*, size_t>> path;
    btree_node* cur = _root;
    while (cur != node) {
        auto it = std::lower_bound(cur->_keys.begin(), cur->_keys.end(), key,
                                   [this](const tree_data_type& a, const tkey& b) {
                                       return compare_keys(a.first, b);
                                   });
        size_t child_idx = it - cur->_keys.begin();
        path.push({cur, child_idx});
        cur = cur->_pointers[child_idx];
    }
    path.push({node, idx});
    return btree_iterator(path, idx);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const {
    return const_cast<B_tree*>(this)->find(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const {
    return find(key) != end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) {
    auto it = begin();
    while (it != end() && compare_keys(it->first, key)) ++it;
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const {
    return const_cast<B_tree*>(this)->lower_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) {
    auto it = begin();
    while (it != end() && !compare_keys(key, it->first)) ++it;
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const {
    return const_cast<B_tree*>(this)->upper_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data) {
    if (!_root) {
        _root = allocate_node();
        _root->_keys.push_back(data);
        _size = 1;
        return {begin(), true};
    }
    if (_root->_keys.size() == maximum_keys_in_node) {
        btree_node* new_root = allocate_node();
        new_root->_pointers.push_back(_root);
        split_child(new_root, 0);
        _root = new_root;
    }
    bool inserted = insert_non_full(_root, data);
    if (inserted) ++_size;
    auto it = find(data.first);
    return {it, inserted};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data) {
    if (!_root) {
        _root = allocate_node();
        _root->_keys.push_back(std::move(data));
        _size = 1;
        return {begin(), true};
    }
    if (_root->_keys.size() == maximum_keys_in_node) {
        btree_node* new_root = allocate_node();
        new_root->_pointers.push_back(_root);
        split_child(new_root, 0);
        _root = new_root;
    }
    bool inserted = insert_non_full(_root, std::move(data));
    if (inserted) ++_size;
    auto it = find(data.first);
    return {it, inserted};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args) {
    tree_data_type data(std::forward<Args>(args)...);
    return insert(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data) {
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data) {
    auto it = find(data.first);
    if (it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    return insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args) {
    tree_data_type data(std::forward<Args>(args)...);
    return insert_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos) {
    if (pos == end()) return end();
    tkey key = pos->first;
    return erase(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos) {
    return erase(btree_iterator(const_cast<btree_node*>(pos._path.top().first), pos._index));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en) {
    while (beg != en) beg = erase(beg);
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en) {
    return erase(btree_iterator(const_cast<btree_node*>(beg._path.top().first), beg._index),
                 btree_iterator(const_cast<btree_node*>(en._path.top().first), en._index));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key) {
    auto it = find(key);
    if (it == end()) return end();
    if (_root && remove_key(_root, key)) {
        --_size;
        if (_root && _root->_keys.empty()) {
            if (!_root->_pointers.empty()) {
                btree_node* new_root = _root->_pointers.front();
                deallocate_node(_root);
                _root = new_root;
            } else {
                deallocate_node(_root);
                _root = nullptr;
            }
        }
    }
    return ++it;
}

#endif // SYS_PROG_B_TREE_H