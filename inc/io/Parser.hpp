#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include <variant>
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
            if (stream.length() >= value.length() && stream._Starts_with(value))
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
};

template <>
struct Parser<std::vector<std::string>>
{
    std::function<std::optional<std::vector<std::string>>(std::string_view &)> func;
    Action<void> call;

    Parser(const std::function<std::optional<std::vector<std::string>>(std::string_view &)> &f)
        : func(f) {};

    std::optional<std::vector<std::string>> operator()(std::string_view &stream) const
    {
        const std::optional<std::vector<std::string>> result = this->func(stream);
        if (result.has_value() && this->call)
        {
            this->call();
        }
        return result;
    }

    Parser<std::vector<std::string>> &operator[](Action<void> &action)
    {
        call = action;
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
            while ( !stream.empty() && (('0' <= stream[index] && stream[index] <= '9')
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
            while ( !stream.empty() && '0' <= stream[index] && stream[index] <= '9')
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
};



// operator>>

template <typename L, typename R>
Parser<std::tuple<L, R>> operator>>(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::tuple<L, R>>(std::function<std::optional<std::tuple<L, R>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::tuple<L, R>>
            {
                std::string_view stream_copy(stream);
                std::optional<L> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<R> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::make_tuple(result_l.value(), result_r.value());
            }));
}

template <typename T>
Parser<std::vector<T>> operator>>(const Parser<T> &left, const Parser<T> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::string_view stream_copy(stream);
                std::optional<T> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<T> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::vector<T>({result_l.value(), result_r.value()});
            }));
}

template <typename T>
Parser<std::vector<T>> operator>>(const Parser<T> &left, const Parser<std::vector<T>> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::string_view stream_copy(stream);
                std::optional<T> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::vector<T>> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                result_r.value().insert(result_r.value().begin(), result_l.value());
                return result_r.value();
            }));
}

template <typename T>
Parser<std::vector<T>> operator>>(const Parser<std::vector<T>> &left, const Parser<T> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::string_view stream_copy(stream);
                std::optional<std::vector<T>> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<T> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                result_l.value().push_back(result_r.value());
                return result_l.value();
            }));
}

template <typename T>
Parser<std::vector<T>> operator>>(const Parser<std::vector<T>> &left, const Parser<std::vector<T>> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::string_view stream_copy(stream);
                std::optional<std::vector<T>> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::vector<T>> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                result_l.value().insert(result_l.value().end(), result_r.value().begin(), result_r.value().end());
                return result_l.value();
            }));
}

template <typename T>
Parser<T> operator>>(const Parser<T> &left, const Parser<std::string> &right)
{
    return Parser<T>(std::function<std::optional<T>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<T>
            {
                std::string_view stream_copy(stream);
                std::optional<T> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::string> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_l.value();
            }));
}

template <typename T>
Parser<T> operator>>(const Parser<T> &left, const Parser<char> &right)
{
    return Parser<T>(std::function<std::optional<T>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<T>
            {
                std::string_view stream_copy(stream);
                std::optional<T> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<char> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_l.value();
            }));
}

template <typename T>
Parser<T> operator>>(const Parser<std::string> &left, const Parser<T> &right)
{
    return Parser<T>(std::function<std::optional<T>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<T>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<T> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_r.value();
            }));
}

template <typename T>
Parser<T> operator>>(const Parser<char> &left, const Parser<T> &right)
{
    return Parser<T>(std::function<std::optional<T>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<T>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<T> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_r.value();
            }));
}

inline Parser<char> operator>>(const Parser<std::string> &left, const Parser<std::string> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::string> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}

inline Parser<char> operator>>(const Parser<std::string> &left, const Parser<char> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<char> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}

inline Parser<char> operator>>(const Parser<char> &left, const Parser<std::string> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::string> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}

inline Parser<char> operator>>(const Parser<char> &left, const Parser<char> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<char> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}

inline Parser<std::vector<char>> operator>>(const Parser<char> &left, const Parser<std::vector<char>> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::vector<char>> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                result_r.value().insert(result_r.value().begin(), result_l.value());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_r.value();
            }));
}

inline Parser<std::vector<char>> operator>>(const Parser<std::vector<char>> &left, const Parser<char> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<char>>
            {
                std::string_view stream_copy(stream);
                std::optional<std::vector<char>> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<char> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                result_l.value().push_back(result_r.value());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_l.value();
            }));
}

inline Parser<char> operator>>(const Parser<std::string> &left, const Parser<std::vector<std::string>> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::vector<std::string>> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}

