#pragma once
#include "Node.hpp"
#include <list>
#include <stack>
#include <vector>
#include <algorithm>
#include <string>
#include <cassert>


template <typename T>
class LinkedList
{
private:
    LinkedListNode<T>* _head_node = new LinkedListNode<T>;
    LinkedListNode<T>* _tail_node = _head_node;
    size_t _length = 0;

public:
    static const size_t nopos = SIZE_MAX;

    class iterator
    {
    private:
        LinkedListNode<T>* _node = nullptr;

    public:
        iterator(){}

        iterator(LinkedListNode<T>* node)
        {
            _node = node;
        }
        
        ~iterator(){};

        iterator operator++()
        {
            if (_node != nullptr)
            {
                _node = _node->get_next_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return *this;
        }

        iterator operator++(int)
        {
            iterator it(_node);
            if (_node != nullptr)
            {
                _node = _node->get_next_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return it;
        }
    
        const bool operator==(const iterator& it) const
        {
            return _node == it._node;
        }

        const bool operator!=(const iterator& it) const
        {
            return _node != it._node;
        }

        T& operator*()
        {
            return _node->get_data();
        }
    };

    class const_iterator
    {
    private:
        LinkedListNode<T>* _node = nullptr;

    public:
        const_iterator(){};

        const_iterator(LinkedListNode<T>* node)
        {
            _node = node;
        }
        
        ~const_iterator(){};

        const_iterator operator++()
        {
            if (_node != nullptr)
            {
                _node = _node->get_next_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator it(_node);
            if (_node != nullptr)
            {
                _node = _node->get_next_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return it;
        }
    
        const bool operator==(const const_iterator& it) const
        {
            return _node == it._node;
        }

        const bool operator!=(const const_iterator& it) const
        {
            return _node != it._node;
        }

        const T& operator*() const
        {
            return _node->get_data();
        }
    };

    class reverse_iterator
    {
    private:
        LinkedListNode<T>* _node = nullptr;

    public:
        reverse_iterator(){};

        reverse_iterator(LinkedListNode<T>* node)
        {
            _node = node;
        }
        
        ~reverse_iterator(){};

        reverse_iterator operator++()
        {
            if (_node != nullptr)
            {
                _node = _node->last_next_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return *this;
        }

        reverse_iterator operator++(int)
        {
            iterator it(_node);
            if (_node != nullptr)
            {
                _node = _node->get_last_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return it;
        }
    
        const bool operator==(const reverse_iterator& it) const
        {
            return _node == it._node;
        }

        const bool operator!=(const reverse_iterator& it) const
        {
            return _node != it._node;
        }

        T& operator*()
        {
            return _node->get_data();
        }
    };

    class const_reverse_iterator
    {
    private:
        LinkedListNode<T>* _node = nullptr;

    public:
        const_reverse_iterator(){};

        const_reverse_iterator(LinkedListNode<T>* node)
        {
            _node = node;
        }
        
        ~const_reverse_iterator(){};

        const_reverse_iterator operator++()
        {
            if (_node != nullptr)
            {
                _node = _node->get_last_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return *this;
        }

        const_reverse_iterator operator++(int)
        {
            iterator it(_node);
            if (_node != nullptr)
            {
                _node = _node->get_last_node();
            }
            else
            {
                throw std::runtime_error("Out of range.");
            }
            return it;
        }
    
        const bool operator==(const const_reverse_iterator& it) const
        {
            return _node == it._node;
        }

        const bool operator!=(const const_reverse_iterator& it) const
        {
            return _node != it._node;
        }

        const T& operator*() const
        {
            return _node->get_data();
        }
    };

public:
    LinkedList(){}
    
    LinkedList(const std::initializer_list<T>& values)
    {
        for (const T& value : values)
        {
            _tail_node->set_next_node(new LinkedListNode(value));
            _tail_node->get_next_node()->set_last_node(_tail_node);
            _tail_node = _tail_node->get_next_node();
        }
        _length = values.size();
    }

    LinkedList(const LinkedList<T>& list)
    {
        for (const T& value : list)
        {
            append(value);
        }
    }

    ~LinkedList()
    {
        for (size_t i = 0; i < _length; ++i)
        {
            _tail_node = _tail_node->get_last_node();
            delete _tail_node->get_next_node();
        }
        delete _head_node;
    }

    void append(const T& value)
    {
        _tail_node->set_next_node(new LinkedListNode(value));
        _tail_node->get_next_node()->set_last_node(_tail_node);
        _tail_node = _tail_node->get_next_node();
        ++_length;
    }

    void insert(const size_t& index, const T& value)
    {
        if (_length < index)
        {
            throw std::runtime_error("Index out of range.");
        }
        else if (_length == index)
        {
            append(value);
        }
        else
        {
            LinkedListNode<T>* temp = _head_node;
            for (size_t i = 0; i < index; ++i)
            {
                temp = temp->get_next_node();
            }
            temp->get_next_node()->set_last_node(new LinkedListNode<T>(value));
            temp->get_next_node()->get_last_node()->set_next_node(temp->get_next_node());
            temp->get_next_node()->get_last_node()->set_last_node(temp);
            temp->set_next_node(temp->get_next_node()->get_last_node());
            ++_length;
        }
    }

    T& at(const size_t& index)
    {
        if (_length <= index)
        {
            throw std::runtime_error("Index out of range.");
        }
        LinkedListNode<T>* temp = _head_node->get_next_node();
        for (size_t i = 0; i < index; ++i)
        {
            temp = temp->get_next_node();
        }
        return temp->get_data();
    }

    const T& at(const size_t& index) const
    {
        if (_length <= index)
        {
            throw std::runtime_error("Index out of range.");
        }
        LinkedListNode<T>* temp = _head_node->get_next_node();
        for (size_t i = 0; i < index; ++i)
        {
            temp = temp->get_next_node();
        }
        return temp->get_data();
    }

    const size_t index(const T& value) const
    {
        LinkedListNode<T>* temp = _head_node->get_next_node();
        for (size_t i = 0; i < _length; ++i)
        {
            if (temp->get_data() == value)
            {
                return i;
            }
            temp = temp->get_next_node();
        }
        return nopos;
    }

    T& operator[](const size_t& index)
    {
        return at(index);
    }

    const T& operator[](const size_t& index) const
    {
        return at(index);
    }

    void remove(const size_t& index)
    {
        if (_length <= index)
        {
            throw std::runtime_error("Index out of range.");
        }
        if (index == _length - 1)
        {
            _tail_node = _tail_node->get_last_node();
            delete _tail_node->get_next_node();
            _tail_node->set_next_node(nullptr);
        }
        else
        {
            LinkedListNode<T>* temp = _head_node;
            for (size_t i = 0; i < index; ++i)
            {
                temp = temp->get_next_node();
            }
            temp->set_next_node(temp->get_next_node()->get_next_node());
            delete temp->get_next_node()->get_last_node();
            temp->get_next_node()->set_last_node(temp);
            temp = nullptr;
        }
        --_length;
    }

    T pop()
    {
        if (_length == 0)
        {
            throw std::runtime_error("LinkedList is empty.");
        }
        T value = _tail_node->get_data();
        _tail_node = _tail_node->get_last_node();
        delete _tail_node->get_next_node();
        _tail_node->set_next_node(nullptr);
        --_length;
        return value;
    }

    void clear()
    {
        for (size_t i = 0; i < _length; ++i)
        {
            _tail_node = _tail_node->get_last_node();
            delete _tail_node->get_next_node();
        }
        _head_node->set_next_node(nullptr);
        _length = 0;
    }

    const size_t size() const
    {
        return _length;
    }

    const bool empty() const
    {
        return _length == 0;
    }

    void reverse()
    {
        if (_length == 0)
        {
            return;
        }
        LinkedListNode<T> *next, *temp = _head_node->get_next_node();
        while (temp != nullptr && temp->get_next_node() != nullptr)
        {
            next = temp->get_next_node();
            temp->set_next_node(temp->get_last_node());
            temp->set_last_node(next);
            temp = next;
        }
        _tail_node = _head_node->get_next_node();
        _tail_node->set_next_node(nullptr);
        temp->get_last_node()->set_last_node(temp);
        temp->set_next_node(temp->get_last_node());
        temp->set_last_node(_head_node);
        _head_node->set_next_node(temp);
        temp = nullptr;
        next = nullptr;
    }

    void assign(const_iterator& start, const_iterator end)
    {
        clear();
        while (start != end)
        {
            append(*start);
            ++start;
        }
    }

    void assign(const size_t& n, const T& value)
    {
        clear();
        for (size_t i = 0; i < n; ++i)
        {
            append(value);
        }
    }

    iterator begin()
    {
        return iterator(_head_node->get_next_node());
    }

    const_iterator begin() const
    {
        return const_iterator(_head_node->get_next_node());
    }

    const_iterator cbegin() const
    {
        return const_iterator(_head_node->get_next_node());
    }

    iterator end()
    {
        return iterator();
    }

    const_iterator end() const
    {
        return const_iterator();
    }

    const_iterator cend() const
    {
        return const_iterator();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(_tail_node);
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(_tail_node);
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(_tail_node);
    }

    reverse_iterator rend()
    {
        return reverse_iterator(_head_node);
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(_head_node);
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(_head_node);
    }

    T& front()
    {
        assert(!empty());
        return _head_node->get_next_node()->get_data();
    }

    const T& front() const
    {
        assert(!empty());
        return _head_node->get_next_node()->get_data();
    }

    T& back()
    {
        assert(!empty());
        return _tail_node->get_data();
    }

    const T& back() const
    {
        assert(!empty());
        return _tail_node->get_data();
    }

};


namespace BinaryTree
{
    template <typename T>
    BinaryTreeNode<T>* create_tree_preorder(std::list<T>& values, const T& none = NULL)
    {
        if (values.empty())
        {
            return nullptr;
        }
        else if (values.front() == none)
        {
            values.pop_front();
            return nullptr;
        }
        else
        {
            BinaryTreeNode<T>* node = new BinaryTreeNode(values.front());
            values.pop_front();
            node->set_left(create_tree_preorder(values, none));
            node->set_right(create_tree_preorder(values, none));
            return node;
        }
    }

    template <typename T>
    BinaryTreeNode<T>* create_tree_floororder(std::list<T>& values, BinaryTreeNode<T>* node, const T& none = NULL)
    {
        if (values.empty() || values.front() == none)
        {
            return node;
        }
        node = new BinaryTreeNode(values.front());
        values.pop_front();
        std::list<BinaryTreeNode<T>*> next_lay, lay = {node};
        while (!values.empty())
        {
            for (BinaryTreeNode<T>* temp : lay)
            {
                if (values.empty())
                {
                    break;
                }
                if (values.front() != none)
                {
                    temp->set_left(new BinaryTreeNode(values.front()));
                    next_lay.push_back(temp->get_left());
                }
                else
                {
                    next_lay.push_back(nullptr);
                }
                values.pop_front();
                if (values.empty())
                {
                    break;
                }
                if (values.front() != none)
                {
                    temp->set_right(new BinaryTreeNode(values.front()));
                    next_lay.push_back(temp->get_left());
                }
                else
                {
                    next_lay.push_back(nullptr);
                }
                values.pop_front();
            }
            lay.assign(next_lay.cbegin(), next_lay.cend());
            next_lay.clear();
        }
        return node;
    }

    template <typename T>
    const std::list<T> preoder_traverse(const BinaryTreeNode<T>* node, std::list<T>& values = std::list<T>())
    {
        if (node != nullptr)
        {
            values.push_back(node->get_data());
            preoder_traverse(node->get_left(), values);
            preoder_traverse(node->get_right(), values);
        }
        return values;
    }

    template <typename T>
    const std::list<T> inorder_traverse(const BinaryTreeNode<T>* node, std::list<T>& values = std::list<T>())
    {
        if (node != nullptr)
        {
            inorder_traverse(node->get_left(), values);
            values.push_back(node->get_data());
            inorder_traverse(node->get_right(), values);
        }
        return values;
    }

    template <typename T>
    const std::list<T> postorder_traverse(const BinaryTreeNode<T>* node, std::list<T>& values = std::list<T>())
    {
        if (node != nullptr)
        {
            inorder_traverse(node->get_left(), values);
            inorder_traverse(node->get_right(), values);
            values.push_back(node->get_data());
        }
        return values;
    }

    template <typename T>
    const std::list<std::list<T>> floororder_traverse(const BinaryTreeNode<T>* node)
    {
        std::list<std::list<T>> values;
        if (node == nullptr)
        {
            return values;
        }
        values.push_back({node->get_data()});
        std::list<BinaryTreeNode<T>*> lay, nodes = {node};
        while (!nodes.empty())
        {
            if (!values.back().empty())
            {
                values.push_back(std::list<T>());
            }
            for (const BinaryTreeNode<T>* temp : nodes)
            {
                if (temp->get_left() != nullptr)
                {
                    lay.pop_back(temp->get_left());
                    values.back().push_back(temp->get_left()->get_data());
                }
                if (temp->get_right() != nullptr)
                {
                    lay.pop_back(temp->get_right());
                    values.back().push_back(temp->get_right()->get_data());
                }
            }
            nodes.assign(lay.cbegin(), lay.cend());
            lay.clear();
        }
        return values;
    }

    template <typename T>
    const void delete_tree(BinaryTreeNode<T>* tree)
    {
        if (tree == nullptr)
        {
            return;
        }
        std::stack<BinaryTreeNode<T>*> stack;
        if (tree->get_left() != nullptr)
        {
            stack.push(tree->get_left());
        }
        if (tree->get_right() != nullptr)
        {
            stack.push(tree->get_right());
        }
        delete tree;
        while (!stack.empty())
        {
            tree = stack.top();
            stack.pop();
            if (tree->get_left() != nullptr)
            {
                stack.push(tree->get_left());
            }
            if (tree->get_right())
            {
                stack.push(tree->get_right());
            }
            delete tree;
        }
        tree = nullptr;
    }

    template <typename T>
    const size_t count_height(const BinaryTreeNode<T>* node)
    {
        if (node == nullptr)
        {
            return 0;
        }
        else
        {
            return std::max(count_height(node->get_left()), count_height(node->get_right())) + 1;
        }
    }

    template <typename T>
    BinaryTreeNode<T>* copy(const BinaryTreeNode<T>* node)
    {
        if (node == nullptr)
        {
            return nullptr;
        }
        BinaryTreeNode<T>* result = new BinaryTreeNode(node->get_data());
        result->set_left(copt(node->get_left()));
        result->set_right(copy(node->get_right()));
        return result;
    }
};


template <typename T>
class BinarySearchTree
{
private:
    BinaryTreeNode<T>* _root = nullptr;
    size_t _length = 0;

public:
    BinarySearchTree(){}

    BinarySearchTree(const BinarySearchTree<T>& tree)
    {
        _root = BinaryTree::copy(&tree);
        _length = tree._length;
    }

    ~BinarySearchTree()
    {
        BinaryTree::delete_tree(_root);
    }

    void load_data(const std::list<T>& values)
    {
        BinaryTree::delete_tree(_root);
        _length = 0;
        _root = new BinaryTreeNode<T>;
        BinaryTreeNode<T>* node;
        for (const T& value : values)
        {
            node = _root;
            while (true)
            {
                if (node->get_data() < value)
                {
                    if (node->get_left() == nullptr)
                    {
                        node->set_left(new BinaryTreeNode(value));
                        ++_length;
                    }
                    else
                    {
                        node = node->get_left();
                    }
                }
                else if (node->get_data() > value)
                {
                    if (node->get_right() == nullptr)
                    {
                        node->set_right(new BinaryTreeNode(value));
                        ++_length;
                    }
                    else
                    {
                        node = node->get_right();
                    }
                }
                else
                {
                    break;
                }
            } 
        }
        node = nullptr;
    }

    const bool has(const T& value) const
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                node = nullptr;
                return true;
            }
        }
        node = nullptr;
        return false;
    }

    BinaryTreeNode<T>* find(const T& value)
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                return node;
            }
        }
        return nullptr;
    }

