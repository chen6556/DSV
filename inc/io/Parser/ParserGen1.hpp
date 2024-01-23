#pragma once
#include "BaseParser.hpp"


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

inline Parser<std::string> operator>>(const Parser<std::string> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
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
                return result_l.value() + result_r.value();
            }));
}

inline Parser<std::string> operator>>(const Parser<std::string> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
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
                result_l.value().push_back(result_r.value());
                return result_l;
            }));
}

inline Parser<std::string> operator>>(const Parser<char> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
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
                result_r.value().insert(result_r.value().begin(), result_l.value());
                return result_r;
            }));
}

inline Parser<std::string> operator>>(const Parser<char> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
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
                return std::string({result_l.value(), result_r.value()});
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

inline Parser<std::string> operator|(const Parser<std::string> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_l = left(stream_copy);
                if (result_l.has_value())
                {
                    stream.remove_prefix(result_l.value().length());
                    return result_l;
                }
                std::optional<char> result_r = right(stream_copy);
                if (result_r.has_value())
                {
                    stream.remove_prefix(1);
                    return std::string({result_r.value()});
                };
                return std::nullopt;
            }));
}

inline Parser<std::string> operator|(const Parser<char> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_l = left(stream_copy);
                if (result_l.has_value())
                {
                    stream.remove_prefix(1);
                    return std::string({result_l.value()});
                }
                std::optional<std::string> result_r = right(stream_copy);
                if (result_r.has_value())
                {
                    stream.remove_prefix(result_r.value().length());
                    return result_r;
                };
                return std::nullopt;
            }));
}

// operator!

template <typename T>
Parser<std::optional<T>> operator!(const Parser<T> &parser)
{
    return Parser<std::optional<T>>(std::function<std::optional<std::optional<T>>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::optional<T>>
            {
                return parser(stream);
            }));
}

// operator*

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

inline Parser<std::string> operator*(const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.emplace_back(temp.value());
                    temp = parser(stream);
                }
                return std::string(result.begin(), result.end());
            }));
}

inline Parser<std::string> operator*(const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                std::optional<std::string> temp = parser(stream);
                std::string result;
                while (temp.has_value())
                {
                    result.append(temp.value());
                    temp = parser(stream);
                }
                return result;
            }));
}

// operator+

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

inline Parser<std::string> operator+(const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
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
                    return std::string(result.begin(), result.end());
                }
            }));
}

inline Parser<std::string> operator+(const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view& stream)>
            ([=](std::string_view& stream)-> std::optional<std::string>
            {
                std::optional<std::string> temp = parser(stream);
                std::string result;
                size_t count = 0;
                while (temp.has_value())
                {
                    result.append(temp.value());
                    temp = parser(stream);
                    ++count;
                }
                if (count > 0)
                {
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

// operator~

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

// operator-

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
