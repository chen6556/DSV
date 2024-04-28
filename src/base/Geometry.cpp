#include "base/Geometry.hpp"
#include <cassert>
#include <algorithm>
#include <array>
#include <functional>
#include "base/EarCut/EarCut.hpp"


using namespace Geo;

Geometry::Geometry(const Geometry &geo)
    : shape_fixed(geo.shape_fixed), is_selected(geo.is_selected),
    point_index(geo.point_index), point_count(geo.point_count)
{}

Geometry &Geometry::operator=(const Geometry &geo)
{
    if (this != &geo)
    {
        shape_fixed = geo.shape_fixed;
        is_selected = geo.is_selected;
        point_index = geo.point_index;
        point_count = geo.point_count;
    }
    return *this;
}

const Type Geometry::type() const
{
    return Type::GEOMETRY;
}

const double Geometry::length() const
{
    return 0;
}

const bool Geometry::empty() const
{
    return true;
}

void Geometry::clear(){}

Geometry *Geometry::clone() const
{
    return new Geometry(*this);
}

void Geometry::transform(const double a, const double b, const double c, const double d, const double e, const double f){}

void Geometry::transform(const double mat[6]){}

void Geometry::translate(const double tx, const double ty){}

void Geometry::rotate(const double x, const double y, const double rad){}

void Geometry::scale(const double x, const double y, const double k){}

Polygon Geometry::convex_hull() const { return Polygon(); }

AABBRect Geometry::bounding_rect() const { return AABBRect(); }

Polygon Geometry::mini_bounding_rect() const { return Polygon(); }


// Point

Point::Point(const double x_, const double y_)
    : x(x_), y(y_)
{}

Point::Point(const Point &point)
    :Geometry(point), x(point.x), y(point.y)
{}

Point::Point(const MarkedPoint &point)
    : x(point.x), y(point.y)
{}

Point &Point::operator=(const Point &point)
{
    if (this != &point)
    {
        Geometry::operator=(point);
        x = point.x;
        y = point.y;
    }
    return *this;
}

const Type Point::type() const
{
    return Type::POINT;
}

const bool Point::operator==(const Point &point) const
{
    return x == point.x && y == point.y;
}

const bool Point::operator!=(const Point &point) const
{
    return x != point.x || y != point.y;
}

const Point &Point::normalize()
{
    const double len = length();
    x /= len;
    y /= len;
    return *this;
}

Point Point::normalized() const
{
    const double len = length();
    return Point(x / len, y / len);
}

Point Point::vertical() const
{
    return Point(-y, x);
}

const double Point::length() const
{
    return std::sqrt(x * x + y * y);
}

const bool Point::empty() const
{
    return x == 0 && y == 0;
}

void Point::clear()
{
    x = 0;
    y = 0;
}

Point *Point::clone() const
{
    return new Point(*this);
}

void Point::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    const double x_ = x, y_ = y;
    x = a * x_ + b * y_ + c;
    y = d * x_ + e * y_ + f;
}

void Point::transform(const double mat[6])
{
    const double x_ = x, y_ = y;
    x = mat[0] * x_ + mat[1] * y_ + mat[2];
    y = mat[3] * x_ + mat[4] * y_ + mat[5];
}

void Point::translate(const double tx, const double ty)
{
    x += tx;
    y += ty;
}

void Point::rotate(const double x_, const double y_, const double rad)
{
    x -= x_;
    y -= y_;
    const double x1 = x, y1 = y;
    x = x1 * std::cos(rad) - y1 * std::sin(rad);
    y = x1 * std::sin(rad) + y1 * std::cos(rad);
    x += x_;
    y += y_;
}

void Point::scale(const double x_, const double y_, const double k)
{
    const double x1 = x, y1 = y;
    x = k * x1 + x_ * (1 - k);
    y = k * y1 + y_ * (1 - k);
}

AABBRect Point::bounding_rect() const
{
    if (length() == 0)
    {
        return AABBRect();
    }
    else
    {
        return AABBRect(std::min(0.0, x), std::min(0.0, y), std::max(0.0, x), std::max(0.0, y));
    }
}

Polygon Point::mini_bounding_rect() const
{
    if (length() == 0)
    {
        return Polygon();
    }
    else
    {
        return AABBRect(std::min(0.0, x), std::min(0.0, y), std::max(0.0, x), std::max(0.0, y));
    }
}

Point Point::operator*(const double k) const
{
    return Point(x * k, y * k);
}

double Point::operator*(const Point &point) const
{
    return x * point.x + y * point.y;
}

double Point::cross(const Point &point) const
{
    return x * point.y - y * point.x;
}

Point Point::operator+(const Point &point) const
{
    return Point(x + point.x, y + point.y);
}

Point Point::operator-(const Point &point) const
{
    return Point(x - point.x, y - point.y);
}

Point Point::operator/(const double k) const
{
    return Point(x / k, y / k);
}

void Point::operator*=(const double k)
{
    x *= k;
    y *= k;
}

void Point::operator+=(const Point &point)
{
    x += point.x;
    y += point.y;
}

void Point::operator-=(const Point &point)
{
    x -= point.x;
    y -= point.y;
}

void Point::operator/=(const double k)
{
    x /= k;
    y /= k;
}

// Polyline

Polyline::Polyline(const Polyline &polyline)
    :Geometry(polyline)
    ,_points(polyline._points)
{}

Polyline::Polyline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    _points.push_back(*begin);
    while (++begin != end)
    {
        if (*begin != _points.back())
        {
            _points.push_back(*begin);
        }
    }
}

Polyline::Polyline(const std::initializer_list<Point>& points)
{
    _points.push_back(*points.begin());
    for (const Point& point : points)
    {
        if (point != _points.back())
        {
            _points.push_back(point);
        }
    }
}

const Type Polyline::type() const
{
    return Type::POLYLINE;
}

const size_t Polyline::size() const
{
    return _points.size();
}

const bool Polyline::empty() const
{
    return _points.empty();
}

const double Polyline::length() const
{
    double reuslt = 0;
    for (size_t i = 1, count = _points.size(); i < count; ++i)
    {
        reuslt += Geo::distance(_points[i], _points[i-1]);
    }
    return reuslt;
}

void Polyline::clear()
{
    _points.clear();
}

Polyline *Polyline::clone() const
{
    return new Polyline(*this);
}

bool Polyline::is_self_intersected() const
{
    if (_points.size() < 4)
    {
        return false;
    }

    Point point;
    for (size_t j = 2, count = _points.size() - 2; j < count; ++j)
    {
        if (Geo::is_intersected(_points[0], _points[1], _points[j], _points[j + 1], point))
        {
            return true;
        }
    }
    for (size_t i = 1, count = _points.size() - 1; i < count; ++i)
    {
        for (size_t j = i + 2; j < count; ++j)
        {
            if (Geo::is_intersected(_points[i], _points[i + 1], _points[j], _points[j + 1], point))
            {
                return true;
            }
        }
    }
    return false;
}

Point &Polyline::operator[](const size_t index)
{
    assert(index < _points.size());
    return _points[index];
}

const Point &Polyline::operator[](const size_t index) const
{
    assert(index < _points.size());
    return _points[index];
}

Polyline &Polyline::operator=(const Polyline &polyline)
{
    if (this != &polyline)
    {
        Geometry::operator=(polyline);
        _points = polyline._points;
    }
    return *this;
}

Polyline Polyline::operator+(const Point &point) const
{
    std::vector<Point> temp(_points);
    for (Point &p : temp)
    {
        p += point;
    }
    return Polyline(temp.begin(), temp.end());
}

Polyline Polyline::operator-(const Point &point) const
{
    std::vector<Point> temp(_points);
    for (Point &p : temp)
    {
        p -= point;
    }
    return Polyline(temp.begin(), temp.end());
}

void Polyline::operator+=(const Point &point)
{
    for (Point &p : _points)
    {
        p += point;
    }
}

void Polyline::operator-=(const Point &point)
{
    for (Point &p : _points)
    {
        p -= point;
    }
}

void Polyline::append(const Point &point)
{
    if (_points.empty() || _points.back() != point)
    {
        _points.push_back(point);
    }
}

void Polyline::append(const Polyline &polyline)
{
    if (_points.empty() ||  _points.back() != polyline._points.front())
    {
        _points.insert(_points.cend(), polyline._points.cbegin(), polyline._points.cend());
    }
    else
    {
        _points.insert(_points.cend(), polyline._points.cbegin() + 1, polyline._points.cend());
    }
}

void Polyline::append(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    if (_points.empty() || _points.back() != *begin)
    {
        _points.insert(_points.end(), begin, end);
    }
    else
    {
        _points.insert(_points.end(), begin + 1, end);
    }
}

void Polyline::insert(const size_t index, const Point &point)
{
    assert(index < _points.size());
    if (_points[index] == point || (index > 0 && _points[index - 1] == point))
    {
        return;
    }
    else
    {
        _points.insert(_points.cbegin() + index, point);
    }
}

void Polyline::insert(const size_t index, const Polyline& polyline)
{
    assert(index < _points.size());
    if (polyline.empty())
    {
        return;
    }
    int i = (index > 0 && _points[index - 1] == polyline._points.front()), j = _points[index] == polyline._points.back();
    _points.insert(_points.cbegin() + index, polyline._points.cbegin() + i, polyline._points.cend() - j);
}

void Polyline::insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    assert(index < _points.size());
    int i = (index > 0 && _points[index] == *begin);
    _points.insert(_points.end(), begin + i, end);
    const size_t len = std::distance(begin, end);
    if (_points[index + len] == _points[index + len + 1])
    {
        _points.erase(_points.begin() + index + len + 1);
    }
}

void Polyline::remove(const size_t index)
{
    assert(index < _points.size());
    _points.erase(_points.begin() + index);
}

void Polyline::remove(const size_t index, const size_t count)
{
    assert(index < _points.size());
    _points.erase(_points.begin() + index, _points.begin() + index + count);
}

Point Polyline::pop(const size_t index)
{
    assert(index < _points.size());
    Point point(std::move(_points[index]));
    _points.erase(_points.begin() + index);
    return point;
}

void Polyline::flip()
{
    std::reverse(_points.begin(), _points.end());
}

Point &Polyline::front()
{
    assert(!_points.empty());
    return _points.front();
}

const Point &Polyline::front() const
{
    assert(!_points.empty());
    return _points.front();
}

Point &Polyline::back()
{
    assert(!_points.empty());
    return _points.back();
}

const Point &Polyline::back() const
{
    assert(!_points.empty());
    return _points.back();
}

std::vector<Point>::iterator Polyline::begin()
{
    return _points.begin();
}

std::vector<Point>::const_iterator Polyline::begin() const
{
    return _points.begin();
}

std::vector<Point>::const_iterator Polyline::cbegin() const
{
    return _points.cbegin();
}

std::vector<Point>::iterator Polyline::end()
{
    return _points.end();
}

std::vector<Point>::const_iterator Polyline::end() const
{
    return _points.end();
}

