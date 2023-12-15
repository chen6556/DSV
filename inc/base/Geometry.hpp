#pragma once

#include "Memo.hpp"
#include <vector>


namespace Geo
{
    const static double PI = 3.14159265358979323846;
    const static double EPSILON = 1e-10;

    class Rectangle;

    class Polygon;

    class Geometry
    {
    protected:
        Memo _memo;
        bool _shape_fixed = false;

    public:
        Geometry() {};

        Geometry(const Geometry &geo);

        virtual ~Geometry() {};

        Geometry &operator=(const Geometry &geo);

        Memo &memo();

        const Memo &memo() const;

        bool &shape_fixed();

        const bool shape_fixed() const;

        virtual const double length() const;

        virtual const bool empty() const;

        virtual void clear();

        virtual Geo::Geometry *clone() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad); // 弧度制

        virtual void scale(const double x, const double y, const double k);

        virtual Polygon convex_hull() const;

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;
    };

    struct Coord
    {
        double x = 0;
        double y = 0;

        Coord(){};

        Coord(const double x_, const double y_);

        Coord(const Coord &coord);

        Coord(const Coord &&coord);

        Coord &operator=(const Coord &coord);

        Coord &operator=(const Coord &&coord);

        const bool operator==(const Coord &coord) const;

        const bool operator!=(const Coord &coord) const;
    };

    class Point : public Geometry // Type:10
    {
    private:
        Coord _pos;

    public:
        Point() { _memo["Type"] = 10; };

        Point(const double x, const double y);

        Point(const Coord &coord);

        Point(const Point &point);

        Point(const Point &&point);

        Point &operator=(const Point &point);

        Point &operator=(const Point &&point);

        Coord &coord();

        const Coord &coord() const;

        const bool operator==(const Point &point) const;

        const bool operator!=(const Point &point) const;

        const Point &normalize();

        Point normalized() const;

        virtual const double length() const;

        virtual const bool empty() const;

        virtual void clear();

        virtual Point *clone() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad);

        virtual void scale(const double x, const double y, const double k);

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;

        Point operator*(const double k) const;

        Point operator+(const Point &point) const;

        Point operator-(const Point &point) const;

        Point operator/(const double k) const;

        void operator*=(const double k);

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        void operator/=(const double k);
    };

    typedef Point Vector;

    class Polyline : public Geometry // Type:20
    {
    private:
        std::vector<Point> _points;

    public:
        Polyline() { _memo["Type"] = 20; };

        Polyline(const Polyline &polyline);

        Polyline(const Polyline &&polyline);

        Polyline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        Polyline(const std::initializer_list<Point> &points);

        const size_t size() const;

        virtual const bool empty() const;

        virtual const double length() const;

        virtual void clear();

        virtual Polyline *clone() const;

        Point &operator[](const size_t index);

        const Point &operator[](const size_t index) const;

        Polyline &operator=(const Polyline &polyline);

        Polyline &operator=(const Polyline &&polyline);

        Polyline operator+(const Point &point) const;

        Polyline operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        virtual void append(const Point &point);

        virtual void append(const Polyline &polyline);

        virtual void append(std::vector<Point>::const_iterator &begin, std::vector<Point>::const_iterator &end);

        virtual void insert(const size_t index, const Point &point);

        virtual void insert(const size_t index, const Polyline &polyline);

        virtual void insert(const size_t index, std::vector<Point>::const_iterator &begin, std::vector<Point>::const_iterator &end);

        virtual void remove(const size_t index);

        virtual Point pop(const size_t index);

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

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad);

        virtual void scale(const double x, const double y, const double k);

        virtual Polygon convex_hull() const;

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;
    };

    class Rectangle : public Geometry // Type:30
    {
    private:
        std::vector<Point> _points;

    public:
        Rectangle() { _memo["Type"] = 30; };

        Rectangle(const double x0, const double y0, const double x1, const double y1);

        Rectangle(const Point &point0, const Point &point1);

        Rectangle(const Rectangle &rect);

        Rectangle(const Rectangle &&rect);

        const double left() const;

        const double top() const;

        const double right() const;

        const double bottom() const;

        Rectangle &operator=(const Rectangle &reac);

        Rectangle &operator=(const Rectangle &&reac);

        virtual const bool empty() const;

        virtual const double length() const;

        virtual void clear();

        virtual Rectangle *clone() const;

        const double area() const;

        const double width() const;

        const double height() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad); // 弧度制

        virtual void scale(const double x, const double y, const double k);

        virtual Polygon convex_hull() const;

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;

        std::vector<Point>::const_iterator begin() const;

        std::vector<Point>::const_iterator cbegin() const;

        std::vector<Point>::const_iterator end() const;

        std::vector<Point>::const_iterator cend() const;

        std::vector<Point>::const_reverse_iterator rbegin() const;

        std::vector<Point>::const_reverse_iterator crbegin() const;

        std::vector<Point>::const_reverse_iterator rend() const;

        std::vector<Point>::const_reverse_iterator crend() const;

        std::vector<Point>::const_iterator find(const Point &point) const;

        Rectangle operator+(const Point &point) const;

        Rectangle operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        const Point center() const;

        const Point &operator[](const size_t index) const;
    };

    class Polygon : public Polyline // Type:40
    {
    public:
        Polygon() { _memo["Type"] = 40; };

        Polygon(const Polygon &polygon);

        Polygon(const Polygon &&polygon);

        Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end);

        Polygon(const std::initializer_list<Point> &points);

        Polygon(const Polyline &polyline);

        Polygon(const Rectangle &rect);

        Polygon &operator=(const Polygon &polygon);

        Polygon &operator=(const Polygon &&polygon);

        virtual Polygon *clone() const;

        virtual void append(const Point &point);

        virtual void append(const Polyline &polyline);

        virtual void append(std::vector<Point>::const_iterator &begin, std::vector<Point>::const_iterator &end);

        virtual void insert(const size_t index, const Point &point);

        virtual void insert(const size_t index, const Polyline &polyline);

        virtual void insert(const size_t index, std::vector<Point>::const_iterator &begin, std::vector<Point>::const_iterator &end);

        virtual void remove(const size_t index);

        virtual Point pop(const size_t index);

        Polygon operator+(const Point &point) const;

        Polygon operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        const double area() const;
    };

    class Circle : public Geometry // Type:50
    {
    private:
        Point _center;
        double _radius = 0;

    public:
        Circle() { _memo["Type"] = 50; };

        Circle(const double x, const double y, const double r);

        Circle(const Point &point, const double r);

        Circle(const Circle &circle);

        Circle(const Circle &&Circle);

        Circle &operator=(const Circle &circle);

        Circle &operator=(const Circle &&circle);

        Point &center();

        const Point &center() const;

        double &radius();

        const double radius() const;

        const double area() const;

        virtual const double length() const;

        virtual const bool empty() const;

        virtual void clear();

        virtual Circle *clone() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad); // 弧度制

        virtual void scale(const double x, const double y, const double k);

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;

        Circle operator+(const Point &point) const;

        Circle operator-(const Point &point) const;

        void operator+=(const Point &point);

        void operator-=(const Point &point);
    };

    class Line : public Geometry // Type:60
    {
    private:
        Point _start_point;
        Point _end_point;

    public:
        Line() { _memo["Type"] = 60; };

        Line(const double x0, const double y0, const double x1, const double y1);

        Line(const Point &start, const Point &end);

        Line(const Line &line);

        Line(const Line &&line);

        Line &operator=(const Line &line);

        Line &operator=(const Line &&line);

        Line operator+(const Point &point);

        Line operator-(const Point &point);

        void operator+=(const Point &point);

        void operator-=(const Point &point);

        virtual const double length() const;

        virtual const bool empty() const;

        virtual void clear();

        virtual Line *clone() const;

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad); // 弧度制

        virtual void scale(const double x, const double y, const double k);

        virtual Rectangle bounding_rect() const;

        Point &front();

        const Point &front() const;

        Point &back();

        const Point &back() const;
    };

    class Bezier : public Polyline // Type:21
    {
    private:
        size_t _order = 2;
        Polyline _shape;

    public:
        Bezier(const size_t n);

        Bezier(const Bezier &bezier);

        Bezier(const Bezier &&bezier);

        Bezier(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const size_t n);

        Bezier(const std::initializer_list<Point> &points, const size_t n);

        const size_t &order() const;

        const Polyline &shape() const;

        void update_shape(const double step = 0.01);

        void append_shape(const double step = 0.01);

        virtual const double length() const;

        virtual void clear();

        virtual Bezier *clone() const;

        Bezier &operator=(const Bezier &bezier);

        Bezier &operator=(const Bezier &&bezier);

        virtual void transform(const double a, const double b, const double c, const double d, const double e, const double f);

        virtual void transform(const double mat[6]);

        virtual void translate(const double tx, const double ty);

        virtual void rotate(const double x, const double y, const double rad);

        virtual void scale(const double x, const double y, const double k);

        virtual Polygon convex_hull() const;

        virtual Rectangle bounding_rect(const bool orthogonality = true) const;
    };

    // functions

    const double distance(const double x0, const double y0, const double x1, const double y1);

    const double distance(const Point &point0, const Point &point1);

    const double distance(const Point &point, const Line &line, const bool infinite = false);

    const double distance(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    const double distance(const Point &point, const Polyline &polyline);

    const double distance(const Point &point, const Polygon &polygon);

    const bool is_inside(const Point &point, const Line &line, const bool infinite = false);

    const bool is_inside(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    const bool is_inside(const Point &point, const Polyline &polyline);

    const bool is_inside(const Point &point, const Polygon &polygon, const bool coincide = false);

    const bool is_inside(const Point &point, const Rectangle &rect, const bool coincide = false);

    const bool is_inside(const Point &point, const Circle &circle, const bool coincide = false);

    const bool is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite = false);

    const bool is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite = true);

    const bool is_intersected(const Rectangle &rect0, const Rectangle &rect1, const bool inside = true);

    const bool is_intersected(const Polyline &polyline0, const Polyline &polyline1);

    const bool is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside = true);

    const bool is_intersected(const Polyline &polyline, const Circle &circle);

    const bool is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside = true);

    const bool is_intersected(const Polygon &polygon, const Circle &circle, const bool inside = true);

    const bool is_intersected(const Circle &circle0, const Circle &circle1, const bool inside = true);

    const bool is_intersected(const Rectangle &rect, const Point &point0, const Point &point1, const bool inside = true);

    const bool is_intersected(const Rectangle &rect, const Line &line, const bool inside = true);

    const bool is_intersected(const Rectangle &rect, const Polyline &polyline, const bool inside = true);

    const bool is_intersected(const Rectangle &rect, const Polygon &polygon, const bool inside = true);

    const bool is_intersected(const Rectangle &rect, const Circle &circle, const bool inside = true);

    const bool is_rectangle(const Polygon &polygon);

};