    const BinaryTreeNode<T>* find(const T& value) const
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                return node;
            }
        }
        return nullptr;
    }

    const bool append(const T& value)
    {
        BinaryTreeNode<T>* temp, node = _root;
        while (node != nullptr)
        {
            temp = node;
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                node = nullptr;
                return false;
            }
        }
        if (value > temp->get_data())
        {
            temp->set_right(new BinaryTreeNode(value));
        }
        else
        {
            temp->set_left(new BinaryTreeNode(value));
        }
        ++_length;
        return true;
    }

    const bool remove(const T& value)
    {
        BinaryTreeNode<T>* node, *temp;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                break;
            }
        }
        if (node == nullptr)
        {
            return false;
        }

        if (node->get_left() == nullptr)
        {
            if (node->get_parent()->get_left() == node)
            {
                node->get_parent()->set_left(node->get_right());                
            }
            else
            {
                node->get_parent()->set_right(node->get_right());
            }
            delete node;
        }
        else if (node->get_right() == nullptr)
        {
            if (node->get_parent()->get_left() == node)
            {
                node->get_parent()->set_left(node->get_left());                
            }
            else
            {
                node->get_parent()->set_right(node->get_left());
            }
            delete node;
        }
        else
        {
            temp = node->get_left();
            while (temp->get_right() != nullptr)
            {
                temp = temp->get_right();
            }
            node->set_data(temp->get_data());
            if (temp->get_parent() == node)
            {
                node->set_left(temp->get_left());
            }
            else
            {
                temp->get_parent()->set_right(temp->get_left());
            }
            delete temp;
        }
        --_length;
        return true;
    }

    void clear()
    {
        BinaryTree::delete_tree(_root);
        _length = 0;
    }

    const size_t size() const
    {
        return _length;
    }

    const bool empty() const
    {
        return _length == 0;
    }
};


