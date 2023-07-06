#pragma once
#include <iostream>


template <typename T>
class LinkedListNode
{
private:
    T _data;
    LinkedListNode<T> *_next_node = nullptr, *_last_node = nullptr;

public:
    LinkedListNode(){}

    LinkedListNode(T value)
    {
        _data = value;
    }

    LinkedListNode(const LinkedListNode<T>& node)
    {
        _data = node._data;
    }

    LinkedListNode(const LinkedListNode<T>&& node)
    {
        _data = std::move(node._data);
    }

    void set_data(const T& value)
    {
        _data = value;
    }

    T& get_data()
    {
        return _data;
    }

    const T& get_data() const
    {
        return _data;
    }

    void set_next_node(LinkedListNode<T> *node)
    {
        _next_node = node;
    }

    LinkedListNode<T>* get_next_node()
    {
        return _next_node;
    }

    const LinkedListNode<T>* get_next_node() const
    {
        return _next_node;
    }

    void set_last_node(LinkedListNode<T> *node)
    {
        _last_node = node;
    }

    LinkedListNode<T>* get_last_node()
    {
        return _last_node;
    }

    const LinkedListNode<T>* get_last_node() const
    {
        return _last_node;
    }

    friend std::ostream& operator<<(std::ostream& o, const LinkedListNode<T>& node)
    {
        o << node.get_data();
        return o;
    }

    LinkedListNode<T>& operator=(const LinkedListNode<T>& node)
    {
        if (this != &node)
        {
            _data = node._data;
        }
        return *this;
    }

    LinkedListNode<T>& operator=(const LinkedListNode<T>&& node)
    {
        if (this != &node)
        {
            _data = std::move(node._data);
        }
        return *this;
    }

    LinkedListNode<T>& operator=(const T& value)
    {
        _data = value;
        return *this;
    }

    const bool operator<(const LinkedListNode<T>& node) const
    {
        return _data < node._data;
    }

    const bool operator==(const LinkedListNode<T>& node) const
    {
        return _data == node._data;
    }

    const bool operator>(const LinkedListNode<T>& node) const
    {
        return _data > node._data;
    }

    const bool operator<=(const LinkedListNode<T>& node) const
    {
        return _data <= node._data;
    }

    const bool operator>=(const LinkedListNode<T>& node) const
    {
        return _data >= node._data;
    }

    const bool operator<(const T& value) const
    {
        return _data < value;
    }

    const bool operator==(const T& value) const
    {
        return _data == value;
    }

    const bool operator>(const T& value) const
    {
        return _data > value;
    }

    const bool operator<=(const T& value) const
    {
        return _data <= value;
    }

    const bool operator>=(const T& value) const
    {
        return _data >= value;
    }
};


template <typename T>
class BinaryTreeNode
{
private:
    T _data;
    BinaryTreeNode<T>* _parent = nullptr;
    BinaryTreeNode<T>* _left = nullptr;
    BinaryTreeNode<T>* _right = nullptr;

public:
    BinaryTreeNode(){}

    BinaryTreeNode(const T& value)
    {
        _data = value;
    }

    BinaryTreeNode(const BinaryTreeNode<T>& node)
    {
        _data = node._data;
        _parent = node._parent;
        _left = node._left;
        _right = node._right;
    }

    BinaryTreeNode(const BinaryTreeNode<T>&& node)
    {
        _data = std::move(node._data);
        _parent = std::move(node._parent);
        _left = std::move(node._left);
        _right = std::move(node._right);
    }

    void set_data(const T& value)
    {
        _data = value;
    }

    T& get_data()
    {
        return _data;
    }

    const T& get_data() const
    {
        return _data;
    }

    BinaryTreeNode<T>* get_parent()
    {
        return _parent;
    }

    const BinaryTreeNode<T>* get_parent() const
    {
        return _parent;
    }

    void set_parent(BinaryTreeNode<T>* node)
    {
        _parent = node;
    }

    BinaryTreeNode<T>* get_left()
    {
        return _left;
    }

    const BinaryTreeNode<T>* get_left() const
    {
        return _left;
    }

    void set_left(BinaryTreeNode<T>* node)
    {
        _left = node;
        if (_left != nullptr)
        {
            _left->set_parent(this);
        }
    }

    BinaryTreeNode<T>* get_right()
    {
        return _right;
    }

    const BinaryTreeNode<T>* get_right() const
    {
        return _right;
    }

    void set_right(BinaryTreeNode<T>* node)
    {
        _right = node;
        if (_right != nullptr)
        {
            _right->set_parent(this);
        }
    }

    BinaryTreeNode<T>& operator=(const BinaryTreeNode<T>& node)
    {
        if (this != &node)
        {
            _data = node._data;
            _parent = node._parent;
            _left = node._left;
            _right = node._right;
        }
        return *this;
    }

    BinaryTreeNode<T>& operator=(const T& value)
    {
        _data = value;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& o, const BinaryTreeNode<T>& node)
    {
        o << node.get_data();
        return o;
    }

    const bool operator<(const BinaryTreeNode<T>& node) const
    {
        return _data < node._data;
    }

    const bool operator==(const BinaryTreeNode<T>& node) const
    {
        return _data == node._data;
    }

    const bool operator>(const BinaryTreeNode<T>& node) const
    {
        return _data > node._data;
    }

    const bool operator<=(const BinaryTreeNode<T>& node) const
    {
        return _data <= node._data;
    }

    const bool operator>=(const BinaryTreeNode<T>& node) const
    {
        return _data >= node._data;
    }

    const bool operator<(const T& value) const
    {
        return _data < value;
    }

