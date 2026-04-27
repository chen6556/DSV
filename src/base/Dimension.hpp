#pragma once
#include <QPainter>
#include "Geometry.hpp"


namespace Dim
{

enum class Type
{
    ALIGNED,
    LINEAR,
    RADIUS,
    DIAMETER,
    ANGLE,
    ARC,
    ORDINATE
};

class Dimension : public Geo::Geometry
{
public:
    Geo::Point anchor[2], label;
    QString txt;
    double arrow_size = 2;
    int font_size = 14;

    void clear() override;

    const Geo::Type type() const override;

    virtual const Type dim_type() const = 0;

    const bool empty() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    virtual void paint(QPainter &painter) const;

    virtual void paintable_lines(std::vector<double> &data) const = 0;

    static void arrow(const Geo::Point &anchor, const Geo::Point &direction, const double size, double data[6]);

    virtual void paintable_arrows(std::vector<double> &data) const = 0;

    virtual bool select(const Geo::Point &point, const double distance) const = 0;

    virtual bool select(const Geo::AABBRect &rect) const = 0;
};


class DimAligned : public Dimension
{
private:
    double _height = 1; // 在(anchor[0], anchor[1])左侧为正

public:
    DimAligned(const Geo::Point &start, const Geo::Point &end, const double height);

    // label到(anchor[0], anchor[1])的距离
    void set_height(const double height);

    const Type dim_type() const override;

    DimAligned *clone() const override;

    // 凸包
    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void set_anchor(const int index, const double x, const double y);

    void paintable_lines(std::vector<double> &data) const override;

    void paintable_arrows(std::vector<double> &data) const override;

    bool select(const Geo::Point &point, const double distance) const override;

    bool select(const Geo::AABBRect &rect) const override;

    double height() const;
};


class DimLinear : public Dimension
{
private:
    bool _horizontal = true;
    double _distance = 1; // label到(anchor[0], anchor[1])中点的正交距离,向上和向右为正

public:
    DimLinear(const Geo::Point &start, const Geo::Point &end, const bool horizontal, const double dis);

    void set_distance(const double dis);

    const Type dim_type() const override;

    DimLinear *clone() const override;

    // 凸包
    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void set_anchor(const int index, const double x, const double y);

    void set_horizontal(const bool horizontal);

    bool is_horizontal() const;

    void paint(QPainter &painter) const override;

    void paintable_lines(std::vector<double> &data) const override;

    void paintable_arrows(std::vector<double> &data) const override;

    bool select(const Geo::Point &point, const double distance) const override;

    bool select(const Geo::AABBRect &rect) const override;

    double height() const;
};


class DimRadius : public Dimension
{
protected:
    double _distance = 1;

public:
    DimRadius(const Geo::Point &start, const Geo::Point &end, const double dis);

    void set_distance(const double dis);

    const Type dim_type() const override;

    DimRadius *clone() const override;

    // 凸包
    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void set_anchor(const int index, const double x, const double y);

    void set_label(const double x, const double y);

    void paintable_lines(std::vector<double> &data) const override;

    void paintable_arrows(std::vector<double> &data) const override;

    bool select(const Geo::Point &point, const double distance) const override;

    bool select(const Geo::AABBRect &rect) const override;

    double distance() const;
};


class DimDiameter : public DimRadius
{
public:
    DimDiameter(const Geo::Point &start, const Geo::Point &end, const double dis);

    const Type dim_type() const override;

    DimDiameter *clone() const override;

    void set_anchor(const int index, const double x, const double y);

    void paintable_lines(std::vector<double> &data) const override;
};


class DimAngle : public Dimension
{
protected:
    bool _minor_arc = true; // 默认劣弧
    double _distance = 1;
    Geo::Point _center;
    Geo::Point _root[2];
    Geo::Point _lines[4];

public:
    DimAngle(const Geo::Point &start, const Geo::Point &center, const Geo::Point &end, const double dis, const Geo::Point &root0,
             const Geo::Point &root1);

    DimAngle(const Geo::Point &line0_head, const Geo::Point &line0_tail, const Geo::Point &line1_head, const Geo::Point &line1_tail,
             const Geo::Point &arc_anchor);

    void set_distance(const double dis);

    void set_minor_arc(const bool value);

    void set_arc_pos(const Geo::Point &pos);

    const Type dim_type() const override;

    DimAngle *clone() const override;

    // 凸包
    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void paintable_lines(std::vector<double> &data) const override;

    void paintable_arrows(std::vector<double> &data) const override;

    bool select(const Geo::Point &point, const double distance) const override;

    bool select(const Geo::AABBRect &rect) const override;

    const Geo::Point &root(const int index) const;

    const Geo::Point &center() const;

    const Geo::Point &point(const int index) const;

    double distance() const;

    bool is_minor_arc() const;
};


class DimArc : public DimAngle
{
private:
    double _radius = 0;

public:
    DimArc(const Geo::Point &start, const Geo::Point &center, const Geo::Point &end, const double dis, const Geo::Point &root0,
           const Geo::Point &root1, const double radius);

    void set_minor_arc(const bool value);

    const Type dim_type() const override;

    DimArc *clone() const override;

    double radius() const;
};


class DimOrdinate : public Dimension
{
private:
    Geo::Point _root[2];

public:
    DimOrdinate(const Geo::Point &point, const Geo::Point &pos);

    const Type dim_type() const override;

    DimOrdinate *clone() const override;

    // 凸包
    Geo::Polygon convex_hull() const override;

    // 外接AABB矩形
    Geo::AABBRect bounding_rect() const override;

    // 最小外接矩形
    Geo::Polygon mini_bounding_rect() const override;

    Geo::AABBRectParams aabbrect_params() const override;

    void set_label(const double x, const double y);

    void paint(QPainter &painter) const override;

    void paintable_lines(std::vector<double> &data) const override;

    void paintable_arrows(std::vector<double> &data) const override;

    bool select(const Geo::Point &point, const double distance) const override;

    bool select(const Geo::AABBRect &rect) const override;

    const Geo::Point &root(const int index) const;
};


}; // namespace Dim