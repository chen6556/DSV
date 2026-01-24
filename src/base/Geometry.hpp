#pragma once

#include <vector>
#include <cfloat>
#include <QString>


namespace Geo
{
const static double PI = 3.14159265358979323846;
const static double EPSILON = 1e-10;

enum class Type
{
    GEOMETRY,
    POINT,
    POLYLINE,
    AABBRECT,
    POLYGON,
    TRIANGLE,
    CIRCLE,
    BEZIER,
    ELLIPSE,
    BSPLINE,
    ARC,
    TEXT,
    CONTAINERGROUP,
    COMBINATION,
    GRAPH
};

class AABBRect;

class Polygon;

class Geometry
{
public:
    bool shape_fixed = false;
    bool is_selected = false;
    unsigned long long point_index = 0;
    unsigned long long point_count = 0;
    QString name;

public:
    Geometry() = default;

    virtual ~Geometry() = default;

    virtual const Type type() const = 0;

    virtual const double length() const;

    virtual const bool empty() const = 0;

    virtual void clear() = 0;

    virtual Geo::Geometry *clone() const = 0;

    virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f) = 0;

    virtual void transform(const double mat[6]) = 0;

    virtual void translate(const double tx, const double ty) = 0;

    virtual void rotate(const double x, const double y, const double rad) = 0; // 弧度制

    virtual void scale(const double x, const double y, const double k) = 0;

    // 凸包
    virtual Polygon convex_hull() const;

    // 外接AABB矩形
    virtual AABBRect bounding_rect() const;

    // 最小外接矩形
    virtual Polygon mini_bounding_rect() const;
};

struct MarkedPoint
{
    bool original = true;
    bool active = true;
    int value = 0;
    double x = 0;
    double y = 0;

    MarkedPoint() = default;

    MarkedPoint(const double x_, const double y_, const bool original_ = true, const int value_ = 0)
        : x(x_), y(y_), value(value_), original(original_)
    {
    }

    bool operator==(const MarkedPoint &point) const
    {
        return x == point.x && y == point.y;
    }

    bool operator!=(const MarkedPoint &point) const
    {
        return x != point.x || y != point.y;
    }
};

class Point : public Geometry
{
public:
    double x = 0;
    double y = 0;

public:
    Point() = default;

    Point(const double x_, const double y_);

    Point(const Point &point) = default;

    Point(const MarkedPoint &point);

    Point &operator=(const Point &point);

    const Type type() const override;

    const bool operator==(const Point &point) const;

    const bool operator!=(const Point &point) const;

    Point &normalize();

    Point normalized() const;

    // 获取左侧的垂直向量
    Point vertical() const;

    // 向量模长
    const double length() const override;

    // 判断是否为零向量
    const bool empty() const override;

    // 变为零向量
    void clear() override;

    Point *clone() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x_, const double y_, const double rad) override;

    void scale(const double x_, const double y_, const double k) override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    Point operator*(const double k) const;

    // 向量点积
    double operator*(const Point &point) const;

    // 向量叉积
    double cross(const Point &point) const;

    Point operator+(const Point &point) const;

    Point operator-(const Point &point) const;

    Point operator/(const double k) const;

    void operator*=(const double k);

    void operator+=(const Point &point);

    void operator-=(const Point &point);

    void operator/=(const double k);
};

using Vector = Point;

class Polyline : public Geometry
{
protected:
    std::vector<Point> _points;

public:
    Polyline() = default;

    Polyline(const Polyline &polyline) = default;

    Polyline(const Polygon &polygon);

    Polyline(std::vector<Point>::const_iterator begin, const std::vector<Point>::const_iterator &end);

    Polyline(const std::initializer_list<Point> &points);

    const Type type() const override;

    const size_t size() const;

    const bool empty() const override;

    const double length() const override;

    void clear() override;

    Polyline *clone() const override;

    bool is_self_intersected() const;

    Point &operator[](const size_t index);

    const Point &operator[](const size_t index) const;

    Point &at(const size_t index);

    const Point &at(const size_t index) const;

