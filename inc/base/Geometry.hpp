#pragma once

#include <vector>
#include <cfloat>


namespace Geo
{
    const static double PI = 3.14159265358979323846;
    const static double EPSILON = 1e-10;

    enum class Type {GEOMETRY, POINT, POLYLINE, AABBRECT, POLYGON, TRIANGLE, CIRCLE, LINE, BEZIER,
        TEXT, CONTAINER, CIRCLECONTAINER, CONTAINERGROUP, COMBINATION, GRAPH};

    class AABBRect;

    class Polygon;

    class Geometry
    {
    public:
        bool shape_fixed = false;
        bool is_selected = false;
        unsigned long long point_index = 0;
        unsigned long long point_count = 0;

    public:
        virtual ~Geometry();

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

        MarkedPoint() {}

        MarkedPoint(const double x_, const double y_, const bool original_ = true, const int value_ = 0)
            : x(x_), y(y_), value(value_), original(original_) {}

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
        Point() {};

        Point(const double x_, const double y_);

        Point(const Point &point);

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
        Polyline() {};

        Polyline(const Polyline &polyline);

        Polyline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

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

        Polyline &operator=(const Polyline &polyline);

        Polyline operator+(const Point &point) const;

        Polyline operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        virtual void append(const Point &point);

        virtual void append(const Polyline &polyline);

        virtual void append(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        virtual void insert(const size_t index, const Point &point);

        virtual void insert(const size_t index, const Polyline &polyline);

        virtual void insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

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
    };

    class AABBRect : public Geometry
    {
    private:
        std::vector<Point> _points;

    public:
        AABBRect();

        AABBRect(const double x0, const double y0, const double x1, const double y1);

        AABBRect(const Point &point0, const Point &point1);

        AABBRect(const AABBRect &rect);

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

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        const Point center() const;

        const Point &operator[](const size_t index) const;
    };

    class Polygon : public Polyline
    {
    public:
        Polygon() {};

        Polygon(const Polygon &polygon);

        Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        Polygon(const std::initializer_list<Point> &points);

        Polygon(const Polyline &polyline);

        Polygon(const AABBRect &rect);

        Polygon &operator=(const Polygon &polygon);

        const Type type() const override;

        Polygon *clone() const override;

        void reorder_points(const bool cw = true);

        // 判断点顺序是否为顺时针
        bool is_cw() const;

        void append(const Point &point) override;

        void append(const Polyline &polyline) override;

        void append(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end) override;

        void insert(const size_t index, const Point &point) override;

        void insert(const size_t index, const Polyline &polyline) override;

        void insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end) override;

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
        Triangle() {};

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

    public:
        Circle() {};

        Circle(const double x, const double y, const double r);

        Circle(const Point &point, const double r);

        Circle(const Circle &circle);

        Circle &operator=(const Circle &circle);

        const Type type() const override;

        const double area() const;

        const double length() const override;

        const bool empty() const override;

        void clear() override;

        Circle *clone() const override;

        void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

        void transform(const double mat[6]) override;

        void scale(const double x, const double y, const double k) override;

        AABBRect bounding_rect() const override;

        Polygon mini_bounding_rect() const override;

        Circle operator+(const Point &point) const;

        Circle operator-(const Point &point) const;
    };

    class Line : public Geometry
    {
    private:
        Point _start_point;
        Point _end_point;

    public:
        Line() {};

        Line(const double x0, const double y0, const double x1, const double y1);

        Line(const Point &start, const Point &end);

        Line(const Line &line);

        const Type type() const override;

        Line &operator=(const Line &line);

        Line operator+(const Point &point);

        Line operator-(const Point &point);

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        const double length() const override;

        const bool empty() const override;

        void clear() override;

        Line *clone() const override;

        void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

        void transform(const double mat[6]) override;

        void translate(const double tx, const double ty) override;

        void rotate(const double x, const double y, const double rad) override; // 弧度制

        void scale(const double x, const double y, const double k) override;

        AABBRect bounding_rect() const override;

        Polygon mini_bounding_rect() const override;

        Point &front();

        const Point &front() const;

        Point &back();

        const Point &back() const;
    };

    class Bezier : public Polyline
    {
    private:
        size_t _order = 2;
        Polyline _shape;

    public:
        Bezier(const size_t n);

        Bezier(const Bezier &bezier);

        Bezier(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const size_t n);

        Bezier(const std::initializer_list<Point> &points, const size_t n);

        const Type type() const override;

        // 贝塞尔曲线阶数
        size_t order() const;

        const Polyline &shape() const;

        void update_shape(const double step = 0.01);

        void append_shape(const double step = 0.01);

        const double length() const override;

        void clear() override;

        Bezier *clone() const override;

        Bezier &operator=(const Bezier &bezier);

        void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

        void transform(const double mat[6]) override;

        void translate(const double tx, const double ty) override;

        void rotate(const double x, const double y, const double rad) override;

        void scale(const double x, const double y, const double k) override;

        Polygon convex_hull() const override;

        AABBRect bounding_rect() const override;

        Polygon mini_bounding_rect() const override;
    };

};