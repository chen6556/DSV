#pragma once
#include "BaseParser.hpp"


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

inline Parser<std::string> operator>>(const Parser<char> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::string({result_left.value(), result_right.value()});
            }));
}

inline Parser<std::string> operator>>(const Parser<char> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_right.value().insert(result_right.value().begin(), result_left.value());
                return result_right;
            }));
}

inline Parser<std::string> operator>>(const Parser<std::string> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_left.value().push_back(result_right.value());
                return result_left;
            }));
}

inline Parser<std::string> operator>>(const Parser<std::string> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_left.value() + result_right.value();
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

inline Parser<char> operator|(const Parser<char> &left, const Parser<char> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::optional<char> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const Parser<std::string> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result_left = left(stream);
                if (result_left.has_value())
                {
                    return result_left;
                }
                std::optional<char> result_right = right(stream);
                if (result_right.has_value())
                {
                    return std::string({result_right.value()});
                }
                return result_left;
            }));
}

inline Parser<std::string> operator|(const Parser<char> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<char> result_left = left(stream);
                if (result_left.has_value())
                {
                    return std::string({result_left.value()});
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const Parser<std::string> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

// operator!

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

inline Parser<std::string> operator!(const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
                std::optional<char> result = parser(stream);
                if (result.has_value())
                {
                    return std::string({result.value()});
                }
                else
                {
                    return std::string();
                }
            }));
}

inline Parser<std::string> operator!(const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
                std::optional<std::string> result = parser(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return std::string();
                }
            }));
}

// operator*

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

inline Parser<std::string> operator*(const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                return std::string(result.begin(), result.end());
            }));
}

inline Parser<std::string> operator*(const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
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

inline Parser<std::string> operator+(const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
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
                    return std::string(result.begin(), result.end());
                }
            }));
}

inline Parser<std::string> operator+(const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
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

template <typename R>
Parser<char> operator-(const Parser<char> &left, const Parser<R> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<char> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<std::string> operator-(const Parser<std::string> &left, const Parser<R> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<std::string> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
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

template <typename L, typename R>
inline Parser<std::string> confix_p(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::nullopt;
            }
            else
            {
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
        }));
}

template <typename A, typename B>
inline auto list(const Parser<A> &value, const Parser<B> &exp)
{
    return value >> *(exp >> value);
}

template <typename L, typename R>
Parser<std::string> pair(const Parser<L> &left, const Parser<R> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
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

inline Parser<std::string> repeat(const size_t times, const Parser<char> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::string(result.begin(), result.end());
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<std::string> repeat(const size_t times, const Parser<std::string> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }

            size_t count = 0;
            std::string result;
            std::optional<std::string> temp;
            for (size_t i = 0; i < times; ++i)
            {
                temp = parser(stream);
                if (temp.has_value())
                {
                    result.append(temp.value());
                    ++count;
                }
            }

            if (count == times)
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

inline Parser<std::string> operator>>(const Parser<char> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::string({result_left.value(), result_right.value()});
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<char>> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::string({result_left.value(), result_right.value()});
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<char>> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return std::string({result_left.value(), result_right.value()});
            }));
}

inline Parser<std::string> operator>>(const Parser<char> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_right.value().insert(result_right.value().begin(), result_left.value());
                return result_right;
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<char>> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_right.value().insert(result_right.value().begin(), result_left.value());
                return result_right;
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<char>> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<char> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_right.value().insert(result_right.value().begin(), result_left.value());
                return result_right;
            }));
}

inline Parser<std::string> operator>>(const Parser<std::string> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_left.value().push_back(result_right.value());
                return result_left;
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<std::string>> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_left.value().push_back(result_right.value());
                return result_left;
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<std::string>> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<char> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                result_left.value().push_back(result_right.value());
                return result_left;
            }));
}

inline Parser<std::string> operator>>(const Parser<std::string> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_left.value() + result_right.value();
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<std::string>> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_left.value() + result_right.value();
            }));
}

inline Parser<std::string> operator>>(const std::reference_wrapper<Parser<std::string>> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::string_view stream_copy(stream);
                std::optional<std::string> result_left = left(stream_copy);
                if (!result_left.has_value())
                {
                    return std::nullopt;
                }

                std::optional<std::string> result_right = right(stream_copy);
                if (!result_right.has_value())
                {
                    return std::nullopt;
                }

                stream.remove_prefix(stream.length() - stream_copy.length());
                return result_left.value() + result_right.value();
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

inline Parser<char> operator|(const Parser<char> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::optional<char> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<char> operator|(const std::reference_wrapper<Parser<char>> &left, const Parser<char> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::optional<char> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<char> operator|(const std::reference_wrapper<Parser<char>> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                std::optional<char> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const Parser<std::string> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result_left = left(stream);
                if (result_left.has_value())
                {
                    return result_left;
                }
                std::optional<char> result_right = right(stream);
                if (result_right.has_value())
                {
                    return std::string({result_right.value()});
                }
                return result_left;
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<std::string>> &left, const Parser<char> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result_left = left(stream);
                if (result_left.has_value())
                {
                    return result_left;
                }
                std::optional<char> result_right = right(stream);
                if (result_right.has_value())
                {
                    return std::string({result_right.value()});
                }
                return result_left;
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<std::string>> &left, const std::reference_wrapper<Parser<char>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result_left = left(stream);
                if (result_left.has_value())
                {
                    return result_left;
                }
                std::optional<char> result_right = right(stream);
                if (result_right.has_value())
                {
                    return std::string({result_right.value()});
                }
                return result_left;
            }));
}

