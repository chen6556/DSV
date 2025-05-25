#pragma once

#include <QString>

#include "base/Geometry.hpp"


class Text : public Geo::AABBRect
{
private:
    QString _text;
    int _text_size;

public:
    unsigned long long text_index = 0;
    unsigned long long text_count = 0;

public:
    Text(const double x, const double y, const int size, const QString &text = "Text");

    Text(const Text &text);

    const Geo::Type type() const override;

    Text &operator=(const Text &text);

    void set_text(const QString &str, const int size);

    void update_size(const int size);

    int text_size() const;

    const QString &text() const;

    Geo::AABBRect &shape();

    const Geo::AABBRect &shape() const;

    void clear() override;

    Text *clone() const override;
};

class Containerized
{
protected:
    QString _txt;

public:
    unsigned long long text_index = 0;
    unsigned long long text_count = 0;

public:
    Containerized() {};

    Containerized(const QString &txt);

    const QString &text() const;

    void set_text(const QString &txt);

    void clear_text();
};

template<typename T>
class Container : public T, public Containerized
{
public:
    Container() {};

    Container(const T &shape)
        : T(shape) {}

    Container(const QString &txt, const T &shape)
        : T(shape), Containerized(txt) {}

    template<typename ...Ts>
    Container(const QString &txt, Ts... args)
        : T(args...), Containerized(txt) {}

    Container(const Container<T> &container)
        : T(container), Containerized(container)
    {}

    Container<T> &operator=(const Container<T> &container)
    {
        if (this != &container)
        {
            T::operator=(container);
            _txt = container._txt;
            text_index = container.text_index;
            text_count = container.text_count;
        }
        return *this;
    }

    T &shape()
    {
        return *dynamic_cast<T *>(this);
    }

    const T &shape() const
    {
        return *dynamic_cast<const T *>(this);
    }

    const Geo::Point center() const
    {
        return static_cast<const T *>(this)->bounding_rect().center();
    }

    const bool empty() const override
    {
        return T::empty() && _txt.isEmpty();
    }

    Container<T> *clone() const override
    {
        return new Container<T>(*this);
    }
};

class Combination;

class ContainerGroup : public Geo::Geometry
{
private:
    std::vector<Geo::Geometry *> _containers;
    double _ratio = 1; // 缩放系数
    bool _visible = true;

public:
    ContainerGroup() {};

    ContainerGroup(const ContainerGroup &containers);

    ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers);

    ContainerGroup(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end);

    ~ContainerGroup();

    const Geo::Type type() const override;

    const bool visible() const;

    void show();

    void hide();

    ContainerGroup *clone() const override;

    void transfer(ContainerGroup &group);

    ContainerGroup &operator=(const ContainerGroup &group);

    std::vector<Geo::Geometry *>::iterator begin();

    std::vector<Geo::Geometry *>::const_iterator begin() const;

    std::vector<Geo::Geometry *>::const_iterator cbegin() const;

    std::vector<Geo::Geometry *>::iterator end();

    std::vector<Geo::Geometry *>::const_iterator end() const;

    std::vector<Geo::Geometry *>::const_iterator cend() const;

    std::vector<Geo::Geometry *>::reverse_iterator rbegin();

    std::vector<Geo::Geometry *>::const_reverse_iterator rbegin() const;

    std::vector<Geo::Geometry *>::const_reverse_iterator crbegin() const;

    std::vector<Geo::Geometry *>::reverse_iterator rend();

    std::vector<Geo::Geometry *>::const_reverse_iterator rend() const;

    std::vector<Geo::Geometry *>::const_reverse_iterator crend() const;

    Geo::Geometry *operator[](const size_t index);

    const Geo::Geometry *operator[](const size_t index) const;

    void clear() override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    void rescale(const double x, const double y);

    Geo::AABBRect bounding_rect() const override;

    const size_t size() const;

    void append(ContainerGroup &group, const bool merge = true);

    void append(Geo::Geometry *object);

    void insert(const size_t index, Geo::Geometry *object);

    void insert(const std::vector<Geo::Geometry *>::iterator &it, Geo::Geometry *object);

    std::vector<Geo::Geometry *>::iterator remove(const size_t index);

    std::vector<Geo::Geometry *>::iterator remove(const std::vector<Geo::Geometry *>::iterator &it);

    std::vector<Geo::Geometry *>::iterator remove(const std::vector<Geo::Geometry *>::reverse_iterator &it);

    Geo::Geometry *pop(const size_t index);

    Geo::Geometry *pop(const std::vector<Geo::Geometry *>::iterator &it);

    Geo::Geometry *pop(const std::vector<Geo::Geometry *>::reverse_iterator &it);

    Geo::Geometry *pop_front();

    Geo::Geometry *pop_back();

    const bool empty() const override;

    Geo::Geometry *front();

    const Geo::Geometry *front() const;

    Geo::Geometry *back();

    const Geo::Geometry *back() const;

    void remove_front();

    void remove_back();
};

class Combination : public ContainerGroup
{
private:
    Geo::AABBRect _border;

public:
    Combination() {};

    Combination(const Combination &combination);

    Combination(const std::initializer_list<Geo::Geometry *> &containers);

    Combination(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end);

    const Geo::Type type() const override;

    void append(Combination *combination);

    void append(Geo::Geometry *geo);

    Combination *clone() const override;

    void transfer(Combination &combination);

    Combination &operator=(const Combination &combination);

    void clear() override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    void update_border();

    const Geo::AABBRect &border() const;
};
