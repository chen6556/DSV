#include "base/Memo.hpp"
#include <stack>


Memo::Memo(){}

Memo::Memo(const Memo& memo)
{
    _memos = memo._memos;
    _value = memo._value;
    _svalue = memo._svalue;
    _type = memo._type;
    _node_type = memo._node_type;
}

Memo::Memo(const Memo&& memo)
{
    _memos = std::move(memo._memos);
    _value = std::move(memo._value);
    _svalue = std::move(memo._svalue);
    _type = std::move(memo._type);
    _node_type = std::move(memo._node_type);
}

Memo::Memo(const bool& value)
{
    _type = Type::BOOL;
    _value.bvalue = value;
}

Memo::Memo(const int& value)
{
    _type = Type::INT;
    _value.ivalue = value;
}

Memo::Memo(const unsigned long long& value)
{
    _type = Type::ID;
    _value.lvalue = value;
}

Memo::Memo(const double& value)
{
    _type = Type::DOUBLE;
    _value.dvalue = value;
}

Memo::Memo(const std::string& value)
{
    _type = Type::STRING;
    _svalue = value;
}

Memo::Memo(const char value[])
{
    _type = Type::STRING;
    _svalue = value;
}

const Memo::Type& Memo::type() const
{
    return _type;
} 

void Memo::reset()
{
    _type = Type::NONE;
    _value.ivalue = 0;
    _svalue.clear();
}

void Memo::set(const bool& value)
{
    _type = Type::BOOL;
    _svalue.clear();
    _value.bvalue = value;
}

void Memo::set(const int& value)
{
    _type = Type::INT;
    _svalue.clear();
    _value.ivalue = value;
}

void Memo::set(const unsigned long long& value)
{
    _type = Type::ID;
    _svalue.clear();
    _value.lvalue = value;
}

void Memo::set(const double& value)
{
    _type = Type::DOUBLE;
    _svalue.clear();
    _value.dvalue = value;
}

void Memo::set(const std::string& value)
{
    _type = Type::STRING;
    _svalue = value;
    _value.ivalue = 0;
}

void Memo::set(const char value[])
{
    _type = Type::STRING;
    _svalue = value;
    _value.ivalue = 0;
}

void Memo::set(const Memo& memo)
{
    _memos = memo._memos;
    _value = memo._value;
    _svalue = memo._svalue;
    _type = memo._type;
    _node_type = memo._node_type;
}

void Memo::clear()
{
    _type = Type::NONE;
    _svalue.clear();
    _value.ivalue = 0;
    _memos.clear();
}

bool& Memo::to_bool()
{
    return _value.bvalue;
}

const bool& Memo::to_bool() const
{
    return _value.bvalue;
}

int& Memo::to_int()
{
    return _value.ivalue;
}

const int& Memo::to_int() const
{
    return _value.ivalue;
}

unsigned long long& Memo::to_ull()
{
    return _value.lvalue;
}

const unsigned long long& Memo::to_ull() const
{
    return _value.lvalue;
}

double& Memo::to_double()
{
    return _value.dvalue;
}

const double& Memo::to_double() const
{
    return _value.dvalue;
}

std::string& Memo::to_string()
{
    return _svalue;
}

const std::string& Memo::to_string() const
{
    return _svalue;
}



Memo& Memo::at(const std::string& key)
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        if (_memos.find(key) == _memos.end())
        {
            _memos.insert(std::make_pair(key,Memo()));
        }
        return _memos.at(key);
    }
    else
    {
        if (_memos.find(key.substr(0,pos)) == _memos.end())
        {
            _memos.insert(std::make_pair(key.substr(0,pos),Memo()));
        }
        return _memos.at(key.substr(0,pos)).at(key.substr(++pos));
    }
}

const Memo& Memo::at(const std::string& key) const
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        return _memos.at(key);
    }
    else
    {
        return _memos.at(key.substr(0,pos)).at(key.substr(++pos));
    }
}

Memo& Memo::operator[](const std::string& key)
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        if (_memos.find(key) == _memos.end())
        {
            _memos.insert(std::make_pair(key,Memo()));
        }
        return _memos.at(key);
    }
    else
    {
        if (_memos.find(key.substr(0,pos)) == _memos.end())
        {
            _memos.insert(std::make_pair(key.substr(0,pos),Memo()));
        }
        return _memos.at(key.substr(0,pos))[key.substr(++pos)];
    }
}

const Memo& Memo::operator[](const std::string& key) const
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        return _memos.at(key);
    }
    else
    {
        return _memos.at(key.substr(0,pos))[key.substr(++pos)];
    }
}



Memo& Memo::at(const size_t& key)
{
    return this->at(std::to_string(key));
}