inline Parser<std::string> operator|(const Parser<char> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<char> result_left = left(stream);
                if (result_left.has_value())
                {
                    return std::string({result_left.value()});
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<char>> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<char> result_left = left(stream);
                if (result_left.has_value())
                {
                    return std::string({result_left.value()});
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<char>> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<char> result_left = left(stream);
                if (result_left.has_value())
                {
                    return std::string({result_left.value()});
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const Parser<std::string> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<std::string>> &left, const Parser<std::string> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
                }
            }));
}

inline Parser<std::string> operator|(const std::reference_wrapper<Parser<std::string>> &left, const std::reference_wrapper<Parser<std::string>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                std::optional<std::string> result = left(stream);
                if (result.has_value())
                {
                    return result;
                }
                else
                {
                    return right(stream);
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

inline Parser<std::string> operator*(const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
                std::optional<char> temp = parser(stream);
                std::vector<char> result;
                while (temp.has_value())
                {
                    result.push_back(temp.value());
                    temp = parser(stream);
                }
                return std::string(result.begin(), result.end());
            }));
}

inline Parser<std::string> operator*(const std::reference_wrapper<Parser<std::string>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::string();
                }
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

inline Parser<std::string> operator+(const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
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
                    return std::string(result.begin(), result.end());
                }
            }));
}

inline Parser<std::string> operator+(const std::reference_wrapper<Parser<std::string>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    return std::nullopt;
                }
                std::optional<std::string> temp = parser(stream);
                std::string result;
                while (temp.has_value())
                {
                    result.append(temp.value());
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

template <typename R>
Parser<char> operator-(const Parser<char> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<char> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<char> operator-(const std::reference_wrapper<Parser<char>> &left, const Parser<R> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<char> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<char> operator-(const std::reference_wrapper<Parser<char>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<char>(std::function<std::optional<char>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<char>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<char> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<std::string> operator-(const Parser<std::string> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<std::string> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<std::string> operator-(const std::reference_wrapper<Parser<std::string>> &left, const Parser<R> &right)
{
    return Parser<char>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<std::string> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
                }
            }));
}

template <typename R>
Parser<std::string> operator-(const std::reference_wrapper<Parser<std::string>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &stream)>
            ([=](std::string_view &stream)-> std::optional<std::string>
            {
                if (stream.empty())
                {
                    std::nullopt;
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
                    return std::nullopt;
                }

                std::string_view sub_stream(&temp_string.front(), temp_string.size());
                const size_t start = sub_stream.length();
                std::optional<std::string> result = left(sub_stream);
                if (result.has_value())
                {
                    stream.remove_prefix(start - sub_stream.length());
                    return result;
                }
                else
                {
                    return std::nullopt;
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

template <typename L, typename R>
inline Parser<std::string> confix_p(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::nullopt;
            }
            else
            {
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
        }));
}

template <typename L, typename R>
inline Parser<std::string> confix_p(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::nullopt;
            }
            else
            {
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
        }));
}

template <typename L, typename R>
inline Parser<std::string> confix_p(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::nullopt;
            }
            else
            {
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
                stream.remove_prefix(stream.length() - stream_copy.length());
                return result;
            }
        }));
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
Parser<std::string> pair(const Parser<L> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
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
Parser<std::string> pair(const std::reference_wrapper<Parser<L>> &left, const Parser<R> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
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
Parser<std::string> pair(const std::reference_wrapper<Parser<L>> &left, const std::reference_wrapper<Parser<R>> &right)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                std::string result(stream.begin(), stream.begin() + stream.length() - stream_copy.length());
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

inline Parser<std::string> repeat(const size_t times, const std::reference_wrapper<Parser<char>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
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
                return std::string(result.begin(), result.end());
            }
            else
            {
                return std::nullopt;
            }
        }));
}

inline Parser<std::string> repeat(const size_t times, const std::reference_wrapper<Parser<std::string>> &parser)
{
    return Parser<std::string>(std::function<std::optional<std::string>(std::string_view &)>(
        [=](std::string_view &stream) -> std::optional<std::string>
        {
            if (stream.empty())
            {
                return std::nullopt;
            }

            size_t count = 0;
            std::string result;
            std::optional<std::string> temp;
            for (size_t i = 0; i < times; ++i)
            {
                temp = parser(stream);
                if (temp.has_value())
                {
                    result.append(temp.value());
                    ++count;
                }
            }

            if (count == times)
            {
                return result;
            }
            else
            {
                return std::nullopt;
            }
        }));
}
