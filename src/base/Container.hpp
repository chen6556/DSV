#pragma once

#include <QFont>
#include <QString>
#include <QPainter>
#include "base/Geometry.hpp"


class Text : public Geo::Geometry
{
private:
    QString _text;
    QFont _font;
    int _anchor_index = 3;
    double _font_size = 10;
    Geo::Point _shape[4] = {Geo::Point(0, 10), Geo::Point(10, 10), Geo::Point(10, 0), Geo::Point(0, 0)};

public:
    unsigned long long text_index = 0;
    unsigned long long text_count = 0;

public:
    Text(const double x, const double y, const QFont &font, QString text = "Text", const int anchor_index = 3);

    Text(const Text &text) = default;

    const Geo::Type type() const override;

    const double length() const override;

    const bool empty() const override;

    Text &operator=(const Text &text) = default;

    void set_text(const QString &str);

    void set_font(const QFont &font);

    const QFont &font() const;

    QFont &font();

    const QString &text() const;

    const Geo::Point &anchor() const;

    Geo::Point center() const;

    const Geo::Point &shape(const int index) const;

    const double width() const;

    const double height() const;

    const double angle() const;

    void clear() override;

    Text *clone() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void paint(QPainter &painter) const;
};

class Combination;

class ContainerGroup : public Geo::Geometry
{
private:
    std::vector<Geo::Geometry *> _containers;
    double _ratio = 1; // 缩放系数
    bool _visible = true;

public:
    ContainerGroup() = default;

    ContainerGroup(const ContainerGroup &containers);

    ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers);

    ContainerGroup(const std::vector<Geo::Geometry *>::const_iterator &begin, const std::vector<Geo::Geometry *>::const_iterator &end);

    ~ContainerGroup() override;

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

    Geo::Geometry *at(const size_t index);

    const Geo::Geometry *at(const size_t index) const;

    void clear() override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    void rescale(const double x, const double y);

    Geo::AABBRect bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    const size_t size() const;

    const size_t count(const Geo::Type type, const bool include_combinated) const;

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
    Combination() = default;

    Combination(const Combination &combination) = default;

    Combination(const std::initializer_list<Geo::Geometry *> &containers);

    Combination(const std::vector<Geo::Geometry *>::const_iterator &begin, const std::vector<Geo::Geometry *>::const_iterator &end);

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