std::vector<Point>::const_iterator Polyline::cend() const
{
    return _points.cend();
}

std::vector<Point>::reverse_iterator Polyline::rbegin()
{
    return _points.rbegin();
}

std::vector<Point>::const_reverse_iterator Polyline::rbegin() const
{
    return _points.rbegin();
}

std::vector<Point>::const_reverse_iterator Polyline::crbegin() const
{
    return _points.crbegin();
}

std::vector<Point>::reverse_iterator Polyline::rend()
{
    return _points.rend();
}

std::vector<Point>::const_reverse_iterator Polyline::rend() const
{
    return _points.rend();
}

std::vector<Point>::const_reverse_iterator Polyline::crend() const
{
    return _points.crend();
}

std::vector<Point>::iterator Polyline::find(const Point &point)
{
    return std::find(_points.begin(), _points.end(), point);
}

std::vector<Point>::const_iterator Polyline::find(const Point &point) const
{
    return std::find(_points.cbegin(), _points.cend(), point);
}

void Polyline::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.transform(a,b,c,d,e,f);});
}

void Polyline::transform(const double mat[6])
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.transform(mat);});
}

void Polyline::translate(const double tx, const double ty)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.translate(tx, ty);});
}

void Polyline::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.rotate(x, y, rad);});
}

void Polyline::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.scale(x, y, k);});
}

Polygon Polyline::convex_hull() const
{
    std::vector<Point> points(_points);
    std::sort(points.begin(), points.end(), [](const Point &a, const Point &b)
        {return a.y < b.y;});
    const Point origin(points.front());
    std::for_each(points.begin(), points.end(), [=](Point &p){p -= origin;});
    std::sort(points.begin() + 1, points.end(), [](const Point &a, const Point &b)
        {
            if (a.x / a.length() != b.x / b.length())
            {
                return a.x / a.length() > b.x / b.length();
            }
            else
            {
                return a.length() < b.length();
            }
        });
    std::for_each(points.begin(), points.end(), [=](Point &p){p += origin;});

    std::vector<Point> hull(points.begin(), points.begin() + 2);
    size_t count = hull.size(), index = 0;
    Geo::Vector vec0, vec1;
    std::vector<bool> used(points.size(), false);
    for (size_t i = 2, end = points.size(); i < end; ++i)
    {        
        vec0 = hull.back() - hull[count - 2];
        vec1 = vec0 + points[i] - hull.back();
        while (count >= 2 && vec0.x * vec1.y - vec1.x * vec0.y < 0)
        {
            hull.pop_back();
            --count;
            vec0 = hull.back() - hull[count - 2];
            vec1 = vec0 + points[i] - hull.back();
        }
        ++count;
        hull.emplace_back(points[i]);
        used[i] = true;
    }

    for (size_t i = 1; count < 2; ++i)
    {
        if (used[i])
        {
            continue;
        }
        hull.emplace_back(points[points.size() - i]);
        ++count;
        used[points.size() - i] = true;  
    }

    for (size_t i = points.size() - 1; i > 0; --i)   
    {
        if (used[i])
        {
            continue;
        }

        vec0 = hull.back() - hull[count - 2];
        vec1 = vec0 + points[i] - hull.back();
        while (count >= 2 && vec0.x * vec1.y - vec1.x * vec0.y < 0)
        {
            hull.pop_back();
            --count;
            vec0 = hull.back() - hull[count - 2];
            vec1 = vec0 + points[i] - hull.back();
        }
        ++count;
        hull.emplace_back(points[i]);
    }

    vec0 = hull.back() - hull[count - 2];
    vec1 = vec0 + points.front() - hull.back();
    if (count >= 2 && vec0.x * vec1.y - vec0.y * vec1.x < 0)
    {
        hull.pop_back();
    }
    
    hull.emplace_back(points.front());    
    return Polygon(hull.cbegin(), hull.cend());
}

AABBRect Polyline::bounding_rect() const
{
    if (_points.empty())
    {
        return AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Point &point : _points)
    {
        x0 = std::min(x0, point.x);
        y0 = std::min(y0, point.y);
        x1 = std::max(x1, point.x);
        y1 = std::max(y1, point.y);
    }
    return AABBRect(x0, y1, x1, y0);
}

Polygon Polyline::mini_bounding_rect() const
{
    if (_points.empty())
    {
        return Polygon();
    }

    double cs, area = DBL_MAX;
    AABBRect rect, temp;
    const Polygon hull(convex_hull());
    for (size_t i = 1, count = hull.size(); i < count; ++i)
    {
        Polygon polygon(hull);
        cs = (polygon[i - 1].x * polygon[i].y - polygon[i].x * polygon[i - 1].y)
            / (polygon[i].length() * polygon[i - 1].length());
        polygon.rotate(polygon[i - 1].x, polygon[i - 1].y, std::acos(cs));
        temp = polygon.bounding_rect();
        if (temp.area() < area)
        {
            rect = temp;
            area = temp.area();
            rect.rotate(polygon[i - 1].x, polygon[i - 1].y, -std::acos(cs));
        }
    }
    return rect;
}

// AABBRect

AABBRect::AABBRect()
{
    _points.assign({Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0)});
}

AABBRect::AABBRect(const double x0, const double y0, const double x1, const double y1)
{
    if (x0 < x1)
    {
        if (y0 > y1)
        {   
            _points.assign({Point(x0, y0), Point(x1, y0), Point(x1, y1), Point(x0, y1), Point(x0, y0)});
        }
        else
        {
            _points.assign({Point(x0, y1), Point(x1, y1), Point(x1, y0), Point(x0, y0), Point(x0, y1)});
        }
    }
    else
    {
        if (y0 > y1)
        {   
            _points.assign({Point(x1, y0), Point(x0, y0), Point(x0, y1), Point(x1, y1), Point(x1, y0)});
        }
        else
        {
            _points.assign({Point(x1, y1), Point(x0, y1), Point(x0, y0), Point(x1, y0), Point(x1, y1)});
        }
    }
}

AABBRect::AABBRect(const Point &point0, const Point &point1)
{
    const double x0 = point0.x, y0 = point0.y, x1 = point1.x, y1 = point1.y;
    if (x0 < x1)
    {
        if (y0 > y1)
        {   
            _points.assign({Point(x0, y0), Point(x1, y0), Point(x1, y1), Point(x0, y1), Point(x0, y0)});
        }
        else
        {
            _points.assign({Point(x0, y1), Point(x1, y1), Point(x1, y0), Point(x0, y0), Point(x0, y1)});
        }
    }
    else
    {
        if (y0 > y1)
        {   
            _points.assign({Point(x1, y0), Point(x0, y0), Point(x0, y1), Point(x1, y1), Point(x1, y0)});
        }
        else
        {
            _points.assign({Point(x1, y1), Point(x0, y1), Point(x0, y0), Point(x1, y0), Point(x1, y1)});
        }
    }
}

AABBRect::AABBRect(const AABBRect &rect)
    :Geometry(rect)
    ,_points(rect._points)
{}

const Type AABBRect::type() const
{
    return Type::AABBRECT;
}

const double AABBRect::left() const
{
    return _points.front().x;
}

const double AABBRect::top() const
{
    return _points.front().y;
}

const double AABBRect::right() const
{
    return _points[2].x;
}

const double AABBRect::bottom() const
{
    return _points[2].y;
}

void AABBRect::set_left(const double value)
{
    _points.front().x = value;
    _points[3].x = value;
    _points.back().x = value;
}

void AABBRect::set_top(const double value)
{
    _points.front().y = value;
    _points[1].y = value;
    _points.back().y = value;
}

void AABBRect::set_right(const double value)
{
    _points[1].x = value;
    _points[2].x = value;
}

void AABBRect::set_bottom(const double value)
{
    _points[2].y = value;
    _points[3].y = value;
}

AABBRect &AABBRect::operator=(const AABBRect &rect)
{
    if (this != &rect)
    {
        Geometry::operator=(rect);
        _points = rect._points;
    }
    return *this;
}

const bool AABBRect::empty() const
{
    return _points.empty();
}

const double AABBRect::length() const
{
    double reuslt = 0;
    for (size_t i = 1, count = _points.size(); i < count; ++i)
    {
        reuslt += Geo::distance(_points[i], _points[i-1]);
    }
    return reuslt;
}

void AABBRect::clear()
{
    _points.clear();
}

AABBRect *AABBRect::clone() const
{
    return new AABBRect(*this);
}

const double AABBRect::area() const
{
    if (empty())
    {
        return 0;
    }
    else
    {
        return distance(_points[0], _points[1]) * distance(_points[1], _points[2]);
    }
}

const double AABBRect::width() const
{
    if (!_points.empty())
    {
        return Geo::distance(_points.front(), _points[1]);
    }
    else
    {
        return -1;
    }
}

const double AABBRect::height() const
{
    if (!_points.empty())
    {
        return Geo::distance(_points[1], _points[2]);
    }
    else
    {
        return -1;
    }
}

void AABBRect::set_width(const double value)
{
    const double d = (value - width()) / 2;
    _points[0].x = _points[3].x = _points[4].x = _points[0].x - d;
    _points[1].x = _points[2].x = _points[1].x + d;
}

void AABBRect::set_height(const double value)
{
    const double d = (value - height()) / 2;
    _points[0].y = _points[1].y = _points[4].y = _points[0].y + d;
    _points[2].x = _points[3].x = _points[2].x + d;
}

void AABBRect::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.transform(a,b,c,d,e,f);});
    if (_points[0].x > _points[1].x)
    {
        std::swap(_points[0], _points[1]);
        std::swap(_points[2], _points[3]);
    }
    if (_points[0].y < _points[2].y)
    {
        std::swap(_points[0], _points[3]);
        std::swap(_points[1], _points[2]);
    }
    _points[4] = _points[0];
}

void AABBRect::transform(const double mat[6])
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.transform(mat);});
    if (_points[0].x > _points[1].x)
    {
        std::swap(_points[0], _points[1]);
        std::swap(_points[2], _points[3]);
    }
    if (_points[0].y < _points[2].y)
    {
        std::swap(_points[0], _points[3]);
        std::swap(_points[1], _points[2]);
    }
    _points[4] = _points[0];
}

void AABBRect::translate(const double tx, const double ty)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.translate(tx, ty);});
}

void AABBRect::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.rotate(x, y, rad);});
}

void AABBRect::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point){point.scale(x, y, k);});
}

Polygon AABBRect::convex_hull() const
{
    return Polygon(_points.cbegin(), _points.cend());
}

AABBRect AABBRect::bounding_rect() const
{
    if (_points.empty())
    {
        return AABBRect();
    }
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Point &point : _points)
    {
        x0 = std::min(x0, point.x);
        y0 = std::min(y0, point.y);
        x1 = std::max(x1, point.x);
        y1 = std::max(y1, point.y);
    }
    return AABBRect(x0, y0, x1, y1);
}