const Memo& Memo::at(const size_t& key) const
{
    return this->at(std::to_string(key));
}

Memo& Memo::operator[](const size_t& key)
{
    return this->at(std::to_string(key));
}

const Memo& Memo::operator[](const size_t& key) const
{
    return this->at(std::to_string(key));
}



void Memo::erase(const std::string& key)
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        _memos.erase(_memos.find(key));
    }
    else
    {
        return _memos.at(key.substr(0, pos)).erase(key.substr(++pos));
    }
}



void Memo::operator=(const Memo& memo)
{
    _memos = memo._memos;
    _value = memo._value;
    _svalue = memo._svalue;
    _type = memo._type;
    _node_type = memo._node_type;
}

bool Memo::has(const std::string& key) const
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        return _memos.find(key) != _memos.end();
    }
    else
    {
        return _memos.at(key.substr(0, pos)).has(key.substr(++pos));
    }
}

const size_t Memo::size() const
{
    return _memos.size();
}

std::ostream& operator<<(std::ostream& o, const Memo& memo)
{
    switch (memo.type())
    {
    case Memo::Type::BOOL:
        o << memo.to_bool();
        break;
    case Memo::Type::DOUBLE:
        o << memo.to_double();
        break;
    case Memo::Type::INT:
        o << memo.to_int();
        break;
    case Memo::Type::STRING:
        o << memo.to_string();
        break;
    default:
        break;
    }

    return o;
}

void Memo::insert_void(const std::string& key)
{
    size_t pos = key.find('.');
    if (pos == std::string::npos)
    {
        if (_memos.find(key) == _memos.end())
        {
            _memos.insert(std::make_pair(key, Memo()));
        }
        else
        {
            _memos.at(key).clear();
        }
    }
    else
    {
        if (_memos.find(key.substr(0, pos)) == _memos.end())
        {
            _memos.insert(std::make_pair(key.substr(0, pos), Memo()));
        }
        _memos.at(key.substr(0, pos)).insert_void(key.substr(++pos));
    }
}

void Memo::insert_void(const size_t& key)
{
    if (_memos.find(std::to_string(key)) == _memos.end())
    {
        _memos.insert(std::make_pair(std::to_string(key), Memo()));
    }
    else
    {
        _memos.at(std::to_string(key)).clear();
    }
}

void Memo::set_node_type(const Type& type)
{
    if (type == Type::NONE || type == Type::LIST || type == Type::DICT)
    {
        _node_type = type;
    }
}

const Memo::Type& Memo::node_type() const
{
    return _node_type;
}



std::map<std::string, Memo>::iterator Memo::begin()
{
    return _memos.begin();
}

std::map<std::string, Memo>::iterator Memo::end()
{
    return _memos.end();
}

std::map<std::string, Memo>::const_iterator Memo::begin() const
{
    return _memos.cbegin();
}

std::map<std::string, Memo>::const_iterator Memo::end() const
{
    return _memos.cend();
}

std::map<std::string, Memo>::const_iterator Memo::cbegin() const
{
    return _memos.cbegin();
}

std::map<std::string, Memo>::const_iterator Memo::cend() const
{
    return _memos.cend();
}

std::map<std::string, Memo>::reverse_iterator Memo::rbegin()
{
    return _memos.rbegin();
}

std::map<std::string, Memo>::reverse_iterator Memo::rend()
{
    return _memos.rend();
}

std::map<std::string, Memo>::const_reverse_iterator Memo::rbegin() const
{
    return _memos.crbegin();
}

std::map<std::string, Memo>::const_reverse_iterator Memo::rend() const
{
    return _memos.crend();
}

std::map<std::string, Memo>::const_reverse_iterator Memo::crbegin() const
{
    return _memos.crbegin();
}

std::map<std::string, Memo>::const_reverse_iterator Memo::crend() const
{
    return _memos.crend();
}


const std::vector<Memo> Memo::dfs() const
{
    std::vector<Memo> memos;
    std::stack<Memo> stack;
    Memo temp;
    for (std::map<std::string, Memo>::const_reverse_iterator it = _memos.crbegin(), end = _memos.crend(); it != end; ++it)
    {
        stack.push(it->second);
    }
    while (!stack.empty())
    {
        if (stack.top().node_type() == Memo::Type::NONE)
        {
            memos.push_back(stack.top());
            stack.pop();
        }
        else
        {
            temp = stack.top();
            stack.pop();
            for (std::map<std::string, Memo>::const_reverse_iterator it = temp.crbegin(), end = temp.crend(); it != end; ++it)
            {
                stack.push(it->second);
            }
        }
    }
    return memos;
}