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
        : func(f) {};

    Parser(const Parser<T> &parser)
        : func(parser.func), call(parser.call) {};
    
    inline std::optional<T> operator()(std::string_view &stream) const
    {
        std::optional<T> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call();
        }
        return result;
    };

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
    Action<void> call;

    Parser(const std::function<std::optional<std::string>(std::string_view &)> &f)
        : func(f) {};

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
        }) {};

    Parser(const Parser<std::string> &parser)
        : func(parser.func), call(parser.call) {};

    std::optional<std::string> operator()(std::string_view &stream) const
    {
        const std::optional<std::string> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call();
        }
        return result;
    }

    Parser<std::string> &operator[](Action<void> &action)
    {
        call = action;
        return *this;
    }

    Parser<std::string> &operator[](const std::function<void(void)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<char>
{
    std::function<std::optional<char>(std::string_view &)> func;
    Action<void> call;

    Parser(const std::function<std::optional<char>(std::string_view &)> &f)
        : func(f) {};

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
        ) {};

    Parser(const Parser<char> &parser)
        : func(parser.func), call(parser.call) {};

    std::optional<char> operator()(std::string_view &stream) const
    {
        const std::optional<char> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call();
        }
        return result;
    }

    Parser<char> &operator[](Action<void> &action)
    {
        call = action;
        return *this;
    }

    Parser<char> &operator[](const std::function<void(void)> &f)
    {
        call = f;
        return *this;
    }
};

template <>
struct Parser<std::vector<char>>
{
    std::function<std::optional<std::vector<char>>(std::string_view &)> func;
    Action<std::string> call;
    Action<void> void_call;

    Parser(const std::function<std::optional<std::vector<char>>(std::string_view &)> &f)
        : func(f) {};

    Parser(const Parser<std::vector<char>> &parser)
        : func(parser.func), call(parser.call), void_call(parser.void_call) {};

    std::optional<std::vector<char>> operator()(std::string_view &stream) const
    {
        const std::optional<std::vector<char>> result = this->func(stream);
        if (result.has_value())
        {
            if (this->call)
            {
                this->call(std::string(result.value().begin(), result.value().end()));
            }
            else if (this->void_call)
            {
                this->void_call();
            }
        }
        return result;
    }

    Parser<std::vector<char>> &operator[](Action<std::string> &action)
    {
        call = action;
        return *this;
    }

    Parser<std::vector<char>> &operator[](Action<void> &action)
    {
        void_call = action;
        return *this;
    }

    Parser<std::vector<char>> &operator[](const std::function<void(const std::string &)> &f)
    {
        call = f;
        return *this;
    }

    Parser<std::vector<char>> &operator[](const std::function<void(void)> &f)
    {
        void_call = f;
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

    Parser() {};

    Parser(const std::function<std::optional<double>(std::string_view &)> &f)
        : func(f) {};

    Parser(const Parser<double> &parser)
        : func(parser.func), call(parser.call) {};

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

    Parser() {};

    Parser(const std::function<std::optional<int>(std::string_view &)> &f)
        : func(f) {};

    Parser(const Parser<int> &parser)
        : func(parser.func), call(parser.call) {};

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



// operator>>

template <typename L, typename R>
Parser<bool> operator>>(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                std::string_view stream_copy(stream);
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (!left(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!left(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (!right(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!right(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                stream.remove_prefix(stream.length() - stream_copy.length());
                return true;
            }));
}

// operator|

template <typename L, typename R>
Parser<bool> operator|(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if constexpr(std::is_same<L, bool>::value && std::is_same<R, bool>::value)
                {
                    return left(stream) || right(stream);
                }
                else if constexpr(std::is_same<L, bool>::value)
                {
                    return left(stream) || right(stream).has_value();
                }
                else if constexpr(std::is_same<R, bool>::value)
                {
                    return left(stream).has_value() || right(stream);
                }
                else
                {
                    return left(stream).has_value() || right(stream).has_value();
                }
            }));
}

template <typename T>
Parser<bool> operator!(const Parser<T> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return true;
                }
                parser(stream);
                return true;
            }));
}

template <typename T>
Parser<bool> operator*(const Parser<T> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return true;
                }
                if constexpr(std::is_same<T, bool>::value)
                {
                    while (parser(stream));
                }
                else
                {
                    while (parser(stream).has_value());
                }
                return true;
            }));
}

inline Parser<std::vector<char>> operator*(const Parser<char> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                if (stream.empty())
                {
                    return std::vector<char>();
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                return result;
            }));
}

template <typename T>
Parser<bool> operator+(const Parser<T> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return false;
                }
                if constexpr(std::is_same<T, bool>::value)
                {
                    if (parser(stream))
                    {
                        while (parser(stream));
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (parser(stream).has_value())
                    {
                        while (parser(stream).has_value());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}

inline Parser<std::vector<char>> operator+(const Parser<char> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                if (result.empty())
                {
                    return std::nullopt;
                }
                else
                {
                    return result;
                }
            }));
}

