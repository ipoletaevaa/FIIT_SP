#include "../include/b_plus_tree.h"

template <typename tkey, typename tvalue, typename compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const compare& cmp, std::allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _node_alloc(alloc), _root(nullptr), _size(0) {}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept { clear(); }

template <typename tkey, typename tvalue, typename compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear_recursive(btree_node* node) {
    if (!node) return;
    if (!node->_is_leaf) {
        for (auto* child : node->_pointers) clear_recursive(child);
    }
    std::allocator_traits<node_alloc_t>::destroy(_node_alloc, node);
    std::allocator_traits<node_alloc_t>::deallocate(_node_alloc, node, 1);
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept {
    clear_recursive(_root);
    _root = nullptr;
    _size = 0;
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::split_child(btree_node* x, size_t i) {
    btree_node* y = x->_pointers[i];
    btree_node* z = std::allocator_traits<node_alloc_t>::allocate(_node_alloc, 1);
    std::allocator_traits<node_alloc_t>::construct(_node_alloc, z, y->_is_leaf);

    for (size_t j = 0; j < t - 1; ++j) z->_keys.push_back(std::move(y->_keys[j + t]));
    if (!y->_is_leaf) {
        for (size_t j = 0; j < t; ++j) z->_pointers.push_back(y->_pointers[j + t]);
    }

    tree_data_type mid = std::move(y->_keys[t - 1]);
    y->_keys.erase(y->_keys.begin() + (t - 1), y->_keys.end());
    if (!y->_is_leaf) y->_pointers.erase(y->_pointers.begin() + t, y->_pointers.end());

    x->_pointers.insert(x->_pointers.begin() + i + 1, z);
    x->_keys.insert(x->_keys.begin() + i, std::move(mid));
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::insert_non_full(btree_node* x, tree_data_type&& data) {
    int i = static_cast<int>(x->_keys.size()) - 1;
    if (x->_is_leaf) {
        while (i >= 0 && compare_keys(data.first, x->_keys[i].first)) --i;
        if (i >= 0 && keys_equal(data.first, x->_keys[i].first)) throw key_already_exists();
        x->_keys.insert(x->_keys.begin() + i + 1, std::move(data));
    } else {
        while (i >= 0 && compare_keys(data.first, x->_keys[i].first)) --i;
        if (i >= 0 && keys_equal(data.first, x->_keys[i].first)) throw key_already_exists();
        ++i;
        if (x->_pointers[i]->_keys.size() == maximum_keys_in_node) {
            split_child(x, i);
            if (compare_keys(x->_keys[i].first, data.first)) ++i;
        }
        insert_non_full(x->_pointers[i], std::move(data));
    }
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool> 
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data) {
    if (!_root) {
        _root = std::allocator_traits<node_alloc_t>::allocate(_node_alloc, 1);
        std::allocator_traits<node_alloc_t>::construct(_node_alloc, _root, true);
        _root->_keys.push_back(std::move(data));
        _size = 1;
        return { find(_root->_keys[0].first), true };
    }
    if (_root->_keys.size() == maximum_keys_in_node) {
        btree_node* s = std::allocator_traits<node_alloc_t>::allocate(_node_alloc, 1);
        std::allocator_traits<node_alloc_t>::construct(_node_alloc, s, false);
        s->_pointers.push_back(_root);
        _root = s;
        split_child(s, 0);
    }
    const tkey& k = data.first;
    insert_non_full(_root, std::move(data));
    _size++;
    return { find(k), true };
}
template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) {
    btree_node* curr = _root;
    std::stack<std::pair<btree_node*, size_t>> path;
    while (curr) {
        size_t i = 0;
        while (i < curr->_keys.size() && compare_keys(curr->_keys[i].first, key)) ++i;
        if (i < curr->_keys.size() && keys_equal(curr->_keys[i].first, key)) {
            path.push({curr, i});
            btree_iterator it;
            it._path = std::move(path);
            return it;
        }
        if (curr->_is_leaf) break;
        path.push({curr, i});
        curr = curr->_pointers[i];
    }
    return end();
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(btree_node* root, bool end) {
    if (end || !root) return;
    btree_node* curr = root;
    while (curr) {
        _path.push({curr, 0});
        curr = curr->_is_leaf ? nullptr : curr->_pointers[0];
    }
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference 
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const {
    auto& top = _path.top();
    return reinterpret_cast<reference>(top.first->_keys[top.second]);
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer 
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const {
    return &(operator*());
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator& 
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++() {
    auto& top = _path.top();
    if (!top.first->_is_leaf) {
        btree_node* curr = top.first->_pointers[top.second + 1];
        top.second++; 
        while (curr) {
            _path.push({curr, 0});
            curr = curr->_is_leaf ? nullptr : curr->_pointers[0];
        }
    } else {
        top.second++;
        while (!_path.empty() && _path.top().second >= _path.top().first->_keys.size()) {
            _path.pop();
        }
    }
    return *this;
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const btree_iterator& other) const {
    return _path == other._path;
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const btree_iterator& other) const {
    return !(*this == other);
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin() {
    return btree_iterator(_root, false);
}

template <typename tkey, typename tvalue, typename compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end() {
    return btree_iterator(_root, true);
}