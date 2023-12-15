#include "base/Geometry.hpp"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cfloat>


using namespace Geo;

Geometry::Geometry(const Geometry &geo)
    :_memo(geo._memo), _shape_fixed(geo._shape_fixed)
{}

Geometry &Geometry::operator=(const Geometry &geo)
{
    if (this != &geo)
    {
        _memo = geo._memo;
        _shape_fixed = geo._shape_fixed;
    }
    return *this;
}

Memo &Geometry::memo()
{
    return _memo;
}

const Memo &Geometry::memo() const
{
    return _memo;
}

bool &Geometry::shape_fixed()
{
    return _shape_fixed;
}

const bool Geometry::shape_fixed() const
{
    return _shape_fixed;
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

AxisAlignedBoundingBox Geometry::bounding_box() const { return AABB(); }

// Coord

Coord::Coord(const double x_, const double y_)
    : x(x_), y(y_) {}

Coord::Coord(const Coord &coord)
    : x(coord.x), y(coord.y) {}

Coord::Coord(const Coord &&coord)
    : x(std::move(coord.x)),
    y(std::move(coord.y)) {}

Coord &Coord::operator=(const Coord &coord)
{
    if (this != &coord)
    {
        x = coord.x;
        y = coord.y;
    }
    return *this;
}

Coord &Coord::operator=(const Coord &&coord)
{
    if (this != &coord)
    {
        x = std::move(coord.x);
        y = std::move(coord.y);
    }
    return *this;
}

const bool Coord::operator==(const Coord &coord) const
{
    return x == coord.x && y == coord.y;
}

const bool Coord::operator!=(const Coord &coord) const
{
    return x != coord.x || y != coord.y;
}

// Point

Point::Point(const double x, const double y)
    : _pos(x, y)
{
    _memo["Type"] = 10;
}

Point::Point(const Coord &coord)
    :_pos(coord)
{
    _memo["Type"] = 10;
}

Point::Point(const Point &point)
    :Geometry(point)
    ,_pos(point._pos)
{}

Point::Point(const Point &&point)
    :Geometry(point)
    ,_pos(std::move(point._pos))
{}

Point &Point::operator=(const Point &point)
{
    if (this != &point)
    {
        Geometry::operator=(point);
        _pos = point._pos;
    }
    return *this;
}

Point &Point::operator=(const Point &&point)
{
    if (this != &point)
    {
        Geometry::operator=(point);
        _pos = std::move(point._pos);
    }
    return *this;
}

Coord &Point::coord()
{
    return _pos;
}

const Coord &Point::coord() const
{
    return _pos;
}

const bool Point::operator==(const Point &point) const
{
    return _pos == point._pos;
}

const bool Point::operator!=(const Point &point) const
{
    return _pos != point._pos;
}

const Point &Point::normalize()
{
    const double len = length();
    _pos.x /= len;
    _pos.y /= len;
    return *this;
}

Point Point::normalized() const
{
    const double len = length();
    return Point(_pos.x / len, _pos.y / len);
}

const double Point::length() const
{
    return std::sqrt(_pos.x * _pos.x + _pos.y * _pos.y);
}

const bool Point::empty() const
{
    return _pos.x == 0 && _pos.y == 0;
}

void Point::clear()
{
    _pos.x = 0;
    _pos.y = 0;
}

Point *Point::clone() const
{
    return new Point(*this);
}

void Point::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    const double x = _pos.x, y = _pos.y;
    _pos.x = a * x + b * y + c;
    _pos.y = d * x + e * y + f;
}