Polygon AABBRect::mini_bounding_rect() const
{
    return *this;
}

std::vector<Point>::const_iterator AABBRect::begin() const
{
    return _points.cbegin();
}

std::vector<Point>::const_iterator AABBRect::cbegin() const
{
    return _points.cbegin();
}

std::vector<Point>::const_iterator AABBRect::end() const
{
    return _points.cend();
}

std::vector<Point>::const_iterator AABBRect::cend() const
{
    return _points.cend();
}

std::vector<Point>::const_reverse_iterator AABBRect::rbegin() const
{
    return _points.crbegin();
}

std::vector<Point>::const_reverse_iterator AABBRect::crbegin() const
{
    return _points.crbegin();
}

std::vector<Point>::const_reverse_iterator AABBRect::rend() const
{
    return _points.crend();
}

std::vector<Point>::const_reverse_iterator AABBRect::crend() const
{
    return _points.crend();
}

std::vector<Point>::const_iterator AABBRect::find(const Point &point) const
{
    return std::find(_points.cbegin(), _points.cend(), point);
}

AABBRect AABBRect::operator+(const Point &point) const
{
    return AABBRect(_points[0].x + point.x, _points[0].y + point.y,
                    _points[2].x + point.x, _points[2].y + point.y);
}

AABBRect AABBRect::operator-(const Point &point) const
{
    return AABBRect(_points[0].x - point.x, _points[0].y - point.y,
                    _points[2].x - point.x, _points[2].y - point.y);
}

void AABBRect::operator+=(const Point &point)
{
    for (Point &p : _points)
    {
        p += point;
    }
}

void AABBRect::operator-=(const Point &point)
{
    for (Point &p : _points)
    {
        p -= point;
    }
}

const Point AABBRect::center() const
{
    return (_points[0] + _points[2]) / 2;
}

const Point &AABBRect::operator[](const size_t index) const
{
    assert(!_points.empty() && index <= 4);
    return _points[index];
}


// Polygon

Polygon::Polygon(const Polygon &polygon)
    :Polyline(polygon)
{}

Polygon::Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
    :Polyline(begin, end)
{
    assert(size() >= 3);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const std::initializer_list<Point>& points)
    :Polyline(points)
{
    assert(size() > 2);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const Polyline &polyline)
    :Polyline(polyline)
{
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const AABBRect& rect)
    :Polyline(rect.cbegin(), rect.cend())
{}

Polygon &Polygon::operator=(const Polygon &polygon)
{
    if (this != &polygon)
    {
        Polyline::operator=(polygon);
    }
    return *this;
}

const Type Polygon::type() const
{
    return Type::POLYGON;
}

Polygon *Polygon::clone() const
{
    return new Polygon(*this);
}

void Polygon::reorder_points(const bool cw)
{
    if (size() < 4)
    {
        return;
    }
    
    double result = 0;
    for (size_t i = 0, count = size() - 1; i < count; ++i)
    {
        result += (_points[i].x * _points[i + 1].y - _points[i + 1].x * _points[i].y);
    }
    if (cw)
    {
        if (result > 0)
        {
            std::reverse(_points.begin(), _points.end());
        }
    }
    else
    {
        if (result < 0)
        {
            std::reverse(_points.begin(), _points.end());
        }
    }
}

bool Polygon::is_cw() const
{
    if (size() < 4)
    {
        return false;
    }

    double result = 0;
    for (size_t i = 0, count = size() - 1; i < count; ++i)
    {
        result += (_points[i].x * _points[i + 1].y - _points[i + 1].x * _points[i].y);
    }
    return result < 0;
}

void Polygon::append(const Point &point)
{
    if (size() < 2)
    {
        Polyline::append(point);
    }
    else
    {
        if (_points.front() == _points.back())
        {
            Polyline::insert(size() - 1, point);
        }
        else
        {
            _points.emplace_back(point);
            _points.emplace_back(_points.front());
        }
    }
}

void Polygon::append(const Polyline &polyline)
{
    if (empty())
    {
        Polyline::append(polyline);
        if (_points.front() != _points.back())
        {
            _points.emplace_back(_points.front());
        }
    }
    else
    {
        if (_points.front() == _points.back())
        {
            Polyline::insert(size() - 1, polyline);
        }
        else
        {
            Polyline::append(polyline);
            _points.emplace_back(_points.front());
        }
    }
}

void Polygon::append(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    if (empty())
    {
        Polyline::append(begin, end);
        if (_points.front() != _points.back())
        {
            _points.emplace_back(_points.front());
        }
    }
    else
    {
        if (_points.front() == _points.back())
        {
            Polyline::insert(size() - 1, begin, end);
        }
        else
        {
            _points.insert(_points.end(), begin, end);
            _points.emplace_back(_points.front());
        }
    }
}

void Polygon::insert(const size_t index, const Point &point)
{
    Polyline::insert(index, point);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
}

void Polygon::insert(const size_t index, const Polyline &polyline)
{
    Polyline::insert(index, polyline);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
}

void Polygon::insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    Polyline::insert(index, begin, end);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
}

void Polygon::remove(const size_t index)
{
    Polyline::remove(index);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
    else if (index == size())
    {
        _points.front() == _points.back();
    }
}

void Polygon::remove(const size_t index, const size_t count)
{
    Polyline::remove(index, count);
    if (size() > 2)
    {
        if (index == 0)
        {
            back() = front();
        }
        else if (index + count >= size())
        {
            front() = back();
        }
    }
}

Point Polygon::pop(const size_t index)
{
    Geo::Point point = Polyline::pop(index);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
    else if (index == size())
    {
        _points.front() = _points.back();
    }
    return point;
}

Polygon Polygon::operator+(const Point &point) const
{
    std::vector<Point> temp(begin(), end());
    for (Point &p : temp)
    {
        p += point;
    }
    return Polygon(temp.begin(), temp.end());
}

Polygon Polygon::operator-(const Point &point) const
{
    std::vector<Point> temp(begin(), end());
    for (Point &p : temp)
    {
        p -= point;
    }
    return Polygon(temp.begin(), temp.end());
}

void Polygon::operator+=(const Point &point)
{
    for (Point &p : _points)
    {
        p += point;
    }
}

void Polygon::operator-=(const Point &point)
{
    for (Point &p : _points)
    {
        p -= point;
    }
}

const double Polygon::area() const
{
    if (size() < 4)
    {
        return 0;
    }
    double result = 0;
    for (size_t i = 0, count = size() - 1; i < count; ++i)
    {
        result += (_points[i].x * _points[i + 1].y - _points[i + 1].x * _points[i].y);
    }
    return std::abs(result) / 2.0;
}

size_t Polygon::next_point_index(const size_t index) const
{
    if (index < size() - 1)
    {
        return index + 1;
    }
    else
    {
        return 1;
    }
}

const Point &Polygon::next_point(const size_t index) const
{
    if (index < size() - 1)
    {
        return _points[index + 1];
    }
    else
    {
        return _points[1];
    }
}

Point &Polygon::next_point(const size_t index)
{
    if (index < size() - 1)
    {
        return _points[index + 1];
    }
    else
    {
        return _points[1];
    }
}

size_t Polygon::last_point_index(const size_t index) const
{
    if (index > 0)
    {
        return index - 1;
    }
    else
    {
        return size() - 2;
    }
}

const Point &Polygon::last_point(const size_t index) const
{
    if (index > 0)
    {
        return _points[index - 1];
    }
    else
    {
        return _points[_points.size() - 2];
    }
}

Point &Polygon::last_point(const size_t index)
{
    if (index > 0)
    {
        return _points[index - 1];
    }
    else
    {
        return _points[_points.size() - 2];
    }
}

