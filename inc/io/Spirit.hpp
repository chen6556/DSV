#pragma once
#include "Rule.hpp"
#include <vector>


template <typename T>
class Spirit
{
private:
    std::vector<Rule<T>> _rules;

protected:
    bool _running = true;

    virtual void pass(const std::string &){};

    virtual void bind(const Scanner &scanner, void (T::*func)(const std::string &) = &T::pass)
    {
        _rules.push_back(Rule<T>(scanner, reinterpret_cast<T *>(this), func));
    };

    virtual Rule<T> rule(const Scanner &scanner, void (T::*func)(const std::string &) = &T::pass)
    {
        return Rule<T>(scanner, reinterpret_cast<T *>(this), func);
    };

    virtual void bind(const Rule<T> &rule)
    {
        _rules.push_back(rule);
    }


public:
    Spirit()
    {
        bind(Scanner(char(255)), &Spirit::pass);
    };

    virtual bool parse(std::fstream &file)
    {
        _running = true;
        file.seekg(0, std::ios_base::end);
        const size_t length = file.tellg();
        size_t pos = 0;
        file.seekg(0, std::ios_base::beg);
        while (_running && pos < length)
        {
            _running = false;
            for (const Rule<T> &rule : _rules)
            {
                if (rule(file))
                {
                    _running = true;
                    break;
                }
            }
            pos = file.tellg();
        }
        return _running;
    };
    
    virtual bool parse(std::stringstream &str)
    {
        _running = true;
        str.seekg(0, std::ios_base::end);
        const size_t length = str.tellg();
        size_t pos = 0;
        str.seekg(0, std::ios_base::beg);
        while (_running && pos < length)
        {
            _running = false;
            for (const Rule<T> &rule : _rules)
            {
                if (rule(str))
                {
                    _running = true;
                    break;
                }
            }
            pos = str.tellg();
        }
        return _running;
    };
};


