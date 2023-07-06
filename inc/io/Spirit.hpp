#pragma once
#include "Rule.hpp"
#include <vector>


template <typename T>
class Spirit
{
private:
    std::vector<Rule<T>> _rules;

protected:
    bool _exec = false;
    bool _running = true;
    bool _finished = true;

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

    virtual void exec(){};

public:
    Spirit(){
        bind(Scanner(char(255)), &Spirit::pass);
    };

    virtual bool parse(std::fstream &file)
    {
        _running = true;
        while (_running && !file.eof())
        {
            if (_exec)
            {
                exec();
                _exec = false;
            }
            _running = false;
            for (const Rule<T> &rule : _rules)
            {
                if (rule(file))
                {
                    _running = true;
                    break;
                }
            }
        } 
        _finished = (file.peek() == EOF);
        return _running;
    };
    
    virtual bool parse(std::stringstream &str)
    {
        _running = true;
        while (_running && !str.eof())
        {
            if (_exec)
            {
                exec();
                _exec = false;
            }
            _running = false;
            for (const Rule<T> &rule : _rules)
            {
                if (rule(str))
                {
                    _running = true;
                    break;
                }
            }
        }
        _finished = (str.peek() == EOF);
        return _running;
    };
};


