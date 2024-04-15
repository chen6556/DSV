#pragma once
#include <vector>


namespace Geo
{
    const static double PI = 3.14159265358979323846;
    const static double EPSILON = 1e-10;

    enum Type {GEOMETRY, POINT, POLYLINE, AABBRECT, POLYGON, TRIANGLE, CIRCLE, LINE, BEZIER,
        TEXT, CONTAINER, CIRCLECONTAINER, CONTAINERGROUP, COMBINATION, GRAPH};

    class AABBRect;

    class Polygon;

    class Geometry
    {
    protected:
        Type _type = Type::GEOMETRY;

    public:
        bool shape_fixed = false;
        bool is_selected = false;
        unsigned long long point_index = 0;
        unsigned long long point_count = 0;

    public:
        Geometry() {};

        Geometry(const Geometry &geo);

        virtual ~Geometry() {};

        Geometry &operator=(const Geometry &geo);

        const Type type() const;

        virtual const double length() const;

        virtual const bool empty() const;

        virtual void clear();

        virtual Geo::Geometry *clone() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad); // 弧度制

        virtual void scale(const double x, const double y, const double k);

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
        Point() { _type = Type::POINT; };

        Point(const double x_, const double y_);

        Point(const Point &point);

        Point(const MarkedPoint &point);

        Point &operator=(const Point &point);

        const bool operator==(const Point &point) const;

        const bool operator!=(const Point &point) const;

        const Point &normalize();

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
        Polyline() { _type = Type::POLYLINE; };

        Polyline(const Polyline &polyline);

        Polyline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        Polyline(const std::initializer_list<Point> &points);

        const size_t size() const;

        const bool empty() const override;

        const double length() const override;

        void clear() override;

        Polyline *clone() const override;

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
        Polygon() { _type = Type::POLYGON; };

        Polygon(const Polygon &polygon);

        Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        Polygon(const std::initializer_list<Point> &points);

        Polygon(const Polyline &polyline);

        Polygon(const AABBRect &rect);

        Polygon &operator=(const Polygon &polygon);

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

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        const double area() const;

        size_t next_point_index(const size_t index) const;

        const Point &next_point(const size_t index) const;

        Point &next_point(const size_t index);

        size_t last_point_index(const size_t index) const;

        const Point &last_point(const size_t index) const;

        Point &last_point(const size_t index);