template <typename T>
Parser<std::string> operator~(const Parser<T> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
                std::optional<T> temp = parser(stream);
                std::vector<char> result;
                while (!temp.has_value() && !stream.empty())
                {
                    result.emplace_back(stream.front());
                    stream.remove_prefix(1);
                    temp = parser(stream);
                }
                if (result.empty())
                {
                    return std::nullopt;
                }
                else
                {
                    return std::string(result.cbegin(), result.cend());
                }
            }));
}

template <typename L, typename R>
Parser<bool> operator-(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    false;
                }

                std::string_view stream_copy(stream);
                std::vector<char> temp_string;
                if constexpr(std::is_same<R, bool>::value)
                {
                    bool temp = right(stream_copy);
                    while (!temp && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                else
                {
                    std::optional<R> temp = right(stream_copy);
                    while (!temp.has_value() && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                if (temp_string.empty())
                {
                    return false;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(sub_stream))
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (left(sub_stream).has_value())
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}



// functions

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
            if (!stream.empty() && 'a' <= stream.front() && stream.front() <= 'b')
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

template <typename A, typename B, typename C>
inline auto confix_p(const Parser<A> &left, const Parser<B> &exp, const Parser<C> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B>
inline auto list(const Parser<A> &value, const Parser<B> &exp)
{
    return value >> *(exp >> value);
}

template <typename L, typename R>
Parser<std::vector<char>> pair(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<L, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return std::nullopt;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return std::nullopt;
                }
            }

            size_t pari_count = 1;
            while (pari_count > 0 && !stream_copy.empty())
            {
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        continue;
                    }
                }
                
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::vector<char> result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const Parser<A> &left, const Parser<B> &exp, const Parser<C> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename T>
Parser<bool> repeat(const size_t times, const Parser<T> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }

            size_t count = 0;
            if constexpr(std::is_same<T, bool>::value)
            {
                for (size_t i = 0; i < times; ++i)
                {
                    if (parser(stream))
                    {
                        ++count;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < times; ++i)
                {
                    if (parser(stream).has_value())
                    {
                        ++count;
                    }
                }
            }
            return count == times;
        }));
}

inline Parser<std::vector<char>> repeat(const size_t times, const Parser<char> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }

            std::vector<char> result;
            std::optional<char> temp;
            for (size_t i = 0; i < times; ++i)
            {
                temp = parser(stream);
                if (temp.has_value())
                {
                    result.emplace_back(temp.value());
                }
            }

            if (result.size() == times)
            {
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}



// ref operators and functions
// ref operator>>

template <typename L, typename R>
Parser<bool> operator>>(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                std::string_view stream_copy(stream);
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (!left(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!left(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (!right(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!right(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                stream.remove_prefix(stream.length() - stream_copy.length());
                return true;
            }));
}

template <typename L, typename R>
Parser<bool> operator>>(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                std::string_view stream_copy(stream);
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (!left(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!left(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (!right(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!right(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                stream.remove_prefix(stream.length() - stream_copy.length());
                return true;
            }));
}

template <typename L, typename R>
Parser<bool> operator>>(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                std::string_view stream_copy(stream);
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (!left(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!left(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (!right(stream_copy))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!right(stream_copy).has_value())
                    {
                        return false;
                    }
                }
                
                stream.remove_prefix(stream.length() - stream_copy.length());
                return true;
            }));
}

// ref operator|

template <typename L, typename R>
Parser<bool> operator|(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if constexpr(std::is_same<L, bool>::value && std::is_same<R, bool>::value)
                {
                    return left(stream) || right(stream);
                }
                else if constexpr(std::is_same<L, bool>::value)
                {
                    return left(stream) || right(stream).has_value();
                }
                else if constexpr(std::is_same<R, bool>::value)
                {
                    return left(stream).has_value() || right(stream);
                }
                else
                {
                    return left(stream).has_value() || right(stream).has_value();
                }
            }));
}

template <typename L, typename R>
Parser<bool> operator|(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if constexpr(std::is_same<L, bool>::value && std::is_same<R, bool>::value)
                {
                    return left(stream) || right(stream);
                }
                else if constexpr(std::is_same<L, bool>::value)
                {
                    return left(stream) || right(stream).has_value();
                }
                else if constexpr(std::is_same<R, bool>::value)
                {
                    return left(stream).has_value() || right(stream);
                }
                else
                {
                    return left(stream).has_value() || right(stream).has_value();
                }
            }));
}