    Polyline &operator=(const Polyline &polyline);

    Polyline operator+(const Point &point) const;

    Polyline operator-(const Point &point) const;

    void operator+=(const Point &point);

    void operator-=(const Point &point);

    virtual void append(const Point &point);

    virtual void append(const Polyline &polyline);

    virtual void append(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end);

    virtual void append(const std::vector<Point>::const_reverse_iterator &rbegin, const std::vector<Point>::const_reverse_iterator &rend);

    virtual void insert(const size_t index, const Point &point);

    virtual void insert(const size_t index, const Polyline &polyline);

    virtual void insert(const size_t index, const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end);

    virtual void insert(const size_t index, const std::vector<Point>::const_reverse_iterator &rbegin,
                        const std::vector<Point>::const_reverse_iterator &rend);

    virtual void remove(const size_t index);

    virtual void remove(const size_t index, const size_t count);

    virtual Point pop(const size_t index);

    void flip();

    Point &front();

    const Point &front() const;

    Point &back();

    const Point &back() const;

    std::vector<Point>::iterator begin();

    std::vector<Point>::const_iterator begin() const;

    std::vector<Point>::const_iterator cbegin() const;

    std::vector<Point>::iterator end();

    std::vector<Point>::const_iterator end() const;

    std::vector<Point>::const_iterator cend() const;

    std::vector<Point>::reverse_iterator rbegin();

    std::vector<Point>::const_reverse_iterator rbegin() const;

    std::vector<Point>::const_reverse_iterator crbegin() const;

    std::vector<Point>::reverse_iterator rend();

    std::vector<Point>::const_reverse_iterator rend() const;

    std::vector<Point>::const_reverse_iterator crend() const;

    std::vector<Point>::iterator find(const Point &point);

    std::vector<Point>::const_iterator find(const Point &point) const;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override;

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    void remove_repeated_points();
};

class AABBRect : public Geometry
{
private:
    std::vector<Point> _points;

public:
    AABBRect();

    AABBRect(const double x0, const double y0, const double x1, const double y1);

    AABBRect(const Point &point0, const Point &point1);

    AABBRect(const AABBRect &rect) = default;

    const Type type() const override;

    const double left() const;

    const double top() const;

    const double right() const;

    const double bottom() const;

    void set_left(const double value);

    void set_top(const double value);

    void set_right(const double value);

    void set_bottom(const double value);

    AABBRect &operator=(const AABBRect &reac);

    const bool empty() const override;

    const double length() const override;

    void clear() override;

    AABBRect *clone() const override;

    const double area() const;

    const double width() const;

    const double height() const;

    void set_width(const double value);

    void set_height(const double value);

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    std::vector<Point>::const_iterator begin() const;

    std::vector<Point>::const_iterator cbegin() const;

    std::vector<Point>::const_iterator end() const;

    std::vector<Point>::const_iterator cend() const;

    std::vector<Point>::const_reverse_iterator rbegin() const;

    std::vector<Point>::const_reverse_iterator crbegin() const;

    std::vector<Point>::const_reverse_iterator rend() const;

    std::vector<Point>::const_reverse_iterator crend() const;

    std::vector<Point>::const_iterator find(const Point &point) const;

    AABBRect operator+(const Point &point) const;

    AABBRect operator-(const Point &point) const;

    AABBRect operator+(const AABBRect &rect) const;

    void operator+=(const Point &point);

    void operator-=(const Point &point);

    void operator+=(const AABBRect &rect);

    const Point center() const;

    const Point &operator[](const size_t index) const;
};

class Polygon : public Polyline
{
public:
    Polygon() = default;

    Polygon(const Polygon &polygon) = default;

    Polygon(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end);

    Polygon(const std::initializer_list<Point> &points);

    Polygon(const Polyline &polyline);

    Polygon(const AABBRect &rect);

    Polygon(const double x, const double y, const double radius, const int n, const double rad, const bool circumscribed);

    Polygon &operator=(const Polygon &polygon);

    const Type type() const override;

    Polygon *clone() const override;