template <typename T>
class AVLTree
{
private:
    BinaryTreeNode<T>* _root = nullptr;
    size_t _length = 0;

private:
    static BinaryTreeNode<T>* min_unbalanced_tree(BinaryTreeNode<T>* node)
    {
        if (node == nullptr || node->get_parent() == nullptr)
        {
            return nullptr;
        }
        BinaryTreeNode<T>* temp = node->get_parent();
        if (std::max(BinaryTree::count_height(temp->get_left()), BinaryTree::count_height(temp->get_right())) -
            std::min(BinaryTree::count_height(temp->get_left()), BinaryTree::count_height(temp->get_right())) > 1)
        {
            return node->get_parent();
        }
        else
        {
            temp = temp->get_parent();
        }
        if (temp == nullptr)
        {
            return nullptr;
        }
        else if (std::max(BinaryTree::count_height(temp->get_left()), BinaryTree::count_height(temp->get_right())) -
            std::min(BinaryTree::count_height(temp->get_left()), BinaryTree::count_height(temp->get_right())) > 1)
        {
            return temp;
        }
        else
        {
            temp = nullptr;
            return nullptr;
        }
    }

    void right_rotate(BinaryTreeNode<T>* node)
    {
        BinaryTreeNode<T> *parent = node->get_parent(), *left = node->get_left();
        node->set_left(left->get_right());
        left->set_right(node);
        if (node == _root)
        {
            _root = left;
        }
        else if (parent->get_left() == node)
        {
            parent->set_left(left);
        }
        else
        {
            parent->set_right(left);
        }
        parent = nullptr, left = nullptr;
    }

