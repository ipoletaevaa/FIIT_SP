#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <cmath>
#include <string>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare
{
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

    class btree_exception : public std::exception
    {
        std::string _msg;
    public:
        explicit btree_exception(std::string msg) : _msg(std::move(msg)) {}
        const char* what() const noexcept override { return _msg.c_str(); }
    };

private:
    static constexpr const size_t max_keys = 2 * t - 1;

    struct btree_node
    {
        boost::container::static_vector<tree_data_type, max_keys> _keys;
        boost::container::static_vector<btree_node*, max_keys + 1> _pointers;
        bool _leaf;

        btree_node(bool leaf) noexcept : _leaf(leaf) {}
    };

    using node_alloc_type = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<btree_node>;

    node_alloc_type _node_allocator;
    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    inline bool key_less(const tkey& lhs, const tkey& rhs) const
    {
        if constexpr (std::is_floating_point_v<tkey>) return (rhs - lhs) >= 1e-9;
        return compare::operator()(lhs, rhs);
    }

    inline bool keys_equal(const tkey& lhs, const tkey& rhs) const
    {
        return !key_less(lhs, rhs) && !key_less(rhs, lhs);
    }

    btree_node* allocate_node(bool leaf)
    {
        btree_node* node = std::allocator_traits<node_alloc_type>::allocate(_node_allocator, 1);
        std::allocator_traits<node_alloc_type>::construct(_node_allocator, node, leaf);
        return node;
    }

    void deallocate_node(btree_node* node)
    {
        if (!node) return;
        std::allocator_traits<node_alloc_type>::destroy(_node_allocator, node);
        std::allocator_traits<node_alloc_type>::deallocate(_node_allocator, node, 1);
    }

    void split_child(btree_node* x, size_t i)
    {
        btree_node* y = x->_pointers[i];
        btree_node* z = allocate_node(y->_leaf);
        for (size_t j = 0; j < t - 1; ++j) z->_keys.push_back(std::move(y->_keys[j + t]));
        if (!y->_leaf) for (size_t j = 0; j < t; ++j) z->_pointers.push_back(y->_pointers[j + t]);
        auto mid = std::move(y->_keys[t - 1]);
        y->_keys.erase(y->_keys.begin() + t - 1, y->_keys.end());
        if (!y->_leaf) y->_pointers.erase(y->_pointers.begin() + t, y->_pointers.end());
        x->_pointers.insert(x->_pointers.begin() + i + 1, z);
        x->_keys.insert(x->_keys.begin() + i, std::move(mid));
    }

public:
    class btree_iterator final
    {
        std::stack<std::pair<btree_node*, size_t>> _path;
    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using self = btree_iterator;

        btree_iterator() = default;
        explicit btree_iterator(std::stack<std::pair<btree_node*, size_t>> path) : _path(std::move(path)) {}

        reference operator*() const noexcept { return reinterpret_cast<reference>(_path.top().first->_keys[_path.top().second]); }
        pointer operator->() const noexcept { return reinterpret_cast<pointer>(&_path.top().first->_keys[_path.top().second]); }

        size_t depth() const noexcept { return _path.empty() ? 0 : _path.size() - 1; }
        size_t index() const noexcept { return _path.empty() ? 0 : _path.top().second; }

        self& operator++()
        {
            if (_path.empty()) return *this;
            auto& top = _path.top();
            if (!top.first->_leaf)
            {
                btree_node* curr = top.first->_pointers[top.second + 1];
                while (curr) { _path.push({curr, 0}); curr = curr->_leaf ? nullptr : curr->_pointers[0]; }
            }
            else
            {
                top.second++;
                while (!_path.empty() && _path.top().second >= _path.top().first->_keys.size())
                {
                    _path.pop();
                    if (!_path.empty()) _path.top().second++;
                }
            }
            return *this;
        }

        bool operator==(const self& other) const noexcept { return _path == other._path; }
        bool operator!=(const self& other) const noexcept { return !(*this == other); }
    };

    using btree_const_iterator = btree_iterator;

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> alloc = pp_allocator<value_type>())
        : compare(cmp), _node_allocator(alloc), _allocator(alloc), _root(nullptr), _size(0) {}

    B_tree(pp_allocator<value_type> alloc, const compare& comp = compare()) : B_tree(comp, alloc) {}

    ~B_tree() noexcept { clear(); }

    void clear() noexcept
    {
        auto del = [this](auto&& self, btree_node* n) -> void {
            if (!n) return;
            if (!n->_leaf) for (auto* c : n->_pointers) self(self, c);
            deallocate_node(n);
        };
        del(del, _root);
        _root = nullptr; _size = 0;
    }

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args)
    {
        tree_data_type data(std::forward<Args>(args)...);
        if (find(data.first) != end()) return {find(data.first), false};
        if (!_root) _root = allocate_node(true);
        if (_root->_keys.size() == max_keys)
        {
            btree_node* s = allocate_node(false);
            s->_pointers.push_back(_root);
            _root = s; split_child(s, 0);
        }
        btree_node* curr = _root;
        while (true)
        {
            size_t i = 0;
            while (i < curr->_keys.size() && key_less(curr->_keys[i].first, data.first)) i++;
            if (curr->_leaf) { curr->_keys.insert(curr->_keys.begin() + i, std::move(data)); _size++; return {find(key_from_data(curr->_keys[i])), true}; }
            else { if (curr->_pointers[i]->_keys.size() == max_keys) { split_child(curr, i); if (key_less(curr->_keys[i].first, data.first)) i++; } curr = curr->_pointers[i]; }
        }
    }

    btree_iterator find(const tkey& key) const
    {
        btree_node* curr = _root;
        std::stack<std::pair<btree_node*, size_t>> path;
        while (curr)
        {
            size_t i = 0;
            while (i < curr->_keys.size() && key_less(curr->_keys[i].first, key)) i++;
            if (i < curr->_keys.size() && keys_equal(curr->_keys[i].first, key)) { path.push({curr, i}); return btree_iterator(std::move(path)); }
            if (curr->_leaf) break;
            path.push({curr, i}); curr = curr->_pointers[i];
        }
        return end();
    }

    tvalue& at(const tkey& key) { auto it = find(key); if (it == end()) throw std::out_of_range("B-tree"); return const_cast<tvalue&>(it->second); }

    btree_iterator erase(const tkey& key)
    {
        auto it = find(key);
        if (it == end()) return end();
        btree_node* curr = _root;
        while (curr)
        {
            size_t i = 0;
            while (i < curr->_keys.size() && key_less(curr->_keys[i].first, key)) i++;
            if (i < curr->_keys.size() && keys_equal(curr->_keys[i].first, key)) { curr->_keys.erase(curr->_keys.begin() + i); _size--; break; }
            curr = curr->_leaf ? nullptr : curr->_pointers[i];
        }
        if (_root && _root->_keys.empty() && !_root->_leaf) { btree_node* old = _root; _root = _root->_pointers[0]; old->_pointers.clear(); deallocate_node(old); }
        return end();
    }

    btree_iterator begin() const
    {
        if (!_root || _size == 0) return end();
        std::stack<std::pair<btree_node*, size_t>> path;
        btree_node* curr = _root;
        while (curr) { path.push({curr, 0}); curr = curr->_leaf ? nullptr : curr->_pointers[0]; }
        return btree_iterator(std::move(path));
    }

    btree_iterator end() const { return btree_iterator(); }
    btree_const_iterator cbegin() const { return begin(); }
    btree_const_iterator cend() const { return end(); }

private:
    static const tkey& key_from_data(const tree_data_type& d) { return d.first; }
};

#endif