    void reorder_points(const bool cw = true);

    // 判断点顺序是否为顺时针
    bool is_cw() const;

    void append(const Point &point) override;

    void append(const Polyline &polyline) override;

    void append(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end) override;

    void append(const std::vector<Point>::const_reverse_iterator &rbegin, const std::vector<Point>::const_reverse_iterator &rend) override;

    void insert(const size_t index, const Point &point) override;

    void insert(const size_t index, const Polyline &polyline) override;

    void insert(const size_t index, const std::vector<Point>::const_iterator &begin,
                const std::vector<Point>::const_iterator &end) override;

    void insert(const size_t index, const std::vector<Point>::const_reverse_iterator &rbegin,
                const std::vector<Point>::const_reverse_iterator &rend) override;

    void remove(const size_t index) override;

    void remove(const size_t index, const size_t count) override;

    Point pop(const size_t index) override;

    Polygon operator+(const Point &point) const;

    Polygon operator-(const Point &point) const;

    const double area() const;

    size_t next_point_index(const size_t index) const;

    const Point &next_point(const size_t index) const;

    Point &next_point(const size_t index);

    size_t last_point_index(const size_t index) const;

    const Point &last_point(const size_t index) const;

    Point &last_point(const size_t index);

    size_t index(const double x, const double y) const;

    size_t index(const Point &point) const;

    Point center_of_gravity() const;

    Point average_point() const;
};

class Triangle : public Geometry
{
private:
    Point _vecs[3] = {Point()};

public:
    Triangle() = default;

    Triangle(const Point &point0, const Point &point1, const Point &point2);

    Triangle(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2);

    Triangle(const Triangle &triangle);

    const Type type() const override;

    const bool empty() const override;

    const double length() const override;

    void clear() override;

    Triangle *clone() const override;

    double area() const;

    // 顶角度数(弧度制)
    double angle(const size_t index) const;

    void reorder_points(const bool cw = true);

    // 判断点顺序是否为顺时针
    bool is_cw() const;

    Point &operator[](const size_t index);

    const Point &operator[](const size_t index) const;

    Triangle &operator=(const Triangle &triangle);

    Triangle operator+(const Point &point) const;

    Triangle operator-(const Point &point) const;

    void operator+=(const Point &point);

    void operator-=(const Point &point);

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    // 内接圆圆心
    Point inner_circle_center() const;

    // 内接圆半径
    double inner_circle_radius() const;
};

class Circle : public Point
{
public:
    double radius = 0;
    static double default_down_sampling_value;

private:
    Polygon _shape;

public:
    Circle() = default;

    Circle(const double x, const double y, const double r);

    Circle(const Point &point, const double r);

    Circle(const double x0, const double y0, const double x1, const double y1);

    Circle(const Point &point0, const Point &point1);

    Circle(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2);

    Circle(const Point &point0, const Point &point1, const Point &point2);

    Circle(const Circle &circle) = default;

    Circle &operator=(const Circle &circle);

    const Type type() const override;

    const double area() const;

    const double length() const override;

    const bool empty() const override;

    void clear() override;

    Circle *clone() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    Circle operator+(const Point &point) const;

    Circle operator-(const Point &point) const;

    void update_shape(const double down_sampling_value);

    const Polygon &shape() const;
};

class CubicBezier : public Polyline
{
private:
    Polyline _shape;

public:
    static double default_step;
    static double default_down_sampling_value;

public:
    CubicBezier();

    CubicBezier(const CubicBezier &bezier);

    CubicBezier(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
           const bool is_path_points);

    CubicBezier(const std::initializer_list<Point> &points, const bool is_path_points);

    const Type type() const override;

    const Polyline &shape() const;

    void update_control_points();

    void update_shape(const double step = 0.01, const double down_sampling_value = 0.02);

    const double length() const override;

    void clear() override;

    CubicBezier *clone() const override;

    CubicBezier &operator=(const CubicBezier &bezier);

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override;

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    Point tangent(const size_t index, const double t) const;

    Point vertical(const size_t index, const double t) const;

