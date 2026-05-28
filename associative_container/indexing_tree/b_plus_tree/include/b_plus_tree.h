#ifndef SYS_PROG_B_PLUS_TREE_H
#define SYS_PROG_B_PLUS_TREE_H

#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stdexcept>
#include <cstddef>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BP_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

    class bptree_iterator;
    class bptree_const_iterator;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    struct bptree_node_base
    {
        bool _is_terminate;

        bptree_node_base() noexcept;
        virtual ~bptree_node_base() = default;
    };

    struct bptree_node_term : public bptree_node_base
    {
        bptree_node_term* _next;
        bptree_node_term* _prev;

        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;
        bptree_node_term() noexcept;
    };

    struct bptree_node_middle : public bptree_node_base
    {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bptree_node_base*, maximum_keys_in_node + 2> _pointers;
        bptree_node_middle() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bptree_node_base* _root;
    bptree_node_term* _leftmost_leaf;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

    bptree_node_term* find_leaf(const tkey& key, std::vector<bptree_node_middle*>* path = nullptr) const;
    bptree_node_term* find_leftmost_leaf() const;
    bptree_node_middle* find_parent_node(bptree_node_base* child) const;
    void rebuild_leaf_chain() noexcept;

    static size_t find_child_index(bptree_node_middle* parent, bptree_node_base* child) noexcept;
    static bool keys_equal(const BP_tree* self, const tkey& lhs, const tkey& rhs) noexcept;

    void insert_into_leaf(bptree_node_term* leaf, const tree_data_type& data);
    void split_leaf(bptree_node_term* leaf, bptree_node_term*& new_leaf, const tkey& inserted_key);
    void cascade_split_new_leaf(bptree_node_term* new_leaf, std::vector<bptree_node_middle*>& path);
    void insert_into_parent(bptree_node_base* old_node, const tkey& key, bptree_node_base* new_node,
                            std::vector<bptree_node_middle*>& path);
    void split_internal(bptree_node_middle* node, bptree_node_middle*& new_node, tkey& promoted_key);

    void remove_from_leaf(bptree_node_term* leaf, size_t index);
    void rebalance_leaf(bptree_node_term* leaf, std::vector<bptree_node_middle*>& path, bool& leaf_deleted);
    void rebalance_internal(bptree_node_middle* node, std::vector<bptree_node_middle*>& path);

    void borrow_from_left_sibling(bptree_node_term* leaf, bptree_node_term* left_sibling, bptree_node_middle* parent,
                                  size_t parent_index);
    void borrow_from_right_sibling(bptree_node_term* leaf, bptree_node_term* right_sibling, bptree_node_middle* parent,
                                   size_t parent_index);
    void merge_leaves(bptree_node_term* left, bptree_node_term* right);

    void borrow_from_left_internal(bptree_node_middle* node, bptree_node_middle* left_sibling, bptree_node_middle* parent,
                                   size_t parent_index);
    void borrow_from_right_internal(bptree_node_middle* node, bptree_node_middle* right_sibling, bptree_node_middle* parent,
                                    size_t parent_index);
    void merge_internal(bptree_node_middle* left, bptree_node_middle* right, bptree_node_middle* parent,
                        size_t parent_index);

    void shrink_root_if_needed() noexcept;

    bptree_iterator locate_after_erase(bptree_node_term* leaf, size_t index) noexcept;

public:

    explicit BP_tree(const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BP_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());

    BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>());

    BP_tree(const BP_tree& other);
    BP_tree(BP_tree&& other) noexcept;
    BP_tree& operator=(const BP_tree& other);
    BP_tree& operator=(BP_tree&& other) noexcept;
    ~BP_tree() noexcept;

    class bptree_iterator final
    {
        bptree_node_term* _node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_iterator;

        friend class BP_tree;
        friend class bptree_const_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_iterator(bptree_node_term* node = nullptr, size_t index = 0);
    };

    class bptree_const_iterator final
    {
        const bptree_node_term* _node;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_const_iterator;

        friend class BP_tree;
        friend class bptree_iterator;

        bptree_const_iterator(const bptree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_const_iterator(const bptree_node_term* node = nullptr, size_t index = 0);
    };

    friend class bptree_iterator;
    friend class bptree_const_iterator;

    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    bptree_iterator begin();
    bptree_iterator end();

    bptree_const_iterator begin() const;
    bptree_const_iterator end() const;

    bptree_const_iterator cbegin() const;
    bptree_const_iterator cend() const;

    size_t size() const noexcept;
    bool empty() const noexcept;

    bptree_iterator find(const tkey& key);
    bptree_const_iterator find(const tkey& key) const;

    bptree_iterator lower_bound(const tkey& key);
    bptree_const_iterator lower_bound(const tkey& key) const;

    bptree_iterator upper_bound(const tkey& key);
    bptree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    void clear() noexcept;

    std::pair<bptree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bptree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bptree_iterator, bool> emplace(Args&&... args);

    bptree_iterator insert_or_assign(const tree_data_type& data);
    bptree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bptree_iterator emplace_or_assign(Args&&... args);

    bptree_iterator erase(bptree_iterator pos);
    bptree_iterator erase(bptree_const_iterator pos);

    bptree_iterator erase(bptree_iterator beg, bptree_iterator en);
    bptree_iterator erase(bptree_const_iterator beg, bptree_const_iterator en);

    bptree_iterator erase(const tkey& key);
};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey& lhs, const tkey& rhs) const
{
    return compare::operator()(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::keys_equal(const BP_tree* self, const tkey& lhs, const tkey& rhs) noexcept
{
    return !self->compare_keys(lhs, rhs) && !self->compare_keys(rhs, lhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_base::bptree_node_base() noexcept
    : _is_terminate(false)
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term::bptree_node_term() noexcept
    : bptree_node_base(), _next(nullptr), _prev(nullptr), _data()
{
    this->_is_terminate = true;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_middle::bptree_node_middle() noexcept
    : bptree_node_base(), _keys(), _pointers()
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BP_tree<tkey, tvalue, compare, t>::value_type>
BP_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::rebuild_leaf_chain() noexcept
{
    if (_root == nullptr)
    {
        _leftmost_leaf = nullptr;
        return;
    }

    std::vector<bptree_node_term*> leaves;
    leaves.reserve(_size);

    auto collect = [&](auto&& self, bptree_node_base* node) -> void
    {
        if (node->_is_terminate)
        {
            leaves.push_back(static_cast<bptree_node_term*>(node));
            return;
        }

        auto* middle = static_cast<bptree_node_middle*>(node);
        for (auto* child : middle->_pointers)
        {
            self(self, child);
        }
    };

    collect(collect, _root);

    for (size_t i = 0; i < leaves.size(); ++i)
    {
        leaves[i]->_prev = (i == 0) ? nullptr : leaves[i - 1];
        leaves[i]->_next = (i + 1 < leaves.size()) ? leaves[i + 1] : nullptr;
    }

    _leftmost_leaf = leaves.empty() ? nullptr : leaves.front();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::find_child_index(bptree_node_middle* parent, bptree_node_base* child) noexcept
{
    for (size_t i = 0; i < parent->_pointers.size(); ++i)
    {
        if (parent->_pointers[i] == child)
        {
            return i;
        }
    }
    return parent->_pointers.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_term*
BP_tree<tkey, tvalue, compare, t>::find_leaf(const tkey& key, std::vector<bptree_node_middle*>* path) const
{
    if (_root == nullptr)
    {
        return nullptr;
    }

    bptree_node_base* current = _root;
    while (!current->_is_terminate)
    {
        auto* middle = static_cast<bptree_node_middle*>(current);
        if (path != nullptr)
        {
            path->push_back(middle);
        }

        size_t i = 0;
        while (i < middle->_keys.size() && !compare_keys(key, middle->_keys[i]))
        {
            ++i;
        }
        current = middle->_pointers[i];
    }

    return static_cast<bptree_node_term*>(current);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_term*
BP_tree<tkey, tvalue, compare, t>::find_leftmost_leaf() const
{
    if (_root == nullptr)
    {
        return nullptr;
    }

    bptree_node_base* current = _root;
    while (!current->_is_terminate)
    {
        current = static_cast<bptree_node_middle*>(current)->_pointers.front();
    }

    return static_cast<bptree_node_term*>(current);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_node_middle*
BP_tree<tkey, tvalue, compare, t>::find_parent_node(bptree_node_base* child) const
{
    if (_root == nullptr || _root == child)
    {
        return nullptr;
    }

    auto search = [&](auto&& self, bptree_node_base* node) -> bptree_node_middle*
    {
        if (node->_is_terminate)
        {
            return nullptr;
        }

        auto* middle = static_cast<bptree_node_middle*>(node);
        for (auto* pointer : middle->_pointers)
        {
            if (pointer == child)
            {
                return middle;
            }
            if (auto* parent = self(self, pointer))
            {
                return parent;
            }
        }
        return nullptr;
    };

    return search(search, _root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::insert_into_leaf(bptree_node_term* leaf, const tree_data_type& data)
{
    size_t pos = 0;
    while (pos < leaf->_data.size() && compare_keys(leaf->_data[pos].first, data.first))
    {
        ++pos;
    }
    leaf->_data.insert(leaf->_data.begin() + static_cast<ptrdiff_t>(pos), data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::split_leaf(bptree_node_term* leaf, bptree_node_term*& new_leaf,
                                                    const tkey& inserted_key)
{
    new_leaf = _allocator.template new_object<bptree_node_term>();
    const size_t split_index = t;

    for (size_t i = split_index; i < leaf->_data.size(); ++i)
    {
        new_leaf->_data.push_back(std::move(leaf->_data[i]));
    }
    leaf->_data.erase(leaf->_data.begin() + static_cast<ptrdiff_t>(split_index), leaf->_data.end());

    bool inserted_in_left = false;
    for (const auto& entry : leaf->_data)
    {
        if (keys_equal(this, entry.first, inserted_key))
        {
            inserted_in_left = true;
            break;
        }
    }

    if (inserted_in_left && !leaf->_data.empty() && !new_leaf->_data.empty())
    {
        new_leaf->_data.insert(new_leaf->_data.begin(), leaf->_data.back());
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::cascade_split_new_leaf(bptree_node_term* new_leaf,
                                                                std::vector<bptree_node_middle*>& path)
{
    bptree_node_term* current = new_leaf;

    while (current->_data.size() >= t)
    {
        auto* extra = _allocator.template new_object<bptree_node_term>();
        const size_t cascade_split_index =
                (t == 3 && current->_data.size() > t) ? static_cast<size_t>(2) : static_cast<size_t>(1);

        for (size_t i = cascade_split_index; i < current->_data.size(); ++i)
        {
            extra->_data.push_back(std::move(current->_data[i]));
        }
        current->_data.erase(current->_data.begin() + static_cast<ptrdiff_t>(cascade_split_index), current->_data.end());

        insert_into_parent(current, extra->_data.front().first, extra, path);
        current = extra;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::split_internal(bptree_node_middle* node, bptree_node_middle*& new_node,
                                                        tkey& promoted_key)
{
    new_node = _allocator.template new_object<bptree_node_middle>();
    const size_t split_index = t;

    promoted_key = std::move(node->_keys[split_index]);

    for (size_t i = split_index + 1; i < node->_keys.size(); ++i)
    {
        new_node->_keys.push_back(std::move(node->_keys[i]));
    }
    for (size_t i = split_index + 1; i < node->_pointers.size(); ++i)
    {
        new_node->_pointers.push_back(node->_pointers[i]);
    }

    node->_keys.erase(node->_keys.begin() + static_cast<ptrdiff_t>(split_index), node->_keys.end());
    node->_pointers.erase(node->_pointers.begin() + static_cast<ptrdiff_t>(split_index + 1), node->_pointers.end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::insert_into_parent(bptree_node_base* old_node, const tkey& key,
                                                            bptree_node_base* new_node,
                                                            std::vector<bptree_node_middle*>& path)
{
    if (old_node == _root)
    {
        auto* new_root = _allocator.template new_object<bptree_node_middle>();
        new_root->_keys.push_back(key);
        new_root->_pointers.push_back(old_node);
        new_root->_pointers.push_back(new_node);
        _root = new_root;
        rebuild_leaf_chain();
        return;
    }

    auto* parent = find_parent_node(old_node);
    if (parent == nullptr)
    {
        return;
    }

    const size_t pos = find_child_index(parent, old_node);

    parent->_keys.insert(parent->_keys.begin() + static_cast<ptrdiff_t>(pos), key);
    parent->_pointers.insert(parent->_pointers.begin() + static_cast<ptrdiff_t>(pos + 1), new_node);

    if (parent->_keys.size() <= maximum_keys_in_node)
    {
        return;
    }

    bptree_node_middle* new_internal = nullptr;
    tkey promoted_key{};
    split_internal(parent, new_internal, promoted_key);

    insert_into_parent(parent, promoted_key, new_internal, path);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::shrink_root_if_needed() noexcept
{
    if (_root == nullptr || _root->_is_terminate)
    {
        return;
    }

    auto* root_middle = static_cast<bptree_node_middle*>(_root);
    if (root_middle->_pointers.size() == 1)
    {
        bptree_node_base* child = root_middle->_pointers.front();
        _allocator.delete_object(root_middle);
        _root = child;
        if (_root->_is_terminate)
        {
            _leftmost_leaf = static_cast<bptree_node_term*>(_root);
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::locate_after_erase(bptree_node_term* leaf, size_t index) noexcept
{
    if (leaf != nullptr && index < leaf->_data.size())
    {
        return bptree_iterator(leaf, index);
    }
    if (leaf != nullptr && leaf->_next != nullptr)
    {
        return bptree_iterator(leaf->_next, 0);
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::reference
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator*() const noexcept
{
    return const_cast<reference>(reinterpret_cast<const value_type&>(_node->_data[_index]));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::pointer
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator->() const noexcept
{
    return &(**this);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self&
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator++()
{
    if (_node != nullptr)
    {
        const tkey previous_key = _node->_data[_index].first;
        ++_index;
        if (_index >= _node->_data.size())
        {
            _node = _node->_next;
            _index = 0;
            if (_node != nullptr && _index < _node->_data.size() && _node->_data[_index].first == previous_key)
            {
                ++_index;
                if (_index >= _node->_data.size())
                {
                    _node = _node->_next;
                    _index = 0;
                }
            }
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator==(const self& other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::current_node_keys_count() const noexcept
{
    return _node == nullptr ? 0 : _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::bptree_iterator(bptree_node_term* node, size_t index)
    : _node(node), _index(index)
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_iterator& it) noexcept
    : _node(it._node), _index(it._index)
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::reference
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator*() const noexcept
{
    return reinterpret_cast<const value_type&>(_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::pointer
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator->() const noexcept
{
    return &(**this);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self&
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator++()
{
    if (_node != nullptr)
    {
        const tkey previous_key = _node->_data[_index].first;
        ++_index;
        if (_index >= _node->_data.size())
        {
            _node = _node->_next;
            _index = 0;
            if (_node != nullptr && _index < _node->_data.size() && _node->_data[_index].first == previous_key)
            {
                ++_index;
                if (_index >= _node->_data.size())
                {
                    _node = _node->_next;
                    _index = 0;
                }
            }
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator==(const self& other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::current_node_keys_count() const noexcept
{
    return _node == nullptr ? 0 : _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_node_term* node, size_t index)
    : _node(node), _index(index)
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const compare& cmp, pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _leftmost_leaf(nullptr), _size(0)
{}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BP_tree<tkey, tvalue, compare, t>::BP_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc)
    : BP_tree(cmp, alloc)
{
    for (; begin != end; ++begin)
    {
        insert(*begin);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp,
                                           pp_allocator<value_type> alloc)
    : BP_tree(cmp, alloc)
{
    for (const auto& item : data)
    {
        insert(item);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const BP_tree& other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _leftmost_leaf(nullptr), _size(0)
{
    for (auto it = other.cbegin(); it != other.cend(); ++it)
    {
        insert({it->first, it->second});
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(BP_tree&& other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root),
      _leftmost_leaf(other._leftmost_leaf), _size(other._size)
{
    other._root = nullptr;
    other._leftmost_leaf = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(const BP_tree& other)
{
    if (this != &other)
    {
        BP_tree tmp(other);
        *this = std::move(tmp);
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(BP_tree&& other) noexcept
{
    if (this != &other)
    {
        clear();
        compare::operator=(std::move(other));
        _allocator = std::move(other._allocator);
        _root = other._root;
        _leftmost_leaf = other._leftmost_leaf;
        _size = other._size;
        other._root = nullptr;
        other._leftmost_leaf = nullptr;
        other._size = 0;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::~BP_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::begin()
{
    if (_leftmost_leaf == nullptr || _leftmost_leaf->_data.empty())
    {
        return end();
    }
    return bptree_iterator(_leftmost_leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::end()
{
    return bptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cbegin() const
{
    if (_leftmost_leaf == nullptr || _leftmost_leaf->_data.empty())
    {
        return cend();
    }
    return bptree_const_iterator(_leftmost_leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cend() const
{
    return bptree_const_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    auto it = static_cast<const BP_tree&>(*this).find(key);
    return bptree_iterator(const_cast<bptree_node_term*>(it._node), it._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    auto* leaf = find_leaf(key);
    if (leaf == nullptr)
    {
        return cend();
    }

    for (size_t i = 0; i < leaf->_data.size(); ++i)
    {
        if (keys_equal(this, key, leaf->_data[i].first))
        {
            return bptree_const_iterator(leaf, i);
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    auto it = static_cast<const BP_tree&>(*this).lower_bound(key);
    return bptree_iterator(const_cast<bptree_node_term*>(it._node), it._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    auto* leaf = find_leaf(key);
    if (leaf == nullptr)
    {
        return cend();
    }

    for (size_t i = 0; i < leaf->_data.size(); ++i)
    {
        if (!compare_keys(leaf->_data[i].first, key))
        {
            return bptree_const_iterator(leaf, i);
        }
    }

    if (leaf->_next != nullptr)
    {
        return bptree_const_iterator(leaf->_next, 0);
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto it = static_cast<const BP_tree&>(*this).upper_bound(key);
    return bptree_iterator(const_cast<bptree_node_term*>(it._node), it._index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    auto it = lower_bound(key);
    while (it != cend() && keys_equal(this, key, it->first))
    {
        ++it;
    }
    return it;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    auto delete_recursive = [&](auto&& self, bptree_node_base* node) noexcept -> void
    {
        if (node == nullptr)
        {
            return;
        }

        if (!node->_is_terminate)
        {
            auto* middle = static_cast<bptree_node_middle*>(node);
            for (auto* child : middle->_pointers)
            {
                self(self, child);
            }
        }
        _allocator.delete_object(node);
    };

    delete_recursive(delete_recursive, _root);
    _root = nullptr;
    _leftmost_leaf = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    if (_root == nullptr)
    {
        auto* leaf = _allocator.template new_object<bptree_node_term>();
        leaf->_data.push_back(data);
        _root = leaf;
        _leftmost_leaf = leaf;
        ++_size;
        return {bptree_iterator(leaf, 0), true};
    }

    std::vector<bptree_node_middle*> path;
    auto* leaf = find_leaf(data.first, &path);

    for (size_t i = 0; i < leaf->_data.size(); ++i)
    {
        if (keys_equal(this, data.first, leaf->_data[i].first))
        {
            return {bptree_iterator(leaf, i), false};
        }
    }

    insert_into_leaf(leaf, data);
    ++_size;

    if (leaf->_data.size() > maximum_keys_in_node)
    {
        bptree_node_term* new_leaf = nullptr;
        split_leaf(leaf, new_leaf, data.first);
        insert_into_parent(leaf, new_leaf->_data.front().first, new_leaf, path);
        cascade_split_new_leaf(new_leaf, path);
        rebuild_leaf_chain();
    }

    for (size_t i = 0; i < leaf->_data.size(); ++i)
    {
        if (keys_equal(this, data.first, leaf->_data[i].first))
        {
            return {bptree_iterator(leaf, i), true};
        }
    }

    for (auto* current = leaf->_next; current != nullptr; current = current->_next)
    {
        for (size_t i = 0; i < current->_data.size(); ++i)
        {
            if (keys_equal(this, data.first, current->_data[i].first))
            {
                return {bptree_iterator(current, i), true};
            }
        }
    }

    return {end(), false};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    tree_data_type moved = std::move(data);
    return insert(moved);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool>
BP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    return insert(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end())
    {
        const_cast<tvalue&>(it->second) = data.second;
        return it;
    }
    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end())
    {
        const_cast<tvalue&>(it->second) = std::move(data.second);
        return it;
    }
    return insert(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator
BP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);
    return insert_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);
    if (it == end())
    {
        throw std::out_of_range("Key not found");
    }
    return const_cast<tvalue&>(it->second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& BP_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);
    if (it == cend())
    {
        throw std::out_of_range("Key not found");
    }
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto [it, inserted] = emplace(key, tvalue{});
    (void)inserted;
    return const_cast<tvalue&>(it->second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BP_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto [it, inserted] = emplace(std::move(key), tvalue{});
    (void)inserted;
    return const_cast<tvalue&>(it->second);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::remove_from_leaf(bptree_node_term* leaf, size_t index)
{
    leaf->_data.erase(leaf->_data.begin() + static_cast<ptrdiff_t>(index));
    --_size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::borrow_from_left_sibling(bptree_node_term* leaf, bptree_node_term* left_sibling,
                                                                  bptree_node_middle* parent, size_t parent_index)
{
    leaf->_data.insert(leaf->_data.begin(), std::move(left_sibling->_data.back()));
    left_sibling->_data.pop_back();
    parent->_keys[parent_index - 1] = leaf->_data.front().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::borrow_from_right_sibling(bptree_node_term* leaf, bptree_node_term* right_sibling,
                                                                   bptree_node_middle* parent, size_t parent_index)
{
    leaf->_data.push_back(std::move(right_sibling->_data.front()));
    right_sibling->_data.erase(right_sibling->_data.begin());
    parent->_keys[parent_index] = right_sibling->_data.front().first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::merge_leaves(bptree_node_term* left, bptree_node_term* right)
{
    for (size_t i = 0; i < right->_data.size(); ++i)
    {
        left->_data.push_back(std::move(right->_data[i]));
    }
    left->_next = right->_next;
    if (right->_next != nullptr)
    {
        right->_next->_prev = left;
    }
    if (_leftmost_leaf == right)
    {
        _leftmost_leaf = left;
    }
    _allocator.delete_object(right);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::borrow_from_left_internal(bptree_node_middle* node,
                                                                    bptree_node_middle* left_sibling,
                                                                    bptree_node_middle* parent, size_t parent_index)
{
    node->_keys.insert(node->_keys.begin(), parent->_keys[parent_index - 1]);
    parent->_keys[parent_index - 1] = left_sibling->_keys.back();
    left_sibling->_keys.pop_back();

    node->_pointers.insert(node->_pointers.begin(), left_sibling->_pointers.back());
    left_sibling->_pointers.pop_back();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::borrow_from_right_internal(bptree_node_middle* node,
                                                                    bptree_node_middle* right_sibling,
                                                                    bptree_node_middle* parent, size_t parent_index)
{
    node->_keys.push_back(parent->_keys[parent_index]);
    parent->_keys[parent_index] = right_sibling->_keys.front();
    right_sibling->_keys.erase(right_sibling->_keys.begin());

    node->_pointers.push_back(right_sibling->_pointers.front());
    right_sibling->_pointers.erase(right_sibling->_pointers.begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::merge_internal(bptree_node_middle* left, bptree_node_middle* right,
                                                        bptree_node_middle* parent, size_t parent_index)
{
    left->_keys.push_back(parent->_keys[parent_index - 1]);
    parent->_keys.erase(parent->_keys.begin() + static_cast<ptrdiff_t>(parent_index - 1));

    for (auto& key : right->_keys)
    {
        left->_keys.push_back(std::move(key));
    }
    for (auto* pointer : right->_pointers)
    {
        left->_pointers.push_back(pointer);
    }
    parent->_pointers.erase(parent->_pointers.begin() + static_cast<ptrdiff_t>(parent_index));
    _allocator.delete_object(right);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::rebalance_internal(bptree_node_middle* node,
                                                            std::vector<bptree_node_middle*>& path)
{
    if (node->_keys.size() >= minimum_keys_in_node)
    {
        return;
    }

    if (node == _root)
    {
        shrink_root_if_needed();
        return;
    }

    if (path.empty())
    {
        return;
    }

    auto* parent = path.back();
    const size_t node_index = find_child_index(parent, node);

    if (node_index > 0)
    {
        auto* left_sibling = static_cast<bptree_node_middle*>(parent->_pointers[node_index - 1]);
        if (left_sibling->_keys.size() > minimum_keys_in_node)
        {
            borrow_from_left_internal(node, left_sibling, parent, node_index);
            return;
        }
    }

    if (node_index + 1 < parent->_pointers.size())
    {
        auto* right_sibling = static_cast<bptree_node_middle*>(parent->_pointers[node_index + 1]);
        if (right_sibling->_keys.size() > minimum_keys_in_node)
        {
            borrow_from_right_internal(node, right_sibling, parent, node_index);
            return;
        }
    }

    if (node_index > 0)
    {
        auto* left_sibling = static_cast<bptree_node_middle*>(parent->_pointers[node_index - 1]);
        merge_internal(left_sibling, node, parent, node_index);
        path.pop_back();
        if (parent->_keys.size() < minimum_keys_in_node)
        {
            rebalance_internal(parent, path);
        }
        else
        {
            shrink_root_if_needed();
        }
    }
    else
    {
        auto* right_sibling = static_cast<bptree_node_middle*>(parent->_pointers[node_index + 1]);
        merge_internal(node, right_sibling, parent, node_index + 1);
        path.pop_back();
        if (parent->_keys.size() < minimum_keys_in_node)
        {
            rebalance_internal(parent, path);
        }
        else
        {
            shrink_root_if_needed();
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::rebalance_leaf(bptree_node_term* leaf,
                                                       std::vector<bptree_node_middle*>& path,
                                                       bool& leaf_deleted)
{
    leaf_deleted = false;

    if (leaf->_data.size() >= minimum_keys_in_node)
    {
        return;
    }

    if (path.empty())
    {
        return;
    }

    auto* parent = path.back();
    const size_t leaf_index = find_child_index(parent, leaf);

    if (leaf_index > 0)
    {
        auto* left_sibling = static_cast<bptree_node_term*>(parent->_pointers[leaf_index - 1]);
        if (left_sibling->_data.size() > minimum_keys_in_node)
        {
            borrow_from_left_sibling(leaf, left_sibling, parent, leaf_index);
            return;
        }
    }

    if (leaf_index + 1 < parent->_pointers.size())
    {
        auto* right_sibling = static_cast<bptree_node_term*>(parent->_pointers[leaf_index + 1]);
        if (right_sibling->_data.size() > minimum_keys_in_node)
        {
            borrow_from_right_sibling(leaf, right_sibling, parent, leaf_index);
            return;
        }
    }

    if (leaf_index > 0)
    {
        auto* left_sibling = static_cast<bptree_node_term*>(parent->_pointers[leaf_index - 1]);
        merge_leaves(left_sibling, leaf);
        leaf_deleted = true;
        parent->_keys.erase(parent->_keys.begin() + static_cast<ptrdiff_t>(leaf_index - 1));
        parent->_pointers.erase(parent->_pointers.begin() + static_cast<ptrdiff_t>(leaf_index));
    }
    else
    {
        auto* right_sibling = static_cast<bptree_node_term*>(parent->_pointers[leaf_index + 1]);
        merge_leaves(leaf, right_sibling);
        parent->_keys.erase(parent->_keys.begin() + static_cast<ptrdiff_t>(leaf_index));
        parent->_pointers.erase(parent->_pointers.begin() + static_cast<ptrdiff_t>(leaf_index + 1));
    }

    path.pop_back();

    if (parent == _root)
    {
        shrink_root_if_needed();
        return;
    }

    if (parent->_keys.size() < minimum_keys_in_node)
    {
        rebalance_internal(parent, path);
    }
    else
    {
        shrink_root_if_needed();
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (_root == nullptr)
    {
        return end();
    }

    auto it = find(key);
    if (it == end())
    {
        return end();
    }

    auto* leaf = it._node;
    const size_t index = it._index;

    std::vector<bptree_node_middle*> path;
    find_leaf(key, &path);

    remove_from_leaf(leaf, index);
    bptree_iterator result = locate_after_erase(leaf, index);

    if (leaf == _root)
    {
        if (leaf->_data.empty())
        {
            _allocator.delete_object(leaf);
            _root = nullptr;
            _leftmost_leaf = nullptr;
            return end();
        }
        return result;
    }

    if (leaf->_data.size() >= minimum_keys_in_node)
    {
        return result;
    }

    const size_t result_index = result._index;
    bptree_node_term* result_node = result._node;

    bptree_node_term* left_sibling_before_merge = nullptr;
    size_t left_sibling_size_before_merge = 0;
    if (!path.empty())
    {
        auto* parent = path.back();
        const size_t leaf_index = find_child_index(parent, leaf);
        if (leaf_index > 0)
        {
            left_sibling_before_merge = static_cast<bptree_node_term*>(parent->_pointers[leaf_index - 1]);
            left_sibling_size_before_merge = left_sibling_before_merge->_data.size();
        }
    }

    bool leaf_deleted = false;
    rebalance_leaf(leaf, path, leaf_deleted);
    shrink_root_if_needed();
    rebuild_leaf_chain();

    if (_root == nullptr)
    {
        return end();
    }

    if (leaf_deleted && result_node == leaf && left_sibling_before_merge != nullptr)
    {
        return bptree_iterator(left_sibling_before_merge, left_sibling_size_before_merge + result_index);
    }

    return result;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator pos)
{
    if (pos._node == nullptr)
    {
        return end();
    }
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator pos)
{
    return erase(bptree_iterator(const_cast<bptree_node_term*>(pos._node), pos._index));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator beg,
                                                                                                        bptree_iterator en)
{
    while (beg != en)
    {
        beg = erase(beg);
    }
    return beg;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator beg,
                                                                                                        bptree_const_iterator en)
{
    return erase(
        bptree_iterator(const_cast<bptree_node_term*>(beg._node), beg._index),
        bptree_iterator(const_cast<bptree_node_term*>(en._node), en._index));
}

#endif // SYS_PROG_B_PLUS_TREE_H