size_t Polygon::index(const double x, const double y) const
{
    for (size_t i = 0, count = _points.size() - 1; i < count; ++i)
    {
        if (_points[i].x == x && _points[i].y == y)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

size_t Polygon::index(const Point &point) const
{
    for (size_t i = 0, count = _points.size() - 1; i < count; ++i)
    {
        if (_points[i] == point)
        {
            return i;
        }
    }
    return SIZE_MAX;
}

// Triangle

Triangle::Triangle(const Point &point0, const Point &point1, const Point &point2)
{
    _vecs[0] = point0;
    _vecs[1] = point1;
    _vecs[2] = point2;
}

Triangle::Triangle(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2)
{
    _vecs[0].x = x0;
    _vecs[0].y = y0;
    _vecs[1].x = x1;
    _vecs[1].y = y1;
    _vecs[2].x = x2;
    _vecs[2].y = y2;
}

Triangle::Triangle(const Triangle &triangle)
{
    _vecs[0] = triangle._vecs[0];
    _vecs[1] = triangle._vecs[1];
    _vecs[2] = triangle._vecs[2];
}

const Type Triangle::type() const
{
    return Type::TRIANGLE;
}

const bool Triangle::empty() const
{
    return _vecs[0] == _vecs[1] || _vecs[1] == _vecs[2] || _vecs[0] == _vecs[2];
}

const double Triangle::length() const
{
    return Geo::distance(_vecs[0], _vecs[1]) + Geo::distance(_vecs[1], _vecs[2]) + Geo::distance(_vecs[0], _vecs[2]);
}

void Triangle::clear()
{
    for (size_t i = 0; i < 3; ++i)
    {
        _vecs[i].clear();
    }
}

Triangle *Triangle::clone() const
{
    return new Triangle(*this);
}

double Triangle::area() const
{
    if (empty())
    {
        return 0;
    }
    const double a = Geo::distance(_vecs[0], _vecs[1]);
    const double b = Geo::distance(_vecs[1], _vecs[2]);
    const double c = Geo::distance(_vecs[0], _vecs[2]);
    const double p = (a + b + c) / 2;
    return std::sqrt(p * (p - a) * (p - b) * (p- c));
}

double Triangle::angle(const size_t index) const
{
    assert(0 <= index && index <= 2);
    if (empty())
    {
        return 0;
    }

    const double len0 = Geo::distance(_vecs[1], _vecs[2]);
    const double len1 = Geo::distance(_vecs[0], _vecs[2]);
    const double len2 = Geo::distance(_vecs[0], _vecs[1]);

    switch (index)
    {
    case 0:
        return std::acos((len1 * len1 + len2 * len2 - len0 * len0) / (2 * len1 * len2));
    case 1:
        return std::acos((len0 * len0 + len2 * len2 - len1 * len1) / (2 * len0 * len2));
    case 2:
        return std::acos((len0 * len0 + len1 * len1 - len2 * len2) / (2 * len0 * len1));
    default:
        return 0; 
    }
}

void Triangle::reorder_points(const bool cw)
{
    if (cw)
    {
        if (!is_cw())
        {
            std::swap(_vecs[1], _vecs[2]);
        }
    }
    else
    {
        if (is_cw())
        {
            std::swap(_vecs[1], _vecs[2]);
        }
    }
}

bool Triangle::is_cw() const
{
    return Geo::is_on_left(_vecs[2], _vecs[1], _vecs[0]);
}

Point &Triangle::operator[](const size_t index)
{
    assert(0 <= index && index <= 2);
    return _vecs[index];
}

const Point &Triangle::operator[](const size_t index) const
{
    assert(0 <= index && index <= 2);
    return _vecs[index];
}

Triangle &Triangle::operator=(const Triangle &triangle)
{
    if (this != &triangle)
    {
        _vecs[0] = triangle._vecs[0];
        _vecs[1] = triangle._vecs[1];
        _vecs[2] = triangle._vecs[2];
    }
    return *this;
}

Triangle Triangle::operator+(const Point &point) const
{
    Triangle triangle(*this);
    triangle._vecs[0] += point;
    triangle._vecs[1] += point;
    triangle._vecs[2] += point;
    return triangle;
}

Triangle Triangle::operator-(const Point &point) const
{
    Triangle triangle(*this);
    triangle._vecs[0] -= point;
    triangle._vecs[1] -= point;
    triangle._vecs[2] -= point;
    return triangle;
}

void Triangle::operator+=(const Point &point)
{
    _vecs[0] += point;
    _vecs[1] += point;
    _vecs[2] += point;
}

void Triangle::operator-=(const Point &point)
{
    _vecs[0] -= point;
    _vecs[1] -= point;
    _vecs[2] -= point;
}

void Triangle::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    _vecs[0].transform(a, b, c, d, e, f);
    _vecs[1].transform(a, b, c, d, e, f);
    _vecs[2].transform(a, b, c, d, e, f);
}

void Triangle::transform(const double mat[6])
{
    _vecs[0].transform(mat);
    _vecs[1].transform(mat);
    _vecs[2].transform(mat);
}

void Triangle::translate(const double tx, const double ty)
{
    _vecs[0].translate(tx, ty);
    _vecs[1].translate(tx, ty);
    _vecs[2].translate(tx, ty);
}

void Triangle::rotate(const double x, const double y, const double rad) // 弧度制
{
    _vecs[0].rotate(x, y, rad);
    _vecs[1].rotate(x, y, rad);
    _vecs[2].rotate(x, y, rad);
}

void Triangle::scale(const double x, const double y, const double k)
{
    _vecs[0].scale(x, y, k);
    _vecs[1].scale(x, y, k);
    _vecs[2].scale(x, y, k);
}

Polygon Triangle::convex_hull() const
{
    if (empty())
    {
        return Polygon();
    }
    else
    {
        return Polygon({_vecs[0], _vecs[1], _vecs[2], _vecs[0]});
    }
}

AABBRect Triangle::bounding_rect() const
{
    if (empty())
    {
        return AABBRect();
    }

    const double left = std::min(_vecs[0].x, std::min(_vecs[1].x, _vecs[2].x));
    const double right = std::max(_vecs[0].x, std::max(_vecs[1].x, _vecs[2].x));
    const double top = std::max(_vecs[0].y, std::max(_vecs[1].y, _vecs[2].y));
    const double bottom = std::min(_vecs[0].y, std::min(_vecs[1].y, _vecs[2].y));
    return AABBRect(left, top, right, bottom);
}

Polygon Triangle::mini_bounding_rect() const
{
    if (empty())
    {
        return Polygon();
    }

    double cs, area = DBL_MAX;
    AABBRect rect, temp;
    for (size_t i = 0; i < 3; ++i)
    {
        Triangle triangle(*this);
        cs = (triangle[i].x * triangle[i < 2 ? i + 1 : 0].y - triangle[i < 2 ? i + 1 : 0].x * triangle[i].y)
            / (triangle[i < 2 ? i + 1 : 0].length() * triangle[i].length());
        triangle.rotate(triangle[i].x, triangle[i].y, std::acos(cs));
        temp = triangle.bounding_rect();
        if (temp.area() < area)
        {
            rect = temp;
            area = temp.area();
            rect.rotate(triangle[i].x, triangle[i].y, -std::acos(cs));
        }
    }
    return rect;
}

Point Triangle::inner_circle_center() const
{
    const double a = Geo::distance(_vecs[1], _vecs[2]);
    const double b = Geo::distance(_vecs[0], _vecs[2]);
    const double c = Geo::distance(_vecs[0], _vecs[1]);
    return (_vecs[0] * a + _vecs[1] * b + _vecs[2] * c) / (a + b + c);
}

double Triangle::inner_circle_radius() const
{
    const double a = Geo::distance(_vecs[1], _vecs[2]);
    const double b = Geo::distance(_vecs[0], _vecs[2]);
    const double c = Geo::distance(_vecs[0], _vecs[1]);
    const double p = (a + b + c) / 2;
    return std::sqrt((p - a) * (p - b) * (p - c) / p);
}


// Circle

Circle::Circle(const double x, const double y, const double r)
    :_center(x, y)
    ,_radius(r)
{
    assert(r > 0);
}

Circle::Circle(const Point &point, const double r)
    :_center(point)
    ,_radius(r)
{
    assert(r > 0);
}

Circle::Circle(const Circle &circle)
    :Geometry(circle)
    ,_center(circle._center)
    ,_radius(circle._radius)
{}

Circle &Circle::operator=(const Circle &circle)
{
    if (this != &circle)
    {
        Geometry::operator=(circle);
        _center = circle._center;
        _radius = circle._radius;
    }
    return *this;
}

const Type Circle::type() const
{
    return Type::CIRCLE;
}

Point &Circle::center()
{
    return _center;
}

const Point &Circle::center() const
{
    return _center;
}

double &Circle::radius()
{
    return _radius;
}

const double Circle::radius() const
{
    return _radius;
}

const double Circle::area() const
{
    return Geo::PI * _radius * _radius;
}

const double Circle::length() const
{
    return 2.0 * Geo::PI * _radius;
}

const bool Circle::empty() const
{
    return _radius <= 0;
}

void Circle::clear()
{
    _radius = 0;
    _center.clear();
}

Circle *Circle::clone() const
{
    return new Circle(*this);
}

void Circle::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    _center.transform(a,b,c,d,e,f);
    _radius *= std::abs(a);
}

void Circle::transform(const double mat[6])
{
    _center.transform(mat);
    _radius *= std::abs(mat[0]);
}

void Circle::translate(const double tx, const double ty)
{
   _center.translate(tx, ty);
}

void Circle::rotate(const double x, const double y, const double rad)
{
    _center.rotate(x, y, rad);
}

void Circle::scale(const double x, const double y, const double k)
{
    _center.scale(x, y, k);
    _radius *= k;
}

AABBRect Circle::bounding_rect() const
{
    if (_radius == 0)
    {
        return AABBRect();
    }
    else
    {
        return AABBRect(_center.x - _radius, _center.y + _radius, _center.x + _radius, _center.y - _radius);
    }
}

Polygon Circle::mini_bounding_rect() const
{
    if (_radius == 0)
    {
        return Polygon();
    }
    else
    {
        return AABBRect(_center.x - _radius, _center.y + _radius, _center.x + _radius, _center.y - _radius);
    }
}

Circle Circle::operator+(const Point &point) const
{
    return Circle(_center + point, _radius);
}

Circle Circle::operator-(const Point &point) const
{
    return Circle(_center - point, _radius);
}

void Circle::operator+=(const Point &point)
{
    _center += point;
}

void Circle::operator-=(const Point &point)
{
    _center -= point;
}


// Line

Line::Line(const double x0, const double y0, const double x1, const double y1)
    :_start_point(x0, y0)
    ,_end_point(x1, y1)
{}

Line::Line(const Point &start, const Point &end)
    :_start_point(start)
    ,_end_point(end)
{}

Line::Line(const Line &line)
    :Geometry(line)
    ,_start_point(line._start_point)
    ,_end_point(line._end_point)
{}

Line &Line::operator=(const Line &line)
{
    if (this != &line)
    {
        Geometry::operator=(line);
        _start_point = line._start_point;
        _end_point = line._end_point;
    }
    return *this;
}

const Type Line::type() const
{
    return Type::LINE;
}

Line Line::operator+(const Point &point)
{
    return Line(_start_point + point, _end_point + point);
}

Line Line::operator-(const Point &point)
{
    return Line(_start_point - point, _end_point - point);
}

void Line::operator+=(const Point &point)
{
    _start_point += point;
    _end_point += point;
}

void Line::operator-=(const Point &point)
{
    _start_point -= point;
    _end_point -= point;
}

const double Line::length() const
{
    return Geo::distance(_start_point, _end_point);
}

const bool Line::empty() const
{
    return _start_point == _end_point;
}

void Line::clear()
{
    _start_point.clear();
    _end_point.clear();
}

Line *Line::clone() const
{
    return new Line(*this);
}

void Line::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    _start_point.transform(a,b,c,d,e,f);
    _end_point.transform(a,b,c,d,e,f);
}

void Line::transform(const double mat[6])
{
    _start_point.transform(mat);
    _end_point.transform(mat);
}

void Line::translate(const double tx, const double ty)
{
    _start_point.translate(tx, ty);
    _end_point.translate(tx, ty);
}

void Line::rotate(const double x, const double y, const double rad)
{
    _start_point.rotate(x, y, rad);
    _end_point.rotate(x, y, rad);
}

void Line::scale(const double x, const double y, const double k)
{
    _start_point.scale(x, y, k);
    _end_point.scale(x, y, k);
}

AABBRect Line::bounding_rect() const
{
    if (_start_point == _end_point)
    {
        return AABBRect();
    }
    else
    {
        return AABBRect(std::min(_start_point.x, _end_point.x),
                        std::max(_start_point.y, _end_point.y),
                        std::max(_start_point.x, _end_point.x),
                        std::min(_start_point.y, _end_point.y));
    }
}

Polygon Line::mini_bounding_rect() const
{
    if (_start_point == _end_point)
    {
        return Polygon();
    }
    else
    {
        return AABBRect(std::min(_start_point.x, _end_point.x),
                        std::max(_start_point.y, _end_point.y),
                        std::max(_start_point.x, _end_point.x),
                        std::min(_start_point.y, _end_point.y));
    }
}

Point &Line::front()
{
    return _start_point;
}

const Point &Line::front() const
{
    return _start_point;
}

Point &Line::back()
{
    return _end_point;
}

const Point &Line::back() const
{
    return _end_point;
}


// Bezier
Bezier::Bezier(const size_t n)
    : _order(n)
{
    _shape.shape_fixed = true;
}

Bezier::Bezier(const Bezier &bezier)
    : Polyline(bezier), _order(bezier._order), _shape(bezier._shape)
{
    _shape.shape_fixed = true;
}

Bezier::Bezier(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const size_t n)
    : Polyline(begin, end), _order(n)
{
    _shape.shape_fixed = true;
    update_shape();
}

Bezier::Bezier(const std::initializer_list<Point> &points, const size_t n)
    : Polyline(points), _order(n)
{
    _shape.shape_fixed = true;
    update_shape();
}

const Type Bezier::type() const
{
    return Type::BEZIER;
}