    Point shape_point(const size_t index, const double t) const;
};

class Ellipse : public Geometry
{
public:
    static double default_down_sampling_value;

private:
    // a[1]点绕中心点旋转角度,总是从arc_angle[0]逆时针旋转到arc_anlge[1]
    double _arc_angle[2] = {0, 0};
    // 参数方程的初值和终值
    double _arc_param[2] = {0, 0};
    Point _a[2], _b[2], _point[2];
    Polyline _shape;

public:
    Ellipse() = default;

    Ellipse(const double x, const double y, const double a, const double b);

    Ellipse(const double x, const double y, const double a, const double b, const double start, const double end, const bool is_param);

    Ellipse(const Point &point, const double a, const double b);

    Ellipse(const Point &point, const double a, const double b, const double start, const double end, const bool is_param);

    Ellipse(const Point &a0, const Point &a1, const Point &b0, const Point &b1);

    Ellipse(const Point &a0, const Point &a1, const Point &b0, const Point &b1, const double start, const double end, const bool is_param);

    Ellipse(const Ellipse &ellipse);

    Ellipse &operator=(const Ellipse &ellipse);

    // 从圆心角计算参数角度
    double angle_to_param(double angle) const;

    // 从参数角计算圆心角
    double param_to_angle(double param) const;

    void update_angle_param(const double start, const double end, const bool is_param);

    const Type type() const override;

    const double area() const;

    const double length() const override;

    const bool empty() const override;

    void clear() override;

    Ellipse *clone() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double x, const double y) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    Ellipse operator+(const Point &point) const;

    Ellipse operator-(const Point &point) const;

    double lengtha() const;

    double lengthb() const;

    double angle() const;

    Point center() const;

    void set_lengtha(const double a);

    void set_lengthb(const double b);

    void set_center(const double x, const double y);

    void reset_parameter(const Geo::Point &a0, const Geo::Point &a1, const Geo::Point &b0, const Geo::Point &b1, const double start_angle,
                         const double end_angle);

    // a0x, a0y, a1x, a1y, b0x, b0y, b1x, b1y, start_angle, end_angle
    void reset_parameter(const double parameters[10]);

    const Point &a0() const;

    const Point &a1() const;

    const Point &b0() const;

    const Point &b1() const;

    Point c0() const;

    Point c1() const;

    // a[1]点绕中心点旋转角度,总是从arc_angle[0]逆时针旋转到arc_anlge[1]
    double arc_angle0() const;

    Geo::Point arc_point0() const;

    // a[1]点绕中心点旋转角度,总是从arc_angle[0]逆时针旋转到arc_anlge[1]
    double arc_angle1() const;

    Geo::Point arc_point1() const;

    // 用于构造椭圆弧的参数初值
    double arc_param0() const;

    // 用于构造椭圆弧的参数终值
    double arc_param1() const;

    void update_shape(const double down_sampling_value);

    const Polyline &shape() const;

    bool is_arc() const;

    // 参数角切向量
    Point param_tangency(const double value) const;

    // 圆心角切向量
    Point angle_tangency(const double value) const;

    // 参数角点
    Point param_point(const double value) const;

    // 圆心角点
    Point angle_point(const double value) const;
};

class BSpline : public Geometry
{
protected:
    Polyline _shape;
    std::vector<double> _knots;
    std::vector<double> _path_values;

public:
    bool controls_model = false;
    std::vector<Point> control_points;
    std::vector<Point> path_points;

    static double default_step;
    static double default_down_sampling_value;

public:
    BSpline();

    BSpline(const BSpline &bspline);

    BSpline &operator=(const BSpline &bspline);

    virtual void update_control_points() = 0;

    virtual void update_shape(const double step, const double down_sampling_value) = 0;

    virtual void insert(const double t) = 0;

    const Polyline &shape() const;

    const double length() const override;

    const bool empty() const override;

    void clear() override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override;

    void scale(const double x, const double y, const double k) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    const Point &front() const;

    const Point &back() const;

    const std::vector<double> &knots() const;

