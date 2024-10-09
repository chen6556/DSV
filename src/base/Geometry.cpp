#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "base/Algorithm.hpp"


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
    : Geometry(rect), _points(rect._points)
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
    : Polyline(polygon)
{}

Polygon::Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
    : Polyline(begin, end)
{
    assert(size() >= 3);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const std::initializer_list<Point>& points)
    : Polyline(points)
{
    assert(size() > 2);
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const Polyline &polyline)
    : Polyline(polyline)
{
    if (_points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const AABBRect& rect)
    : Polyline(rect.cbegin(), rect.cend())
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
        _points.front() = _points.back();
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
    : Geometry(triangle)
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
    : Point(x, y), radius(r)
{
    assert(r > 0);
}

Circle::Circle(const Point &point, const double r)
    : Point(point), radius(r)
{
    assert(r > 0);
}

Circle::Circle(const Circle &circle)
    : Point(circle), radius(circle.radius)
{}

Circle &Circle::operator=(const Circle &circle)
{
    if (this != &circle)
    {
        Point::operator=(circle);
        radius = circle.radius;
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
}

void Circle::transform(const double mat[6])
{
    Point::transform(mat);
    radius *= std::abs(mat[0]);
}

void Circle::scale(const double x, const double y, const double k)
{
    Point::scale(x, y, k);
    radius *= k;
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

size_t Bezier::order() const
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

