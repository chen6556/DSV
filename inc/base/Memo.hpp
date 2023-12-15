#pragma once

#include <map>
#include <string>
#include <iostream>
#include <vector>


class Memo
{
private:
    union
    {
        bool bvalue;
        int ivalue;
        unsigned long long lvalue;
        double dvalue = 0.0;
    } _value;
    std::string _svalue;
    std::map<std::string, Memo> _memos;

public:
    enum Type
    {
        NONE,
        BOOL,
        INT,
        ID,
        DOUBLE,
        STRING,
        LIST,
        DICT
    } _type = Type::NONE,
      _node_type = Type::NONE;
    
    Memo();
    Memo(const Memo &memo);
    Memo(const Memo &&memo);
    Memo(const bool &value);
    Memo(const int &value);
    Memo(const unsigned long long &value);
    Memo(const double &value);
    Memo(const std::string &value);
    Memo(const char value[]);

    const Type &type() const;
    void reset();
    void set(const bool &value);
    void set(const int &value);
    void set(const unsigned long long &value);
    void set(const double &value);
    void set(const std::string &value);
    void set(const char value[]);
    void set(const Memo &memo);
    void clear();

    bool &to_bool();
    const bool &to_bool() const;
    int &to_int();
    const int &to_int() const;
    unsigned long long &to_ull();
    const unsigned long long &to_ull() const;
    double &to_double();
    const double &to_double() const;
    std::string &to_string();
    const std::string &to_string() const;

    Memo &at(const std::string &key);
    const Memo &at(const std::string &key) const;
    Memo &operator[](const std::string &key);
    const Memo &operator[](const std::string &key) const;

    Memo &at(const size_t &key);
    const Memo &at(const size_t &key) const;
    Memo &operator[](const size_t &key);
    const Memo &operator[](const size_t &key) const;

    void erase(const std::string &key);

    void operator=(const Memo &memo);
    bool has(const std::string &key) const;
    const size_t size() const;

    friend std::ostream &operator<<(std::ostream &o, const Memo &memo);

    template <typename T>
    void operator=(const T &value)
    {
        set(value);
    }

    template <typename T>
    void operator=(const std::initializer_list<T> &values)
    {
        size_t count = 0;
        for (const T &value : values)
        {
            insert(std::to_string(count++), value);
        }
    }

    template <typename T>
    void insert(const std::string &key, const T &value)
    {
        size_t pos = key.find('.');
        if (pos == std::string::npos)
        {
            if (_memos.find(key) == _memos.end())
            {
                _memos.insert(std::make_pair(key, Memo(value)));
            }
            else
            {
                _memos.at(key).set(value);
            }
        }
        else
        {
            if (_memos.find(key.substr(0, pos)) == _memos.end())
            {
                _memos.insert(std::make_pair(key.substr(0, pos), Memo()));
            }
            _memos.at(key.substr(0, pos)).insert(key.substr(++pos), value);
        }
    }

    template <typename T>
    void insert(const size_t &key, const T &value)
    {
        if (_memos.find(std::to_string(key)) == _memos.end())
        {
            _memos.insert(std::make_pair(std::to_string(key), Memo(value)));
        }
        else
        {
            _memos.at(std::to_string(key)).set(value);
        }
    }

    void insert_void(const std::string &key);
    void insert_void(const size_t &key);

    void set_node_type(const Type &type);
    const Type &node_type() const;

    std::map<std::string, Memo>::iterator begin();
    std::map<std::string, Memo>::iterator end();
    std::map<std::string, Memo>::const_iterator begin() const;
    std::map<std::string, Memo>::const_iterator end() const;
    std::map<std::string, Memo>::const_iterator cbegin() const;
    std::map<std::string, Memo>::const_iterator cend() const;

    std::map<std::string, Memo>::reverse_iterator rbegin();
    std::map<std::string, Memo>::reverse_iterator rend();
    std::map<std::string, Memo>::const_reverse_iterator rbegin() const;
    std::map<std::string, Memo>::const_reverse_iterator rend() const;
    std::map<std::string, Memo>::const_reverse_iterator crbegin() const;
    std::map<std::string, Memo>::const_reverse_iterator crend() const;

    const std::vector<Memo> dfs() const;
};
