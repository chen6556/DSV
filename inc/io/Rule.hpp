#pragma once
#include "Scanner.hpp"


template <typename T>
class Rule
{
private:
    void (T::*_func)(const std::string &);
    Scanner _scanner;
    T *_spirit = nullptr;
    std::list<Rule<T>> _rules;

    enum Type {NONE, OR, LIST};
    Type _type = Type::NONE;
    bool _active = true;

    bool check(std::fstream &file) const
    {
        if (_rules.empty())
        {
            const size_t count = _scanner(file).size();
            file.seekg(count, std::ios::cur);
            return count > 0 || _scanner.optional();
        }
        else
        {
            const std::streampos pos = file.tellg();
            switch (_type)
            {
            case Type::OR:
                for (const Rule<T> &rule : _rules)
                {
                    if (rule.check(file))
                    {
                        return true;
                    }
                }
                return false;
            case Type::LIST:
                for (const Rule<T> &rule : _rules)
                {
                    if (!rule.check(file))
                    {
                        return false;
                    }
                }
                return true;
            default:
                break;
            }
            return false;
        }
    };

    bool check(std::stringstream &str) const
    {
         if (_rules.empty())
        {
            const size_t count = _scanner(str).size();
            str.seekg(count, std::ios::cur);
            return count > 0 || _scanner.optional();
        }
        else
        {
            switch (_type)
            {
            case Type::OR:
                for (const Rule<T> &rule : _rules)
                {
                    if (rule.check(str))
                    {
                        return true;
                    }
                }
                return false;
            case Type::LIST:
                for (const Rule<T> &rule : _rules)
                {
                    if (!rule.check(str))
                    {
                        return false;
                    }
                }
                return true;
            default:
                break;
            }
            return false;
        }
    };

public:
    Rule(const Scanner &scanner, T *spirit, void (T::*func)(const std::string &) = nullptr)
        : _scanner(scanner), _spirit(spirit), _func(func)
    {};

    ~Rule(){};


    Rule<T> operator|(const Rule<T> &rule) const
    {
        Rule<T> r(Scanner(), _spirit);
        r._type = Type::OR;
        if (_type == Type::OR)
        {
            r._rules.assign(_rules.begin(), _rules.end());
        }
        else
        {
            r._rulers.push_back(Rule<T>(_scanner, this, _func));
        }
        r._rules.push_back(rule);
        return r;
    }
    
    Rule<T> operator<<(const Rule<T> &rule) const
    {
        Rule<T> r(Scanner(), _spirit);
        r._type = Type::LIST;
        if (_type == Type::LIST)
        {
            r._rules.assign(_rules.begin(), _rules.end());
        }
        else
        {
            r._rules.push_back(Rule<T>(_scanner, this, _func));
        }
        r._rules.push_back(rule);
        return r;
    }
    
    friend Rule<T> operator!(const Rule<T> &rule)
    {
        Rule<T> r(rule._scanner, rule._spirit, rule._func);
        r._type = Rule<T>::Type::NOT;
        if (!rule._rules.empty())
        {
            r._rules.assign(rule);
        }
        return r;
    }


    bool operator()(std::fstream &file) const
    {   
        const std::streampos pos = file.tellg();
        if (!check(file))
        {
            file.seekg(pos);
            return false;
        }
        file.seekg(pos);

        if (_rules.empty())
        {
            const std::string result = _scanner(file);
            file.seekg(result.size(), std::ios::cur);
            if (!result.empty())
            {
                (_spirit->*_func)(result);
            }
        }
        else
        {
            switch (_type)
            {
            case Type::OR:
                for (const Rule<T> &rule : _rules)
                {
                    if (rule(file))
                    {
                        return true;
                    }
                }
            case Type::LIST:
                for (const Rule<T> &rule : _rules)
                {
                    rule(file);
                }
            default:
                break;
            }
        }
        return true;
    };

    bool operator()(std::stringstream &str) const
    {
        const std::streampos pos = str.tellg();
        if (!check(str))
        {
            str.seekg(pos);
            return false;
        }
        str.seekg(pos);

        if (_rules.empty())
        {
            const std::string result = _scanner(str);
            str.seekg(result.size(), std::ios::cur);
            if (!result.empty())
            {
                (_spirit->*_func)(result);
            }
        }
        else
        {
            switch (_type)
            {
            case Type::OR:
                for (const Rule<T> &rule : _rules)
                {
                    if (rule(str))
                    {
                        return true;
                    }
                }
            case Type::LIST:
                for (const Rule<T> &rule : _rules)
                {
                    rule(str);
                }
            default:
                break;
            }
        }

        return true;
    };   
};