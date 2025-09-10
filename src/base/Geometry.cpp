#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>

#include "base/Algorithm.hpp"
#include "base/Math.hpp"

using namespace Geo;

Geometry::~Geometry() {}

const double Geometry::length() const { return 0; }

Polygon Geometry::convex_hull() const { return Polygon(); }

AABBRect Geometry::bounding_rect() const { return AABBRect(); }

Polygon Geometry::mini_bounding_rect() const { return Polygon(); }


// Point

Point::Point(const double x_, const double y_)
    : x(x_), y(y_)
{}

Point::Point(const Point &point)
    : Geometry(point), x(point.x), y(point.y)
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

Point &Point::normalize()
{
    const double len = std::sqrt(x * x + y * y);
    x /= len;
    y /= len;
    return *this;
}

Point Point::normalized() const
{
    const double len = std::sqrt(x * x + y * y);
    return Point(x / len, y / len);
}

Point Point::vertical() const
{
    return Point(-y, x);
}

const double Point::length() const
{
    return std::hypot(x, y);
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
    : Geometry(polyline), _points(polyline._points)
{}

Polyline::Polyline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    _points.emplace_back(*begin);
    while (++begin != end)
    {
        if (*begin != _points.back())
        {
            _points.emplace_back(*begin);
        }
    }
}