const size_t &Bezier::order() const
{
    return _order;
}

const Polyline &Bezier::shape() const
{
    return _shape;
}

void Bezier::update_shape(const double step)
{
    assert(0 < step && step < 1);
    _shape.clear();
    std::vector<int> temp(1, 1), nums(_order + 1, 1);
    for (size_t i = 1; i <= _order; ++i)
    {
        for (size_t j = 1; j < i; ++j)
        {
            nums[j] = temp[j - 1] + temp[j]; 
        }
        temp.assign(nums.begin(), nums.begin() + i + 1);
    }

    double t = 0;
    Point point;
    for (size_t i = 0, end = _points.size() - _order; i < end; i += _order)
    {
        _shape.append(_points[i]);
        t = 0;
        while (t <= 1)
        {
            point.clear();
            for (size_t j = 0; j <= _order; ++j)
            {
                point += (_points[j + i] * (nums[j] * std::pow(1 - t, _order - j) * std::pow(t, j))); 
            }
            _shape.append(point);
            t += step;
        }
    }
    _shape.append(_points.back());
}

void Bezier::append_shape(const double step)
{
    assert(0 < step && step < 1);
    if ((_points.size() - 1) % _order > 0)
    {
        return;
    }

    std::vector<int> temp(1, 1), nums(_order + 1, 1);
    for (size_t i = 1; i <= _order; ++i)
    {
        for (size_t j = 1; j < i; ++j)
        {
            nums[j] = temp[j - 1] + temp[j]; 
        }
        temp.assign(nums.begin(), nums.begin() + i + 1);
    }

    double t = 0;
    Point point;
    const size_t i = _points.size() - _order - 1;
    while (t <= 1)
    {
        point.clear();
        for (size_t j = 0; j <= _order; ++j)
        {
            point += (_points[j + i] * (nums[j] * std::pow(1 - t, _order - j) * std::pow(t, j))); 
        }
        _shape.append(point);
        t += step;
    }
    _shape.append(_points.back());
}

const double Bezier::length() const
{
    return _shape.length();
}

void Bezier::clear()
{
    _shape.clear();
    Polyline::clear();
}

Bezier *Bezier::clone() const
{
    return new Bezier(*this);
}

Bezier &Bezier::operator=(const Bezier &bezier)
{
    if (this != &bezier)
    {
        Polyline::operator=(bezier);
        _shape = bezier._shape;
    }
    return *this;
}

void Bezier::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    Polyline::transform(a, b, c, d, e, f);
    _shape.transform(a, b, c, d, e, f);
}

void Bezier::transform(const double mat[6])
{
    Polyline::transform(mat);
    _shape.transform(mat);
}

void Bezier::translate(const double tx, const double ty)
{
    Polyline::translate(tx, ty);
    _shape.translate(tx, ty);
}

void Bezier::rotate(const double x, const double y, const double rad)
{
    Polyline::rotate(x, y, rad);
    _shape.rotate(x, y, rad);
}

void Bezier::scale(const double x, const double y, const double k)
{
    Polyline::scale(x, y, k);
    _shape.scale(x, y, k);
}

Polygon Bezier::convex_hull() const
{
    return _shape.convex_hull();
}

AABBRect Bezier::bounding_rect() const
{
    return _shape.bounding_rect();
}

Polygon Bezier::mini_bounding_rect() const
{
    return _shape.mini_bounding_rect();
}



// functions

