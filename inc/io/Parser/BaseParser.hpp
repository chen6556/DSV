#pragma once

#include <string_view>
#include <optional>
#include <variant>
#include <vector>
#include "Action.hpp"


template <typename T>
struct Parser
{
    std::function<std::optional<T>(std::string_view &)> func;
    Action<void> call;

    Parser(const std::function<std::optional<T>(std::string_view &)> &f)
        : func(f) {}

    Parser(const Parser<T> &parser)
        : func(parser.func), call(parser.call) {}

    inline std::optional<T> operator()(std::string_view &stream) const
    {
        std::optional<T> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call();
        }
        return result;
    }

    Parser<T> &operator[](Action<void> &action)
    {
        call = action;
        return *this;
    }

    Parser<T> &operator[](const std::function<void(void)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<bool>
{
    std::function<bool(std::string_view &)> func;
    Action<void> call;

    Parser(const std::function<bool(std::string_view &)> &f)
        : func(f) {};

    Parser() {}

    template <typename T>
    Parser(const Parser<T> &parser)
        : func([=](std::string_view &stream){return parser.func(stream).has_value();}), call(parser.call) {}

    Parser(const Parser<bool> &parser)
        : func(parser.func), call(parser.call) {}

    inline bool operator()(std::string_view &stream) const
    {
        if (this->func(stream))
        {
            if (this->call)
            {
                this->call();
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    Parser<bool> &operator[](Action<void> &action)
    {
        call = action;
        return *this;
    }

    Parser<bool> &operator[](const std::function<void(void)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<std::string>
{
    std::function<std::optional<std::string>(std::string_view &)> func;
    Action<void> void_call;
    Action<std::string> call;

    Parser(const std::function<std::optional<std::string>(std::string_view &)> &f)
        : func(f) {}

    Parser(const std::string &value)
        : func([=](std::string_view &stream) -> std::optional<std::string>
        {
            if (stream.length() >= value.length() && stream.substr(0, value.length()) == value)
            {
                stream.remove_prefix(value.length());
                return value;
            }
            else
            {
                return std::nullopt;
            }
        }) {}

    Parser(const Parser<std::string> &parser)
        : func(parser.func), call(parser.call), void_call(parser.void_call) {}

    std::optional<std::string> operator()(std::string_view &stream) const
    {
        const std::optional<std::string> result = this->func(stream);
        if (result.has_value())
        {
            if (this->void_call)
            {
                this->void_call();
            }
            else if (this->call)
            {
                this->call(result.value());
            }
        }
        return result;
    }

    Parser<std::string> &operator[](Action<void> &action)
    {
        void_call = action;
        return *this;
    }

    Parser<std::string> &operator[](const std::function<void(void)> &f)
    {
        void_call = f;
        return *this;
    }

    Parser<std::string> &operator[](Action<std::string> &action)
    {
        call = action;
        return *this;
    }

    Parser<std::string> &operator[](const std::function<void(const std::string)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<char>
{
    std::function<std::optional<char>(std::string_view &)> func;
    Action<void> void_call;
    Action<char> call;

    Parser(const std::function<std::optional<char>(std::string_view &)> &f)
        : func(f) {}

    Parser(const char value)
        : func([=](std::string_view &stream) -> std::optional<char>
            {
                if (!stream.empty() && stream.front() == value)
                {
                    stream.remove_prefix(1);
                    return value;
                }
                else
                {
                    return std::nullopt;
                }
            }
        ) {}

    Parser(const Parser<char> &parser)
        : func(parser.func), call(parser.call), void_call(parser.void_call) {}

    std::optional<char> operator()(std::string_view &stream) const
    {
        const std::optional<char> result = this->func(stream);
        if (result.has_value())
        {
            if (this->void_call)
            {
                this->void_call();
            }
            else if (this->call)
            {
                this->call(result.value());
            }
        }
        return result;
    }

    Parser<char> &operator[](Action<void> &action)
    {
        void_call = action;
        return *this;
    }

    Parser<char> &operator[](const std::function<void(void)> &f)
    {
        void_call = f;
        return *this;
    }

    Parser<char> &operator[](Action<char> &action)
    {
        call = action;
        return *this;
    }

    Parser<char> &operator[](const std::function<void(const char)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<double>
{
    std::function<std::optional<double>(std::string_view &)> func = 
        [](std::string_view &stream) -> std::optional<double>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            size_t index = 0;
            std::vector<char> num;
            if (stream.front() == '+')
            {
                ++index;
            }
            else if (stream.front() == '-')
            {
                ++index;
                num.emplace_back('-');
            }
            bool find_point = false;
            while (index < stream.length() && (('0' <= stream[index] && stream[index] <= '9')
                || (stream[index] == '.' && !find_point)) )
            {
                num.emplace_back(stream[index]);
                if (stream[index++] == '.')
                {
                    find_point = true;
                }
            }
            if (index < stream.length() && !num.empty() && (stream[index] == 'e' || stream[index] == 'E'))
            {
                num.emplace_back(stream[index++]);
                if (index < stream.length() && stream[index] == '-')
                {
                    num.emplace_back(stream[index++]);
                }
                while (index < stream.length() && ('0' <= stream[index] && stream[index] <= '9'))
                {
                    num.emplace_back(stream[index++]);
                }
                while (!num.empty() && (num.back() < '0' || num.back() > '9'))
                {
                    num.pop_back();
                    --index;
                }
            }
            if (num.empty())
            {
                return std::nullopt;
            }
            else
            {
                stream.remove_prefix(index);
                return std::stod(std::string(num.cbegin(), num.cend()));
            }
        };

    Action<double> call;

    Parser() {}

    Parser(const std::function<std::optional<double>(std::string_view &)> &f)
        : func(f) {}

    Parser(const Parser<double> &parser)
        : func(parser.func), call(parser.call) {}

    std::optional<double> operator()(std::string_view &stream) const
    {
        const std::optional<double> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call(result.value());
        }
        return result;
    }

    Parser<double> &operator[](Action<double> &action)
    {
        call = action;
        return *this;
    }

    Parser<double> &operator[](const std::function<void(const double)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<int>
{
    std::function<std::optional<int>(std::string_view &)> func = 
        [](std::string_view &stream) -> std::optional<int>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            size_t index = 0;
            std::vector<char> num;
            if (stream.front() == '+')
            {
                ++index;
            }
            else if (stream.front() == '-')
            {
                ++index;
                num.emplace_back('-');
            }
            while (index < stream.length() && '0' <= stream[index] && stream[index] <= '9')
            {
                num.emplace_back(stream[index++]);
            }
            if (num.empty())
            {
                return std::nullopt;
            }
            else
            {
                stream.remove_prefix(index);
                return std::stoi(std::string(num.cbegin(), num.cend()));
            }
        };

    Action<int> call;

    Parser() {}

    Parser(const std::function<std::optional<int>(std::string_view &)> &f)
        : func(f) {}

    Parser(const Parser<int> &parser)
        : func(parser.func), call(parser.call) {}

    std::optional<int> operator()(std::string_view &stream) const
    {
        const std::optional<int> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call(result.value());
        }
        return result;
    }

    Parser<int> &operator[](Action<int> &action)
    {
        call = action;
        return *this;
    }

    Parser<int> &operator[](const std::function<void(const int)> &f)
    {
        call = f;
        return *this;
    }
};


inline Parser<std::string> str_p(const std::string &value)
{
    return Parser<std::string>(value);
}

inline Parser<char> ch_p(const char value)
{
    return Parser<char>(value);
}

inline Parser<char> anychar_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty())
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<char> alpha_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty() && (('a' <= stream.front() && stream.front() <= 'z') 
                || ('A' <= stream.front() && stream.front() <= 'Z')))
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<char> alphaa_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty() && 'A' <= stream.front() && stream.front() <= 'Z')
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<char> alphab_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty() && 'a' <= stream.front() && stream.front() <= 'z')
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<char> alnum_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty() && (('a' <= stream.front() && stream.front() <= 'z') 
                || ('A' <= stream.front() && stream.front() <= 'Z') 
                || ('0' <= stream.front() && stream.front() <= '9')))
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<char> eol_p()
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<char>
        {
            if (!stream.empty() && (stream.front() == 10 || stream.front() == 13))
            {
                const char ch = stream.front();
                stream.remove_prefix(1);
                return ch;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<double> float_p()
{
    return Parser<double>();
}

inline Parser<int> int_p()
{
    return Parser<int>();
}

inline Parser<int> digit_p()
{
    return Parser<int>(std::function<std::optional<int>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<int>
        {
            if (!stream.empty() && '0' <= stream.front() && stream.front() <= '9')
            {
                const int value = stream.front() - '0';
                stream.remove_prefix(1);
                return value;
            }
            else
            {
                return std::nullopt;
            }
        }));
}