    void left_rotate(BinaryTreeNode<T>* node)
    {
        BinaryTreeNode<T> *parent = node->get_parent(), *right = node->get_right();
        node->set_right(right->get_left());
        right->set_left(node);
        if (node == _root)
        {
            _root = right;
        }
        else if (parent->get_left() == node)
        {
            parent->set_left(right);
        }
        else
        {
            parent->set_right(right);
        }        
    }

    void balance(BinaryTreeNode<T>* node)
    {
        if (node == nullptr)
        {
            return;
        }
        // 右子树高于左子树 R
        if (BinaryTree::count_height(node->get_right()) > BinaryTree::count_height(node->get_left()) &&
            BinaryTree::count_height(node->get_right()) - BinaryTree::count_height(node->get_left()) > 1)
        {
            // RL
            if (node->get_right() != nullptr
                && BinaryTree::count_height(node->get_right()->get_left()) > BinaryTree::count_height(node->get_right()->get_right())
                && BinaryTree::count_height(node->get_right()->get_left()) - BinaryTree::count_height(node->get_right()->get_right()) > 1)
            {
                right_rotate(node->get_right());
            }
            // RR
            left_rotate(node);
        } // 左子树高于右子树 L
        else if (BinaryTree::count_height(node->get_left()) > BinaryTree::count_height(node->get_right()) &&
                BinaryTree::count_height(node->get_left()) - BinaryTree::count_height(node->get_right()) > 1)
        {
            // LR
            if (node->get_left() != nullptr
                && BinaryTree::count_height(node->get_left()->get_right()) > BinaryTree::count_height(node->get_left()->get_left())
                && BinaryTree::count_height(node->get_left()->get_right()) - BinaryTree::count_height(node->get_left()->get_left()) > 1)
            {
                left_rotate(node->get_left());
            }
            // LL
            right_rotate(node);
        }
    }

public:
    AVLTree(){}