void Point::transform(const double mat[6])
{
    transform(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
}

void Point::translate(const double tx, const double ty)
{
    _pos.x += tx;
    _pos.y += ty;
}

void Point::rotate(const double x, const double y, const double rad)
{
    _pos.x -= x;
    _pos.y -= y;
    const double x_ = _pos.x, y_ = _pos.y;
    _pos.x = x_ * std::cos(rad) - y_ * std::sin(rad);
    _pos.y = x_ * std::sin(rad) + y_ * std::cos(rad);
    _pos.x += x;
    _pos.y += y;
}

void Point::scale(const double x, const double y, const double k)
{
    const double x_ = _pos.x, y_ = _pos.y;
    _pos.x = k * x_ + x * (1 - k);
    _pos.y = k * y_ + y * (1 - k);
}

AxisAlignedBoundingBox Point::bounding_box() const
{
    if (length() == 0)
    {
        return AABB();
    }
    else
    {
        return AABB(std::min(0.0, _pos.x), std::min(0.0, _pos.y), std::max(0.0, _pos.x), std::max(0.0, _pos.y));
    }
}

Point Point::operator*(const double k) const
{
    return Point(_pos.x * k, _pos.y * k);
}

Point Point::operator+(const Point &point) const
{
    return Point(_pos.x + point._pos.x, _pos.y + point._pos.y);
}

Point Point::operator-(const Point &point) const
{
    return Point(_pos.x - point._pos.x, _pos.y - point._pos.y);
}

Point Point::operator/(const double k) const
{
    return Point(_pos.x / k, _pos.y / k);
}

void Point::operator*=(const double k)
{
    _pos.x *= k;
    _pos.y *= k;
}

void Point::operator+=(const Point &point)
{
    _pos.x += point._pos.x;
    _pos.y += point._pos.y;
}

void Point::operator-=(const Point &point)
{
    _pos.x -= point._pos.x;
    _pos.y -= point._pos.y;
}

void Point::operator/=(const double k)
{
    _pos.x /= k;
    _pos.y /= k;
}

// Polyline

Polyline::Polyline(const Polyline &polyline)
    :Geometry(polyline)
    ,_points(polyline._points)
{
    _memo["Type"] = 20;
}

Polyline::Polyline(const Polyline &&polyline)
    :Geometry(polyline)
    ,_points(std::move(polyline._points))
{
    _memo["Type"] = 20;
}

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
    _memo["Type"] = 20;
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
    _memo["Type"] = 20;
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
        _memo["Type"] = 20;
    }
    return *this;
}

Polyline &Polyline::operator=(const Polyline &&polyline)
{
    if (this != &polyline)
    {
        Geometry::operator=(polyline);
        _points = std::move(polyline._points);
        _memo["Type"] = 20;
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

Point Polyline::pop(const size_t index)
{
    assert(index < _points.size());
    Point point(std::move(_points[index]));
    _points.erase(_points.begin() + index);
    return point;
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
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.transform(a,b,c,d,e,f);});
}

void Polyline::transform(const double mat[6])
{
    transform(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
}

void Polyline::translate(const double tx, const double ty)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.translate(tx, ty);});
}

void Polyline::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.rotate(x, y, rad);});
}

void Polyline::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.scale(x, y, k);});
}

Polygon Polyline::convex_hull() const
{
    std::vector<Point> points(_points);
    std::sort(points.begin(), points.end(), [](const Point &a, const Point &b)
        {return a.coord().y < b.coord().y;});
    const Point origin(points.front());
    std::for_each(points.begin(), points.end(), [&](Point &p){p -= origin;});
    std::sort(points.begin() + 1, points.end(), [](const Point &a, const Point &b)
        {
            if (a.coord().x / a.length() != b.coord().x / b.length())
            {
                return a.coord().x / a.length() > b.coord().x / b.length();
            }
            else
            {
                return a.length() < b.length();
            }
        });
    std::for_each(points.begin(), points.end(), [&](Point &p){p += origin;});

    std::vector<Point> hull(points.begin(), points.begin() + 2);
    size_t count = hull.size(), index = 0;
    Geo::Vector vec0, vec1;
    std::vector<bool> used(points.size(), false);
    for (size_t i = 2, end = points.size(); i < end; ++i)
    {        
        vec0 = hull.back() - hull[count - 2];
        vec1 = vec0 + points[i] - hull.back();
        while (count >= 2 && vec0.coord().x * vec1.coord().y - vec1.coord().x * vec0.coord().y < 0)
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
        while (count >= 2 && vec0.coord().x * vec1.coord().y - vec1.coord().x * vec0.coord().y < 0)
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
    if (count >= 2 && vec0.coord().x * vec1.coord().y - vec0.coord().y * vec1.coord().x < 0)
    {
        hull.pop_back();
    }
    
    hull.emplace_back(points.front());    
    return Polygon(hull.cbegin(), hull.cend());
}

AxisAlignedBoundingBox Polyline::bounding_box() const
{
    if (_points.empty())
    {
        return AABB();
    }
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Point &point : _points)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    return AABB(x0, y0, x1, y1);
}

// Rectangle

Rectangle::Rectangle(const double x0, const double y0, const double x1, const double y1)
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
    _memo["Type"] = 30;
}

Rectangle::Rectangle(const Point &point0, const Point &point1)
{
    const double x0 = point0.coord().x, y0 = point0.coord().y, x1 = point1.coord().x, y1 = point1.coord().y;
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
    _memo["Type"] = 30;
}

Rectangle::Rectangle(const Rectangle &rect)
    :Geometry(rect)
    ,_points(rect._points)
{
    _memo["Type"] = 30;
}

Rectangle::Rectangle(const Rectangle &&rect)
    :Geometry(rect)
    ,_points(std::move(rect._points))
{
    _memo["Type"] = 30;
}