const double Geo::distance(const double x0, const double y0, const double x1, const double y1)
{
    return std::sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

const double Geo::distance(const Point &point0, const Point &point1)
{
    return std::sqrt((point0.x - point1.x) * (point0.x - point1.x)
                        + (point0.y - point1.y) * (point0.y - point1.y));
}

const double Geo::distance(const Point &point, const Line &line, const bool infinite)
{
    if (line.front().x == line.back().x)
    {
        if (infinite)
        {
            return std::abs(point.x - line.front().x);
        }
        else
        {
            if ((point.y >= line.front().y && point.y <= line.back().y) ||
                (point.y <= line.front().y && point.y >= line.back().y))
            {
                return std::abs(point.x - line.front().x);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    else if (line.front().y == line.back().y)
    {
        if (infinite)
        {
            return std::abs(point.y - line.front().y);
        }
        else
        {
            if ((point.x >= line.front().x && point.x <= line.back().x) ||
                (point.x <= line.front().x && point.x >= line.back().x))
            {
                return std::abs(point.y - line.front().y);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    
    const double a = line.back().y - line.front().y, 
                b = line.front().x - line.back().x,
                c = line.back().x * line.front().y - line.front().x * line.back().y;
    if (infinite)
    {
        return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double k = ((point.x - line.front().x) * (line.back().x - line.front().x) +
            (point.y - line.front().y) * (line.back().y - line.front().y)) /
            (std::pow(line.back().x - line.front().x, 2) + std::pow(line.back().y - line.front().y, 2)); 
        const double x = line.front().x + k * (line.back().x - line.front().x);

        if ((x >= line.front().x && x <= line.back().x) || (x <= line.front().x && x >= line.back().x))
        {
            return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
        }
    }
}

const double Geo::distance(const Point &point, const Point &start, const Point &end, const bool infinite)
{
    if (start.x == end.x)
    {
        if (infinite)
        {
            return std::abs(point.x - start.x);
        }
        else
        {
            if ((point.y >= start.y && point.y <= end.y) ||
                (point.y <= start.y && point.y >= end.y))
            {
                return std::abs(point.x - start.x);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    else if (start.y == end.y)
    {
        if (infinite)
        {
            return std::abs(point.y - start.y);
        }
        else
        {
            if ((point.x >= start.x && point.x <= end.x) ||
                (point.x <= start.x && point.x >= end.x))
            {
                return std::abs(point.y - start.y);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    
    const double a = end.y - start.y, 
                b = start.x - end.x,
                c = end.x * start.y - start.x * end.y;
    if (infinite)
    {
        return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double k = ((point.x - start.x) * (end.x - start.x) +
            (point.y - start.y) * (end.y - start.y)) /
            (std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2)); 
        const double x = start.x + k * (end.x - start.x);

        if ((x >= start.x && x <= end.x) || (x <= start.x && x >= end.x))
        {
            return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance(point, start), Geo::distance(point, end));
        }
    }
}

const double Geo::distance(const Point &point, const Polyline &polyline)
{
    double dis = DBL_MAX;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance(point, polyline[i - 1], polyline[i]));
    }
    return dis;
}

const double Geo::distance(const Point &point, const Polygon &polygon)
{
    double dis = DBL_MAX;
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance(point, polygon[i - 1], polygon[i]));
    }
    return dis;
}


const bool Geo::is_inside(const Point &point, const Line &line, const bool infinite)
{
    return Geo::distance(point, line.front(), line.back(), infinite) < Geo::EPSILON;
}

const bool Geo::is_inside(const Point &point, const Point &start, const Point &end, const bool infinite)
{
    return Geo::distance(point, start, end, infinite) < Geo::EPSILON;
}

const bool Geo::is_inside(const Point &point, const Polyline &polyline)
{
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::is_inside(point, polyline[i-1], polyline[i]))
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_inside(const Point &point, const Polygon &polygon, const bool coincide)
{
    if (!polygon.empty() && Geo::is_inside(point, polygon.bounding_rect(), coincide))
    {
        size_t count = 0;
        double x = (-FLT_MAX);
        for (const Geo::Point &p : polygon)
        {
            x = std::max(x, p.x);
        }
        Geo::Point temp, end(x + 80, point.y);
        if (coincide)
        {
            for (size_t i = 1, len = polygon.size(); i < len; ++i)
            {
                if (Geo::is_inside(point, polygon[i-1], polygon[i]))
                {
                    return true;
                }
                if (Geo::is_intersected(point, end, polygon[i-1], polygon[i], temp))
                {
                    if (polygon[i - 1].y == polygon[i].y)
                    {
                        if ((polygon[i == 1 ? len-2 : i-2].y < polygon[i].y && polygon[i == len-1 ? 1 : i+1].y > polygon[i].y)
                            || (polygon[i == 1 ? len-2 : i-2].y > polygon[i].y && polygon[i == len-1 ? 1 : i+1].y < polygon[i].y))
                        {
                            ++count;
                        }
                    }
                    else
                    {
                        if (Geo::distance(temp, polygon[i-1]) < Geo::EPSILON)
                        {
                            if ((polygon[i == 1 ? len-2 : i-2].y < polygon[i-1].y && polygon[i].y < polygon[i-1].y) ||
                                (polygon[i == 1 ? len-2 : i-2].y > polygon[i-1].y && polygon[i].y > polygon[i-1].y))
                            {
                                ++count;
                            }
                        }
                        else
                        {
                            ++count;
                        }
                    }
                }
            }
        }
        else
        {
            for (size_t i = 1, len = polygon.size(); i < len; ++i)
            {
                if (Geo::is_inside(point, polygon[i-1], polygon[i]))
                {
                    return false;
                }
                if (Geo::is_intersected(point, end, polygon[i-1], polygon[i], temp))
                {
                    if (polygon[i - 1].y == polygon[i].y)
                    {
                        if ((polygon[i == 1 ? len-2 : i-2].y < polygon[i].y && polygon[i == len-1 ? 1 : i+1].y > polygon[i].y)
                            || (polygon[i == 1 ? len-2 : i-2].y > polygon[i].y && polygon[i == len-1 ? 1 : i+1].y < polygon[i].y))
                        {
                            ++count;
                        }
                    }
                    else
                    {
                        if (Geo::distance(temp, polygon[i-1]) < Geo::EPSILON)
                        {
                            if ((polygon[i == 1 ? len-2 : i-2].y < polygon[i-1].y && polygon[i].y < polygon[i-1].y) ||
                                (polygon[i == 1 ? len-2 : i-2].y > polygon[i-1].y && polygon[i].y > polygon[i-1].y))
                            {
                                ++count;
                            }
                        }
                        else
                        {
                            ++count;
                        }
                    }
                }
            }
        }
        return count % 2 == 1;
    }
    else
    {
        return false;
    }
}

const bool Geo::is_inside(const Point &point, const AABBRect &rect, const bool coincide)
{
    if (rect.empty())
    {
        return false;
    }
    const double x = point.x, y = point.y;
    if (coincide)
    {
        return rect.left() <= x && x <= rect.right() && rect.bottom() <= y && y <= rect.top();
    }
    else
    {
        return rect.left() < x && x < rect.right() && rect.bottom() < y && y < rect.top();
    }
}

const bool Geo::is_inside(const Point &point, const Circle &circle, const bool coincide)
{
    if (circle.empty())
    {
        return false;
    }
    if (coincide)
    {
        return Geo::distance(point, circle.center()) <= circle.radius();
    }
    else
    {
        return Geo::distance(point, circle.center()) < circle.radius();
    }
}

const bool Geo::is_inside(const Point &point, const Point &point0, const Point &point1, const Point &point2, const bool coincide)
{
    if (coincide)
    {
        const bool a = (point2.x - point.x) * (point0.y - point.y) 
            >= (point0.x - point.x) * (point2.y - point.y);
        const bool b = (point0.x - point.x) * (point1.y - point.y)
            >= (point1.x - point.x) * (point0.y - point.y);
        const bool c = (point1.x - point.x) * (point2.y - point.y)
            >= (point2.x - point.x) * (point1.y - point.y);

        return a == b && b == c;
    }
    else
    {
        const bool a = (point2.x - point.x) * (point0.y - point.y) 
            > (point0.x - point.x) * (point2.y - point.y);
        const bool b = (point0.x - point.x) * (point1.y - point.y)
            > (point1.x - point.x) * (point0.y - point.y);
        const bool c = (point1.x - point.x) * (point2.y - point.y)
            > (point2.x - point.x) * (point1.y - point.y);

        return a == b && b == c;
    }
}

const bool Geo::is_inside(const Point &point, const Triangle &triangle, const bool coincide)
{
    return Geo::is_inside(point, triangle[0], triangle[1], triangle[2], coincide);
}

const bool Geo::is_inside(const Point &start, const Point &end, const Triangle &triangle)
{
    return Geo::is_inside(start, triangle) && Geo::is_inside(end, triangle);
}

const bool Geo::is_inside(const Triangle &triangle0, const Triangle &triangle1)
{
    return Geo::is_inside(triangle0[0], triangle1) && Geo::is_inside(triangle0[1], triangle1)
        && Geo::is_inside(triangle0[2], triangle1);
}


const bool Geo::is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3)
{
    if (point0.x == point1.x && point2.x == point3.x)
    {
        return true;
    }
    else
    {
        return ((point0.y - point1.y) * (point2.x - point3.x)) ==
            ((point2.y - point3.y) * (point0.x - point1.x));
    }
}

const bool Geo::is_parallel(const Line &line0, const Line &line1)
{
    return Geo::is_parallel(line0.front(), line0.back(), line1.front(), line1.back());
}


const bool Geo::is_coincide(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    if ((start0 == start1 && end0 == end1) || (start0 == end1 && end0 == start0))
    {
        return true;
    }

    if (start0.x == end0.x)
    {
        if (start1.x == end1.x && start0.x == start1.x)
        {
            const bool result0 = (start1.y < start0.y && start0.y < end1.y) 
                || (end1.y < start0.y && start0.y < start1.y);
            const bool result1 = (start1.y < end0.y && end0.y < end1.y)
                || (end1.y < end0.y && end0.y < start1.y);
            const bool result2 = (start0.y < start1.y && start1.y < end0.y)
                || (end0.y < start1.y && start1.y < start0.y);
            const bool result3 = (start0.y < end1.y && end1.y < end0.y)
                || (end0.y < end1.y && end1.y < start0.y);
            return result0 || result1 || result2 || result3;
        }
        else
        {
            return false;
        }
    }
    else if (start0.y == end0.y)
    {
        if (start1.y == end1.y && start0.y == start1.y)
        {
            const bool result0 = (start1.x < start0.x && start0.x < end1.x)
                || (end1.x < start0.x && start0.x < start1.x);
            const bool result1 = (start1.x < end0.x && end0.x < end1.x)
                || (end1.x < end0.x && end0.x < start1.x);
            const bool result2 = (start0.x < start1.x && start1.x < end0.x)
                || (end0.x < start1.x && start1.x < start0.x);
            const bool result3 = (start0.x < end1.x && end1.x < end0.x)
                || (end0.x < end1.x && end1.x < start0.x);
            return result0 || result1 || result2 || result3;
        }
        else
        {
            return false;
        }
    }

    const double a0 = end0.y - start0.y, 
                b0 = start0.x - end0.x,
                c0 = end0.x * start0.y - start0.x * end0.y;
    const double a1 = end1.y - start1.y, 
                b1 = start1.x - end1.x,
                c1 = end1.x * start1.y - start1.x * end1.y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON && std::abs(a0 * c1 - a1 * c0) < Geo::EPSILON
        && std::abs(b0 * c1 - b1 * c0) < Geo::EPSILON)
    {
        return Geo::distance((start0 + end0) / 2, (start1 + end1) / 2) * 2
            < Geo::distance(start0, end0) + Geo::distance(start1, end1);
    }
    else
    {
        return false;
    }
}

const bool Geo::is_coincide(const Point &start, const Point &end, const Polygon &polygon)
{
    const size_t index0 = polygon.index(start.x, start.y), index1 = polygon.index(end.x, end.y);
    if (std::max(index0, index1) - std::min(index0, index1) == 1)
    {
        return true;
    }

    if (index0 < SIZE_MAX)
    {
        return Geo::is_coincide(start, end, start, polygon.last_point(index0))
            || Geo::is_coincide(start, end, start, polygon.next_point(index0));
    }
    else if (index1 < SIZE_MAX)
    {
        return Geo::is_coincide(start, end, end, polygon.last_point(index1))
            || Geo::is_coincide(start, end, end, polygon.next_point(index1));
    }
    else
    {
        for (size_t i = 1, count = polygon.size(); i < count; ++i)
        {
            if (Geo::is_coincide(start, end, polygon[i - 1], polygon[i]))
            {
                return true;
            }
        }
        return false;
    }
}



const bool Geo::is_part(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    if (is_coincide(start0, end0, start1, end1))
    {
        if (start0.x == end0.x)
        {
            const double top0 = std::max(start0.y, end0.y), bottom0 = std::min(start0.y, end0.y);
            const double top1 = std::max(start1.y, end1.y), bottom1 = std::min(start1.y, end1.y);
            return bottom1 <= bottom0 && bottom0 <= top1 && bottom1 <= top0 && top0 <= top1;
        }
        else
        {
            const double left0 = std::min(start0.x, end0.x), right0 = std::max(start0.x, end0.x);
            const double left1 = std::min(start1.x, end1.x), right1 = std::max(start1.x, end1.x);
            return left1 <= left0 && left0 <= right0 && left1 <= right0 && right0 <= right1;
        }
    }
    else
    {
        return false;
    }
}

const bool Geo::is_part(const Line &line0, const Line &line1)
{
    return is_part(line0.front(), line0.back(), line1.front(), line1.back());
}


const bool Geo::is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite)
{
    if (point0 == point2 || point0 == point3)
    {
        output = point0;
        return true;
    }
    else if (point1 == point2 || point1 == point3)
    {
        output = point1;
        return true;
    }

    if (!infinite)
    {
        const double left0 = std::min(point0.x, point1.x), left1 = std::min(point2.x, point3.x);
        const double right0 = std::max(point0.x, point1.x), right1 = std::max(point2.x, point3.x);
        const double top0 = std::max(point0.y, point1.y), top1 = std::max(point2.y, point3.y);
        const double bottom0 = std::min(point0.y, point1.y), bottom1 = std::min(point2.y, point3.y);
        if (left0 > right1 || right0 < left1 || top0 < bottom1 || bottom0 > top1)
        {
            return false;
        }
    }

    const double a0 = point1.y - point0.y, 
                b0 = point0.x - point1.x,
                c0 = point1.x * point0.y - point0.x * point1.y;
    const double a1 = point3.y - point2.y,
                b1 = point2.x - point3.x,
                c1 = point3.x * point2.y - point2.x * point3.y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON)
    {
        if (std::abs(a0 * c1 - a1 * c0) > Geo::EPSILON || std::abs(b0 * c1 - b1 * c0) > Geo::EPSILON)
        {
            return false;
        }
        if (infinite)
        {
            return true;
        }
        else
        {
            const double a = Geo::distance((point0 + point1) / 2, (point2 + point3) / 2) * 2;
            const double b = Geo::distance(point0, point1) + Geo::distance(point2, point3);
            if (a < b)
            {
                return true;
            }
            else if (a == b)
            {
                if (Geo::distance(point0, point2) < Geo::EPSILON ||
                    Geo::distance(point0, point3) < Geo::EPSILON)
                {
                    output = point0;
                }
                else
                {
                    output = point1;
                }
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    output.x = (c1 * b0 - c0 * b1) / (a0 * b1 - a1 * b0), output.y = (c0 * a1 - c1 * a0) / (a0 * b1 - a1 * b0);

    if (Geo::is_inside(point0, point2, point3))
    {
        output = point0;
    }
    else if (Geo::is_inside(point1, point2, point3))
    {
        output = point1;
    }
    else if (Geo::is_inside(point2, point0, point1))
    {
        output = point2;
    }
    else if (Geo::is_inside(point3, point0, point1))
    {
        output = point3;
    }

    if (point0.x == point1.x)
    {
        output.x = point0.x;
    }
    else if (point2.x == point3.x)
    {
        output.x = point2.x;
    }

    if (point0.y == point1.y)
    {
        output.y = point0.y;
    }
    else if (point2.y == point3.y)
    {
        output.y = point2.y;
    }

    if (infinite)
    {
        return true;
    }
    else
    {
        const double left = std::max(std::min(point0.x, point1.x), std::min(point2.x, point3.x));
        const double right = std::min(std::max(point0.x, point1.x), std::max(point2.x, point3.x));
        const double top = std::min(std::max(point0.y, point1.y), std::max(point2.y, point3.y));
        const double bottom = std::max(std::min(point0.y, point1.y), std::min(point2.y, point3.y));

        return left - 5e-14 <= output.x && output.x <= right + 5e-14
            && bottom - 5e-14 <= output.y && output.y <= top + 5e-14;
    }
}

const bool Geo::is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite)
{
    return Geo::is_intersected(line0.front(), line0.back(), line1.front(), line1.back(), output, infinite);
}

const bool Geo::is_intersected(const AABBRect &rect0, const AABBRect &rect1, const bool inside)
{
    if (rect0.empty() || rect1.empty())
    {
        return false;
    }
    
    if (rect0.right() < rect1.left() || rect0.left() > rect1.right() || rect0.bottom() > rect1.top() || rect0.top() < rect1.bottom())
    {
        return false;
    }
    
    if (inside)
    {
        return true;
    }
    else
    {
        return !((rect0.top() < rect1.top() && rect0.right() < rect1.right() && rect0.bottom() > rect1.bottom() && rect0.left() > rect1.left())
            || (rect1.top() < rect0.top() && rect1.right() < rect0.right() && rect1.bottom() > rect0.bottom() && rect1.left() > rect0.left()));
    }
}

const bool Geo::is_intersected(const Polyline &polyline0, const Polyline &polyline1)
{
    if (polyline0.empty() || polyline1.empty() || !Geo::is_intersected(polyline0.bounding_rect(), polyline1.bounding_rect()))
    {
        return false;
    }
    Point point;
    for (size_t i = 1, count0 = polyline0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polyline1.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polyline0[i-1], polyline0[i], polyline1[j-1], polyline1[j], point))
            {
                return true;
            }
        }
    }
    return false;
}

const bool Geo::is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside)
{
    if (polyline.empty() || polygon.empty() || !Geo::is_intersected(polygon.bounding_rect(), polyline.bounding_rect()))
    {
        return false;
    }

    Point point;
    for (size_t i = 1, count0 = polyline.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polygon.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polyline[i-1], polyline[i], polygon[j-1], polygon[j], point))
            {
                return true;
            }
            else if (inside && Geo::is_inside(polyline[i-1], polygon))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        return Geo::is_inside(polyline.back(), polygon);
    }
    else
    {
        return false;
    }
}

