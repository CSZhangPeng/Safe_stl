﻿// Note : this is an internal file.
// Nobody but library developer should try to use it directly.

#ifndef FREE_TREE_HPP
#define FREE_TREE_HPP

#include "functional.hpp"
#include "memory.hpp"
#include "iterator.hpp"
#include "algorithm.hpp"

namespace saber
{

// I don't use RB-Tree or AVL-Tree since I'm not good at data structure
// However, this problem could be solved soon
template <typename ItemType, typename ItemCompare, typename Allocator>
class free_tree
{
public:
    using allocator_type  = Allocator;
    using value_type      = ItemType;
    using value_compare   = ItemCompare;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;

    class tree_iterator;

    using reference       = ItemType&;
    using const_reference = const ItemType&;
    using pointer       = typename allocator_traits<Allocator>::pointer;
    using const_pointer = typename allocator_traits<Allocator>::const_pointer;

    explicit free_tree(const ItemCompare& _comp = ItemCompare(),
                       const Allocator& _alloc = Allocator());
    ~free_tree();

    template <typename... Args>
    pair<tree_iterator, bool> emplace(Args&& ..._args);
    tree_iterator find(const ItemType& _key);
    tree_iterator erase(tree_iterator _pos);

    tree_iterator begin() noexcept;
    tree_iterator end() noexcept;

private:
    friend class tree_iterator;

    template <typename TreeNode>
    struct tree_node_base_proxy
    {
        tree_node_base_proxy() = default;
        ~tree_node_base_proxy() = default;

        TreeNode *left_child = nullptr;
        TreeNode *right_child = nullptr;
        TreeNode *parent = nullptr;

        bool is_left_child(void) const
        { return parent->left_child == this; }
    };

    struct tree_node;
    using tree_node_base = tree_node_base_proxy<tree_node>;

    struct tree_node : public tree_node_base
    {
        template <typename... Args>
        tree_node(Args&&... _args) :
            tree_node_base(), item(std::forward<Args>(_args)...)
        {}
        ~tree_node() {}
        ItemType item;
    };

    using node_allocator_type =
        typename Allocator::template rebind<tree_node>::other;

    value_compare less_comp;
    allocator_type alloc;
    node_allocator_type node_alloc;
    tree_node_base *root;

    void remove_tree_node(tree_node *_node);
    void unplug_tree_node(tree_node *_node);
    tree_node* find_leftmost_node(tree_node *_root);
    void destroy_tree(tree_node *_root);
}; // class saber::free_tree



template <typename IT, typename IC, typename AL>
class free_tree<IT, IC, AL>::tree_iterator
{
    friend class free_tree;
public:
    using value_type      = typename free_tree::value_type;
    using reference       = typename free_tree::reference;
    using const_reference = typename free_tree::const_reference;
    using size_type       = typename free_tree::size_type;
    using difference_type = typename free_tree::difference_type;

    explicit tree_iterator() = default;
    ~tree_iterator() = default;

    reference operator* (void);
    tree_iterator& operator++ (void);
    tree_iterator& operator-- (void);