        size_t index(const double x, const double y) const;
    };

    class Triangle : public Geometry
    {
    private:
        Point _vecs[3] = {Point()};
    
    public:
        Triangle() { _type = Type::TRIANGLE; };

        Triangle(const Point &point0, const Point &point1, const Point &point2);

        Triangle(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2);

        Triangle(const Triangle &triangle);

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

    class Circle : public Geometry
    {
    private:
        Point _center;
        double _radius = 0;

    public:
        Circle() { _type = Type::CIRCLE; };

        Circle(const double x, const double y, const double r);

        Circle(const Point &point, const double r);

        Circle(const Circle &circle);

        Circle &operator=(const Circle &circle);

        Point &center();

        const Point &center() const;

        double &radius();

        const double radius() const;

        const double area() const;

        const double length() const override;

        const bool empty() const override;

        void clear() override;

        Circle *clone() const override;

        void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

        void transform(const double mat[6]) override;

        void translate(const double tx, const double ty) override;

        void rotate(const double x, const double y, const double rad) override; // 弧度制

        void scale(const double x, const double y, const double k) override;

        AABBRect bounding_rect() const override;

        Polygon mini_bounding_rect() const override;

        Circle operator+(const Point &point) const;

        Circle operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);
    };

    class Line : public Geometry
    {
    private:
        Point _start_point;
        Point _end_point;

    public:
        Line() { _type = Type::LINE; };

        Line(const double x0, const double y0, const double x1, const double y1);

        Line(const Point &start, const Point &end);

        Line(const Line &line);

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

        // 贝塞尔曲线阶数
        const size_t &order() const;

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

    // functions

    // 两点间距离
    const double distance(const double x0, const double y0, const double x1, const double y1);

    // 两点间距离
    const double distance(const Point &point0, const Point &point1);

    // 点到直线距离,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离
    const double distance(const Point &point, const Line &line, const bool infinite = false);

    // 点到直线距离,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离
    const double distance(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    // 点到多段线距离,计算点到每一段有限长线段的距离,取最近距离
    const double distance(const Point &point, const Polyline &polyline);

    // 点到多边形距离,计算点到每一段有限长线段的距离,取最近距离
    const double distance(const Point &point, const Polygon &polygon);

    // 判断点是否在有限长线段或直线上
    const bool is_inside(const Point &point, const Line &line, const bool infinite = false);

    // 判断点是否在有限长线段或直线上
    const bool is_inside(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    // 判断点是否在多段线上
    const bool is_inside(const Point &point, const Polyline &polyline);

    // 判断点是否在多边形内,coincide决定是否包含点在多边形上的情况
    const bool is_inside(const Point &point, const Polygon &polygon, const bool coincide = false);

    // 判断点是否在AABB矩形内,coincide决定是否包含点在AABB矩形上的情况
    const bool is_inside(const Point &point, const AABBRect &rect, const bool coincide = false);

    // 判断点是否在圆内,coincide决定是否包含点在圆上的情况
    const bool is_inside(const Point &point, const Circle &circle, const bool coincide = false);

    // 判断点是否在三角形内,coincide决定是否包含点在三角形上的情况
    const bool is_inside(const Point &point, const Point &point0, const Point &point1, const Point &point2, const bool coincide = false);

    // 判断点是否在三角形内,coincide决定是否包含点在三角形上的情况
    const bool is_inside(const Point &point, const Triangle &triangle, const bool coincide = false);

    // 判断有限长线段是否完全在三角形内,线段与三角形相交或有端点在三角形上均不算在三角形内部
    const bool is_inside(const Point &start, const Point &end, const Triangle &triangle);

    // 判断一个三角形是否完全在另一个三角形内部,与三角形相交或有顶点在三角形上均不算在三角形内部
    const bool is_inside(const Triangle &triangle0, const Triangle &triangle1);

    // 判断两直线是否平行
    const bool is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3);

    // 判断两直线是否平行
    const bool is_parallel(const Line &line0, const Line &line1);

    // 判断两线段是否有重合,仅有一个端点重合不算两线段重合
    const bool is_coincide(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 判断一个有限长线段是否是另一个有限长线段的一部分
    const bool is_part(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 判断一个有限长线段是否是另一个有限长线段的一部分
    const bool is_part(const Line &line0, const Line &line1);

    // 判断两线段是否相交尝试并获取交点,共线相交时仅在一个端点相交时获取交点
    const bool is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite = false);

    // 判断两线段是否相交尝试并获取交点,共线相交时仅在一个端点相交时获取交点
    const bool is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite = true);

    // 判断两个AABB矩形是否相交,inside决定完全在AABB矩形内部是否算相交
    const bool is_intersected(const AABBRect &rect0, const AABBRect &rect1, const bool inside = true);

    // 判断两多段线是否相交
    const bool is_intersected(const Polyline &polyline0, const Polyline &polyline1);

    // 判断多段线是否与多边形相交,inside决定多段线完全在多边形内部是否算相交
    const bool is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside = true);

    // 判断多段线是否与圆相交
    const bool is_intersected(const Polyline &polyline, const Circle &circle);

    // 判断两多边形是否相交,inside决定完全在多边形内部是否算相交
    const bool is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside = true);

    // 判断多边形与圆是否相交,inside决定多边形完全在圆内或圆完全在多边形内是否算相交
    const bool is_intersected(const Polygon &polygon, const Circle &circle, const bool inside = true);

    // 判断两圆是否相交,inside决定完全在圆内是否算相交
    const bool is_intersected(const Circle &circle0, const Circle &circle1, const bool inside = true);

    // 判断AABB矩形是否与有限长线段相交,线段完全在AABB矩形内也算相交
    const bool is_intersected(const AABBRect &rect, const Point &point0, const Point &point1);

    // 判断AABB矩形是否与有限长线段相交,线段完全在AABB矩形内也算相交
    const bool is_intersected(const AABBRect &rect, const Line &line);

    // 判断AABB矩形是否与多段线相交,多段线完全在AABB矩形内也算相交
    const bool is_intersected(const AABBRect &rect, const Polyline &polyline);

    // 判断AABB矩形是否与多边形相交,多边形完全在AABB矩形内或AABB矩形完全在多边形内也算相交
    const bool is_intersected(const AABBRect &rect, const Polygon &polygon);

    // 判断AABB矩形是否与圆相交,圆完全在AABB矩形内或AABB矩形完全在圆内也算相交
    const bool is_intersected(const AABBRect &rect, const Circle &circle);

    // 判断有限长线段是否与三角形相交,线段完全在三角形内不算相交
    const bool is_intersected(const Point &start, const Point &end, const Triangle &triangle, Point &output0, Point &output1);

    // 判断有限长线段是否与三角形相交,线段完全在三角形内不算相交
    const bool is_intersected(const Line &line, const Triangle &triangle, Point &output0, Point &output1);

    // 判断点是否在直线的左侧
    const bool is_on_left(const Point &point, const Point &start, const Point &end);

    // 判断多边形是否是矩形
    const bool is_Rectangle(const Polygon &polygon);

    // 计算两向量叉积
    double cross(const double x0, const double y0, const double x1, const double y1);

    // 计算两向量叉积
    double cross(const Vector &vec0, const Vector &vec1);

    // 计算两向量叉积
    double cross(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    Polygon circle_to_polygon(const double x, const double y, const double r);

    Polygon circle_to_polygon(const Circle &circle);

    std::vector<size_t> ear_cut_to_indexs(const Polygon &polygon);

    std::vector<size_t> ear_cut_to_indexs_test(const Polygon &polygon);

    std::vector<MarkedPoint> ear_cut_to_coords(const Polygon &polygon);

    std::vector<Point> ear_cut_to_points(const Polygon &polygon);

    std::vector<Triangle> ear_cut_to_triangles(const Polygon &polygon);

    bool offset(const Polyline &input, Polyline &result, const double distance);

    bool offset(const Polygon &input, Polygon &result, const double distance);

    bool offset(const Circle &input, Circle &result, const double distance);

    bool offset(const AABBRect &input, AABBRect &result, const double distance);

    bool polygon_union(const Polygon &polygon0, const Polygon &polygon1, Polygon &output);
};