    AVLTree(const std::initializer_list<T>& values)
    {
        load_data(values);
    }

    AVLTree(const AVLTree<T>& tree)
    {
        _root = BinaryTree::copy(tree);
        _length = tree._length;
    }

    ~AVLTree()
    {
        BinaryTree::delete_tree(_root);
    }

    const size_t& size() const
    {
        return _length;
    }

    const bool empty() const
    {
        return _length == 0;
    }

    void load_data(const std::list<T>& values)
    {
        if (_root != nullptr)
        {
            BinaryTree::delete_tree(_root);
        }
        typename std::list<T>::const_iterator it = values.cbegin(), end = values.cend();
        _root = new BinaryTreeNode(*it);
        ++_length;
        BinaryTreeNode<T>* node;
        while (++it != end)
        {
            node = _root;
            while (true)
            {
                if (*it > node->get_data())
                {
                    if (node->get_right() == nullptr)
                    {
                        node->set_right(new BinaryTreeNode(*it));
                        ++_length;
                    }
                    else
                    {
                        node = node->get_right();
                    }
                }
                else if (*it < node->get_data())
                {
                    if (node->get_left() == nullptr)
                    {
                        node->set_left(new BinaryTreeNode(*it));
                        ++_length;
                    }
                    else
                    {
                        node = node->get_left();
                    }
                }
                else
                {
                    break;
                }
            }
            balance(min_unbalanced_tree(node));
        }
    }
    
    const bool has(const T& value) const
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                node = nullptr;
                return true;
            }
        }
        node = nullptr;
        return false;
    }

    BinaryTreeNode<T>* find(const T& value)
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                return node;
            }
        }
        return nullptr;
    }

    const BinaryTreeNode<T>* find(const T& value) const
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                return node;
            }
        }
        return nullptr;
    }

    const bool append(const T& value)
    {
        BinaryTreeNode<T> *temp = _root, *node = _root;
        while (node != nullptr)
        {
            temp = node;
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                temp = nullptr, node = nullptr;
                return false;
            }
        }
        if (temp == nullptr)
        {
            _root = new BinaryTreeNode(value);
        }
        else if (temp->get_data() > value)
        {
            temp->set_left(new BinaryTreeNode(value));
        }
        else
        {
            temp->set_right(new BinaryTreeNode(value));
        }
        balance(min_unbalanced_tree(temp));
        node = nullptr, temp = nullptr;
        ++_length;
        return true;
    }

    const bool remove(const T& value)
    {
        BinaryTreeNode<T>* node = _root;
        while (node != nullptr)
        {
            if (node->get_data() < value)
            {
                node = node->get_right();
            }
            else if (node->get_data() > value)
            {
                node = node->get_left();
            }
            else
            {
                break;
            }
        }
        if (node == nullptr)
        {
            return false;
        }

        if (node->get_right() == nullptr)
        {
            if (node->get_parent()->get_left() == node)
            {
                node->get_parent()->set_left(node->get_left());
            }
            else
            {
                node->get_parent()->get_right(node->get_left());
            }
            balance(min_unbalanced_tree(node));
            delete node;
        }
        else if (node->get_left() == nullptr)
        {
            if (node->get_parent()->get_left() == node)
            {
                node->get_parent()->set_left(node->get_right());                
            }
            else
            {
                node->get_parent()->set_right(node->get_right());
            }
            balance(min_unbalanced_tree(node));
            delete node;
        }
        else
        {
            BinaryTreeNode<T>* temp = node->get_left();
            while (temp->get_right() != nullptr)
            {
                temp = temp->get_right();
            }
            node->set_data(temp->get_data());
            if (temp->get_parent() == node)
            {
                node->set_left(temp->get_left());
            }
            else
            {
                temp->get_parent()->set_right(temp->get_left());
            }
            delete temp;
            balance(min_unbalanced_tree(node));
        }
        --_length;
        return true;
    }

    void clear()
    {
        BinaryTree::delete_tree(_root);
        _length = 0;
    }
};


// 负数权重表示不连通
namespace DSGraph
{
    void show_mat(const std::vector<std::vector<int>>& mat);

    const std::vector<size_t> dfs(const std::vector<std::vector<int>>& mat, const size_t& start);

    const std::vector<size_t> bfs(const std::vector<std::vector<int>>& mat, const size_t& start);

    const std::vector<std::vector<int>> prim(const std::vector<std::vector<int>>& mat);