    bool operator== (const tree_iterator& _another) const;
    bool operator!= (const tree_iterator& _another) const;

private:
    using tree_node_base = typename free_tree::tree_node_base;
    explicit tree_iterator(tree_node_base* _node) : actual_node(_node) {}
    tree_node_base *actual_node = nullptr;
}; // class saber::free_tree::tree_iterator



template <typename IT, typename IC, typename AL>
free_tree<IT, IC, AL>::free_tree(const IC& _comp, const AL& _alloc) :
    less_comp(_comp),
    alloc(_alloc),
    node_alloc(alloc)
{
    root = new tree_node_base();
    root->parent = reinterpret_cast<tree_node*>(root);
    root->right_child = reinterpret_cast<tree_node*>(root);
    root->left_child = reinterpret_cast<tree_node*>(root);
}

template <typename IT, typename IC, typename AL>
free_tree<IT, IC, AL>::~free_tree()
{
    if (root->left_child != root) destroy_tree(root->left_child);
    delete root;
}

template <typename IT, typename IC, typename AL>
template <typename... Args>
pair<typename free_tree<IT, IC, AL>::tree_iterator, bool>
free_tree<IT, IC, AL>::emplace(Args&& ..._args)
{
    tree_node *new_node =
            allocator_traits<node_allocator_type>::allocate(node_alloc, 1);
    construct(new_node, std::forward<Args>(_args)...);

    if (root->left_child == root)
    {
        root->left_child = new_node;
        new_node->parent = reinterpret_cast<tree_node*>(root);;

        return pair<tree_iterator, bool>(
                    tree_iterator(root->left_child),
                    true);
    }
    else
    {
        for (tree_node *node_iter = root->left_child;;)
        {
            if (less_comp(new_node->item, node_iter->item))
            {
                if (node_iter->left_child == nullptr)
                {
                    node_iter->left_child = new_node;
                    new_node->parent = node_iter;

                    return pair<tree_iterator, bool>(
                                tree_iterator(node_iter->left_child),
                                true);
                }
                else
                {
                    node_iter = node_iter->left_child;
                }
            }
            else if (less_comp(node_iter->item, new_node->item))
            {
                if (node_iter->right_child == nullptr)
                {
                    node_iter->right_child = new_node;
                    new_node->parent = node_iter;

                    return pair<tree_iterator, bool>(
                                tree_iterator(node_iter->right_child),
                                true);
                }
                else
                {
                    node_iter = node_iter->right_child;
                }
            }
            else
            {
                destroy_at(new_node);
                allocator_traits<node_allocator_type>::deallocate(
                            node_alloc, new_node, 1);
                return
                    pair<tree_iterator, bool>(tree_iterator(node_iter), false);
            }
        }
    }
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator
free_tree<IT, IC, AL>::find(const IT &_key)
{
    if (begin() == end())
    {
        return end();
    }
    tree_node *node_it = root->left_child;
    while (node_it != nullptr)
    {
        if (less_comp(node_it->item, _key))
        {
            node_it = node_it->right_child;
        }
        else if (less_comp(_key, node_it->item))
        {
            node_it = node_it->left_child;
        }
        else
        {
            return tree_iterator(node_it);
        }
    }
    return end();
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator
free_tree<IT, IC, AL>::erase(tree_iterator _pos)
{
    tree_iterator ret = _pos;
    ++ret;
    remove_tree_node(reinterpret_cast<tree_node*>(_pos.actual_node));
    return ret;
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator
free_tree<IT, IC, AL>::begin() noexcept
{
    if (root->left_child == root) return tree_iterator(root);

    tree_node_base *node_it;
    for(node_it = root->left_child;
        node_it->left_child != nullptr;
        node_it = node_it->left_child);
    return tree_iterator(node_it);
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator
free_tree<IT, IC, AL>::end() noexcept
{
    return tree_iterator(root);
}

template <typename IT, typename IC, typename AL>
void
free_tree<IT, IC, AL>::remove_tree_node(tree_node *_node)
{
    unplug_tree_node(_node);
    destroy_at(reinterpret_cast<tree_node*>(_node));
    allocator_traits<node_allocator_type>::deallocate(
        node_alloc,
        reinterpret_cast<tree_node*>(_node),
        1);
}

template <typename IT, typename IC, typename AL>
void
free_tree<IT, IC, AL>::unplug_tree_node(tree_node *_node)
{
    if (_node->left_child == nullptr
        && _node->right_child == nullptr)
    {
        if (_node->is_left_child()) _node->parent->left_child = nullptr;
        else _node->parent->right_child = nullptr;
    }
    else if (_node->left_child == nullptr)
    {
        if (_node->is_left_child())
        {
            _node->parent->left_child = _node->right_child;
        }
        else
        {
            _node->parent->right_child = _node->right_child;
        }
        _node->right_child->parent = _node->parent;
    }
    else if (_node->right_child == nullptr)
    {
        if (_node->is_left_child())
        {
            _node->parent->left_child = _node->left_child;
        }
        else
        {
            _node->parent->right_child = _node->left_child;
        }
        _node->left_child->parent = _node->parent;
    }
    else
    {
        tree_node *replacement_node =
                find_leftmost_node(_node->right_child);

        unplug_tree_node(replacement_node);
        replacement_node->left_child = _node->left_child;
        _node->left_child->parent = replacement_node;
        replacement_node->right_child = _node->right_child;
        _node->right_child->parent = replacement_node;
        replacement_node->parent = _node->parent;

        (_node->is_left_child() ?
            replacement_node->parent->left_child :
            replacement_node->parent->right_child
        ) = replacement_node;
    }

    _node->parent = nullptr;
    _node->left_child = nullptr;
    _node->right_child = nullptr;
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_node*
free_tree<IT, IC, AL>::find_leftmost_node(tree_node *_root)
{
    while (_root->left_child != nullptr) _root = _root->left_child;
    return _root;
}

template <typename IT, typename IC, typename AL>
void
free_tree<IT, IC, AL>::destroy_tree(tree_node *_root)
{
    if (_root->left_child) destroy_tree(_root->left_child);
    if (_root->right_child) destroy_tree(_root->right_child);
    destroy_at(_root);
    allocator_traits<node_allocator_type>::deallocate(node_alloc, _root, 1);
}



template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator::reference
free_tree<IT, IC, AL>::tree_iterator::operator*()
{
    return reinterpret_cast<tree_node*>(actual_node)->item;
}

template <typename IT, typename IC, typename AL>
typename free_tree<IT, IC, AL>::tree_iterator&
free_tree<IT, IC, AL>::tree_iterator::operator++ ()
{
    // This algorithm is also copy-pasted from EA-STL.
    // Thanks to EA-STL. without its help, I can't have things done so easy.
    if (actual_node->right_child != nullptr)
    {
        actual_node = actual_node->right_child;

        while (actual_node->left_child)
        {
            actual_node = actual_node->left_child;
        }
    }
    else
    {
        tree_node *node_iter = actual_node->parent;

        while (actual_node == node_iter->right_child)
        {
            actual_node = node_iter;
            node_iter = node_iter->parent;
        }

        if (actual_node->right_child != node_iter)
        {
            actual_node = node_iter;
        }
    }

    return *this;
}

template <typename IT, typename IC, typename AL>
bool
free_tree<IT, IC, AL>::tree_iterator::operator== (
        const tree_iterator& _another) const
{
    return actual_node == _another.actual_node;
}

template <typename IT, typename IC, typename AL>
bool
free_tree<IT, IC, AL>::tree_iterator::operator!= (
        const tree_iterator& _another) const
{
    return !operator== (_another);
}

} // namespace saber

#endif // FREE_TREE_HPP