const bool Geo::is_intersected(const Polyline &polyline, const Circle &circle)
{
    for (size_t i = 0, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(circle.center(), polyline[i - 1], polyline[1]) < circle.radius())
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside)
{
    if (polygon0.empty() || polygon1.empty() || !Geo::is_intersected(polygon0.bounding_rect(), polygon1.bounding_rect()))
    {
        return false;
    }
    Point point;
    for (size_t i = 1, count0 = polygon0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polygon1.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polygon0[i-1], polygon0[i], polygon1[j-1], polygon1[j], point))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        for (const Point &point : polygon0)
        {
            if (Geo::is_inside(point, polygon1, true))
            {
                return true;
            }
        }
        for (const Point &point : polygon1)
        {
            if (Geo::is_inside(point, polygon0, true))
            {
                return true;
            }
        }
    }
    return false;
}

const bool Geo::is_intersected(const Polygon &polygon, const Circle &circle, const bool inside)
{
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (Geo::distance(circle.center(), polygon[i - 1], polygon[1]) < circle.radius())
        {
            return true;
        }
    }

    if (!inside)
    {
        return false;
    }

    if (Geo::is_inside(circle.center(), polygon, true) || std::any_of(polygon.begin(), polygon.end(), [&](const Geo::Point &point) { return Geo::is_inside(point, circle, true); }))
    {
        return true;
    }

    return false;
}

const bool Geo::is_intersected(const Circle &circle0, const Circle& circle1, const bool inside)
{
    if (inside)
    {
        return Geo::distance(circle0.center(), circle1.center()) <= circle0.radius() + circle1.radius();
    }
    else
    {
        const double distance = Geo::distance(circle0.center(), circle1.center());
        return distance <= circle0.radius() + circle1.radius() && distance >= std::abs(circle0.radius() - circle1.radius());
    }
}

const bool Geo::is_intersected(const AABBRect &rect, const Point &point0, const Point &point1)
{
    if (is_inside(point0, rect) || is_inside(point1, rect))
    {
        return true;
    }

    const double x_max = std::max(point0.x, point1.x);
    const double x_min = std::min(point0.x, point1.x);
    const double y_max = std::max(point0.y, point1.y);
    const double y_min = std::min(point0.y, point1.y);

    if (x_max < rect.left() || x_min > rect.right() || y_max < rect.bottom() || y_min > rect.top())
    {
        return false;
    }
    else
    {
        if ((x_min >= rect.left() && x_max <= rect.right()) || (y_min >= rect.bottom() && y_max <= rect.top()))
        {
            return true;
        }
        else
        {
            const double dx = point1.x - point0.x;
            const double dy = point1.y - point0.y;
            const bool b0 = (rect[0].x - point0.x) * dy >= (rect[0].y - point0.y) * dx;
            const bool b1 = (rect[1].x - point0.x) * dy >= (rect[1].y - point0.y) * dx;
            const bool b2 = (rect[2].x - point0.x) * dy >= (rect[2].y - point0.y) * dx;
            const bool b3 = (rect[3].x - point0.x) * dy >= (rect[3].y - point0.y) * dx;
            return !(b0 == b1 && b1 == b2 && b2 == b3);
        }
    }
}

const bool Geo::is_intersected(const AABBRect &rect, const Line &line)
{
    return Geo::is_intersected(rect, line.front(), line.back());
}

const bool Geo::is_intersected(const AABBRect &rect, const Polyline &polyline)
{
    if (polyline.empty() || !Geo::is_intersected(rect, polyline.bounding_rect()))
    {
        return false;
    }
    
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (is_intersected(rect, polyline[i - 1], polyline[i]))
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_intersected(const AABBRect &rect, const Polygon &polygon)
{
    if (polygon.empty() || !Geo::is_intersected(rect, polygon.bounding_rect()))
    {
        return false;
    }
    
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (is_intersected(rect, polygon[i - 1], polygon[i]))
        {
            return true;
        }
    }
    for (size_t i = 0; i < 4; ++i)
    {
        if (is_inside(rect[i], polygon))
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_intersected(const AABBRect &rect, const Circle &circle)
{
    if (circle.empty() || !Geo::is_intersected(rect, circle.bounding_rect()))
    {
        return false;
    }

    if (Geo::is_inside(circle.center(), rect, true))
    {
        return true;
    }
    for (const Geo::Point &point : rect)
    {
        if (Geo::is_inside(point, circle, true))
        {
            return true;
        }
    }
    for (size_t i = 1; i < 5; ++i)
    {
        if (Geo::distance(circle.center(), rect[i-1], rect[i]) <= circle.radius())
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_intersected(const Point &start, const Point &end, const Triangle &triangle, Point &output0, Point &output1)
{
    if (Geo::is_inside(start, end, triangle) || !Geo::is_intersected(triangle.bounding_rect(), start, end))
    {
        return false;
    }

    const bool a = Geo::is_intersected(start, end, triangle[0], triangle[1], output0) ||
        Geo::is_intersected(start, end, triangle[1], triangle[2], output0) ||
        Geo::is_intersected(start, end, triangle[0], triangle[2], output0);
    const bool b = Geo::is_intersected(start, end, triangle[0], triangle[2], output1) ||
        Geo::is_intersected(start, end, triangle[1], triangle[2], output1) ||
        Geo::is_intersected(start, end, triangle[0], triangle[1], output1);
    return a || b;
}

const bool Geo::is_intersected(const Line &line, const Triangle &triangle, Point &output0, Point &output1)
{
    return Geo::is_intersected(line.front(), line.back(), triangle, output0, output1);
}




const bool Geo::is_on_left(const Point &point, const Point &start, const Point &end)
{
    return (end.x - start.x) * (point.y - start.y) -
        (end.y - start.y) * (point.x - end.x) > 0;
}


const bool Geo::is_Rectangle(const Polygon &polygon)
{
    Geo::Polygon points(polygon);
    if (polygon.size() > 5)
    {
        for (size_t i = 2, count = polygon.size(); i < count; ++i)
        {
            if (Geo::is_inside(points[i - 1], points[i - 2], points[i]))
            {
                points.remove(--i);
                --count;
            }
        }
    }
    if (polygon.size() != 5)
    {
        return false;
    }

    if (Geo::distance(points[0], points[1]) == Geo::distance(points[2], points[3]) &&
        Geo::distance(points[1], points[2]) == Geo::distance(points[0], points[3]))
    {
        const Geo::Point vec0 = points[0] - points[1], vec1 = points[2] - points[1];
        return std::abs(vec0.x * vec1.x + vec0.y * vec1.y) == 0;
    }
    else
    {
        return false;
    }
}


double Geo::cross(const double x0, const double y0, const double x1, const double y1)
{
    return x0 * y1 - x1 * y0;
}

double Geo::cross(const Vector &vec0, const Vector &vec1)
{
    return vec0.x * vec1.y - vec1.x * vec0.y;
}

double Geo::cross(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    return Geo::cross(end0 - start0, end1 - start1);
}


Polygon Geo::circle_to_polygon(const double x, const double y, const double r)
{
    double c = r * Geo::PI;
    const double degree = std::asin(1 / r) * 2;
    Vector vec(0, r);
    const Point center(x, y);
    std::vector<Point> points;
    while (c-- > 0)
    {
        points.emplace_back(center + vec);
        vec.rotate(0, 0, degree);
    }
    return Polygon(points.cbegin(), points.cend());
}

Polygon Geo::circle_to_polygon(const Circle &circle)
{
    return Geo::circle_to_polygon(circle.center().x, circle.center().y, circle.radius());
}


std::vector<size_t> Geo::ear_cut_to_indexs(const Polygon &polygon)
{
    std::vector<std::vector<std::array<double, 2>>> points;
    points.emplace_back(std::vector<std::array<double, 2>>());
    for (const Point &point : polygon)
    {
        points.front().emplace_back(std::array<double, 2>({point.x, point.y}));
    }
    return mapbox::earcut<size_t>(points);
}

std::vector<size_t> Geo::ear_cut_to_indexs_test(const Polygon &polygon)
{
    std::vector<size_t> indexs, ear_indexs;
    if (polygon.is_cw())
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(count - i);
        }
    }
    else
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(i);
        }
    }

    bool is_ear;
    while (indexs.size() > 3)
    {
        for (size_t pre, cur, nxt, i = 0, count = indexs.size(); i < count; ++i)
        {
            pre = i > 0 ? indexs[i - 1] : indexs[count - 1];
            cur = indexs[i];
            nxt = i < count - 1 ? indexs[i + 1] : indexs[0];
            if ((polygon[cur] - polygon[pre]).cross(polygon[nxt] - polygon[cur]) > 0)
            {
                is_ear = true;
                for (size_t index : indexs)
                {
                    if (index == pre || index == cur || index == nxt)
                    {
                        continue;
                    }
                    if (is_inside(polygon[index], polygon[pre], polygon[cur], polygon[nxt]))
                    {
                        is_ear = false;
                        break;
                    }
                }
                if (is_ear)
                {
                    ear_indexs.push_back(pre);
                    ear_indexs.push_back(cur);
                    ear_indexs.push_back(nxt);
                    indexs.erase(indexs.begin() + i--);
                    --count;
                }
            }
        }
    }

    ear_indexs.insert(ear_indexs.end(), indexs.begin(), indexs.end());
    return ear_indexs;
}

std::vector<MarkedPoint> Geo::ear_cut_to_coords(const Polygon &polygon)
{
    std::vector<MarkedPoint> result;
    for (size_t i : ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i].x, polygon[i].y);
    }
    return result;
}

std::vector<Point> Geo::ear_cut_to_points(const Polygon &polygon)
{
    std::vector<Point> result;
    for (size_t i : ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i]);
    }
    return result;
}