const double Rectangle::left() const
{
    assert(!_points.empty());
    return _points.front().coord().x;
}

const double Rectangle::top() const
{
    assert(!_points.empty());
    return _points.front().coord().y;
}

const double Rectangle::right() const
{
    assert(!_points.empty());
    return _points[2].coord().x;
}

const double Rectangle::bottom() const
{
    assert(!_points.empty());
    return _points[2].coord().y;
}

Rectangle &Rectangle::operator=(const Rectangle &rect)
{
    if (this != &rect)
    {
        Geometry::operator=(rect);
        _points = rect._points;
        _memo["Type"] = 30;
    }
    return *this;
}

Rectangle &Rectangle::operator=(const Rectangle &&rect)
{
    if (this != &rect)
    {
        Geometry::operator=(rect);
        _points = std::move(rect._points);
        _memo["Type"] = 30;
    }
    return *this;
}

const bool Rectangle::empty() const
{
    return _points.empty();
}

const double Rectangle::length() const
{
    double reuslt = 0;
    for (size_t i = 1, count = _points.size(); i < count; ++i)
    {
        reuslt += Geo::distance(_points[i], _points[i-1]);
    }
    return reuslt;
}

void Rectangle::clear()
{
    _points.clear();
}

Rectangle *Rectangle::clone() const
{
    return new Rectangle(*this);
}

const double Rectangle::area() const
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

const double Rectangle::width() const
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

const double Rectangle::height() const
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

void Rectangle::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.transform(a,b,c,d,e,f);});
}

void Rectangle::transform(const double mat[6])
{
    transform(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
}
    
void Rectangle::translate(const double tx, const double ty)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.translate(tx, ty);});
}

void Rectangle::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.rotate(x, y, rad);});
}

void Rectangle::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [&](Point &point){point.scale(x, y, k);});
}

Polygon Rectangle::convex_hull() const
{
    return Polygon(_points.cbegin(), _points.cend());
}

AxisAlignedBoundingBox Rectangle::bounding_box() const
{
    if (_points.empty())
    {
        return AABB();
    }
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Point &point : _points)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    return AABB(x0, y0, x1, y1);
}

std::vector<Point>::const_iterator Rectangle::begin() const
{
    return _points.cbegin();
}

std::vector<Point>::const_iterator Rectangle::cbegin() const
{
    return _points.cbegin();
}

std::vector<Point>::const_iterator Rectangle::end() const
{
    return _points.cend();
}

std::vector<Point>::const_iterator Rectangle::cend() const
{
    return _points.cend();
}

std::vector<Point>::const_reverse_iterator Rectangle::rbegin() const
{
    return _points.crbegin();
}

std::vector<Point>::const_reverse_iterator Rectangle::crbegin() const
{
    return _points.crbegin();
}

std::vector<Point>::const_reverse_iterator Rectangle::rend() const
{
    return _points.crend();
}

std::vector<Point>::const_reverse_iterator Rectangle::crend() const
{
    return _points.crend();
}

std::vector<Point>::const_iterator Rectangle::find(const Point &point) const
{
    return std::find(_points.cbegin(), _points.cend(), point);
}

Rectangle Rectangle::operator+(const Point &point) const
{
    return Rectangle(_points[0].coord().x + point.coord().x, _points[0].coord().y + point.coord().y,
                    _points[2].coord().x + point.coord().x, _points[2].coord().y + point.coord().y);
}

Rectangle Rectangle::operator-(const Point &point) const
{
    return Rectangle(_points[0].coord().x - point.coord().x, _points[0].coord().y - point.coord().y,
                    _points[2].coord().x - point.coord().x, _points[2].coord().y - point.coord().y);
}

void Rectangle::operator+=(const Point &point)
{
    for (Point &p : _points)
    {
        p += point;
    }
}

void Rectangle::operator-=(const Point &point)
{
    for (Point &p : _points)
    {
        p -= point;
    }
}

const Point Rectangle::center() const
{
    return (_points[0] + _points[2]) / 2;
}

const Point &Rectangle::operator[](const size_t index) const
{
    assert(!_points.empty() && index <= 4);
    return _points[index];
}


// Polygon

Polygon::Polygon(const Polygon &polygon)
    :Polyline(polygon)
{
    _memo["Type"] = 40; 
}

Polygon::Polygon(const Polygon &&polygon)
    :Polyline(std::move(polygon))
{
    _memo["Type"] = 40; 
}