    void set_knots(const std::vector<double>::const_iterator &begin, const std::vector<double>::const_iterator &end);

    virtual Point at(const double t) const = 0;

    virtual Point tangent(const double t) const = 0;

    virtual Point vertical(const double t) const = 0;

    virtual void extend_front(const Geo::Point &expoint) = 0;

    virtual void extend_back(const Geo::Point &expoint) = 0;

    static void rbasis(const int order, const double t, const size_t npts, const std::vector<double> &x, std::vector<double> &output);

    static void rbspline(const int order, const size_t npts, const size_t p1, const std::vector<double> &knots, const std::vector<Point> &b,
                         std::vector<Point> &p);

private:
    static void rbspline_subfunc(const int order, const size_t npts, const double step, const size_t start, const size_t end,
                         const std::vector<double> *knots, const std::vector<Point> *b, std::vector<Point> *p);
};

class QuadBSpline : public BSpline
{
public:
    QuadBSpline() = default;

    QuadBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end, const bool is_path_points);

    QuadBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                const std::vector<double> &knots, const bool is_path_points);

    QuadBSpline(const std::initializer_list<Point> &points, const bool is_path_points);

    const Type type() const override;

    void update_control_points() override;

    static bool get_three_points_control(const Point &point0, const Point &point1, const Point &point2, Point &output);

    static void get_matrix(const size_t count, const std::vector<double> &dt, std::vector<double> &output);

    static void knot(const size_t num, std::vector<double> &output);

    void update_shape(const double step, const double down_sampling_value) override;

    QuadBSpline *clone() const override;

    void insert(const double t) override;

    Point at(const double t) const override;

    Point tangent(const double t) const override;

    Point vertical(const double t) const override;

    void extend_front(const Geo::Point &expoint) override;

    void extend_back(const Geo::Point &expoint) override;
};

class CubicBSpline : public BSpline
{
public:
    CubicBSpline() = default;

    CubicBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end, const bool is_path_points);

    CubicBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                 const std::vector<double> &knots, const bool is_path_points);

    CubicBSpline(const std::initializer_list<Point> &points, const bool is_path_points);

    const Type type() const override;

    void update_control_points() override;

    void update_shape(const double step, const double down_sampling_value) override;

    CubicBSpline *clone() const override;

    void insert(const double t) override;

    Point at(const double t) const override;

    Point tangent(const double t) const override;

    Point vertical(const double t) const override;

    void extend_front(const Geo::Point &expoint) override;

    void extend_back(const Geo::Point &expoint) override;
};

class Arc : public Geometry
{
public:
    double x = 0, y = 0, radius = 0;
    Point control_points[3];

    enum class ParameterType
    {
        StartCenterAngle,
        StartEndAngle,
        StartEndRadius
    };

private:
    Polyline _shape;

public:
    Arc() = default;

    Arc(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2);

    Arc(const Point &point0, const Point &point1, const Point &point2);

    Arc(const double x0, const double y0, const double x1, const double y1, const double param, const ParameterType type,
        const bool counterclockwise);

    Arc(const Point &point0, const Point &point1, const double param, const ParameterType type, const bool counterclockwise);

    Arc(const double x, const double y, const double radius, const double start_angle, const double end_angle, const bool counterclockwise);

    Arc(const double x0, const double y0, const double x1, const double y1, const double bulge);

    Arc(const Point &point0, const Point &point1, const double bulge);

    Arc(const Arc &arc);

    Arc &operator=(const Arc &arc);

    const Type type() const override;

    const double area() const;

    const double length() const override;

    const bool empty() const override;

    void clear() override;

    Arc *clone() const override;

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void scale(const double x, const double y, const double k) override;

    void rotate(const double x, const double y, const double rad) override;

    Polygon convex_hull() const override;

    AABBRect bounding_rect() const override;

    Polygon mini_bounding_rect() const override;

    void update_shape(const double down_sampling_value);

    const Polyline &shape() const;

    bool is_cw() const;

    double angle() const;
};
}; // namespace Geo