std::vector<Triangle> Geo::ear_cut_to_triangles(const Polygon &polygon)
{
    std::vector<size_t> indexs;
    if (polygon.is_cw())
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(count - i);
        }
    }
    else
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(i);
        }
    }

    std::vector<Triangle> triangles;
    bool is_ear;
    while (indexs.size() > 3)
    {
        for (size_t pre, cur, nxt, i = 0, count = indexs.size(); i < count; ++i)
        {
            pre = i > 0 ? indexs[i - 1] : indexs[count - 1];
            cur = indexs[i];
            nxt = i < count - 1 ? indexs[i + 1] : indexs[0];
            if ((polygon[cur] - polygon[pre]).cross(polygon[nxt] - polygon[cur]) > 0)
            {
                is_ear = true;
                for (size_t index : indexs)
                {
                    if (index == pre || index == cur || index == nxt)
                    {
                        continue;
                    }
                    if (is_inside(polygon[index], polygon[pre], polygon[cur], polygon[nxt]))
                    {
                        is_ear = false;
                        break;
                    }
                }
                if (is_ear)
                {
                    triangles.emplace_back(polygon[pre], polygon[cur], polygon[nxt]);
                    indexs.erase(indexs.begin() + i--);
                    --count;
                }
            }
        }
    }

    if (indexs.size() == 3)
    {
        triangles.emplace_back(polygon[indexs[0]], polygon[indexs[1]], polygon[indexs[2]]);
    }
    
    return triangles;
}


bool Geo::offset(const Polyline &input, Polyline &result, const double distance)
{
    if (distance != 0)
    {
        Polyline temp(input);
        result.clear();
        double area = 0;
        for (size_t i = 1, count = temp.size(); i < count; ++i)
        {
            area += (temp[i].x * (temp[i+1 != count ? i+1 : 0].y - temp[i-1].y));
        }
        area += (temp.front().x * (temp[1].y - temp.back().y));
        if (area > 0)
        {
            temp.flip();
        }
        Point a, b;
        result.append(temp.front() + (temp[1] - temp[0]).vertical().normalize() * distance);
        for (size_t i = 1, end = temp.size() - 1; i < end; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : end]).vertical().normalize();
            b = (temp[i < end ? i + 1 : 0] - temp[i]).vertical().normalize();
            result.append(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(temp.back() + (temp.back() - temp[temp.size() - 2]).vertical().normalize() * distance);
        return true;
    }
    else
    {
        result = input;
        return true;
    }
}

bool Geo::offset(const Polygon &input, Polygon &result, const double distance)
{
    if (distance == 0)
    {
        result = input;
        return true;
    }

    Polygon temp(input);
    temp.reorder_points();
    result.clear();
    std::vector<Point> points;
    Point a, b;
    std::vector<bool> error_edges;
    if (distance > 0)
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result[j] = a;
                    }
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = DBL_MAX;
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (a.x < DBL_MAX && a.y < DBL_MAX)
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }

        for (size_t i = 0, count = result.size(); i < count; ++i)
        {
            if (std::isnan(result[i].x) || std::isnan(result[i].y))
            {
                result.remove(i--);
                --count;
            }
        }
        result.back() = result.front();
    }
    else
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            const Point a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            const Point b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result[j] = a;
                    }
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = DBL_MAX;
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (a.x < DBL_MAX && a.y < DBL_MAX)
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }

        for (size_t i = 0, count = result.size(); i < count; ++i)
        {
            if (std::isnan(result[i].x) || std::isnan(result[i].y))
            {
                result.remove(i--);
                --count;
            }
        }
        result.back() = result.front();
    }

    return true;
}

bool Geo::offset_test(const Polygon &input, Polygon &result, const double distance)
{
    if (distance == 0)
    {
        result = input;
        return true;
    }

    Polygon temp(input);
    temp.reorder_points();
    result.clear();
    std::vector<Point> points;
    Point a, b;
    std::vector<bool> error_edges;
    if (distance > 0)
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result[j] = a;
                    }
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = DBL_MAX;
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (a.x < DBL_MAX && a.y < DBL_MAX)
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }

        for (size_t i = 0, count = result.size(); i < count; ++i)
        {
            if (std::isnan(result[i].x) || std::isnan(result[i].y))
            {
                result.remove(i--);
                --count;
            }
        }
        result.back() = result.front();
    }
    else
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            const Point a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            const Point b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result[j] = a;
                    }
                    a.x = a.y = DBL_MAX;
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (a.x < DBL_MAX && a.y < DBL_MAX)
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = DBL_MAX;
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (a.x < DBL_MAX && a.y < DBL_MAX)
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }

        for (size_t i = 0, count = result.size(); i < count; ++i)
        {
            if (std::isnan(result[i].x) || std::isnan(result[i].y))
            {
                result.remove(i--);
                --count;
            }
        }
        result.back() = result.front();
    }

    std::vector<Polygon> polygons;
    if (Geo::merge_ear_cut_triangles(Geo::ear_cut_to_triangles(result), polygons))
    {
        std::sort(polygons.begin(), polygons.end(),
            [](const Polygon &a, const Polygon &b) { return a.area() < b.area(); });
        temp = polygons.back();
        polygons.pop_back();
        bool flag;
        std::vector<Polygon> polygons2;
        while (!polygons.empty())
        {
            flag = true;
            for (size_t i = 0, count = polygons.size(); i < count; ++i)
            {
                if (Geo::polygon_union(temp, polygons[i], polygons2))
                {
                    if (polygons2.size() > 1)
                    {
                        temp = *std::max_element(polygons2.begin(), polygons2.end(), 
                            [](const Polygon &a, const Polygon &b) { return a.area() < b.area(); });
                    }
                    else
                    {
                        temp = polygons2.front();
                    }
                    polygons.erase(polygons.begin() + i);
                    polygons2.clear();
                    flag = false;
                    break;
                }
            }
            if (flag)
            {
                break;
            }
        }

        if (!temp.is_self_intersected())
        {
            result = temp;
        }
    }

    return true;
}

bool Geo::offset(const Circle &input, Circle &result, const double distance)
{
    if (distance >= 0 || -distance < input.radius())
    {
        result.center() = input.center();
        result.radius() = input.radius() + distance;
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::offset(const AABBRect &input, AABBRect &result, const double distance)
{
    if (distance >= 0 || -distance * 2 < std::min(input.width(), input.height()))
    {
        result.set_top(input.top() + distance);
        result.set_right(input.right() + distance);
        result.set_bottom(input.bottom() + distance);
        result.set_left(input.left() + distance);
        return true;
    }
    else
    {
        return false;
    }
}


bool Geo::polygon_union(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(false);
    polygon3.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Point point, pre_point; // 找到交点并计算其几何数
    const AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }

            if (!is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        if (std::all_of(polygon0.begin(), polygon0.end(), [&](const Point &point) { return Geo::is_inside(point, polygon1, true); }))
        {
            output.emplace_back(polygon1); // 无交点,且polygon0的点都在polygon1内,则并集必然为polygon1
            return true;
        }
        else if (std::all_of(polygon1.begin(), polygon1.end(), [&](const Point &point) { return Geo::is_inside(point, polygon0, true); }))
        {
            output.emplace_back(polygon0); // 无交点,且polygon1的点都在polygon0内,则并集必然为polygon0
            return true;
        }
        else
        {
            return false;
        }
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points0[i - 1]) < Geo::distance(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points0.size() - 1; i > 1;)
    {
        if (polygon0.front() == points0[i])
        {
            if (!points0[i].original)
            {
                points0.insert(points0.begin(), points0[i]);
                points0.erase(points0.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points1[i - 1]) < Geo::distance(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points1.size() - 1; i > 1;)
    {
        if (polygon1.front() == points1[i])
        {
            if (!points1[i].original)
            {
                points1.insert(points1.begin(), points1[i]);
                points1.erase(points1.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::polygon_intersection(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(false);
    polygon3.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Point point, pre_point; // 找到交点并计算其几何数
    const AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            if (!is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        if (std::all_of(polygon0.begin(), polygon0.end(), [&](const Point &point) { return Geo::is_inside(point, polygon1, true); }))
        {
            output.emplace_back(polygon0); // 无交点,且polygon0的点都在polygon1内,则交集必然为polygon0
            return true;
        }
        else if (std::all_of(polygon1.begin(), polygon1.end(), [&](const Point &point) { return Geo::is_inside(point, polygon0, true); }))
        {
            output.emplace_back(polygon1); // 无交点,且polygon1的点都在polygon0内,则交集必然为polygon1
            return true;
        }
        else
        {
            return false;
        }
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points0[i - 1]) < Geo::distance(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points1[i - 1]) < Geo::distance(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value < 1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value < 1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::polygon_difference(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(true);
    polygon3.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Point point, pre_point; // 找到交点并计算其几何数
    const AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            if (!is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points0[i - 1]) < Geo::distance(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points0.size() - 1; i > 1;)
    {
        if (polygon0.front() == points0[i])
        {
            if (!points0[i].original)
            {
                points0.insert(points0.begin(), points0[i]);
                points0.erase(points0.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points1[i - 1]) < Geo::distance(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points1.size() - 1; i > 1;)
    {
        if (polygon1.front() == points1[i])
        {
            if (!points1[i].original)
            {
                points1.insert(points1.begin(), points1[i]);
                points1.erase(points1.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 || 
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}


bool Geo::merge_ear_cut_triangles(const std::vector<Triangle> &triangles, std::vector<Polygon> &polygons)
{
    if (triangles.empty())
    {
        return false;
    }
    
    polygons.clear();
    const size_t triangles_count = triangles.size();
    size_t merged_count = 1, index = 0;
    int index0, index1, index2;
    std::vector<bool> merged(triangles_count, false), current_triangles(triangles_count, false);
    Geo::Polygon points;
    bool flag;

    while (merged_count < triangles_count)
    {
        while (index < triangles_count && merged[index])
        {
            ++index;
        }
        if (index == triangles_count)
        {
            break;
        }

        points.append(triangles[index][0]);
        points.append(triangles[index][1]);
        points.append(triangles[index][2]);
        merged[index] = true;
        current_triangles.assign(triangles_count, false);
        current_triangles[index++] = true;

        flag = true;
        for (size_t i = index; i < triangles_count; ++i)
        {
            if (merged[i])
            {
                continue;
            }

            for (size_t j = 1, count = points.size(); j < count; ++j)
            {
                index0 = index1 = index2 = -1;
                if (points[j - 1] == triangles[i][0])
                {
                    index0 = 0;
                }
                else if (points[j - 1] == triangles[i][1])
                {
                    index0 = 1;
                }
                else if (points[j - 1] == triangles[i][2])
                {
                    index0 = 2;
                }

                if (points[j] == triangles[i][0])
                {
                    index1 = 0;
                }
                else if (points[j] == triangles[i][1])
                {
                    index1 = 1;
                }
                else if (points[j] == triangles[i][2])
                {
                    index1 = 2;
                }

                if (index0 == -1 || index1 == -1)
                {
                    continue;
                }
                index2 = 3 - index0 - index1;

                flag = true;
                for (size_t k = 0; k < triangles_count; ++k)
                {
                    if (current_triangles[k] && Geo::is_inside(triangles[i][index2], triangles[k]))
                    {
                        flag = false;
                        break;
                    }
                }

                if (flag)
                {
                    points.insert(j, triangles[i][index2]);
                    ++merged_count;
                    merged[i] = true;
                    current_triangles[i] = true;
                    break;
                }
            }
        }
        
        index = 0;
        polygons.emplace_back(points);
        points.clear();
    }

    return !polygons.empty();
}