    const bool operator==(const T& value) const
    {
        return _data == value;
    }

    const bool operator>(const T& value) const
    {
        return _data > value;
    }

    const bool operator<=(const T& value) const
    {
        return _data <= value;
    }

    const bool operator>=(const T& value) const
    {
        return _data >= value;
    }
};


template <typename T>
class AdjacencyNode
{
private:
    T _data;
    AdjacencyNode<T> *_next = nullptr, *_node = nullptr;
    int _weight = NULL;

public:
    AdjacencyNode(){}

    AdjacencyNode(const T& data)
    {
        _data = data;
    }

    AdjacencyNode(const T& data, const int& weight)
    {
        _data = data;
        _weight = weight;
    }

    AdjacencyNode(const T& data, const int& weight, AdjacencyNode<T>* node)
    {
        _data = data;
        _weight = weight;
        _next = node;
    }

    AdjacencyNode(const T& data, AdjacencyNode<T>* node)
    {
        _data = data;
        _next = node;
    }

    AdjacencyNode(const AdjacencyNode<T>& node)
    {
        _data = node._data;
        _weight = node._weight;
        _next = node._next;
    }

    AdjacencyNode(const AdjacencyNode&& node)
    {
        _data = std::move(node._data);
        _weight = std::move(node._weight);
        _next = std::move(node._next);
    }

    void set_date(const T& value)
    {
        _data = value;
    }

    T& get_data()
    {
        return _data;
    }

    const T& get_data() const
    {
        return _data;
    }

    void set_weight(const int& value)
    {
        _weight = value;
    }

    int& get_weight()
    {
        return _weight;
    }

    const int& get_weight() const
    {
        return _weight;
    }

    void set_next(AdjacencyNode<T>* node)
    {
        _next = node;
    }

    AdjacencyNode<T>* get_next()
    {
        return _next;
    }

    const  AdjacencyNode<T>* get_next() const
    {
        return _next;
    }

    void set_node(AdjacencyNode<T>* node)
    {
        _node = node;
    }

    AdjacencyNode<T>* get_node()
    {
        return _node;
    }

    const AdjacencyNode<T>* get_node() const
    {
        return _node;
    }
};


template <typename T>
class OrthogonalLink;


template <typename T>
class OrthogonalNode
{
private:
    T _data;
    OrthogonalLink<T> *_tail_link = nullptr;
    OrthogonalLink<T> *_head_link = nullptr;

public:
    OrthogonalNode(){}

    OrthogonalNode(const T& value)
    {
        _data = value;
    }

    OrthogonalNode(const OrthogonalNode<T>& node)
    {
        _data = node._data;
        _head_link = node._head_link;
        _tail_link = node._tail_link;
    }

    OrthogonalNode(const OrthogonalNode<T>&& node)
    {
        _data = std::move(node._data);
        _head_link = std::move(node._head_link);
        _tail_link = std::move(node._tail_link);
    }

    ~OrthogonalNode(){}

    void set_data(const T& value)
    {
        _data = value;
    }

    T& get_data()
    {
        return _data;
    }

    const T& get_data() const
    {
        return _data;
    }

    void set_tail_link(OrthogonalLink<T>* link)
    {
        _tail_link = link;
    }

    OrthogonalLink<T>* get_tail_link()
    {
        return _tail_link;
    }

    const OrthogonalLink<T>* get_tail_link() const
    {
        return _tail_link;
    }

    void set_head_link(OrthogonalLink<T>* link)
    {
        _head_link = link;
    }

    OrthogonalLink<T>* get_head_link()
    {
        return _head_link;
    }

    const OrthogonalLink<T>* get_head_link() const
    {
        return _head_link;
    }

    friend std::ostream& operator<<(std::ostream& o, const OrthogonalNode<T>& node)
    {
        o << node.get_data();
        return o;
    }
};


template <typename T>
class OrthogonalLink
{
private:
    OrthogonalNode<T>* _head_node = nullptr;
    OrthogonalNode<T>* _tail_node = nullptr;
    OrthogonalLink<T>* _head_link = nullptr;
    OrthogonalLink<T>* _tail_link = nullptr;
    int _weight = NULL;

public:
    OrthogonalLink(){}

    OrthogonalLink(OrthogonalNode<T>* tail, OrthogonalNode<T>* head, const int& weight)
    {
        _head_node = head;
        _tail_node = tail;
        _weight = weight;
    }

    ~OrthogonalLink(){}

    void set_weight(const int& weight)
    {
        _weight = weight;
    }

    const int& get_weight() const
    {
        return _weight;
    }

    void set_head_node(OrthogonalNode<T>* node)
    {
        _head_node = node;
    }

    OrthogonalNode<T>* get_head_node()
    {
        return _head_node;
    }

    const OrthogonalNode<T>* get_head_node() const
    {
        return _head_node;
    }

    void set_tail_node(OrthogonalNode<T>* node)
    {
        _tail_node = node;
    }

    OrthogonalNode<T>* get_tail_node()
    {
        return _tail_node;
    }

    const OrthogonalNode<T>* get_tail_node() const
    {
        return _tail_node;
    }

    void set_head_link(OrthogonalLink<T>* link)
    {
        _head_link = link;
    }

    OrthogonalLink<T>* get_head_link()
    {
        return _head_link;
    }

    const OrthogonalLink<T>* get_head_link() const
    {
        return _head_link;
    }

    void set_tail_link(OrthogonalLink<T>* link)
    {
        _tail_link = link;
    }

    OrthogonalLink<T>* get_tail_link()
    {
        return _tail_link;
    }

    const OrthogonalLink<T>* get_tail_link() const
    {
        return _tail_link;
    }
};