    const std::vector<std::vector<size_t>> dijkstra(const std::vector<std::vector<int>>& mat, const size_t& start);

    const std::vector<std::vector<size_t>> floyd(const std::vector<std::vector<int>>& mat);

};


template <typename T>
class AdjacencyList
{
private:
    size_t _length = 0;
    AdjacencyNode<T> *_head_node = nullptr;
    
protected:
    const std::vector<AdjacencyNode<T>*> nodes() const
    {
        std::vector<AdjacencyNode<T>*> result;
        AdjacencyNode<T>* node = _head_node;
        while (node != nullptr)
        {
            result.push_back(node);
            node = node->get_node();
        }
        return result;
    }

public:
    AdjacencyList(){}

    AdjacencyList(const std::initializer_list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        load_data(values, mat);
    }

    AdjacencyList(const AdjacencyList<T>& list)
    {
        _head_node = new AdjacencyNode(*list._head_node);
        AdjacencyNode<T> *index_temp, *index, *next_temp, *next, *node_temp = _head_node, *node = list._head_node->get_node();
        while (node != nullptr)
        {
            node_temp->set_node(new AdjacencyNode(*node));
            node = node->get_node();
            node_temp = node_temp->get_node();
        }
        node_temp = _head_node, node = list._head_node;
        while (node_temp != nullptr)
        {
            next = node->get_next();
            next_temp = node_temp;
            while (next != nullptr)
            {
                next_temp->set_next(new AdjacencyNode(*next));
                next_temp = next_temp->get_next();
                index = list._head_node, index_temp = _head_node;
                while (index != next->get_node())
                {
                    index_temp = index_temp->get_node();
                    index = index->get_node();
                }
                next_temp->set_node(index_temp);
                next = next->get_next();
            }
            node_temp = node_temp->get_node();
            node = node->get_node();
        }
        index_temp = nullptr, index = nullptr, next_temp = nullptr, next = nullptr, node_temp = nullptr, node = nullptr;
    }

    virtual ~AdjacencyList()
    {
        delete_AdjacencyNode(_head_node);
    }

    virtual void load_data(const std::list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        if (values.size() != mat.size())
        {
            return;
        }
        delete_AdjacencyNode(_head_node);
        typename std::list<T>::const_iterator it = values.cbegin(), end = values.cend();
        _head_node = new AdjacencyNode(*it);
        AdjacencyNode<T> *temp, *node = _head_node;
        
        while (++it != end)
        {
            node->set_node(new AdjacencyNode(*it));
            node = node->get_node();
        }
        _length = values.size();
        
        for (size_t i = 0; i < _length; ++i)
        {
            node = _head_node;
            for (size_t j = 0; j < i; ++j)
            {
                node = node->get_node();
            }
            for (size_t j = 0; j < _length; ++j)
            {
                if (mat[i][j] > 0)
                {
                    node->set_next(new AdjacencyNode<T>);
                    node = node->get_next();
                    node->set_weight(mat[i][j]);
                    temp = _head_node;
                    for (size_t k = 0; k < j; ++k)
                    {
                        temp = temp->get_node();
                    }
                    node->set_node(temp);
                }
            }
        }
    }

    static void delete_AdjacencyNode(AdjacencyNode<T>* node)
    {
        AdjacencyNode<T> *temp2, *temp = node;
        while (node != nullptr)
        {
            temp = node->get_next();
            while (temp != nullptr)
            {
                temp2 = temp->get_next();
                delete temp;
                temp = temp2;
            }
            temp = node->get_node();
            delete node;
            node = temp;
        }
        node = nullptr;
    }

    const std::vector<std::vector<int>> to_mat() const
    {
        std::vector<std::vector<int>> mat;
        if (_head_node == nullptr)
        {
            return mat;
        }
        mat.assign(_length, std::vector<int>(_length, -1));
        for (size_t i = 0; i < _length; ++i)
        {
            mat[i][i] = 0;
        }
        AdjacencyNode<T> *temp, *next, *node = _head_node;
        size_t j, i = 0;
        while (node != nullptr)
        {
            next = node->get_next();
            while (next != nullptr)
            {
                j = 0;
                temp = _head_node;
                while (temp != next->get_node())
                {
                    ++j;
                }
                mat[i][j] = next->get_weight();
                next = next->get_next();
            }
            node = node->get_node();
            ++i;
        }
        temp = nullptr, next = nullptr, node = nullptr;
        return mat;
    }

    const size_t& size() const
    {
        return _length;
    }

    const bool empty() const
    {
        return _length == 0;
    }

    virtual void clear()
    {
        delete_AdjacencyNode(_head_node);
        _length = 0;
    }

    const std::vector<T> dfs(const size_t& start) const
    {
        std::vector<T> marks;
        if (_head_node == nullptr)
        {
            return marks;
        }
        AVLTree<AdjacencyNode<T>*> tree;
        std::stack<AdjacencyNode<T>*> stack, reverse_stack;
        AdjacencyNode<T> *temp, *node = _head_node;
        for (size_t i = 0; i < start; ++i)
        {
            node = node->get_node();
        }
        stack.push(node);
        while (!stack.empty())
        {
            node = stack.top();
            if (tree.append(node))
            {
                marks.push_back(node->get_data());
            }
            stack.pop();
            temp = node->get_next();
            while (temp != nullptr)
            {
                if (!tree.has(temp->get_node()))
                {
                    reverse_stack.push(temp->get_node());
                }
                temp = temp->get_next();
            }
            while (!reverse_stack.empty())
            {
                stack.push(reverse_stack.top());
                reverse_stack.pop();
            }
        }
        return marks;
    }

