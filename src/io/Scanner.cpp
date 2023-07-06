#include "io/Scanner.hpp"
#include <algorithm>
#include <vector>
#include <assert.h>
#include <iostream>

Scanner::Scanner()
{}

Scanner::Scanner(const char text)
{
    _txt.push_back(text);
}

Scanner::Scanner(const std::string &text)
    : _txt(text)
{}

Scanner::Scanner(const Scanner &scanner)
    :_type(scanner._type), _scanners(scanner._scanners), _txt(scanner._txt), _optional(scanner._optional)
{}

Scanner::~Scanner()
{}



Scanner Scanner::repeat(const Scanner &scanner, const size_t times)
{
    Scanner s;
    s._type = Scanner::Type::REPEAT;
    s._scanners.push_back(scanner);
    s._repeat_tiems = times;
    return s;
}

Scanner Scanner::repeat(const std::string &scanner, const size_t times)
{
    Scanner s(scanner);
    s._type = Scanner::Type::REPEAT;
    s._repeat_tiems = times;
    return s;
}

Scanner Scanner::repeat(const char scanner, const size_t times)
{
    return Scanner::repeat(std::string(1, scanner), times);
}

Scanner Scanner::optional(const Scanner &scanner)
{
    Scanner s(scanner);
    s._optional = true;
    return s;
}

Scanner Scanner::optional(const std::string &scanner)
{
    Scanner s(scanner);
    s._optional = true;
    return s;
}

Scanner Scanner::optional(const char scanner)
{
    return Scanner::optional(std::string(1, scanner));
}

Scanner Scanner::range(const int a, const int b)
{
    assert(a < b);
    Scanner scanner;
    scanner._type = Scanner::OR;
    int i = a;
    while (i <= b)
    {
        scanner._scanners.push_back(Scanner(std::to_string(i++)));
    } 
    return scanner;
}

Scanner Scanner::range(const char a, const char b)
{
    assert(a < b);
    Scanner scanner;
    scanner._type = Scanner::Type::OR;
    char i = a;
    while (i <= b && i < 127)
    {
        scanner._scanners.push_back(Scanner(i++));
    }
    if (i == 127)
    {
        scanner._scanners.push_back(Scanner(i));
    }
    return scanner;
}




const bool &Scanner::optional() const
{
    return _optional;
}

const Scanner &Scanner::operator=(const Scanner &scanner)
{
    if (this != &scanner)
    {
        _type = scanner._type;
        _optional = scanner._optional;
        if (scanner._txt.empty())
        {
            _txt.clear();
            _scanners.assign(scanner._scanners.begin(), scanner._scanners.end());
        }
        else
        {
            _scanners.clear();
            _txt = scanner._txt;
        }
    }
    return *this;
}

const Scanner &Scanner::operator=(const std::string &scanner)
{
    _type = NONE;
    _txt = scanner;
    _scanners.clear();
    return *this;
}




Scanner Scanner::operator|(const Scanner &scanner) const
{
    Scanner s;
    s._type = Type::OR;
    if (_type == Type::OR)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(scanner);
    return s;
}

Scanner Scanner::operator&(const Scanner &scanner) const
{
    Scanner s;
    s._type = Type::AND;
    if (_type == Type::AND)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(scanner);
    return s;
}

Scanner Scanner::operator<<(const Scanner &scanner) const
{
    Scanner s;
    s._type = Type::LIST;
    if (_type == Type::LIST)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(scanner);
    return s;
}

Scanner operator!(const Scanner &scanner)
{
    Scanner s(scanner);
    s._type = Scanner::Type::NOT;
    return s;
}


Scanner Scanner::operator|(const std::string &scanner) const
{
    Scanner s;
    s._type = Type::OR;
    if (_type == Type::OR)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(Scanner(scanner));
    return s;
}

Scanner Scanner::operator&(const std::string &scanner) const
{
    Scanner s;
    s._type = Type::AND;
    if (_type == Type::AND)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(Scanner(scanner));
    return s;
}