template <typename L, typename R>
Parser<bool> operator|(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if constexpr(std::is_same<L, bool>::value && std::is_same<R, bool>::value)
                {
                    return left(stream) || right(stream);
                }
                else if constexpr(std::is_same<L, bool>::value)
                {
                    return left(stream) || right(stream).has_value();
                }
                else if constexpr(std::is_same<R, bool>::value)
                {
                    return left(stream).has_value() || right(stream);
                }
                else
                {
                    return left(stream).has_value() || right(stream).has_value();
                }
            }));
}

// ref operator!

template <typename T>
Parser<bool> operator!(const std::reference_wrapper<Parser<T>> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return true;
                }
                parser(stream);
                return true;
            }));
}

// ref operator*

template <typename T>
Parser<bool> operator*(const std::reference_wrapper<Parser<T>> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return true;
                }
                if constexpr(std::is_same<T, bool>::value)
                {
                    while (parser(stream));
                }
                else
                {
                    while (parser(stream).has_value());
                }
                return true;
            }));
}

inline Parser<std::vector<char>> operator*(const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                if (stream.empty())
                {
                    return std::vector<char>();
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                return result;
            }));
}

// ref operator+

template <typename T>
Parser<bool> operator+(const std::reference_wrapper<Parser<T>> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    return false;
                }
                if constexpr(std::is_same<T, bool>::value)
                {
                    if (parser(stream))
                    {
                        while (parser(stream));
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (parser(stream).has_value())
                    {
                        while (parser(stream).has_value());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}

inline Parser<std::vector<char>> operator+(const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                if (result.empty())
                {
                    return std::nullopt;
                }
                else
                {
                    return result;
                }
            }));
}

// ref operator~

template <typename T>
Parser<std::string> operator~(const std::reference_wrapper<Parser<T>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
                std::optional<T> temp = parser(stream);
                std::vector<char> result;
                while (!temp.has_value() && !stream.empty())
                {
                    result.emplace_back(stream.front());
                    stream.remove_prefix(1);
                    temp = parser(stream);
                }
                if (result.empty())
                {
                    return std::nullopt;
                }
                else
                {
                    return std::string(result.cbegin(), result.cend());
                }
            }));
}

// ref operator-