    const std::vector<T> bfs(const size_t& start) const
    {
        std::vector<T> marks;
        if (_head_node == nullptr)
        {
            return marks;
        }
        AVLTree<AdjacencyNode<T>*> tree;
        AdjacencyNode<T> *next, *node = _head_node;
        std::list<AdjacencyNode<T>*> next_lay, lay;
        for (size_t i = 0; i < start; ++i)
        {
            node = node->get_node();
        }
        marks.push_back(node->get_data());
        tree.append(node);
        lay.push_back(node);
        while (!lay.empty())
        {
            for (AdjacencyNode<T>* node : lay)
            {
                next = node->get_next();
                while (next != nullptr)
                {
                    if (tree.append(next->get_node()))
                    {
                        next_lay.push_back(next->get_node());
                        marks.push_back(next->get_node()->get_data());
                    }
                    next = next->get_next();
                }
            }
            lay.assign(next_lay.cbegin(), next_lay.cend());
            next_lay.clear();
        }
        return marks;
    }

    const std::vector<T> data() const
    {
        std::vector<T> result;
        AdjacencyNode<T>* node = _head_node;
        while (node != nullptr)
        {
            result.push_back(node->get_data());
            node = node->get_node();
        }
        return result;
    }
};


template <typename T>
class OrthogonalList
{
private:
    std::vector<OrthogonalNode<T>*> _nodes;

public:
    OrthogonalList(){}
    
    OrthogonalList(const std::initializer_list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        load_data(values, mat);
    }

    virtual ~OrthogonalList()
    {
        OrthogonalLink<T> *link, *next;
        for (OrthogonalNode<T>* node : _nodes)
        {
            link = node->get_tail_link();
            while (link != nullptr)
            {
                next = link->get_tail_link();
                delete link;
                link = next;
            }
        }
        while (!_nodes.empty())
        {
            delete _nodes.back();
            _nodes.pop_back();
        }
    }

    const size_t size() const
    {
        return _nodes.size();
    }

    const bool empty() const
    {
        return _nodes.empty();
    }

    const std::vector<T> data() const
    {
        std::vector<T> result;
        for (const OrthogonalNode<T>* node : _nodes)
        {
            result.push_back(node->get_data());
        }
        return result;
    }

    void clear()
    {
        OrthogonalLink<T> *link, *next;
        for (OrthogonalNode<T>* node : _nodes)
        {
            link = node->get_tail_link();
            while (link != nullptr)
            {
                next = link->get_tail_link();
                delete link;
                link = next;
            }
        }
        while (!_nodes.empty())
        {
            delete _nodes.back();
            _nodes.pop_back();
        }
    }