inline Parser<char> operator>>(const Parser<std::vector<std::string>> &left, const Parser<std::string> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::string_view stream_copy(stream);
                std::optional<std::vector<std::string>> result_l = left(stream_copy);
                if (!result_l.has_value())
                {
                    return std::nullopt;
                }
                std::optional<std::string> result_r = right(stream_copy);
                if (!result_r.has_value())
                {
                    return std::nullopt;
                };
                stream.remove_prefix(stream.length() - stream_copy.length());
                return ' ';
            }));
}



// operator|

template <typename L, typename R>
Parser<std::variant<L, R>> operator|(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::variant<L, R>>(std::function<std::optional<std::variant<L, R>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::variant<L, R>>
            {
                std::optional<L> result_l = left(stream);
                if (result_l.has_value())
                {
                    return result_l.value();
                }
                std::optional<R> result_r = right(stream);
                if (result_r.has_value())
                {
                    return result_r.value();
                };
                return std::nullopt;
            }));
}

template <typename T>
Parser<T> operator|(const Parser<T> &left, const Parser<T> &right)
{
    return Parser<T>(std::function<std::optional<T>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<T>
            {
                std::optional<T> result_l = left(stream);
                if (result_l.has_value())
                {
                    return result_l.value();
                }
                std::optional<T> result_r = right(stream);
                if (result_r.has_value())
                {
                    return result_r.value();
                };
                return std::nullopt;
            }));
}

template <typename T>
Parser<std::vector<T>> operator|(const Parser<T> &left, const Parser<std::vector<T>> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::optional<T> result_l = left(stream);
                if (result_l.has_value())
                {
                    return std::vector<T>({result_l.value()});
                }
                std::optional<std::vector<T>> result_r = right(stream);
                if (result_r.has_value())
                {
                    return result_r.value();
                };
                return std::nullopt;
            }));
}

template <typename T>
Parser<std::vector<T>> operator|(const Parser<std::vector<T>> &left, const Parser<T> &right)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::optional<std::vector<T>> result_l = left(stream);
                if (result_l.has_value())
                {
                    return result_l.value();
                }
                std::optional<T> result_r = right(stream);
                if (result_r.has_value())
                {
                    return std::vector<T>({result_r.value()});
                };
                return std::nullopt;
            }));
}



template <typename T>
Parser<std::optional<T>> operator!(const Parser<T> &parser)
{
    return Parser<std::optional<T>>(std::function<std::optional<std::optional<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::optional<T>>
            {
                return parser(stream);
            }));
}

template <typename T>
Parser<std::vector<T>> operator*(const Parser<T> &parser)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::vector<T>>
            {
                std::optional<T> temp = parser(stream);
                std::vector<T> result;
                while (temp.has_value())
                {
                    result.emplace_back(temp.value());
                    temp = parser(stream);
                }
                return result;
            }));
}

template <typename T>
Parser<std::vector<T>> operator+(const Parser<T> &parser)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::vector<T>>
            {
                std::optional<T> temp = parser(stream);
                std::vector<T> result;
                while (temp.has_value())
                {
                    result.emplace_back(temp.value());
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
Parser<L> operator-(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<L>(std::function<std::optional<L>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<L>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }

                std::string_view stream_copy(stream);
                std::optional<R> temp = right(stream_copy);
                std::vector<char> temp_string;
                while (!temp.has_value() && !stream_copy.empty())
                {
                    temp_string.emplace_back(stream_copy.front());
                    stream_copy.remove_prefix(1);
                    temp = right(stream_copy);
                }
                if (temp_string.empty())
                {
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<L> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result.value();
                }
                else
                {
                    return std::nullopt;
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
    return Parser<char>(10) | Parser<char>(13);
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
inline Parser<std::vector<char>> pair(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::vector<char>>(std::function<std::optional<std::vector<char>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<char>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }
            std::string_view stream_copy(stream);
            if (!left(stream_copy).has_value())
            {
                return std::nullopt;
            }

            size_t pari_count = 1;
            while (pari_count > 0 && !stream_copy.empty())
            {
                if (right(stream_copy).has_value())
                {
                    --pari_count;
                }
                else if (left(stream_copy).has_value())
                {
                    ++pari_count;
                }
                else
                {
                    stream_copy.remove_prefix(1);
                }
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

template <typename T>
inline Parser<std::vector<T>> repeat(const size_t times, const Parser<T> &parser)
{
    return Parser<std::vector<T>>(std::function<std::optional<std::vector<T>>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::vector<T>>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }

            std::vector<T> result;
            std::optional<T> temp;
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
