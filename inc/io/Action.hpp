#pragma once
#include <string>
#include <functional>


template <typename N>
struct Action
{
    std::function<void(const N &)> func;

    Action() {};

    Action(const Action<N> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(const N &))
        : func(std::bind(f, s)) {};

    inline void operator()(const N &value) const
    {
        return func(value);
    }

    operator bool() const
    {
        return bool(func);
    }
};

template <>
struct Action<void>
{
    std::function<void(void)> func;

    Action() {};

    Action(const Action<void> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(void))
        : func(std::bind(f, s)) {};

    inline void operator()() const
    {
        return func();
    }

    operator bool() const
    {
        return bool(func);
    }
};

template <>
struct Action<std::string>
{
    std::function<void(const std::string &)> func;

    Action() {};

    Action(const Action<std::string> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(const std::string &))
        : func(std::bind(f, s, std::placeholders::_1)) {};

    inline void operator()(const std::string &value) const
    {
        return func(value);
    }

    operator bool() const
    {
        return bool(func);
    }
};

template <>
struct Action<char>
{
    std::function<void(const char)> func;

    Action() {};

    Action(const Action<char> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(const char))
        : func(std::bind(f, s, std::placeholders::_1)) {};

    inline void operator()(const char value) const
    {
        return func(value);
    }

    operator bool() const
    {
        return bool(func);
    }
};

template <>
struct Action<double>
{
    std::function<void(const double)> func;

    Action() {};

    Action(const Action<double> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(const double))
        : func(std::bind(f, s, std::placeholders::_1)) {};

    inline void operator()(const double value) const
    {
        return func(value);
    }

    operator bool() const
    {
        return bool(func);
    }
};

template <>
struct Action<int>
{
    std::function<void(const int)> func;

    Action() {};

    Action(const Action<int> &action)
        : func(action.func) {};

    template <typename T>
    Action(T *s, void (T::*f)(const int))
        : func(std::bind(f, s, std::placeholders::_1)) {};

    inline void operator()(const int value) const
    {
        return func(value);
    }

    operator bool() const
    {
        return bool(func);
    }
};