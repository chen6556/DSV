#pragma once

#include "base/Geometry.hpp"
#include <QString>


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

class Container : public Geo::Polygon
{
private:
    QString _txt;

public:
    unsigned long long text_index = 0;
    unsigned long long text_count = 0;

public:
    Container() { _type = Geo::Type::CONTAINER; };

    Container(const Geo::Polygon &shape);

    Container(const QString &txt, const Geo::Polygon &shape);

    Container(const Container &container);

    Container &operator=(const Container &container);

    Geo::Polygon &shape();

    const Geo::Polygon &shape() const;

    const QString &text() const;

    void set_text(const QString &txt);

    void clear_text();

    const Geo::Point center() const;

    const bool empty() const override;

    Container *clone() const override;
};

class CircleContainer : public Geo::Circle
{
private:
    QString _txt;

public:
    unsigned long long text_index = 0;
    unsigned long long text_count = 0;

public:
    CircleContainer() { _type = Geo::Type::CIRCLECONTAINER; };

    CircleContainer(const Geo::Circle &shape);

    CircleContainer(const QString &txt, const Geo::Circle &shape);

    CircleContainer(const QString &txt, const double x, const double y, const double r);

    CircleContainer(const CircleContainer &container);

    CircleContainer &operator=(const CircleContainer &container);

    Geo::Circle &shape();

    const Geo::Circle &shape() const;

    const QString &text() const;

    void set_text(const QString &txt);

    void clear_text();

    const bool empty() const override;

    CircleContainer *clone() const override;
};

class Combination;

class ContainerGroup : public Geo::Geometry
{
private:
    std::vector<Geo::Geometry *> _containers;
    double _ratio = 1; // 缩放系数
    bool _visible = true;

public:
    QString name;

public:
    ContainerGroup() { _type = Geo::Type::CONTAINERGROUP; };

    ContainerGroup(const ContainerGroup &containers);

    ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers);

    ContainerGroup(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end);

    ~ContainerGroup();

    const bool &visible() const;

    void show();

    void hide();

    ContainerGroup *clone() const;

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

    void append(Container *container);

    void append(CircleContainer *container);

    void append(Geo::Polyline *container);

    void append(Geo::Bezier *bezier);

    void append(Combination *combination);

    void append(ContainerGroup &group, const bool merge = true);

    void append(Text *text);

    void insert(const size_t index, Container *container);

    void insert(const size_t index, CircleContainer *container);

    void insert(const size_t index, Combination *combination);

    void insert(const size_t index, Geo::Polyline *polyline);

    void insert(const size_t index, Geo::Bezier *bezier);

    void insert(const size_t index, Text *text);

    std::vector<Geo::Geometry *>::iterator remove(const size_t index);

    std::vector<Geo::Geometry *>::iterator remove(const std::vector<Geo::Geometry *>::iterator &it);

    std::vector<Geo::Geometry *>::iterator remove(const std::vector<Geo::Geometry *>::reverse_iterator &it);

    Geo::Geometry *pop(const size_t index);

    Geo::Geometry *pop(const std::vector<Geo::Geometry *>::iterator &it);

    Geo::Geometry *pop(const std::vector<Geo::Geometry *>::reverse_iterator &it);

    Geo::Geometry *pop_front();

    Geo::Geometry *pop_back();

    const bool empty() const;

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
    Combination() { _type = Geo::Type::COMBINATION; };

    Combination(const Combination &combination);

    void append(Combination *combination);

    void append(Geo::Geometry *geo);

    Combination *clone() const;

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