template <typename L, typename R>
Parser<bool> operator-(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    false;
                }

                std::string_view stream_copy(stream);
                std::vector<char> temp_string;
                if constexpr(std::is_same<R, bool>::value)
                {
                    bool temp = right(stream_copy);
                    while (!temp && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                else
                {
                    std::optional<R> temp = right(stream_copy);
                    while (!temp.has_value() && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                if (temp_string.empty())
                {
                    return false;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(sub_stream))
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (left(sub_stream).has_value())
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}

template <typename L, typename R>
Parser<bool> operator-(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    false;
                }

                std::string_view stream_copy(stream);
                std::vector<char> temp_string;
                if constexpr(std::is_same<R, bool>::value)
                {
                    bool temp = right(stream_copy);
                    while (!temp && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                else
                {
                    std::optional<R> temp = right(stream_copy);
                    while (!temp.has_value() && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                if (temp_string.empty())
                {
                    return false;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(sub_stream))
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (left(sub_stream).has_value())
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}

template <typename L, typename R>
Parser<bool> operator-(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &stream)>
            ([=](std::string_view &stream)-> bool
            {
                if (stream.empty())
                {
                    false;
                }

                std::string_view stream_copy(stream);
                std::vector<char> temp_string;
                if constexpr(std::is_same<R, bool>::value)
                {
                    bool temp = right(stream_copy);
                    while (!temp && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                else
                {
                    std::optional<R> temp = right(stream_copy);
                    while (!temp.has_value() && !stream_copy.empty())
                    {
                        temp_string.emplace_back(stream_copy.front());
                        stream_copy.remove_prefix(1);
                        temp = right(stream_copy);
                    }
                }
                if (temp_string.empty())
                {
                    return false;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(sub_stream))
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    if (left(sub_stream).has_value())
                    {
                        stream.remove_prefix(start - sub_stream.length());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }));
}

// ref confix_p

template <typename A, typename B, typename C>
inline auto confix_p(const Parser<A> &left, const Parser<B> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const Parser<A> &left, const std::reference_wrapper<Parser<B>> &exp, const Parser<C> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const std::reference_wrapper<Parser<A>> &left, const Parser<B> &exp, const Parser<C> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const Parser<A> &left, const std::reference_wrapper<Parser<B>> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const std::reference_wrapper<Parser<A>> &left, const Parser<B> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const std::reference_wrapper<Parser<A>> &left, const std::reference_wrapper<Parser<B>> &exp, const Parser<C> &right)
{
    return left >> (exp - right) >> right;
}

template <typename A, typename B, typename C>
inline auto confix_p(const std::reference_wrapper<Parser<A>> &left, const std::reference_wrapper<Parser<B>> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return left >> (exp - right) >> right;
}

// ref list

template <typename A, typename B>
inline auto list(const Parser<A> &value, const std::reference_wrapper<Parser<B>> &exp)
{
    return value >> *(exp >> value);
}

template <typename A, typename B>
inline auto list(const std::reference_wrapper<Parser<A>> &value, const Parser<B> &exp)
{
    return value >> *(exp >> value);
}

template <typename A, typename B>
inline auto list(const std::reference_wrapper<Parser<A>> &value, const std::reference_wrapper<Parser<B>> &exp)
{
    return value >> *(exp >> value);
}

// ref pair

template <typename L, typename R>
Parser<std::vector<char>> pair(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<L, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return std::nullopt;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return std::nullopt;
                }
            }

            size_t pari_count = 1;
            while (pari_count > 0 && !stream_copy.empty())
            {
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        continue;
                    }
                }
                
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::vector<char> result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

template <typename L, typename R>
Parser<std::vector<char>> pair(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<L, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return std::nullopt;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return std::nullopt;
                }
            }

            size_t pari_count = 1;
            while (pari_count > 0 && !stream_copy.empty())
            {
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        continue;
                    }
                }
                
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::vector<char> result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

template <typename L, typename R>
Parser<std::vector<char>> pair(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<L, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return std::nullopt;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return std::nullopt;
                }
            }

            size_t pari_count = 1;
            while (pari_count > 0 && !stream_copy.empty())
            {
                if constexpr(std::is_same<R, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        continue;
                    }
                }
                
                if constexpr(std::is_same<L, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::vector<char> result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const Parser<A> &left, const Parser<B> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const Parser<A> &left, const std::reference_wrapper<Parser<B>> &exp, const Parser<C> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const std::reference_wrapper<Parser<A>> &left, const Parser<B> &exp, const Parser<C> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const Parser<A> &left, const std::reference_wrapper<Parser<B>> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const std::reference_wrapper<Parser<A>> &left, const Parser<B> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const std::reference_wrapper<Parser<A>> &left, const std::reference_wrapper<Parser<B>> &exp, const Parser<C> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

template <typename A, typename B, typename C>
Parser<bool> pair(const std::reference_wrapper<Parser<A>> &left, const std::reference_wrapper<Parser<B>> &exp, const std::reference_wrapper<Parser<C>> &right)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }
            
            std::string_view stream_copy(stream);
            if constexpr(std::is_same<A, bool>::value)
            {
                if (!left(stream_copy))
                {
                    return false;
                }
            }
            else
            {
                if (!left(stream_copy).has_value())
                {
                    return false;
                }
            }

            size_t pari_count = 1, left_length = stream.length() - stream_copy.length();
            size_t temp = 0, right_length = 0;
            while (pari_count > 0 && !stream_copy.empty())
            {
                temp = stream_copy.length();
                if constexpr(std::is_same<C, bool>::value)
                {
                    if (right(stream_copy))
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                else
                {
                    if (right(stream_copy).has_value())
                    {
                        --pari_count;
                        right_length = temp - stream_copy.length();
                        continue;
                    }
                }
                
                if constexpr(std::is_same<A, bool>::value)
                {
                    if (left(stream_copy))
                    {
                        ++pari_count;
                        continue;
                    }
                }
                else
                {
                    if (left(stream_copy).has_value())
                    {
                        ++pari_count;
                        continue;
                    }
                }

                stream_copy.remove_prefix(1);
            }
            if (pari_count == 0)
            {
                std::string_view stream_copy2 = stream.substr(0, stream.length() - stream_copy.length());
                stream_copy2.remove_prefix(left_length);
                stream_copy2.remove_suffix(right_length);
                exp(stream_copy2);
                if (stream_copy2.empty())
                {
                    stream.remove_prefix(stream.length() - stream_copy.length());
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }));
}

// ref repeat

template <typename T>
Parser<bool> repeat(const size_t times, const std::reference_wrapper<Parser<T>> &parser)
{
    return Parser<bool>(std::function<bool(std::string_view &)>(
        [=](std::string_view &stream) -> bool
        {
            if (stream.empty())
            {
                return false;
            }

            size_t count = 0;
            if constexpr(std::is_same<T, bool>::value)
            {
                for (size_t i = 0; i < times; ++i)
                {
                    if (parser(stream))
                    {
                        ++count;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < times; ++i)
                {
                    if (parser(stream).has_value())
                    {
                        ++count;
                    }
                }
            }
            return count == times;
        }));
}

inline Parser<std::vector<char>> repeat(const size_t times, const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }

            std::vector<char> result;
            std::optional<char> temp;
            for (size_t i = 0; i < times; ++i)
            {
                temp = parser(stream);
                if (temp.has_value())
                {
                    result.emplace_back(temp.value());
                }
            }

            if (result.size() == times)
            {
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}