Polygon::Polygon(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
    :Polyline(begin, end)
{
    assert(size() >= 3);
    if (back() != front())
    {
        Polyline::append(front());
    }
    _memo["Type"] = 40; 
}

Polygon::Polygon(const std::initializer_list<Point>& points)
    :Polyline(points)
{
    assert(size() > 2);
    if (back() != front())
    {
        Polyline::append(front());
    }
    _memo["Type"] = 40; 
}

Polygon::Polygon(const Polyline &polyline)
    :Polyline(polyline)
{
    if (polyline.back() != polyline.front())
    {
        Polyline::append(polyline.front());
    }
    _memo["Type"] = 40; 
}

Polygon::Polygon(const Rectangle& rect)
    :Polyline(rect.cbegin(), rect.cend())
{
    _memo = rect.memo();
    _memo["Type"] = 40; 
}

Polygon &Polygon::operator=(const Polygon &polygon)
{
    if (this != &polygon)
    {
        Polyline::operator=(polygon);
        _memo["Type"] = 40;
    }
    return *this;
}

Polygon &Polygon::operator=(const Polygon &&polygon)
{
    if (this != &polygon)
    {
        Polyline::operator=(std::move(polygon));
        _memo["Type"] = 40;
    }
    return *this;
}

Polygon *Polygon::clone() const
{
    return new Polygon(*this);
}

void Polygon::append(const Point &point)
{
    if (empty())
    {
        Polyline::append(point);
    }
    else
    {
        Polyline::insert(size() - 1, point);
    }
}

void Polygon::append(const Polyline &polyline)
{
    if (empty())
    {
        Polyline::append(polyline);
        if (polyline.front() != polyline.back())
        {
            Polyline::append(polyline.front());
        }
    }
    else
    {
        Polyline::insert(size() - 1, polyline);
    }
}

void Polygon::append(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    if (empty())
    {
        Polyline::append(begin, end);
        if (front() != back())
        {
            Polyline::append(front());
        }
    }
    else
    {
        Polyline::insert(size() - 1, begin, end);
    }
}

void Polygon::insert(const size_t index, const Point &point)
{
    assert(index < size() - 1);
    Polyline::insert(index, point);
    if (index == 0)
    {
        Polyline::remove(size() - 1);
        Polyline::append(front());   
    }
}

void Polygon::insert(const size_t index, const Polyline &polyline)
{
    assert(index < size() - 1);
    Polyline::insert(index, polyline);
    if (index == 0)
    {
        Polyline::remove(size() - 1);
        Polyline::append(front());   
    }
}

void Polygon::insert(const size_t index, std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end)
{
    assert(index < size() - 1);
    Polyline::insert(index, begin, end);
    if (index == 0)
    {
        Polyline::remove(size() - 1);
        Polyline::append(front());   
    }
}

void Polygon::remove(const size_t index)
{
    assert(index < size() - 1);
    Polyline::remove(index);
}

Point Polygon::pop(const size_t index)
{
    assert(index < size() - 1);
    return Polyline::pop(index);
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
    for (Point &p : *this)
    {
        p += point;
    }
}

void Polygon::operator-=(const Point &point)
{
    for (Point &p : *this)
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
    for (size_t i = 1, count = size(); i < count; ++i)
    {
        result += (operator[](i).coord().x * (operator[](i+1 != count ? i+1 : 0).coord().y - operator[](i-1).coord().y));
    }
    return std::abs(result) / 2.0;
}


// Circle

Circle::Circle(const double x, const double y, const double r)
    :_center(x, y)
    ,_radius(r)
{
    assert(r > 0);
    _memo["Type"] = 50;
}

Circle::Circle(const Point &point, const double r)
    :_center(point)
    ,_radius(r)
{
    assert(r > 0);
    _memo["Type"] = 50;
}

Circle::Circle(const Circle &circle)
    :Geometry(circle)
    ,_center(circle._center)
    ,_radius(circle._radius)
{
    _memo["Type"] = 50;
}

Circle::Circle(const Circle &&circle)
    :Geometry(circle)
    ,_center(std::move(circle._center))
    ,_radius(std::move(circle._radius))
{
    _memo["Type"] = 50;
}

Circle &Circle::operator=(const Circle &circle)
{
    if (this != &circle)
    {
        Geometry::operator=(circle);
        _center = circle._center;
        _radius = circle._radius;
        _memo["Type"] = 50;
    }
    return *this;
}

Circle &Circle::operator=(const Circle &&circle)
{
    if (this != &circle)
    {
        Geometry::operator=(circle);
        _center = std::move(circle._center);
        _radius = std::move(circle._radius);
        _memo["Type"] = 50;
    }
    return *this;
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

AxisAlignedBoundingBox Circle::bounding_box() const
{
    if (_radius == 0)
    {
        return AABB();
    }
    else
    {
        return AABB(_center.coord().x - _radius, _center.coord().y - _radius, _center.coord().x + _radius, _center.coord().y + _radius);
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
{
    _memo["Type"] = 60;
}

Line::Line(const Point &start, const Point &end)
    :_start_point(start)
    ,_end_point(end)
{
    _memo["Type"] = 60;
}

Line::Line(const Line &line)
    :Geometry(line)
    ,_start_point(line._start_point)
    ,_end_point(line._end_point)
{
    _memo["Type"] = 60;
}

Line::Line(const Line &&line)
    :Geometry(line)
    ,_start_point(std::move(line._start_point))
    ,_end_point(std::move(line._end_point))
{
    _memo["Type"] = 60;
}

Line &Line::operator=(const Line &line)
{
    if (this != &line)
    {
        Geometry::operator=(line);
        _start_point = line._start_point;
        _end_point = line._end_point;
        _memo["Type"] = 60;
    }
    return *this;
}

Line &Line::operator=(const Line &&line)
{
    if (this != &line)
    {
        Geometry::operator=(line);
        _start_point = std::move(line._start_point);
        _end_point = std::move(line._end_point);
        _memo["Type"] = 60;
    }
    return *this;
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

AxisAlignedBoundingBox Line::bounding_box() const
{
    if (_start_point == _end_point)
    {
        return AABB();
    }
    else
    {
        return AABB(std::min(_start_point.coord().x, _end_point.coord().x),
                         std::min(_start_point.coord().y, _end_point.coord().y),
                         std::max(_start_point.coord().x, _end_point.coord().x),
                         std::max(_start_point.coord().y, _end_point.coord().y));
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
    _memo["Type"] = 21;
    _shape.shape_fixed() = true;
}

Bezier::Bezier(const Bezier &bezier)
    : Polyline(bezier), _order(bezier._order), _shape(bezier._shape)
{
    _memo["Type"] = 21;
    _shape.shape_fixed() = true;
}

Bezier::Bezier(const Bezier &&bezier)
    : Polyline(std::move(bezier)), _order(std::move(bezier._order)), _shape(std::move(bezier._shape))
{
    _memo["Type"] = 21;
    _shape.shape_fixed() = true;
}

Bezier::Bezier(std::vector<Point>::const_iterator begin, std::vector<Point>::const_iterator end, const size_t n)
    : Polyline(begin, end), _order(n)
{
    _memo["Type"] = 21;
    _shape.shape_fixed() = true;
    update_shape();
}

Bezier::Bezier(const std::initializer_list<Point> &points, const size_t n)
    : Polyline(points), _order(n)
{
    _memo["Type"] = 21;
    _shape.shape_fixed() = true;
    update_shape();
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
    for (size_t i = 0, end = this->size() - _order; i < end; i += _order)
    {
        _shape.append(this->operator[](i));
        t = 0;
        while (t <= 1)
        {
            point.clear();
            for (size_t j = 0; j <= _order; ++j)
            {
                point += (this->operator[](j + i) * (nums[j] * std::pow(1 - t, _order - j) * std::pow(t, j))); 
            }
            _shape.append(point);
            t += step;
        }
    }
    _shape.append(this->back());
}

void Bezier::append_shape(const double step)
{
    assert(0 < step && step < 1);
    if ((this->size() - 1) % _order > 0)
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
    const size_t i = this->size() - _order - 1;
    while (t <= 1)
    {
        point.clear();
        for (size_t j = 0; j <= _order; ++j)
        {
            point += (this->operator[](j + i) * (nums[j] * std::pow(1 - t, _order - j) * std::pow(t, j))); 
        }
        _shape.append(point);
        t += step;
    }
    _shape.append(this->back());
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
        _memo["Type"] = 21;
    }
    return *this;
}

Bezier &Bezier::operator=(const Bezier &&bezier)
{
    if (this != &bezier)
    {
        Polyline::operator=(std::move(bezier));
        _shape = std::move(bezier._shape);
        _memo["Type"] = 21;
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

AxisAlignedBoundingBox Bezier::bounding_box() const
{
    return _shape.bounding_box();
}



// functions

const double Geo::distance(const double x0, const double y0, const double x1, const double y1)
{
    return std::sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

const double Geo::distance(const Point &point0, const Point &point1)
{
    return std::sqrt((point0.coord().x - point1.coord().x) * (point0.coord().x - point1.coord().x)
                        + (point0.coord().y - point1.coord().y) * (point0.coord().y - point1.coord().y));
}

const double Geo::distance(const Point &point, const Line &line, const bool infinite)
{
    if (line.front().coord().x == line.back().coord().x)
    {
        if (infinite)
        {
            return std::abs(point.coord().x - line.front().coord().x);
        }
        else
        {
            if ((point.coord().y >= line.front().coord().y && point.coord().y <= line.back().coord().y) ||
                (point.coord().y <= line.front().coord().y && point.coord().y >= line.back().coord().y))
            {
                return std::abs(point.coord().x - line.front().coord().x);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    else if (line.front().coord().y == line.back().coord().y)
    {
        if (infinite)
        {
            return std::abs(point.coord().y - line.front().coord().y);
        }
        else
        {
            if ((point.coord().x >= line.front().coord().x && point.coord().x <= line.back().coord().x) ||
                (point.coord().x <= line.front().coord().x && point.coord().x >= line.back().coord().x))
            {
                return std::abs(point.coord().y - line.front().coord().y);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    
    const double a = line.back().coord().y - line.front().coord().y, 
                b = line.front().coord().x - line.back().coord().x,
                c = line.back().coord().x * line.front().coord().y - line.front().coord().x * line.back().coord().y;
    if (infinite)
    {
        return std::abs(a * point.coord().x + b * point.coord().y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double x = (b * b * point.coord().x - a * b * point.coord().y - a * c) / (a * a + b * b);
        if ((x >= line.front().coord().x && x <= line.back().coord().x) || (x <= line.front().coord().x && x >= line.back().coord().x))
        {
            return std::abs(a * point.coord().x + b * point.coord().y + c) / std::sqrt(a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
        }
    }
}

const double Geo::distance(const Point &point, const Point &start, const Point &end, const bool infinite)
{
    if (start.coord().x == end.coord().x)
    {
        if (infinite)
        {
            return std::abs(point.coord().x - start.coord().x);
        }
        else
        {
            if ((point.coord().y >= start.coord().y && point.coord().y <= end.coord().y) ||
                (point.coord().y <= start.coord().y && point.coord().y >= end.coord().y))
            {
                return std::abs(point.coord().x - start.coord().x);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    else if (start.coord().y == end.coord().y)
    {
        if (infinite)
        {
            return std::abs(point.coord().y - start.coord().y);
        }
        else
        {
            if ((point.coord().x >= start.coord().x && point.coord().x <= end.coord().x) ||
                (point.coord().x <= start.coord().x && point.coord().x >= end.coord().x))
            {
                return std::abs(point.coord().y - start.coord().y);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    
    const double a = end.coord().y - start.coord().y, 
                b = start.coord().x - end.coord().x,
                c = end.coord().x * start.coord().y - start.coord().x * end.coord().y;
    if (infinite)
    {
        return std::abs(a * point.coord().x + b * point.coord().y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double x = (b * b * point.coord().x - a * b * point.coord().y - a * c) / (a * a + b * b),
                    y = (a * a * point.coord().y - a * b * point.coord().x - b * c) / (a * a + b * b);
        if (((x >= start.coord().x && x <= end.coord().x) || (x <= start.coord().x && x >= end.coord().x))
            && ((y >= start.coord().y && y <= end.coord().y) || (y <= start.coord().y && y >= end.coord().y)))
        {
            return std::abs(a * point.coord().x + b * point.coord().y + c) / std::sqrt(a * a + b * b);
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
    if (!polygon.empty() && Geo::is_inside(point, polygon.bounding_box(), coincide))
    {
        size_t count = 0;
        double x = (-FLT_MAX);
        for (const Geo::Point &p : polygon)
        {
            x = std::max(x, p.coord().x);
        }
        Geo::Point temp, end(x + 80, point.coord().y);
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
                    if (polygon[i - 1].coord().y == polygon[i].coord().y)
                    {
                        if ((polygon[i == 1 ? len-2 : i-2].coord().y < polygon[i].coord().y && polygon[i == len-1 ? 1 : i+1].coord().y > polygon[i].coord().y)
                            || (polygon[i == 1 ? len-2 : i-2].coord().y > polygon[i].coord().y && polygon[i == len-1 ? 1 : i+1].coord().y < polygon[i].coord().y))
                        {
                            ++count;
                        }
                    }
                    else
                    {
                        if (Geo::distance(temp, polygon[i-1]) < Geo::EPSILON)
                        {
                            if ((polygon[i == 1 ? len-2 : i-2].coord().y < polygon[i-1].coord().y && polygon[i].coord().y < polygon[i-1].coord().y) ||
                                (polygon[i == 1 ? len-2 : i-2].coord().y > polygon[i-1].coord().y && polygon[i].coord().y > polygon[i-1].coord().y))
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
                    if (polygon[i - 1].coord().y == polygon[i].coord().y)
                    {
                        if ((polygon[i == 1 ? len-2 : i-2].coord().y < polygon[i].coord().y && polygon[i == len-1 ? 1 : i+1].coord().y > polygon[i].coord().y)
                            || (polygon[i == 1 ? len-2 : i-2].coord().y > polygon[i].coord().y && polygon[i == len-1 ? 1 : i+1].coord().y < polygon[i].coord().y))
                        {
                            ++count;
                        }
                    }
                    else
                    {
                        if (Geo::distance(temp, polygon[i-1]) < Geo::EPSILON)
                        {
                            if ((polygon[i == 1 ? len-2 : i-2].coord().y < polygon[i-1].coord().y && polygon[i].coord().y < polygon[i-1].coord().y) ||
                                (polygon[i == 1 ? len-2 : i-2].coord().y > polygon[i-1].coord().y && polygon[i].coord().y > polygon[i-1].coord().y))
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

const bool Geo::is_inside(const Point &point, const Rectangle &rect, const bool coincide)
{
    if (rect.empty())
    {
        return false;
    }
    const double x = point.coord().x, y = point.coord().y;
    if (rect[0].coord().y == rect[1].coord().y)
    {
        if (coincide)
        {
            return rect.left() <= x && x <= rect.right() && rect.bottom() <= y && y <= rect.top();
        }
        else
        {
            return rect.left() < x && x < rect.right() && rect.bottom() < y && y < rect.top();
        }
    }
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Point &p : rect)
    {
        x0 = std::min(x0, p.coord().x);
        y0 = std::min(y0, p.coord().y);
        x1 = std::max(x1, p.coord().x);
        y1 = std::max(y1, p.coord().y);
    }
    Point output, end(x1 + 80, y);
    size_t count = 0;
    if (coincide)
    {
        if (x < x0 || x > x1 || y < y0 || y > y1)
        {
            return false;
        }
        for (size_t i = 1; i < 5; ++i)
        {
            if (Geo::is_inside(point, rect[i-1], rect[i]))
            {
                return true;
            }
            if (rect[i-1].coord().y != rect[i].coord().y && Geo::is_intersected(point, end, rect[i-1], rect[i], output))
            {
                ++count;
            }
        }
        return count % 2 == 1;
    }
    else
    {
        if (x <= x0 || x >= x1 || y <= y0 || y >= y1)
        {
            return false;
        }
        for (size_t i = 1; i < 5; ++i)
        {
            if (Geo::is_inside(point, rect[i-1], rect[i]))
            {
                return false;
            }
            if (rect[i-1].coord().y != rect[i].coord().y && Geo::is_intersected(point, end, rect[i-1], rect[i], output))
            {
                ++count;
            }
        }
        return count % 2 == 1;
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


const bool Geo::is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite)
{
    const double a0 = point1.coord().y - point0.coord().y, 
                b0 = point0.coord().x - point1.coord().x,
                c0 = point1.coord().x * point0.coord().y - point0.coord().x * point1.coord().y;
    const double a1 = point3.coord().y - point2.coord().y, 
                b1 = point2.coord().x - point3.coord().x,
                c1 = point3.coord().x * point2.coord().y - point2.coord().x * point3.coord().y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON)
    {
        if (std::abs(c0 - c1) >= Geo::EPSILON)
        {
            return false;
        }
        if (infinite)
        {
            return std::abs(c0 - c1) < Geo::EPSILON;
        }
        else
        {
            return Geo::is_inside(point0, point2, point3) || Geo::is_inside(point1, point2, point3)
                || Geo::is_inside(point2, point0, point1) || Geo::is_inside(point3, point0, point1);
        }
    }
    output.coord().x = (c1 * b0 - c0 * b1) / (a0 * b1 - a1 * b0), output.coord().y = (c0 * a1 - c1 * a0) / (a0 * b1 - a1 * b0);
    if (infinite)
    {
        return true;
    }
    else
    {
        return ((output.coord().x >= point0.coord().x - Geo::EPSILON && output.coord().x <= point1.coord().x + Geo::EPSILON) 
                || (output.coord().x <= point0.coord().x + Geo::EPSILON && output.coord().x >= point1.coord().x - Geo::EPSILON))
            && ((output.coord().x >= point2.coord().x - Geo::EPSILON && output.coord().x <= point3.coord().x + Geo::EPSILON) 
                || (output.coord().x <= point2.coord().x + Geo::EPSILON && output.coord().x >= point3.coord().x - Geo::EPSILON))
            && ((output.coord().y >= point0.coord().y - Geo::EPSILON && output.coord().y <= point1.coord().y + Geo::EPSILON) 
                || (output.coord().y <= point0.coord().y + Geo::EPSILON && output.coord().y >= point1.coord().y - Geo::EPSILON))
            && ((output.coord().y >= point2.coord().y - Geo::EPSILON && output.coord().y <= point3.coord().y + Geo::EPSILON) 
                || (output.coord().y <= point2.coord().y + Geo::EPSILON && output.coord().y >= point3.coord().y - Geo::EPSILON));
    }
}

const bool Geo::is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite)
{
    return Geo::is_intersected(line0.front(), line0.back(), line1.front(), line1.back(), output, infinite);
}

const bool Geo::is_intersected(const Rectangle &rect0, const Rectangle &rect1, const bool inside)
{
    if (rect0.empty() || rect1.empty())
    {
        return false;
    }
    Geo::Point output;
    if (Geo::is_intersected(rect0[0], rect0[2], rect1[0], rect1[2], output) || 
        Geo::is_intersected(rect0[0], rect0[2], rect1[1], rect1[3], output))
    {
        return true;
    }

    for (size_t i = 1; i < 5; ++i)
    {
        for (size_t j = 1; j < 5; ++j)
        {
            if (Geo::is_intersected(rect0[i-1], rect0[i], rect1[j-1], rect1[j], output))
            {
                return true;
            }
        }
    }

    if (inside)
    {
        for (const Point &point : rect0)
        {
            if (Geo::is_inside(point, rect1, true))
            {
                return true;
            }
        }
        for (const Point &point : rect1)
        {
            if (Geo::is_inside(point, rect0, true))
            {
                return true;
            }
        }
    }
    
    return false;
}

const bool Geo::is_intersected(const Polyline &polyline0, const Polyline &polyline1)
{
    if (polyline0.empty() || polyline1.empty() || !Geo::is_intersected(polyline0.bounding_box(), polyline1.bounding_box()))
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
    if (polyline.empty() || polygon.empty() || !Geo::is_intersected(polygon.bounding_box(), polyline.bounding_box()))
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
    if (polygon0.empty() || polygon1.empty() || !Geo::is_intersected(polygon0.bounding_box(), polygon1.bounding_box()))
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

const bool Geo::is_intersected(const Rectangle &rect, const Point &point0, const Point &point1, const bool inside)
{
    if (!Geo::is_intersected(rect, Rectangle(point0, point1)))
    {
        return false;
    }
    if (inside)
    {
        if (Geo::is_inside(point0, rect, true) || Geo::is_inside(point1, rect, true))
        {
            return true;
        }
    }
    Geo::Point point;
    for (size_t i = 1; i < 5; ++i)
    {
        if (Geo::is_intersected(point0, point1, rect[i-1], rect[i], point))
        {
            return true;
        }
    }
    return false;
}

const bool Geo::is_intersected(const Rectangle &rect, const Line &line, const bool inside)
{
    return Geo::is_intersected(rect, line.front(), line.back());
}

const bool Geo::is_intersected(const Rectangle &rect, const Polyline &polyline, const bool inside)
{
    if (polyline.empty() || !Geo::is_intersected(rect, polyline.bounding_box()))
    {
        return false;
    }
    
    if (inside)
    {
        for (const Geo::Point &point : polyline)
        {
            if (Geo::is_inside(point, rect, true))
            {
                return true;
            }
        }
    }
    Point point;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        for (size_t j = 1; j < 5; ++j)
        {
            if (Geo::is_intersected(polyline[i-1], polyline[i], rect[j-1], rect[j], point))
            {
                return true;
            }
        }
    }
    return false;
}

const bool Geo::is_intersected(const Rectangle &rect, const Polygon &polygon, const bool inside)
{
    if (polygon.empty() || !Geo::is_intersected(rect, polygon.bounding_box()))
    {
        return false;
    }
    
    if (inside)
    {
        for (const Geo::Point &point : polygon)
        {
            if (Geo::is_inside(point, rect, true))
            {
                return true;
            }
        }
        for (const Geo::Point &point : rect)
        {
            if (Geo::is_inside(point, polygon, true))
            {
                return true;
            }
        }
    }
    Geo::Point point;
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        for (size_t j = 1; j < 5; ++j)
        {
            if (Geo::is_intersected(polygon[i-1], polygon[i], rect[j-1], rect[j], point))
            {
                return true;
            }
        }
    }
    return false;
}

const bool Geo::is_intersected(const Rectangle &rect, const Circle &circle, const bool inside)
{
    if (circle.empty() || !Geo::is_intersected(rect, circle.bounding_box()))
    {
        return false;
    }

    if (inside)
    {
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


const bool Geo::is_rectangle(const Polygon &polygon)
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
        const Geo::Coord vec0 = (points[0] - points[1]).coord(), vec1 = (points[2] - points[1]).coord();
        return std::abs(vec0.x * vec1.x + vec0.y * vec1.y) == 0;
    }
    else
    {
        return false;
    }
}

