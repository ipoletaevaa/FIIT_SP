#include "../include/b_tree.h"

// 1. Исправлен конструктор узла (добавлен bool leaf)
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node(bool leaf) noexcept : _leaf(leaf) {}

// 2. Методы поиска и итераторы сделаны const, как в заголовке
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator 
B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    btree_node* curr = _root;
    std::stack<std::pair<btree_node*, size_t>> path;
    while (curr)
    {
        size_t i = 0;
        while (i < curr->_keys.size() && (static_cast<const compare*>(this)->operator()(curr->_keys[i].first, key))) i++;
        if (i < curr->_keys.size() && !(static_cast<const compare*>(this)->operator()(curr->_keys[i].first, key)) && !(static_cast<const compare*>(this)->operator()(key, curr->_keys[i].first)))
        {
            path.push({curr, i});
            return btree_iterator(path);
        }
        if (curr->_leaf) break;
        path.push({curr, i});
        curr = curr->_pointers[i];
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator 
B_tree<tkey, tvalue, compare, t>::begin() const
{
    if (!_root || _size == 0) return end();
    std::stack<std::pair<btree_node*, size_t>> path;
    btree_node* curr = _root;
    while (curr) { path.push({curr, 0}); curr = curr->_leaf ? nullptr : curr->_pointers[0]; }
    return btree_iterator(path);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator 
B_tree<tkey, tvalue, compare, t>::end() const 
{ 
    return btree_iterator(); 
}

// 3. Явная инстанциация (необходима, если код шаблона в .cpp)
template class B_tree<int, std::string, std::less<int>, 1024>;
template class B_tree<int, std::string, std::less<int>, 3>;
template class B_tree<int, std::string, std::less<int>, 5>;
template class B_tree<int, std::string, std::less<int>, 7>;
template class B_tree<int, std::string, std::less<int>, 2>;
template class B_tree<int, std::string, std::less<int>, 4>;