    void load_data(const std::list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        clear();
        for (const T& value : values)
        {
            _nodes.push_back(new OrthogonalNode(value));
        }
        const size_t count = mat.size();
        OrthogonalLink<T>* link;
        std::vector<OrthogonalLink<T>*> links;
        for (size_t i = 0; i < count; ++i)
        {
            for (size_t j = 0; j < count; ++j)
            {
                if (mat[i][j] > 0)
                {
                    links.push_back(new OrthogonalLink(_nodes[i], _nodes[j], mat[i][j]));
                    if (_nodes[i]->get_tail_link() == nullptr)
                    {
                        _nodes[i]->set_tail_link(links.back());
                        link = links.back();
                    }
                    else
                    {
                        link->set_tail_link(links.back());
                        link = link->get_tail_link();
                    }
                }
            }
        }

        for (size_t j = 0; j < count; ++j)
        {
            link = nullptr;
            for (size_t i = 0; i < count; ++i)
            {
                if (mat[i][j] > 0)
                {
                    for (OrthogonalLink<T>* temp_link : links)
                    {
                        if (temp_link->get_head_node() == _nodes[j] &&
                            temp_link->get_tail_node() == _nodes[i])
                        {
                            if (link == nullptr)
                            {
                                _nodes[j]->set_head_link(temp_link);
                                link = temp_link;
                            }
                            else
                            {
                                link->set_head_link(temp_link);
                                link = link->get_head_link();
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    std::vector<T> dfs(const size_t& start) const
    {
        std::vector<T> result;
        if (_nodes.empty())
        {
            return result;
        }
        AVLTree<OrthogonalNode<T>*> tree;
        std::stack<OrthogonalNode<T>*> temp, nodes;
        OrthogonalLink<T>* link;
        nodes.push(_nodes[start]);
        while (!nodes.empty())
        {
            if (tree.append(nodes.top()))
            {
                result.push_back(nodes.top()->get_data());
            }
            link = nodes.top()->get_tail_link();
            nodes.pop();
            while (link != nullptr)
            {
                if (!tree.has(link->get_head_node()))
                {
                    temp.push(link->get_head_node());
                }
                link = link->get_tail_link();
            }
            while (!temp.empty())
            {
                nodes.push(temp.top());
                temp.pop();
            }
        }
        link = nullptr;
        return result;
    }

    std::vector<T> bfs(const size_t& start) const
    {
        std::vector<T> result;
        if (_nodes.empty())
        {
            return result;
        }
        AVLTree<OrthogonalNode<T>*> tree;
        std::list<OrthogonalNode<T>*> lay, next_lay;
        lay.push_back(_nodes[start]);
        result.push_back(_nodes[start]->get_data());
        tree.append(_nodes[start]);
        OrthogonalLink<T>* link;
        while (!lay.empty())
        {
            for (OrthogonalNode<T>* node : lay)
            {
                link = node->get_tail_link();
                while (link != nullptr)
                {
                    if (tree.append(link->get_head_node()))
                    {
                        result.push_back(link->get_head_node()->get_data());
                        next_lay.push_back(link->get_head_node());
                    }
                    link = link->get_tail_link();
                }
            }
            lay.assign(next_lay.cbegin(), next_lay.cend());
            next_lay.clear();
        }
        link = nullptr;
        return result;
    }

    const std::vector<std::vector<int>> kruskal() const
    { 
        std::vector<OrthogonalLink<T>*> links;
        OrthogonalLink<T> *link = nullptr;
        for (OrthogonalNode<T>* node : _nodes)
        {
            link = node->get_tail_link();
            while (link != nullptr)
            {
                links.push_back(link);
                link = link->get_tail_link();
            }
        }
        link = nullptr;
        const size_t count = _nodes.size();
        size_t tail_index, head_index, tag, n = 0;
        std::vector<std::vector<int>> result(count, std::vector<int>(count, 0));
        std::vector<size_t> node_tags;
        for (size_t i = 0; i < count; ++i)
        {
            node_tags.push_back(i);
        }
        
        std::sort(links.begin(), links.end(), [](const OrthogonalLink<T>* link0, const OrthogonalLink<T>* link1){return link0->get_weight() < link1->get_weight();});

        for (const OrthogonalLink<T>* link : links)
        {
            tail_index = std::distance(_nodes.begin(), std::find(_nodes.begin(), _nodes.end(), link->get_tail_node()));
            head_index = std::distance(_nodes.begin(), std::find(_nodes.begin(), _nodes.end(), link->get_head_node()));
            if (node_tags[tail_index] != node_tags[head_index]) // 避免形成环路
            {
                result[tail_index][head_index] = link->get_weight();
                ++n;
                tag = node_tags[head_index];
                for (size_t i = 0; i < count; ++i)
                {
                    if (node_tags[i] == tag)
                    {
                        node_tags[i] = node_tags[tail_index];
                    }
                }
            }
            if (n == count - 1)
            {
                break;
            }
        }
        return result;
    }

    const std::vector<std::vector<int>> to_mat() const
    {
        std::vector<std::vector<int>> mat;
        if (_nodes.empty())
        {
            return mat;
        }
        const size_t count = _nodes.size();
        mat.assign(count, std::vector<int>(count, -1));
        OrthogonalLink<T>* link;
        for (size_t i = 0; i < count; ++i)
        {
            link = _nodes[i]->get_tail_link();
            while (link != nullptr)
            {
                mat[i][std::distance(_nodes.begin(), std::find(_nodes.begin(), _nodes.end(), link->get_head_node()))] = link->get_weight();
                link = link->get_tail_link();
            }
            mat[i][i] = 0;
        }
        return mat;
    }
};


template <typename T>
class AOENetwork: public AdjacencyList<T>
{
private:
    std::vector<size_t> _indegrees;

public:
    AOENetwork(){}

    AOENetwork(const std::initializer_list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        load_data(values, mat);
    }

    virtual void load_data(const std::list<T>& values, const std::vector<std::vector<int>>& mat)
    {
        AdjacencyList<T>::load_data(values, mat);
        const size_t count = values.size();
        _indegrees.assign(count, 0);
        for (size_t i = 0; i < count; ++i)
        {
            for (size_t j = 0; j < count; ++j)
            {
                if (mat[i][j] > 0)
                {
                    ++_indegrees[j];
                }
            }
        }
    }

    virtual void clear()
    {
        AdjacencyList<T>::clear();
        _indegrees.clear();
    }

    std::vector<T> topological_sort() const
    {
        std::vector<T> result;
        if (AdjacencyList<T>::empty())
        {
            return result;
        }
        std::vector<size_t> indegrees(_indegrees);
        AVLTree<AdjacencyNode<T>*> tree;
        const std::vector<AdjacencyNode<T>*> node_list = AdjacencyList<T>::nodes();
        AdjacencyNode<T>* node = nullptr;
        const size_t count = AdjacencyList<T>::size();
        while (result.size() < count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                if (indegrees[i] == 0 && tree.append(node_list[i]))
                {
                    result.push_back(node_list[i]->get_data());
                    node = node_list[i]->get_next();
                    break;
                }
            }
            while (node != nullptr)
            {
                --indegrees[std::distance(node_list.begin(), std::find(node_list.begin(), node_list.end(), node->get_node()))];
                node = node->get_next();
            }
        }
        return result;
    }

    // std::vector<T> critical_path() const
    // {

    // }
};