Scanner Scanner::operator<<(const std::string &scanner) const
{
    Scanner s;
    s._type = Type::LIST;
    if (_type == Type::LIST)
    {
        s._scanners.assign(_scanners.begin(), _scanners.end());
    }
    else
    {
        s._scanners.push_back(*this);
    }
    s._scanners.push_back(Scanner(scanner));
    return s;
}

Scanner operator!(const std::string &scanner)
{
    Scanner s(scanner);
    s._type = Scanner::Type::NOT;
    return s;
}




std::string Scanner::operator()(std::fstream &file) const
{
    std::string result;
    if (_txt.empty() && _scanners.empty())
    {
        return result;
    }
    int index = 0;
    char ch;
    bool flag = true;
    const std::streampos pos = file.tellg();
    std::vector<std::string> result_temp;
    if (_txt.empty())
    {
        if (_scanners.empty())
        {
            return result;
        }
        switch (_type)
        {
        case NONE:
            result = _scanners.front()(file);
            break;
        case OR:
            for (const Scanner &scanner : _scanners)
            {
                result = scanner(file);
                if (!result.empty())
                {
                    break;
                }
            }
            break;
        case REPEAT:
            if (_repeat_tiems == 0)
            {
                result_temp.push_back(_scanners.front()(file));             
                while (!result_temp.front().empty())
                {
                    result.append(result_temp.back().begin(), result_temp.back().end());
                    file.seekg(result_temp.back().size(), std::ios::cur);
                    result_temp.pop_back();
                    result_temp.push_back(_scanners.front()(file));
                }
            }
            else
            {
                size_t count = 0;
                result_temp.push_back(_scanners.front()(file));
                while (count++ < _repeat_tiems && !result_temp.back().empty())
                {
                    result.append(result_temp.back().begin(), result_temp.back().end());
                    file.seekg(result_temp.back().size(), std::ios::cur);
                    result_temp.pop_back();
                    result_temp.push_back(_scanners.front()(file));
                }
            }
            break;
        case NOT:
            while (_scanners.front()(file).empty())
            {
                file.seekg(pos);
                file.seekg(index++, std::ios::cur);
                file.get(ch);
                result.push_back(ch);
            }
            break;
        case AND:
            for (const Scanner &scanner : _scanners)
            {
                if (flag)
                {
                    result = scanner(file);
                    flag = false;
                }
                else
                {
                    std::stringstream ss(result);
                    result = scanner(ss);
                }
            }
            break;
        case LIST:
            for (const Scanner &scanner : _scanners)
            {
                result_temp.push_back(scanner(file));
                if (result_temp.back().empty() && !scanner._optional)
                {
                    result.clear();
                    break;
                }
                file.seekg(result_temp.back().size(), std::ios::cur);
                result.append(result_temp.back());
                result_temp.pop_back();
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch (_type)
        {
        case NONE:
            for (size_t i = 0, count = _txt.size(); i < count; ++i)
            {
                if (!file.get(ch) || _txt[i] != ch)
                {
                    flag = false;
                    break;
                }
            }
            if (flag)
            {
                result.assign(_txt.begin(), _txt.end());
            }
            break;
        case REPEAT:
            if (_repeat_tiems == 0)
            {
                while (file.get(ch) && ch == _txt[index])
                {
                    result.push_back(ch);
                    if (++index == _txt.size())
                    {
                        index = 0;
                    }
                }
            }
            else
            {
                size_t count = 0;
                while (count < _repeat_tiems && file.get(ch) && ch == _txt[index])
                {
                    result.push_back(ch);
                    if (++index == _txt.size())
                    {
                        index = 0;
                        ++count;
                    }
                }
            }
            if (index > 0)
            {
                result.erase(result.size() - index, index);
                file.seekg(-index, std::ios::cur);
            }     
            break;
        case NOT:
            while (file.get(ch) && index != _txt.size())
            {
                result.push_back(ch);
                if (ch != _txt[index++])
                {
                    index = 0;
                }
            }
            result.erase(result.size() - index, index);
            break;
        default:
            break;
        }
    }

    if (file.eof())
    {
        file.clear();
    }
    file.seekg(pos);
    return result;
}

std::string Scanner::operator()(std::stringstream &str) const
{
    std::string result;
    if (_txt.empty() && _scanners.empty())
    {
        return result;
    }
    int index = 0;
    char ch;
    bool flag = true;
    const std::streampos pos = str.tellg();
    std::vector<std::string> result_temp;
    if (_txt.empty())
    {
        switch (_type)
        {
        case NONE:
            result = _scanners.front()(str);
            break;
        case OR:
            for (const Scanner &scanner : _scanners)
            {
                result = scanner(str);
                if (!result.empty())
                {
                    break;
                }
            }
            break;
        case REPEAT:
            if (_repeat_tiems == 0)
            {
                result_temp.push_back(_scanners.front()(str));
                while (!result_temp.back().empty())
                {
                    result.append(result_temp.back().begin(), result_temp.back().end());
                    str.seekg(result_temp.back().size(), std::ios::cur);
                    result_temp.pop_back();
                    result_temp.push_back(_scanners.front()(str));
                }
            }
            else
            {
                size_t count = 0;
                result_temp.push_back(_scanners.front()(str));  
                while (count++ < _repeat_tiems && !result_temp.back().empty())
                {
                    result.append(result_temp.front().begin(), result_temp.front().end());
                    str.seekg(result_temp.back().size(), std::ios::cur);
                    result_temp.pop_back();
                    result_temp.push_back(_scanners.front()(str));
                }
            }
            break;
        case NOT:
            while (_scanners.front()(str).empty())
            {
                str.seekg(pos);
                str.seekg(index++, std::ios::cur);
                str.get(ch);
                result.push_back(ch);
            }
            break;
        case AND:
            for (const Scanner &scanner : _scanners)
            {
                if (flag)
                {
                    result = scanner(str);
                    flag = false;
                }
                else
                {   
                    std::stringstream ss(result);
                    result = scanner(ss);
                }
            }
            break;
        case LIST:
            for (const Scanner &scanner : _scanners)
            {
                result_temp.push_back(scanner(str));
                if (result_temp.back().empty() && !scanner._optional)
                {
                    result.clear();
                    break;
                }
                str.seekg(result_temp.back().size(), std::ios::cur);
                result.append(result_temp.back());
                result_temp.pop_back();
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch (_type)
        {
        case NONE:
            for (size_t i = 0, count = _txt.size(); i < count; ++i)
            {
                if (!str.get(ch) || _txt[i] != ch)
                {
                    flag = false;
                    break;
                }
            }
            if (flag)
            {
                result.assign(_txt.begin(), _txt.end());
            }
            break;
        case REPEAT:
            if (_repeat_tiems == 0)
            {
                while (str.get(ch) && ch == _txt[index])
                {
                    result.push_back(ch);
                    if (++index == _txt.size())
                    {
                        index = 0;
                    }
                }
            }
            else
            {
                size_t count = 0;
                while (count < _repeat_tiems && str.get(ch) && ch == _txt[index])
                {
                    result.push_back(ch);
                    if (++index == _txt.size())
                    {
                        index = 0;
                        ++count;
                    }
                }
            }
            if (index > 0)
            {
                result.erase(result.size() - index, index);
            }     
            break;
        case NOT:
            while (str.get(ch) && index != _txt.size())
            {
                result.push_back(ch);
                if (ch != _txt[index++])
                {
                    index = 0;
                }
            }
            result.erase(result.size() - index, index);
            break;
        default:
            break;
        }
    }

    if (str.eof())
    {
        str.clear();
    }
    str.seekg(pos, std::ios::beg);
    return result;
}