Polyline::Polyline(const std::initializer_list<Point> &points)
{
    _points.emplace_back(*points.begin());
    for (const Point& point : points)
    {
        if (point != _points.back())
        {
            _points.emplace_back(point);
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
    return Polyline(temp.cbegin(), temp.cend());
}

Polyline Polyline::operator-(const Point &point) const
{
    std::vector<Point> temp(_points);
    for (Point &p : temp)
    {
        p -= point;
    }
    return Polyline(temp.cbegin(), temp.cend());
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
    const size_t index = _points.size();
    if (_points.empty() || _points.back() != *begin)
    {
        _points.insert(_points.end(), begin, end);
    }
    else
    {
        _points.insert(_points.end(), begin + 1, end);
    }
    for (size_t i = index <= 1 ? 1 : index - 1, count = _points.size(); i < count; ++i)
    {
        if (_points[i - 1] == _points[i])
        {
            _points.erase(_points.begin() + i--);
            --count;
        }
    }
}

void Polyline::append(std::vector<Point>::const_reverse_iterator rbegin, std::vector<Point>::const_reverse_iterator rend)
{
    const size_t index = _points.size();
    if (_points.empty() || _points.back() != *rbegin)
    {
        _points.insert(_points.end(), rbegin, rend);
    }
    else
    {
        _points.insert(_points.end(), rbegin + 1, rend);
    }
    for (size_t i = index <= 1 ? 1 : index - 1, count = _points.size(); i < count; ++i)
    {
        if (_points[i - 1] == _points[i])
        {
            _points.erase(_points.begin() + i--);
            --count;
        }
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
    _points.insert(_points.begin() + index, begin + i, end);
    for (size_t i = index <= 1 ? 1 : index - 1, count = _points.size(); i < count; ++i)
    {
        if (_points[i - 1] == _points[i])
        {
            _points.erase(_points.begin() + i--);
            --count;
        }
    }
}

void Polyline::insert(const size_t index, std::vector<Point>::const_reverse_iterator rbegin, std::vector<Point>::const_reverse_iterator rend)
{
    assert(index < _points.size());
    int i = (index > 0 && _points[index] == *rbegin);
    _points.insert(_points.begin() + index, rbegin + i, rend);
    for (size_t i = index <= 1 ? 1 : index - 1, count = _points.size(); i < count; ++i)
    {
        if (_points[i - 1] == _points[i])
        {
            _points.erase(_points.begin() + i--);
            --count;
        }
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
    : Geometry(rect), ClosedShape(rect), _points(rect._points)
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
        ClosedShape::operator=(rect);
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
    : Polyline(polygon), ClosedShape(polygon)
{}

Polygon::Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
    : Polyline(begin, end)
{
    assert(size() >= 3);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

Polygon::Polygon(const std::initializer_list<Point>& points)
    : Polyline(points)
{
    assert(size() > 2);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

Polygon::Polygon(const Polyline &polyline)
    : Polyline(polyline)
{
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

Polygon::Polygon(const AABBRect& rect)
    : Polyline(rect.cbegin(), rect.cend())
{
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

Polygon &Polygon::operator=(const Polygon &polygon)
{
    if (this != &polygon)
    {
        Polyline::operator=(polygon);
        ClosedShape::operator=(polygon);
    }
    return *this;
}

const Type Polygon::type() const
{
    return Type::POLYGON;
}

void Polygon::clear()
{
    Polyline::clear();
    triangle_indices.clear();
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

void Polygon::insert(const size_t index, const Point &point)
{
    Polyline::insert(index, point);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

void Polygon::insert(const size_t index, const Polyline &polyline)
{
    Polyline::insert(index, polyline);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
}

void Polygon::insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    Polyline::insert(index, begin, end);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
        _points.front() = _points.back();
    }
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
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
    triangle_indices = Geo::ear_cut_to_indexs(*this);
    return point;
}

Polygon Polygon::operator+(const Point &point) const
{
    std::vector<Point> temp(_points);
    for (Point &p : temp)
    {
        p += point;
    }
    return Polygon(temp.cbegin(), temp.cend());
}

Polygon Polygon::operator-(const Point &point) const
{
    std::vector<Point> temp(_points);
    for (Point &p : temp)
    {
        p -= point;
    }
    return Polygon(temp.cbegin(), temp.cend());
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

Point Polygon::center_of_gravity() const
{
    Point result;
    double s, a = 0;
    for (size_t i = 1, count = _points.size(); i < count; ++i)
    {
        s = _points[i - 1].x * _points[i].y - _points[i - 1].y * _points[i].x;
        result.x += ((_points[i - 1].x + _points[i].x) * s / 3);
        result.y += ((_points[i - 1].y + _points[i].y) * s / 3);
        a += s;
    }
    result.x /= a;
    result.y /= a;
    return result;
}

Point Polygon::average_point() const
{
    Point point;
    for (size_t i = 0, count = _points.size() - 1; i < count; ++i)
    {
        point += _points[i];
    }
    point.x /= (_points.size() - 1);
    point.y /= (_points.size() - 1);
    return point;
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
    : Geometry(triangle), ClosedShape(triangle)
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
    assert(index <= 2);
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
        Geometry::operator=(triangle);
        ClosedShape::operator=(triangle);
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
double Circle::default_down_sampling_value = 0.02;

Circle::Circle(const double x, const double y, const double r)
    : Point(x, y), radius(r)
{
    assert(r > 0);
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const Point &point, const double r)
    : Point(point), radius(r)
{
    assert(r > 0);
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const Circle &circle)
    : Point(circle), ClosedShape(circle), radius(circle.radius), _shape(circle._shape)
{}

Circle &Circle::operator=(const Circle &circle)
{
    if (this != &circle)
    {
        Point::operator=(circle);
        ClosedShape::operator=(circle);
        radius = circle.radius;
        _shape = circle._shape;
    }
    return *this;
}

const Type Circle::type() const
{
    return Type::CIRCLE;
}

const double Circle::area() const
{
    return Geo::PI * radius * radius;
}

const double Circle::length() const
{
    return 2.0 * Geo::PI * radius;
}

const bool Circle::empty() const
{
    return radius <= 0;
}

void Circle::clear()
{
    radius = 0;
    triangle_indices.clear();
    Point::clear();
}

Circle *Circle::clone() const
{
    return new Circle(*this);
}

void Circle::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    Point::transform(a,b,c,d,e,f);
    radius *= std::abs(a);
    if (std::abs(a) == 1 && std::abs(e) == 1)
    {
        _shape.transform(a, b, c, d, e, f);
    }
    else
    {
        update_shape(Geo::Circle::default_down_sampling_value);
    }
}

void Circle::transform(const double mat[6])
{
    Point::transform(mat);
    radius *= std::abs(mat[0]);
    if (std::abs(mat[0]) == 1 && std::abs(mat[4]) == 1)
    {
        _shape.transform(mat);
    }
    else
    {
        update_shape(Geo::Circle::default_down_sampling_value);
    }
}

void Circle::translate(const double tx, const double ty)
{
    Point::translate(tx, ty);
    _shape.translate(tx, ty);
}

void Circle::scale(const double x, const double y, const double k)
{
    Point::scale(x, y, k);
    radius *= k;
    update_shape(Geo::Circle::default_down_sampling_value);
}

Polygon Circle::convex_hull() const
{
    if (radius == 0)
    {
        return Polygon();
    }
    else
    {
        return AABBRect(x - radius, y + radius, x + radius, y - radius);
    }
}

AABBRect Circle::bounding_rect() const
{
    if (radius == 0)
    {
        return AABBRect();
    }
    else
    {
        return AABBRect(x - radius, y + radius, x + radius, y - radius);
    }
}

Polygon Circle::mini_bounding_rect() const
{
    if (radius == 0)
    {
        return Polygon();
    }
    else
    {
        return AABBRect(x - radius, y + radius, x + radius, y - radius);
    }
}

Circle Circle::operator+(const Point &point) const
{
    return Circle(x + point.x, y + point.y, radius);
}

Circle Circle::operator-(const Point &point) const
{
    return Circle(x - point.x, y - point.y, radius);
}

void Circle::update_shape(const double down_sampling_value)
{
    _shape = Geo::circle_to_polygon(x, y, radius, down_sampling_value);
    triangle_indices = Geo::ear_cut_to_indexs(_shape);
}

const Polygon &Circle::shape() const
{
    return _shape;
}


// Line

Line::Line(const double x0, const double y0, const double x1, const double y1)
    : _start_point(x0, y0), _end_point(x1, y1)
{}

Line::Line(const Point &start, const Point &end)
    : _start_point(start), _end_point(end)
{}

Line::Line(const Line &line)
    :Geometry(line), _start_point(line._start_point), _end_point(line._end_point)
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

Polygon Line::convex_hull() const
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
double Bezier::default_step = 0.01;
double Bezier::default_down_sampling_value = 0.02;

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
    update_shape(Bezier::default_step, Bezier::default_down_sampling_value);
}

Bezier::Bezier(const std::initializer_list<Point> &points, const size_t n)
    : Polyline(points), _order(n)
{
    _shape.shape_fixed = true;
    update_shape(Bezier::default_step, Bezier::default_down_sampling_value);
}

const Type Bezier::type() const
{
    return Type::BEZIER;
}

size_t Bezier::order() const
{
    return _order;
}

const Polyline &Bezier::shape() const
{
    return _shape;
}

void Bezier::update_shape(const double step, const double down_sampling_value)
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
    Geo::down_sampling(_shape, down_sampling_value);
}

void Bezier::append_shape(const double step, const double down_sampling_value)
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
    Geo::down_sampling(_shape, down_sampling_value);
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


// Ellipse
double Ellipse::default_down_sampling_value = 0.02;

Ellipse::Ellipse(const double x, const double y, const double a, const double b)
{
    assert(a > 0 && b > 0);
    if (a >= b)
    {
        _a[0].x = x - a;
        _a[1].x = x + a;
        _a[0].y = _a[1].y = y;
        _b[0].y = y + b;
        _b[1].y = y - b;
        _b[0].x = _b[1].x = x;
    }
    else
    {
        _a[0].x = x - b;
        _a[1].x = x + b;
        _a[0].y = _a[1].y = y;
        _b[0].y = y + a;
        _b[1].y = y - a;
        _b[0].x = _b[1].x = x;
    }
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &point, const double a, const double b)
{
    assert(a > 0 && b > 0);
    if (a >= b)
    {
        _a[0].x = point.x - a;
        _a[1].x = point.x + a;
        _a[0].y = _a[1].y = point.y;
        _b[0].y = point.y + b;
        _b[1].y = point.y - b;
        _b[0].x = _b[1].x = point.x;
    }
    else
    {
        _a[0].x = point.x - b;
        _a[1].x = point.x + b;
        _a[0].y = _a[1].y = point.y;
        _b[0].y = point.y + a;
        _b[1].y = point.y - a;
        _b[0].x = _b[1].x = point.x;
    }
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &a0, const Point &a1, const Point &b0, const Point &b1)
{
    _a[0] = a0, _a[1] = a1;
    _b[0] = b0, _b[1] = b1;
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Ellipse &ellipse)
    : Geometry(ellipse), ClosedShape(ellipse), _shape(ellipse._shape)
{
    _a[0] = ellipse._a[0];
    _a[1] = ellipse._a[1];
    _b[0] = ellipse._b[0];
    _b[1] = ellipse._b[1];
}

Ellipse &Ellipse::operator=(const Ellipse &ellipse)
{
    if (this != &ellipse)
    {
        Geometry::operator=(ellipse);
        ClosedShape::operator=(ellipse);
        _a[0] = ellipse._a[0];
        _a[1] = ellipse._a[1];
        _b[0] = ellipse._b[0];
        _b[1] = ellipse._b[1];
        _shape = ellipse._shape;
    }
    return *this;
}

const Type Ellipse::type() const
{
    return Geo::Type::ELLIPSE;
}

const double Ellipse::area() const
{
    return Geo::PI * Geo::distance(_a[0], _a[1]) * Geo::distance(_b[0], _b[1]) / 4;
}

const double Ellipse::length() const
{
    const double a = Geo::distance(_a[0], _a[1]) / 2, b = Geo::distance(_b[0], _b[1]) / 2;
    const double t = (a - b) / (a + b);
    return Geo::PI * (a + b) * (1 + 3 * std::pow(t, 2) / (10 + std::sqrt(4 - 3 * std::pow(t, 2))));
}

const bool Ellipse::empty() const
{
    return _a[0] == _a[1] || _b[0] == _b[1];
}

void Ellipse::clear()
{
    _a[0].clear();
    _a[1].clear();
    _b[0].clear();
    _b[1].clear();
    triangle_indices.clear();
}

Ellipse *Ellipse::clone() const
{
    return new Ellipse(*this);
}

void Ellipse::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    _a[0].transform(a, b, c, d, e, f);
    _a[1].transform(a, b, c, d, e, f);
    _b[0].transform(a, b, c, d, e, f);
    _b[1].transform(a, b, c, d, e, f);
    if (std::abs(a) == 1 && std::abs(e) == 1)
    {
        _shape.transform(a, b, c, d, e, f);
    }
    else
    {
        update_shape(Geo::Ellipse::default_down_sampling_value);
    }
}

void Ellipse::transform(const double mat[6])
{
    _a[0].transform(mat);
    _a[1].transform(mat);
    _b[0].transform(mat);
    _b[1].transform(mat);
    if (std::abs(mat[0]) == 1 && std::abs(mat[4]) == 1)
    {
        _shape.transform(mat);
    }
    else
    {
        update_shape(Geo::Ellipse::default_down_sampling_value);
    }
}

void Ellipse::translate(const double x, const double y)
{
    _a[0].translate(x, y);
    _a[1].translate(x, y);
    _b[0].translate(x, y);
    _b[1].translate(x, y);
    _shape.translate(x, y);
}

void Ellipse::rotate(const double x, const double y, const double rad)
{
    _a[0].rotate(x, y, rad);
    _a[1].rotate(x, y, rad);
    _b[0].rotate(x, y, rad);
    _b[1].rotate(x, y, rad);
    _shape.rotate(x, y, rad);
}

void Ellipse::scale(const double x, const double y, const double k)
{
    _a[0].scale(x, y, k);
    _a[1].scale(x, y, k);
    _b[0].scale(x, y, k);
    _b[1].scale(x, y, k);
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Polygon Ellipse::convex_hull() const
{
    const Geo::Point center = (_a[0] + _a[1] + _b[0] + _b[1]) / 4;
    const double a = Geo::distance(_a[0], _a[1]) / 2, b = Geo::distance(_b[0], _b[1]) / 2;
    const double left = center.x - a, top = center.y + b, right = center.x + a, bottom = center.y - b;
    Geo::Polygon polygon({Geo::Point(left, top), Geo::Point(right, top), Geo::Point(right, bottom), Geo::Point(left, bottom)});
    polygon.rotate(center.x, center.y, Geo::angle(_a[0], _a[1]));
    return polygon;
}

AABBRect Ellipse::bounding_rect() const
{
    const Geo::Point center = (_a[0] + _a[1] + _b[0] + _b[1]) / 4;
    const double aa = Geo::distance_square(_a[0], _a[1]) / 4, bb = Geo::distance_square(_b[0], _b[1]) / 4;
    const Geo::Vector vec = _a[0] - _a[1];
    const double cc = std::pow(vec.x, 2) / (std::pow(vec.x, 2) + std::pow(vec.y, 2));
    const double ss = 1 - cc;
    const double left = center.x - std::sqrt(aa * cc + bb * ss);
    const double top = center.y + std::sqrt(aa * ss + bb * cc);
    const double right = center.x + std::sqrt(aa * cc + bb * ss);
    const double bottom = center.y - std::sqrt(aa * ss + bb * cc);
    return AABBRect(left, top, right, bottom);
}

Polygon Ellipse::mini_bounding_rect() const
{
    const Geo::Point center = (_a[0] + _a[1] + _b[0] + _b[1]) / 4;
    const double a = Geo::distance(_a[0], _a[1]) / 2, b = Geo::distance(_b[0], _b[1]) / 2;
    const double left = center.x - a, top = center.y + b, right = center.x + a, bottom = center.y - b;
    Geo::Polygon polygon({Geo::Point(left, top), Geo::Point(right, top), Geo::Point(right, bottom), Geo::Point(left, bottom)});
    polygon.rotate(center.x, center.y, Geo::angle(_a[0], _a[1]));
    return polygon;
}

Ellipse Ellipse::operator+(const Point &point) const
{
    return Ellipse(_a[0] + point, _a[1] + point, _b[0] + point, _b[1] + point);
}

Ellipse Ellipse::operator-(const Point &point) const
{
    return Ellipse(_a[0] - point, _a[1] - point, _b[0] - point, _b[1] - point);
}

double Ellipse::lengtha() const
{
    return Geo::distance(_a[0], _a[1]) / 2;
}

double Ellipse::lengthb() const
{
    return Geo::distance(_b[0], _b[1]) / 2;
}

double Ellipse::angle() const
{
    double value = Geo::angle(_a[0], _a[1]);
    return value >= 0 ? value : value + Geo::PI;
}

Geo::Point Ellipse::center() const
{
    return (_a[0] + _a[1] + _b[0] + _b[1]) / 4;
}

void Ellipse::set_lengtha(const double a)
{
    if (_a[0] == _a[1])
    {
        if (_b[0] == _b[1])
        {
            _a[0].x -= a;
            _a[1].x += a;
        }
        else
        {
            const Geo::Vector vec((_b[0] - _b[1]).vertical().normalize());
            _a[0] -= (vec * a);
            _a[1] += (vec * a);
        } 
    }
    else
    {
        const Geo::Vector vec((_a[0] - _a[1]).normalize());
        const double l = a - lengtha();
        _a[0] += (vec * l);
        _a[1] -= (vec * l);
    }
}

void Ellipse::set_lengthb(const double b)
{
    if (_b[0] == _b[1])
    {
        if (_a[0] == _a[1])
        {
            _b[0].y += b;
            _b[1].y -= b;
        }
        else
        {
            const Geo::Vector vec((_a[0] - _a[1]).vertical().normalize());
            _b[0] += (vec * b);
            _b[1] -= (vec * b);
        }
    }
    else
    {
        const Geo::Vector vec((_b[0] - _b[1]).normalize());
        const double l = b - lengthb();
        _b[0] += (vec * l);
        _b[1] -= (vec * l);
    }
}

void Ellipse::set_center(const double x, const double y)
{
    const Geo::Point anchor = center();
    translate(x - anchor.x, y - anchor.y);
}

void Ellipse::reset_parameter(const Geo::Point &a0, const Geo::Point &a1, const Geo::Point &b0, const Geo::Point &b1)
{
    _a[0] = a0;
    _a[1] = a1;
    _b[0] = b0;
    _b[1] = b1;
}

void Ellipse::reset_parameter(const double parameters[8])
{
    _a[0].x = parameters[0], _a[0].y = parameters[1];
    _a[1].y = parameters[2], _a[1].y = parameters[3];
    _b[0].x = parameters[4], _b[0].y = parameters[5];
    _b[1].x = parameters[6], _b[1].y = parameters[7];
}

const Point &Ellipse::a0() const
{
    return _a[0];
}

const Point &Ellipse::a1() const
{
    return _a[1];
}

const Point &Ellipse::b0() const
{
    return _b[0];
}

const Point &Ellipse::b1() const
{
    return _b[1];
}

Point Ellipse::c0() const
{
    if (Geo::distance_square(_a[0], _a[1]) >= Geo::distance_square(_b[0], _b[1]))
    {
        const double aa = Geo::distance_square(_a[0], _a[1]);
        const double bb = Geo::distance_square(_b[0], _b[1]);
        return center() + (_a[0] - _a[1]).normalize() * std::sqrt(aa - bb) / 2;
    }
    else
    {
        const double aa = Geo::distance_square(_b[0], _b[1]);
        const double bb = Geo::distance_square(_a[0], _a[1]);
        return center() + (_b[0] - _b[1]).normalize() * std::sqrt(aa - bb) / 2;
    }
}

Point Ellipse::c1() const
{
    if (Geo::distance_square(_a[0], _a[1]) >= Geo::distance_square(_b[0], _b[1]))
    {
        const double aa = Geo::distance_square(_a[0], _a[1]);
        const double bb = Geo::distance_square(_b[0], _b[1]);
        return center() + (_a[1] - _a[0]).normalize() * std::sqrt(aa - bb) / 2;
    }
    else
    {
        const double aa = Geo::distance_square(_b[0], _b[1]);
        const double bb = Geo::distance_square(_a[0], _a[1]);
        return center() + (_b[1] - _b[0]).normalize() * std::sqrt(aa - bb) / 2;
    }
}

void Ellipse::update_shape(const double down_sampling_value)
{
    const Geo::Point point = center();
    _shape = Geo::ellipse_to_polygon(point.x, point.y, lengtha(), lengthb(), angle(), down_sampling_value);
    triangle_indices = Geo::ear_cut_to_indexs(_shape);
}

const Polygon &Ellipse::shape() const
{
    return _shape;
}


// BSpline
double BSpline::default_step = 0.2;
double BSpline::default_down_sampling_value = 0.02;

BSpline::BSpline()
{   
    _shape.shape_fixed = true;
}

BSpline::BSpline(const BSpline &bspline)
    : Geometry(bspline), _shape(bspline._shape), control_points(bspline.control_points), path_points(bspline.path_points)
{
    _shape.shape_fixed = true;
}

BSpline &BSpline::operator=(const BSpline &bspline)
{
    if (this != &bspline)
    {
        Geometry::operator=(bspline);
        _shape = bspline._shape;
        control_points = bspline.control_points;
        path_points = bspline.path_points;
    }
    return *this;
}

const Polyline &BSpline::shape() const
{
    return _shape;
}

const double BSpline::length() const
{
    return _shape.length();
}

const bool BSpline::empty() const
{
    return control_points.empty() && path_points.empty();
}

void BSpline::clear()
{
    _shape.clear();
    control_points.clear();
    path_points.clear();
}

void BSpline::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    _shape.transform(a, b, c, d, e, f);
    std::for_each(path_points.begin(), path_points.end(), [=](Point &point) { point.transform(a, b, c, d, e, f); });
    std::for_each(control_points.begin(), control_points.end(), [=](Point &point) { point.transform(a, b, c, d, e, f); });
}

void BSpline::transform(const double mat[6])
{
    _shape.transform(mat);
    std::for_each(path_points.begin(), path_points.end(), [=](Point &point) { point.transform(mat); });
    std::for_each(control_points.begin(), control_points.end(), [=](Point &point) { point.transform(mat); });
}

void BSpline::translate(const double tx, const double ty)
{
    _shape.translate(tx, ty);
    std::for_each(path_points.begin(), path_points.end(), [=](Point &point) { point.translate(tx, ty); });
    std::for_each(control_points.begin(), control_points.end(), [=](Point &point) { point.translate(tx, ty); });
}

void BSpline::rotate(const double x, const double y, const double rad)
{
    _shape.rotate(x, y, rad);
    std::for_each(path_points.begin(), path_points.end(), [=](Point &point) { point.rotate(x, y, rad); });
    std::for_each(control_points.begin(), control_points.end(), [=](Point &point) { point.rotate(x, y, rad); });
}

void BSpline::scale(const double x, const double y, const double k)
{
    _shape.scale(x, y, k);
    std::for_each(path_points.begin(), path_points.end(), [=](Point &point) { point.scale(x, y, k); });
    std::for_each(control_points.begin(), control_points.end(), [=](Point &point) { point.scale(x, y, k); });
}

Polygon BSpline::convex_hull() const
{
    return _shape.convex_hull();
}

AABBRect BSpline::bounding_rect() const
{
    return _shape.bounding_rect();
}

Polygon BSpline::mini_bounding_rect() const
{
    return _shape.mini_bounding_rect();
}

const Point &BSpline::front() const
{
    return _shape.front();
}

const Point &BSpline::back() const
{
    return _shape.back();
}

const std::vector<double> &BSpline::knots() const
{
    return _knots;
}


// QuadBSpline
QuadBSpline::QuadBSpline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(begin, end);
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        control_points.assign(begin, end);
        const size_t num = control_points.size();
        path_points.resize(num - 2);
        knot(num, _knots);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

QuadBSpline::QuadBSpline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const std::vector<double> &knots, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(begin, end);
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        _knots.assign(knots.begin(), knots.end());
        control_points.assign(begin, end);
        const size_t num = control_points.size();
        path_points.resize(num - 2);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

QuadBSpline::QuadBSpline(const std::initializer_list<Point> &points, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(points.begin(), points.end());
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        control_points.assign(points.begin(), points.end());
        const size_t num = control_points.size();
        path_points.resize(num - 2);
        knot(num, _knots);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

const Type QuadBSpline::type() const
{
    return Type::BSPLINE;
}

void QuadBSpline::update_control_points() // 从LibreCAD抄的
{
    control_points.clear();
    const size_t n = path_points.size();
    if(n < 4)
    {
        if(n > 0)
        {
            control_points.emplace_back(path_points[0]);
        }
        if(Point control; n > 2 && get_three_points_control(path_points[0], path_points[1], path_points[2], control))
        {
            control_points.emplace_back(control);
        }
        if(n > 1)
        {
            control_points.emplace_back(path_points[n - 1]);
        }
        return;
    }

    const int dim = n - 2;
    std::vector<double> dt(dim);
    {
        // 使用向心参数化算法,计算每段的参数分布(参数值在0-1之间),累积弦长法面对极端不均匀的分布时会受到很大干扰
        double dl1 = std::sqrt(Geo::distance(path_points[1], path_points[0]));
        double dl2 = std::sqrt(Geo::distance(path_points[2], path_points[1]));
        dt[0] = dl1 / (dl1 + dl2 / 2.0); 
        /* "dl2 / 2.0"是为了更平滑地处理二次B样条插值首段的参数化，使得首段的参数分布更自然，避免端点处曲线过于“尖锐”或“拉伸”。
        对于中间段，常用的弦长参数化是 dt[i] = dl1 / (dl1 + dl2)，这样每一段的参数分布与相邻两段的长度成比例。
        但对于首段，如果直接用 dt[0] = dl1 / (dl1 + dl2)，会导致首段的参数分布只考虑了前两段的长度，忽略了后续曲线的“延续性”，容易在端点产生不自然的曲率变化。
        让首段的参数分布不仅考虑 P0-P1 的长度，还适当考虑了 P1-P2 的影响，但权重减半（即只取一半）。
        这样做可以让首段的插值更平滑，首端的切线方向和曲率更自然，减少端点处的异常。*/
        for(int i = 1; i < dim - 1; ++i)
        {
            dl1 = dl2;
            dl2 = std::sqrt(Geo::distance(path_points[i + 2], path_points[i + 1]));
            dt[i] = dl1 / (dl1 + dl2);
        }
        dl1 = dl2;
        dl2 = std::sqrt(Geo::distance(path_points[dim], path_points[dim + 1]));
        dt[dim - 1] = dl1 / (dl1 + 2.0 * dl2);
        /* "dl1 + 2.0 * dl2"是为了让末段的参数分布更加平滑，兼顾末尾两段的长度，提升B样条曲线在末端的平滑性和自然性。
        对于中间段，常用弦长参数化是 dt[i] = dl1 / (dl1 + dl2)，即只考虑本段和下一段的长度。
        但对于末段，如果直接用 dt = dl1 / (dl1 + dl2)，只考虑了最后两段的长度，容易导致末端曲线的切线或曲率变化不自然，出现“尖角”或“拉伸”。
        让末段的参数分布更加“偏向”末端，末段的参数跨度更大，等价于让末端的影响力加倍。
        这样可以让末端的插值更平滑，末端的切线方向和曲率更自然，减少端点处的异常。*/
        /*首段用 dt[0] = dl1 / (dl1 + dl2 / 2.0)，末段用 dt[dim-1] = dl1 / (dl1 + 2.0 * dl2)，
        两者对称，都是为了让两端的参数分布更平滑，兼顾端点与相邻段的长度。*/
    }

    std::vector<double> matrix;
    get_matrix(n, dt, matrix); // 计算出平方根分解后的矩阵
    if (matrix.empty())
    {
        return;
    }

    double *dx = new double[dim];
    double *dy = new double[dim];
    double *dx2 = new double[dim];
    double *dy2 = new double[dim];
    {
        double *pdDiag = matrix.data();
        double *pdDiag1 = &matrix[n - 2];
        double *pdDiag2 = &matrix[2 * n - 5];

        // 由 Ly = b 求解y
        dx[0] = (path_points[1].x - path_points[0].x * std::pow(1.0 - dt[0], 2)) / pdDiag[0];
        dy[0] = (path_points[1].y - path_points[0].y * std::pow(1.0 - dt[0], 2)) / pdDiag[0];
        for (int i = 1; i < dim - 1; i++)
        {
            dx[i] = (path_points[i + 1].x - pdDiag2[i - 1] * dx[i - 1]) / pdDiag[i];
            dy[i] = (path_points[i + 1].y - pdDiag2[i - 1] * dy[i - 1]) / pdDiag[i];
        }
        dx[dim - 1] = ((path_points[dim].x - path_points[dim + 1].x * std::pow(dt[n - 3], 2)) -
                        pdDiag2[dim - 2] * dx[dim - 2]) / pdDiag[dim - 1];
        dy[dim - 1] = ((path_points[dim].y - path_points[dim + 1].y * std::pow(dt[n - 3], 2)) -
                        pdDiag2[dim - 2] * dy[dim - 2]) /  pdDiag[dim - 1];

        // 由 Trans(L)x = y 求解x
        dx2[dim - 1] = dx[dim - 1] / pdDiag[dim - 1];
        dy2[dim - 1] = dy[dim - 1] / pdDiag[dim - 1];
        for (int i = dim - 2; i >= 0; i--)
        {
            dx2[i] = (dx[i] - pdDiag1[i] * dx2[i + 1]) / pdDiag[i];
            dy2[i] = (dy[i] - pdDiag1[i] * dy2[i + 1]) / pdDiag[i];
        }

        control_points.emplace_back(path_points[0]);
        for (int i = 0; i < dim; i++)
        {
            control_points.emplace_back(dx2[i], dy2[i]);
        }
        control_points.emplace_back(path_points[n - 1]);
    }
    delete[] dx;
    delete[] dy;
    delete[] dx2;
    delete[] dy2;
}

bool QuadBSpline::get_three_points_control(const Point &point0, const Point &point1, const Point &point2, Point &output)  // 从LibreCAD抄的
{
    double dl1 = Geo::distance(point1, point0);
	double dl2 = Geo::distance(point2, point1);
	double dt = dl1 / (dl1 + dl2);
    if (dt < 1e-10 || dt > 1.0 - 1e-10)
    {
        return false;
    }
    else
    {
        output.x = (point1.x - point0.x * std::pow(1.0 - dt, 2) - point2.x * std::pow(dt, 2)) / (dt * (1 - dt) * 2);
        output.y = (point1.y - point0.y * std::pow(1.0 - dt, 2) - point2.y * std::pow(dt, 2)) / (dt * (1 - dt) * 2);
        return true;
    }
}

void QuadBSpline::get_matrix(const size_t count, const std::vector<double>& dt, std::vector<double> &output)  // 从LibreCAD抄的
{
    if(count < 4 || dt.size() != count - 2)
    {
        return;
    }

    size_t dim = 3 * count - 8; // 主对角线元素(n-2) + 上对角线元素(n-3) + 下对角线元素(n-3)
    double *res = new double[dim]; // 三对角矩阵
    {
        double *pdDiag = res; // 主对角线元素
        double *pdDiag1 = &res[count - 2]; // 上对角线元素
        double *pdDiag2 = &res[2 * count - 5]; // 下对角线元素

        double x3 = std::pow(dt[0], 2) / 2.0; // 首部点使用非均匀B样条曲线公式计算
        double x2 = 2.0 * dt[0] * (1.0 - dt[0]) + x3;
        pdDiag[0] = std::sqrt(x2); // L矩阵主对角线第一个元素,使用平方根分解而非追赶分解
        pdDiag1[0] = x3 / pdDiag[0]; // U矩阵上对角线第一个元素

        for(size_t i = 1; i < count - 3; ++i)
        {
            double x1 = std::pow(1.0 - dt[i], 2) / 2.0;
            x3 = std::pow(dt[i], 2) / 2.0; // 除去首尾两点,中间部分的点按照均匀B样条曲线公式计算
            x2 = x1 + 2.0 * dt[i] * (1.0 - dt[i]) + x3;

            pdDiag2[i - 1] = x1 / pdDiag[i - 1]; // L矩阵下对角线元素
            pdDiag[i] = std::sqrt(x2 - pdDiag1[i - 1] * pdDiag2[i - 1]); // L矩阵主对角线元素,使用平方根分解而非追赶分解
            pdDiag1[i] = x3 / pdDiag[i]; // U矩阵上对角线元素
        }

        double x1 = std::pow(1.0 - dt[count - 3], 2) / 2.0; // 尾部点使用非均匀B样条曲线公式计算
        x2 = x1 + 2.0 * dt[count - 3] * (1.0 - dt[count - 3]);
        pdDiag2[count - 4] = x1 / pdDiag[count - 4]; // U矩阵下对角线最后一个元素
        pdDiag[count - 3] = std::sqrt(x2 - pdDiag1[count - 4] * pdDiag2[count - 4]); // L矩阵主对角线最后一个元素,使用平方根分解而非追赶分解
    }
    output.assign(res, res + dim);
    delete[] res;
}

void QuadBSpline::knot(const size_t num, std::vector<double> &output)  // 从LibreCAD抄的
{
    output.resize(num + 3, 0);
	//use uniform knots
	std::iota(output.begin() + 3, output.begin() + num + 1, 1);
	std::fill(output.begin() + num + 1, output.end(), output[num]);
}

void QuadBSpline::rbasis(const double t, const int npts, const std::vector<double> &x, std::vector<double> &output)  // 从LibreCAD抄的
{
    const int nplusc = npts + 3;
    std::vector<double> temp(nplusc, 0);
    // calculate the first order nonrational basis functions n[i]
    for (int i = 0; i< nplusc - 1; ++i)
    {
        if ((t >= x[i]) && (t < x[i+1]))
        {
            temp[i] = 1;
        }
    }

    /* calculate the higher order nonrational basis functions */

    for (int k = 2; k <= 3; ++k)
    {
        for (int i = 0; i < nplusc - k; ++i)
        {
            // if the lower order basis function is zero skip the calculation
            if (temp[i] != 0)
            {
                temp[i] = ((t - x[i]) * temp[i]) / (x[i + k - 1] - x[i]);
            }
            // if the lower order basis function is zero skip the calculation
            if (temp[i + 1] != 0)
            {
                temp[i] += ((x[i + k] - t) * temp[i + 1]) / (x[i + k] - x[i + 1]);
            }
        }
    }

    // pick up last point
    if (t >= x[nplusc - 1])
    {
        temp[npts-1] = 1;
    }

    // calculate sum for denominator of rational basis functions
    double sum = 0;
    for (int i = 0; i < npts; ++i)
    {
        sum += temp[i];
    }

    output.resize(npts, 0);
    // form rational basis functions and put in r vector
    if (sum != 0)
    {
        for (int i = 0; i < npts; ++i)
        {
            output[i] = temp[i] / sum;
        }
    }
}

void QuadBSpline::rbspline(const size_t npts, const size_t p1, const std::vector<double> &knots, const std::vector<Point>& b, std::vector<Point>& p)  // 从LibreCAD抄的
{
    const size_t nplusc = npts + 3;
    // calculate the points on the rational B-spline curve
    double t = knots[0];
    const double step = (knots[nplusc - 1] - t) / (p1 - 1);

    for (Geo::Point &vp: p)
    {
        if (knots[nplusc - 1] - t < 5e-6)
        {
            t = knots[nplusc-1];
        }
        // generate the basis function for this value of t
        std::vector<double> nbasis;
        rbasis(t, npts, knots, nbasis);
        // generate a point on the curve
		for (size_t i = 0; i < npts; ++i)
        {
            vp += b[i] * nbasis[i];
        }
        t += step;
    }
}

void QuadBSpline::update_shape(const double step, const double down_sampling_value)
{
    const size_t npts = control_points.size();

    double length = 0;
    for (size_t i = 1, count = control_points.size(); i < count; ++i)
    {
        length += Geo::distance(control_points[i - 1], control_points[i]);
    }

    // resolution:
    const size_t points_count = std::max(npts * 8.0, length / step);

    std::vector<Point> points(points_count, Point(0, 0));
    rbspline(npts, points_count, _knots, control_points, points);

    _shape.clear();
    _shape.append(path_points.empty() ? control_points.front() : path_points.front());
    _shape.append(points.begin(), points.end());
    _shape.append(path_points.empty() ? control_points.back() : path_points.back());
    Geo::down_sampling(_shape, down_sampling_value);
}

QuadBSpline *QuadBSpline::clone() const
{
    return new QuadBSpline(*this);
}


// CubicBSpline
CubicBSpline::CubicBSpline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(begin, end);
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
    }
    else
    {
        control_points.assign(begin, end);
        const size_t num = control_points.size();
        _knots.resize(num + 4, 0);
        std::iota(_knots.begin() + 4, _knots.begin() + num + 1, 1);
        std::fill(_knots.begin() + num + 1, _knots.end(), _knots[num]);
        path_points.resize(num - 2);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

CubicBSpline::CubicBSpline(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const std::vector<double> &knots, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(begin, end);
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
    }
    else
    {
        _knots.assign(knots.begin(), knots.end());
        control_points.assign(begin, end);
        const size_t num = control_points.size();
        path_points.resize(num - 2);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

CubicBSpline::CubicBSpline(const std::initializer_list<Point> &points, const bool is_path_points)
{
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        path_points.assign(points.begin(), points.end());
        for (size_t i = path_points.size() - 1; i > 1; --i)
        {
            if (path_points[i] == path_points[i - 1])
            {
                path_points.erase(path_points.begin() + i);
            }
        }
        update_control_points();
    }
    else
    {
        control_points.assign(points.begin(), points.end());
        const size_t num = control_points.size();
        _knots.resize(num + 4, 0);
        std::iota(_knots.begin() + 4, _knots.begin() + num + 1, 1);
        std::fill(_knots.begin() + num + 1, _knots.end(), _knots[num]);
        path_points.resize(num - 2);
        rbspline(num, num - 2, _knots, control_points, path_points);
    }
    update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

const Type CubicBSpline::type() const
{
    return Geo::Type::BSPLINE;
}

void CubicBSpline::update_control_points()
{
    control_points.clear();
    const size_t n = path_points.size();
    if (n < 3)
    {
        if(n > 0)
        {
            control_points.emplace_back(path_points.front());
        }
        if(n > 1)
        {
            control_points.emplace_back(path_points.back());
        }
        return;
    }

    double *delta = new double[n + 5];
    {
        _knots.assign(n + 6, 0);
        double *l = new double[n - 1];
        double l1 = 0;
        for (size_t i = 0; i < n - 1; ++i)
        {
            l[i] = std::sqrt(Geo::distance(path_points[i + 1], path_points[i]));
            l1 += l[i];
        }
        for (size_t i = 0; i <= 3; ++i)
        {
            _knots[i] = 0;
            _knots[i + n + 2] = 1;
        }
        for (size_t i = 4; i < n + 2; ++i)
        {   
            double l2 = 0;
            for (size_t j = 0; j < i - 3; ++j)
            {
                l2 += l[j];
            }
            _knots[i] = l2 / l1;
        }
        delete[] l;

        for (size_t i = 0; i < n + 5; ++i)
        {
            delta[i] = _knots[i + 1] - _knots[i];
        }
    }

    double *a = new double[n];
    double *b = new double[n];
    double *c = new double[n];
    double *e = new double[n];
    double *f = new double[n];
    // 抛物线条件
    a[0] = 1 - delta[3] * delta[4] / std::pow(delta[3] + delta[4], 2);
    b[0] = delta[3] / (delta[3] + delta[4]) * (delta[4] / (delta[3] + delta[4]) - delta[3] / (delta[3] + delta[4] + delta[5]));
    c[0] = std::pow(delta[3], 2) / ((delta[3] + delta[4]) * (delta[3] + delta[4] + delta[5]));
    e[0] = (path_points[0].x + 2 * path_points[1].x) / 3;
    f[0] = (path_points[0].y + 2 * path_points[1].y) / 3;
    a[n - 1] = -std::pow(delta[n + 1], 2) / ((delta[n] + delta[n + 1]) * (delta[n] + delta[n] + delta[n + 1]));
    b[n - 1] = delta[n + 1] / (delta[n] + delta[n + 1]) * (delta[n + 1] / (delta[n] + delta[n] + delta[n + 1]) - delta[n] / (delta[n] + delta[n + 1]));
    c[n - 1] = delta[n] * delta[n + 1] / std::pow(delta[n] + delta[n + 1], 2) - 1;
    e[n - 1] = -(path_points[n - 1].x + 2 * path_points[n - 2].x) / 3;
    f[n - 1] = -(path_points[n - 1].y + 2 * path_points[n - 2].y) / 3;
    for (size_t i = 1; i < n - 1; ++i)
    {
        a[i] = std::pow(delta[i + 3], 2) / (delta[i + 1] + delta[i + 2] + delta[i + 3]);
        b[i] = delta[i + 3] * (delta[i + 1] + delta[i + 2]) / (delta[i + 1] + delta[i + 2] + delta[i + 3])
            + delta[i + 2] * (delta[i + 3] + delta[i + 4]) / (delta[i + 2] + delta[i + 3] + delta[i + 4]);
        c[i] = std::pow(delta[i + 2], 2) / (delta[i + 2] + delta[i + 3] + delta[i + 4]);
        e[i] = (delta[i + 2] + delta[i + 3]) * path_points[i].x;
        f[i] = (delta[i + 2] + delta[i + 3]) * path_points[i].y;
    }
    delete[] delta;
    double *matrix = new double[n * n];
    std::fill_n(matrix, n * n, 0);
    matrix[0] = a[0];
    matrix[1] = b[0];
    matrix[2] = c[0];
    matrix[n * n - 3] = a[n - 1];
    matrix[n * n - 2] = b[n - 1];
    matrix[n * n - 1] = c[n - 1];
    for (size_t i = 1; i < n - 1; ++i)
    {
        matrix[i * n + i - 1] = a[i];
        matrix[i * n + i] = b[i];
        matrix[i * n + i + 1] = c[i];
    }
    delete[] a;
    delete[] b;
    delete[] c;

    double *x = new double[n];
    double *y = new double[n];
    Math::solve(matrix, n, e, x);
    Math::solve(matrix, n, f, y);
    delete[] matrix;
    delete[] e;
    delete[] f;
    control_points.emplace_back(path_points.front());
    for (size_t i = 0; i < n; ++i)
    {
        control_points.emplace_back(x[i], y[i]);
    }
    control_points.emplace_back(path_points.back());
    delete[] x;
    delete[] y;
}

void CubicBSpline::rbasis(const double t, const int npts, const std::vector<double> &x, std::vector<double> &output)
{
    const int nplusc = npts + 4;
    std::vector<double> temp(nplusc, 0);
    // calculate the first order nonrational basis functions n[i]
    for (int i = 0; i< nplusc - 1; ++i)
    {
        if ((t >= x[i]) && (t < x[i+1]))
        {
            temp[i] = 1;
        }
    }

    /* calculate the higher order nonrational basis functions */

    for (int k = 2; k <= 4; ++k)
    {
        for (int i = 0; i < nplusc - k; ++i)
        {
            // if the lower order basis function is zero skip the calculation
            if (temp[i] != 0)
            {
                temp[i] = ((t - x[i]) * temp[i]) / (x[i + k - 1] - x[i]);
            }
            // if the lower order basis function is zero skip the calculation
            if (temp[i + 1] != 0)
            {
                temp[i] += ((x[i + k] - t) * temp[i + 1]) / (x[i + k] - x[i + 1]);
            }
        }
    }

    // pick up last point
    if (t >= x[nplusc - 1])
    {
        temp[npts-1] = 1;
    }

    // calculate sum for denominator of rational basis functions
    double sum = 0;
    for (int i = 0; i < npts; ++i)
    {
        sum += temp[i];
    }

    output.resize(npts, 0);
    // form rational basis functions and put in r vector
    if (sum != 0)
    {
        for (int i = 0; i < npts; ++i)
        {
            output[i] = temp[i] / sum;
        }
    }
}

void CubicBSpline::rbspline(const size_t npts, const size_t p1, const std::vector<double> &knots, const std::vector<Point> &b, std::vector<Point> &p)
{
    const size_t nplusc = npts + 4;
    // calculate the points on the rational B-spline curve
    double t = knots[0];
    const double step = (knots[nplusc - 1] - t) / (p1 - 1);

    for (Geo::Point &vp: p)
    {
        if (knots[nplusc - 1] - t < 5e-6)
        {
            t = knots[nplusc-1];
        }
        // generate the basis function for this value of t
        std::vector<double> nbasis;
        rbasis(t, npts, knots, nbasis);
        // generate a point on the curve
		for (size_t i = 0; i < npts; ++i)
        {
            vp += b[i] * nbasis[i];
        }
        t += step;
    }
}

void CubicBSpline::update_shape(const double step, const double down_sampling_value)
{
    const size_t npts = control_points.size();

    if (npts == 2)
    {
        _shape.clear();
        _shape.append(control_points.front());
        _shape.append(control_points.back());
        return;
    }

    double length = 0;
    for (size_t i = 1, count = control_points.size(); i < count; ++i)
    {
        length += Geo::distance(control_points[i - 1], control_points[i]);
    }

    // resolution:
    const size_t points_count = std::max(npts * 8.0, length / step);

    std::vector<Point> points(points_count, Point(0, 0));
    rbspline(npts, points_count, _knots, control_points, points);

    _shape.clear();
    _shape.append(path_points.empty() ? control_points.front() : path_points.front());
    _shape.append(points.begin(), points.end());
    _shape.append(path_points.empty() ? control_points.back() : path_points.back());
    Geo::down_sampling(_shape, down_sampling_value);
}

CubicBSpline *CubicBSpline::clone() const
{
    return new CubicBSpline(*this);
}