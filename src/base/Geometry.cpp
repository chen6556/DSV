#include <algorithm>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>

#include "base/Algorithm.hpp"
#include "base/Math.hpp"

using namespace Geo;


const double Geometry::length() const
{
    return 0;
}

Polygon Geometry::convex_hull() const
{
    return Polygon();
}

AABBRect Geometry::bounding_rect() const
{
    return AABBRect();
}

Polygon Geometry::mini_bounding_rect() const
{
    return Polygon();
}


// Point

Point::Point(const double x_, const double y_) : x(x_), y(y_)
{
}

Point::Point(const MarkedPoint &point) : x(point.x), y(point.y)
{
}

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
    const double len = std::hypot(x, y);
    x /= len;
    y /= len;
    return *this;
}

Point Point::normalized() const
{
    const double len = std::hypot(x, y);
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
    return AABBRect(x, y, x, y);
}

Polygon Point::mini_bounding_rect() const
{
    return AABBRect(x, y, x, y);
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

Polyline::Polyline(const Polygon &polygon) : _points(polygon.begin(), polygon.end())
{
}

Polyline::Polyline(std::vector<Point>::const_iterator begin, const std::vector<Point>::const_iterator &end)
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
    for (const Point &point : points)
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
        reuslt += Geo::distance(_points[i], _points[i - 1]);
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

Point &Polyline::at(const size_t index)
{
    return _points.at(index);
}

const Point &Polyline::at(const size_t index) const
{
    return _points.at(index);
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
    if (_points.empty() || polyline.empty() || _points.back() != polyline._points.front())
    {
        _points.insert(_points.cend(), polyline._points.cbegin(), polyline._points.cend());
    }
    else
    {
        _points.insert(_points.cend(), polyline._points.cbegin() + 1, polyline._points.cend());
    }
}

void Polyline::append(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end)
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

void Polyline::append(const std::vector<Point>::const_reverse_iterator &rbegin, const std::vector<Point>::const_reverse_iterator &rend)
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

void Polyline::insert(const size_t index, const Polyline &polyline)
{
    assert(index < _points.size());
    if (polyline.empty())
    {
        return;
    }
    int i = (index > 0 && _points[index - 1] == polyline._points.front()), j = _points[index] == polyline._points.back();
    _points.insert(_points.cbegin() + index, polyline._points.cbegin() + i, polyline._points.cend() - j);
}

void Polyline::insert(const size_t index, const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end)
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

void Polyline::insert(const size_t index, const std::vector<Point>::const_reverse_iterator &rbegin,
                      const std::vector<Point>::const_reverse_iterator &rend)
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
    Point point(_points[index]);
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
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.transform(a, b, c, d, e, f); });
}

void Polyline::transform(const double mat[6])
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.transform(mat); });
}

void Polyline::translate(const double tx, const double ty)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.translate(tx, ty); });
}

void Polyline::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.rotate(x, y, rad); });
}

void Polyline::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.scale(x, y, k); });
}

Polygon Polyline::convex_hull() const
{
    std::vector<Point> points(_points);
    std::sort(points.begin(), points.end(), [](const Point &a, const Point &b) { return a.y < b.y; });
    const Point origin(points.front());
    std::for_each(points.begin(), points.end(), [=](Point &p) { p -= origin; });
    std::sort(points.begin() + 1, points.end(),
              [](const Point &a, const Point &b)
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
    std::for_each(points.begin(), points.end(), [=](Point &p) { p += origin; });

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

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
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

    double cs = 0, area = DBL_MAX;
    AABBRect rect, temp;
    const Polygon hull(convex_hull());
    for (size_t i = 1, count = hull.size(); i < count; ++i)
    {
        Polygon polygon(hull);
        cs = (polygon[i - 1].x * polygon[i].y - polygon[i].x * polygon[i - 1].y) / (polygon[i].length() * polygon[i - 1].length());
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
    return _points.size() < 5;
}

const double AABBRect::length() const
{
    double reuslt = 0;
    for (size_t i = 1, count = _points.size(); i < count; ++i)
    {
        reuslt += Geo::distance(_points[i], _points[i - 1]);
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
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.transform(a, b, c, d, e, f); });
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
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.transform(mat); });
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
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.translate(tx, ty); });
}

void AABBRect::rotate(const double x, const double y, const double rad)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.rotate(x, y, rad); });
}

void AABBRect::scale(const double x, const double y, const double k)
{
    std::for_each(_points.begin(), _points.end(), [=](Point &point) { point.scale(x, y, k); });
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
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
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
    return AABBRect(_points[0].x + point.x, _points[0].y + point.y, _points[2].x + point.x, _points[2].y + point.y);
}

AABBRect AABBRect::operator-(const Point &point) const
{
    return AABBRect(_points[0].x - point.x, _points[0].y - point.y, _points[2].x - point.x, _points[2].y - point.y);
}

AABBRect AABBRect::operator+(const AABBRect &rect) const
{
    return AABBRect(std::min(left(), rect.left()), std::max(top(), rect.top()), std::max(right(), rect.right()),
                    std::min(bottom(), rect.bottom()));
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

void AABBRect::operator+=(const AABBRect &rect)
{
    if (rect.empty())
    {
        return;
    }
    if (_points.empty())
    {
        _points.assign(rect._points.begin(), rect._points.end());
    }
    else
    {
        if (_points[0].x > rect[0].x)
        {
            _points[0].x = _points[3].x = _points[4].x = rect[0].x;
        }
        if (_points[0].y < rect[0].y)
        {
            _points[0].y = _points[1].y = _points[4].y = rect[0].y;
        }
        if (_points[2].x < rect[2].x)
        {
            _points[1].x = _points[2].x = rect[2].x;
        }
        if (_points[2].y > rect[2].y)
        {
            _points[2].y = _points[3].y = rect[2].y;
        }
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

Polygon::Polygon(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end) : Polyline(begin, end)
{
    if (!_points.empty() && _points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const std::initializer_list<Point> &points) : Polyline(points)
{
    if (!_points.empty() && _points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const Polyline &polyline) : Polyline(polyline)
{
    if (!_points.empty() && _points.back() != _points.front())
    {
        _points.emplace_back(_points.front());
    }
}

Polygon::Polygon(const AABBRect &rect) : Polyline(rect.cbegin(), rect.cend())
{
}

Polygon::Polygon(const double x, const double y, const double radius, const int n, const double rad, const bool circumscribed)
{
    assert(n >= 3 && radius >= 0);
    if (radius == 0)
    {
        return;
    }
    const Geo::Point anchor(x, y);
    const double step = 2 * Geo::PI / n;
    Geo::Vector vec;
    if (circumscribed)
    {
        vec.x = radius * std::cos(rad);
        vec.y = radius * std::sin(rad);
    }
    else
    {
        vec.x = radius / std::cos(Geo::PI / n) * std::cos(Geo::PI / n + rad);
        vec.y = radius / std::cos(Geo::PI / n) * std::sin(Geo::PI / n + rad);
    }
    for (double angle = 0; angle <= 2 * Geo::PI; angle += step)
    {
        _points.emplace_back(anchor + vec);
        vec.rotate(0, 0, step);
    }
    if (_points.front() != _points.back())
    {
        _points.emplace_back(_points.front());
    }
}

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

void Polygon::append(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end)
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

void Polygon::append(const std::vector<Point>::const_reverse_iterator &rbegin, const std::vector<Point>::const_reverse_iterator &rend)
{
    if (empty())
    {
        Polyline::append(rbegin, rend);
        if (_points.front() != _points.back())
        {
            _points.emplace_back(_points.front());
        }
    }
    else
    {
        if (_points.front() == _points.back())
        {
            Polyline::insert(size() - 1, rbegin, rend);
        }
        else
        {
            _points.insert(_points.end(), rbegin, rend);
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

void Polygon::insert(const size_t index, const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end)
{
    Polyline::insert(index, begin, end);
    if (index == 0)
    {
        _points.back() = _points.front();
    }
}

void Polygon::insert(const size_t index, const std::vector<Point>::const_reverse_iterator &rbegin,
                     const std::vector<Point>::const_reverse_iterator &rend)
{
    Polyline::insert(index, rbegin, rend);
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
    double s = 0, a = 0;
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

Triangle::Triangle(const Triangle &triangle) : Geometry(triangle)
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
    for (Geo::Point &point : _vecs)
    {
        point.clear();
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
    return std::sqrt(p * (p - a) * (p - b) * (p - c));
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

    const double left = std::min({_vecs[0].x, _vecs[1].x, _vecs[2].x});
    const double right = std::max({_vecs[0].x, _vecs[1].x, _vecs[2].x});
    const double top = std::max({_vecs[0].y, _vecs[1].y, _vecs[2].y});
    const double bottom = std::min({_vecs[0].y, _vecs[1].y, _vecs[2].y});
    return AABBRect(left, top, right, bottom);
}

Polygon Triangle::mini_bounding_rect() const
{
    if (empty())
    {
        return Polygon();
    }

    double cs = 0, area = DBL_MAX;
    AABBRect rect, temp;
    for (size_t i = 0; i < 3; ++i)
    {
        Triangle triangle(*this);
        cs = (triangle[i].x * triangle[i < 2 ? i + 1 : 0].y - triangle[i < 2 ? i + 1 : 0].x * triangle[i].y) /
             (triangle[i < 2 ? i + 1 : 0].length() * triangle[i].length());
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

Circle::Circle(const double x, const double y, const double r) : Point(x, y), radius(r)
{
    assert(r >= 0);
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const Point &point, const double r) : Point(point), radius(r)
{
    assert(r >= 0);
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const double x0, const double y0, const double x1, const double y1)
    : Point((x0 + x1) / 2, (y0 + y1) / 2), radius(std::hypot(x0 - x1, y0 - y1) / 2)
{
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const Point &point0, const Point &point1) : Circle(point0.x, point0.y, point1.x, point1.y)
{
}

Circle::Circle(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2)
{
    const double a = x0 - x1, b = y0 - y1, c = x0 - x2, d = y0 - y2;
    const double e = (x0 * x0 - x1 * x1 + y0 * y0 - y1 * y1) / 2;
    const double f = (x0 * x0 - x2 * x2 + y0 * y0 - y2 * y2) / 2;
    const double t = b * c - a * d;
    assert(t != 0);
    x = (b * f - d * e) / t, y = (c * e - a * f) / t;
    radius = (std::hypot(x - x0, y - y0) + std::hypot(x - x1, y - y1) + std::hypot(x - x2, y - y2)) / 3;
    update_shape(Geo::Circle::default_down_sampling_value);
}

Circle::Circle(const Point &point0, const Point &point1, const Point &point2)
    : Circle(point0.x, point0.y, point1.x, point1.y, point2.x, point2.y)
{
}

Circle &Circle::operator=(const Circle &circle)
{
    if (this != &circle)
    {
        Point::operator=(circle);
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
    Point::clear();
}

Circle *Circle::clone() const
{
    return new Circle(*this);
}

void Circle::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    Geo::Point points[4] = {{x - radius, y}, {x, y + radius}, {x + radius, y}, {x, y - radius}};
    for (Geo::Point &point : points)
    {
        point.transform(a, b, c, d, e, f);
    }
    radius = (Geo::distance(points[0], points[2]) + Geo::distance(points[1], points[3])) / 4;
    Point::transform(a, b, c, d, e, f);
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
    Geo::Point points[4] = {{x - radius, y}, {x, y + radius}, {x + radius, y}, {x, y - radius}};
    for (Geo::Point &point : points)
    {
        point.transform(mat);
    }
    radius = (Geo::distance(points[0], points[2]) + Geo::distance(points[1], points[3])) / 4;
    Point::transform(mat);
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
}

const Polygon &Circle::shape() const
{
    return _shape;
}


// Bezier
double Bezier::default_step = 0.01;
double Bezier::default_down_sampling_value = 0.02;

Bezier::Bezier(const size_t n) : _order(n)
{
    assert(n == 3 || n == 2);
    _shape.shape_fixed = true;
}

Bezier::Bezier(const Bezier &bezier) : Polyline(bezier), _order(bezier._order), _shape(bezier._shape)
{
    _shape.shape_fixed = true;
}

Bezier::Bezier(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end, const size_t n,
               const bool is_path_points)
    : Polyline(begin, end), _order(n)
{
    assert(n == 3 || n == 2);
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        update_control_points();
    }
    update_shape(Bezier::default_step, Bezier::default_down_sampling_value);
}

Bezier::Bezier(const std::initializer_list<Point> &points, const size_t n, const bool is_path_points) : Polyline(points), _order(n)
{
    assert(n == 3 || n == 2);
    _shape.shape_fixed = true;
    if (is_path_points)
    {
        update_control_points();
    }
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

void Bezier::update_control_points()
{
    std::vector<Geo::Point> paths(_points);
    _points.erase(_points.begin() + 1, _points.end());
    Geo::Point mid0((paths[1] + paths[2]) / 2), mid1((paths[0] + paths[1]) / 2);
    _points.emplace_back(mid1 + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 2);
    if (_order == 2)
    {
        for (size_t i = 1, count = paths.size() - 1; i < count; ++i)
        {
            mid1 = paths[i] - _points.back();
            const double dis = Geo::distance(_points.back(), paths[i]) / 2;
            _points.emplace_back(paths[i]);
            _points.emplace_back(paths[i] + mid1.normalize() * dis);
        }
        _points.emplace_back(paths.back());
    }
    else
    {
        for (size_t i = 1, count = paths.size() - 1; i < count; ++i)
        {
            mid0 = mid1;
            mid1 = (paths[i] + paths[i + 1]) / 2;
            _points.emplace_back(paths[i] + (mid0 - mid1).normalize() * Geo::distance(mid0, mid1) / 2);
            _points.emplace_back(paths[i]);
            _points.emplace_back(paths[i] + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 2);
        }
        _points.emplace_back(mid1 + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 2);
        _points.emplace_back(paths.back());
        _points[1] = (_points[0] + _points[2]) / 2;
        _points[_points.size() - 2] = (_points.back() + _points[_points.size() - 3]) / 2;
    }
}

void Bezier::update_shape(const double step, const double down_sampling_value)
{
    assert(0 < step && step < 1);
    _shape.clear();
    if (_points.size() <= _order)
    {
        return;
    }
    std::vector<int> nums(_order + 1, 1);
    if (_order == 2)
    {
        nums[1] = 2;
    }
    else
    {
        nums[1] = nums[2] = 3;
    }

    for (size_t i = 0, end = _points.size() - _order; i < end; i += _order)
    {
        _shape.append(_points[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
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
        _order = bezier._order;
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

Point Bezier::tangent(const size_t index, const double t) const
{
    if (_points.size() < _order * (index + 1) + 1)
    {
        return Point();
    }
    std::vector<int> nums(_order, 1);
    if (_order == 3)
    {
        nums[1] = 2;
    }

    Point start, end;
    for (int i = 0; i < _order; ++i)
    {
        start += (_points[i + index] * (nums[i] * std::pow(1 - t, _order - i - 1) * std::pow(t, i)));
    }
    for (int i = 0; i < _order; ++i)
    {
        end += (_points[i + index + 1] * (nums[i] * std::pow(1 - t, _order - i - 1) * std::pow(t, i)));
    }
    return end - start;
}

Point Bezier::vertical(const size_t index, const double t) const
{
    if (_points.size() < _order * (index + 1) + 1)
    {
        return Point();
    }

    Point start, end;
    {
        std::vector<int> nums(_order, 1);
        if (_order == 3)
        {
            nums[1] = 2;
        }
        for (int i = 0; i < _order; ++i)
        {
            start += (_points[i + index] * (nums[i] * std::pow(1 - t, _order - i - 1) * std::pow(t, i)));
        }
        for (int i = 0; i < _order; ++i)
        {
            end += (_points[i + index + 1] * (nums[i] * std::pow(1 - t, _order - i - 1) * std::pow(t, i)));
        }
    }
    Point vec(end - start);

    {
        std::vector<int> nums(_order + 1, 1);
        if (_order == 2)
        {
            nums[1] = 2;
        }
        else
        {
            nums[1] = nums[2] = 3;
        }
        Point anchor;
        for (int i = 0; i <= _order; ++i)
        {
            anchor += (_points[i + index] * (nums[i] * std::pow(1 - t, _order - i) * std::pow(t, i)));
        }
        vec.rotate(anchor.x, anchor.y, Geo::PI / 2);
    }

    return vec;
}


// Ellipse
double Ellipse::default_down_sampling_value = 0.02;

Ellipse::Ellipse(const double x, const double y, const double a, const double b)
{
    assert(a >= 0 && b >= 0);
    _a[0].x = x - a;
    _a[1].x = x + a;
    _a[0].y = _a[1].y = y;
    _b[0].y = y + b;
    _b[1].y = y - b;
    _b[0].x = _b[1].x = x;
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const double x, const double y, const double a, const double b, const double start, const double end, const bool is_param)
{
    assert(a >= 0 && b >= 0);
    _a[0].x = x - a;
    _a[1].x = x + a;
    _a[0].y = _a[1].y = y;
    _b[0].y = y + b;
    _b[1].y = y - b;
    _b[0].x = _b[1].x = x;
    update_angle_param(start, end, is_param);
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &point, const double a, const double b)
{
    assert(a >= 0 && b >= 0);
    _a[0].x = point.x - a;
    _a[1].x = point.x + a;
    _a[0].y = _a[1].y = point.y;
    _b[0].y = point.y + b;
    _b[1].y = point.y - b;
    _b[0].x = _b[1].x = point.x;
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &point, const double a, const double b, const double start, const double end, const bool is_param)
{
    assert(a >= 0 && b >= 0);
    _a[0].x = point.x - a;
    _a[1].x = point.x + a;
    _a[0].y = _a[1].y = point.y;
    _b[0].y = point.y + b;
    _b[1].y = point.y - b;
    _b[0].x = _b[1].x = point.x;
    update_angle_param(start, end, is_param);
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &a0, const Point &a1, const Point &b0, const Point &b1)
{
    _a[0] = a0, _a[1] = a1;
    _b[0] = b0, _b[1] = b1;
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Point &a0, const Point &a1, const Point &b0, const Point &b1, const double start, const double end,
                 const bool is_param)
{
    _a[0] = a0, _a[1] = a1;
    _b[0] = b0, _b[1] = b1;
    update_angle_param(start, end, is_param);
    update_shape(Geo::Ellipse::default_down_sampling_value);
}

Ellipse::Ellipse(const Ellipse &ellipse) : Geometry(ellipse), _shape(ellipse._shape)
{
    _a[0] = ellipse._a[0];
    _a[1] = ellipse._a[1];
    _b[0] = ellipse._b[0];
    _b[1] = ellipse._b[1];
    _arc_angle[0] = ellipse._arc_angle[0];
    _arc_angle[1] = ellipse._arc_angle[1];
    _arc_param[0] = ellipse._arc_param[0];
    _arc_param[1] = ellipse._arc_param[1];
}

Ellipse &Ellipse::operator=(const Ellipse &ellipse)
{
    if (this != &ellipse)
    {
        Geometry::operator=(ellipse);
        _a[0] = ellipse._a[0];
        _a[1] = ellipse._a[1];
        _b[0] = ellipse._b[0];
        _b[1] = ellipse._b[1];
        _arc_angle[0] = ellipse._arc_angle[0];
        _arc_angle[1] = ellipse._arc_angle[1];
        _arc_param[0] = ellipse._arc_param[0];
        _arc_param[1] = ellipse._arc_param[1];
        _shape = ellipse._shape;
    }
    return *this;
}

double Ellipse::angle_to_param(double angle) const
{
    angle = Geo::rad_to_2PI(angle);
    if (angle <= Geo::PI / 2)
    {
        return angle == Geo::PI / 2 ? Geo::PI / 2 : std::atan(std::tan(angle) * lengtha() / lengthb());
    }
    else if (angle <= Geo::PI)
    {
        return angle == Geo::PI ? Geo::PI : std::atan(std::tan(angle) * lengtha() / lengthb()) + Geo::PI;
    }
    else if (angle <= Geo::PI * 3 / 2)
    {
        return angle == Geo::PI * 3 / 2 ? Geo::PI * 3 / 2 : std::atan(std::tan(angle) * lengtha() / lengthb()) + Geo::PI;
    }
    else
    {
        return angle == Geo::PI * 2 ? Geo::PI * 2 : std::atan(std::tan(angle) * lengtha() / lengthb()) + Geo::PI * 2;
    }
}

double Ellipse::param_to_angle(double param) const
{
    param = Geo::rad_to_2PI(param);
    if (param <= Geo::PI / 2)
    {
        return param == Geo::PI / 2 ? Geo::PI / 2 : std::atan(std::tan(param) * lengthb() / lengtha());
    }
    else if (param <= Geo::PI)
    {
        return param == Geo::PI ? Geo::PI : std::atan(std::tan(param) * lengthb() / lengtha()) + Geo::PI;
    }
    else if (param <= Geo::PI * 3 / 2)
    {
        return param == Geo::PI * 3 / 2 ? Geo::PI * 3 / 2 : std::atan(std::tan(param) * lengthb() / lengtha()) + Geo::PI;
    }
    else
    {
        return param == Geo::PI * 2 ? Geo::PI * 2 : std::atan(std::tan(param) * lengthb() / lengtha()) + Geo::PI * 2;
    }
}

void Ellipse::update_angle_param(const double start, const double end, const bool is_param)
{
    if (is_param)
    {
        _arc_param[0] = start, _arc_param[1] = end;
        for (int i = 0; i < 2; ++i)
        {
            while (_arc_param[i] > Geo::PI * 2)
            {
                _arc_param[i] -= Geo::PI * 2;
            }
            while (_arc_param[i] < 0)
            {
                _arc_param[i] += Geo::PI * 2;
            }
        }
        _arc_angle[0] = param_to_angle(_arc_param[0]);
        _arc_angle[1] = param_to_angle(_arc_param[1]);
    }
    else
    {
        _arc_angle[0] = start, _arc_angle[1] = end;
        for (int i = 0; i < 2; ++i)
        {
            while (_arc_angle[i] > Geo::PI * 2)
            {
                _arc_angle[i] -= Geo::PI * 2;
            }
            while (_arc_angle[i] < 0)
            {
                _arc_angle[i] += Geo::PI * 2;
            }
        }
        _arc_param[0] = angle_to_param(_arc_angle[0]);
        _arc_param[1] = angle_to_param(_arc_angle[1]);
    }
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
    return Geo::angle(_a[0], _a[1]);
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

void Ellipse::reset_parameter(const Geo::Point &a0, const Geo::Point &a1, const Geo::Point &b0, const Geo::Point &b1,
                              const double start_anlge, const double end_angle)
{
    _a[0] = a0, _a[1] = a1;
    _b[0] = b0, _b[1] = b1;
    _arc_angle[0] = start_anlge, _arc_angle[1] = end_angle;
    for (int i = 0; i < 2; ++i)
    {
        while (_arc_angle[i] > Geo::PI * 2)
        {
            _arc_angle[i] -= Geo::PI * 2;
        }
        while (_arc_angle[i] < 0)
        {
            _arc_angle[i] += Geo::PI * 2;
        }
    }
    _arc_param[0] = angle_to_param(_arc_angle[0]);
    _arc_param[1] = angle_to_param(_arc_angle[1]);
}

void Ellipse::reset_parameter(const double parameters[10])
{
    _a[0].x = parameters[0], _a[0].y = parameters[1];
    _a[1].y = parameters[2], _a[1].y = parameters[3];
    _b[0].x = parameters[4], _b[0].y = parameters[5];
    _b[1].x = parameters[6], _b[1].y = parameters[7];
    _arc_angle[0] = parameters[8], _arc_angle[1] = parameters[9];
    for (int i = 0; i < 2; ++i)
    {
        while (_arc_angle[i] > Geo::PI * 2)
        {
            _arc_angle[i] -= Geo::PI * 2;
        }
        while (_arc_angle[i] < 0)
        {
            _arc_angle[i] += Geo::PI * 2;
        }
    }
    _arc_param[0] = angle_to_param(_arc_angle[0]);
    _arc_param[1] = angle_to_param(_arc_angle[1]);
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

double Ellipse::arc_angle0() const
{
    return _arc_angle[0];
}

Geo::Point Ellipse::arc_point0() const
{
    const Geo::Point anchor = center();
    const double a = lengtha(), b = lengthb(), rad = angle();
    return Geo::Point(anchor.x + a * std::cos(rad) * std::cos(_arc_param[0]) - b * std::sin(rad) * std::sin(_arc_param[0]),
                      anchor.y + a * std::sin(rad) * std::cos(_arc_param[0]) + b * std::cos(rad) * std::sin(_arc_param[0]));
}

double Ellipse::arc_angle1() const
{
    return _arc_angle[1];
}

Geo::Point Ellipse::arc_point1() const
{
    const Geo::Point anchor = center();
    const double a = lengtha(), b = lengthb(), rad = angle();
    return Geo::Point(anchor.x + a * std::cos(rad) * std::cos(_arc_param[1]) - b * std::sin(rad) * std::sin(_arc_param[1]),
                      anchor.y + a * std::sin(rad) * std::cos(_arc_param[1]) + b * std::cos(rad) * std::sin(_arc_param[1]));
}

double Ellipse::arc_param0() const
{
    return _arc_param[0];
}

double Ellipse::arc_param1() const
{
    return _arc_param[1];
}

void Ellipse::update_shape(const double down_sampling_value)
{
    const Geo::Point point = center();
    if (_arc_angle[0] == _arc_angle[1] || _arc_angle[1] - _arc_angle[0] == Geo::PI * 2)
    {
        _shape.clear();
        _shape.append(Geo::ellipse_to_polygon(point.x, point.y, lengtha(), lengthb(), angle(), down_sampling_value));
    }
    else
    {
        _shape =
            Geo::ellipse_to_polyline(point.x, point.y, lengtha(), lengthb(), angle(), _arc_param[0], _arc_param[1], down_sampling_value);
    }
}

const Polyline &Ellipse::shape() const
{
    return _shape;
}

bool Ellipse::is_arc() const
{
    return _arc_angle[0] != _arc_angle[1] && std::abs(_arc_angle[1] - _arc_angle[0]) < Geo::PI * 2;
}

Geo::Point Ellipse::param_tangency(const double value) const
{
    const double a = Geo::distance(_a[0], _a[1]) / 2;
    const double b = Geo::distance(_b[0], _b[1]) / 2;
    const double aa = Geo::distance_square(_a[0], _a[1]) / 4;
    const double bb = Geo::distance_square(_b[0], _b[1]) / 4;
    Geo::Point point0(a * std::cos(value), b * std::sin(value));
    const double line_param[2] = {point0.x / aa, point0.y / bb};
    Geo::Point point1;
    if (line_param[0] == 0)
    {
        point1.x = point0.x - 10, point1.y = 1 / line_param[1];
        if (!Geo::is_on_left(point1, Geo::Point(0, 0), point0))
        {
            point1.x = point0.x + 10;
        }
    }
    else if (line_param[1] == 0)
    {
        point1.x = point0.x, point1.y = point0.y + 10;
        if (!Geo::is_on_left(point1, Geo::Point(0, 0), point0))
        {
            point1.y = point0.y - 10;
        }
    }
    else
    {
        point1.x = point0.x - 10, point1.y = (1 - (point0.x - 10) * line_param[0]) / line_param[1];
        if (!Geo::is_on_left(point1, Geo::Point(0, 0), point0))
        {
            point1.x = point0.x + 10;
            point1.y = (1 - point1.x * line_param[0]) / line_param[1];
        }
    }

    const Geo::Point center((_a[0] + _a[1] + _b[0] + _b[1]) / 4);
    const double angle = Geo::angle(_a[0], _a[1]);
    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    point1 = Geo::to_coord(point1, coord.x, coord.y, -angle);
    point0.x = center.x + a * std::cos(angle) * std::cos(value) - b * std::sin(angle) * std::sin(value);
    point0.y = center.y + a * std::sin(angle) * std::cos(value) + b * std::cos(angle) * std::sin(value);
    return point1 - point0;
}

Geo::Point Ellipse::angle_tangency(const double value) const
{
    return param_tangency(angle_to_param(value));
}

Geo::Point Ellipse::param_point(const double value) const
{
    const Geo::Point center((_a[0] + _a[1] + _b[0] + _b[1]) / 4);
    const double angle = Geo::angle(_a[0], _a[1]);
    const double a = Geo::distance(_a[0], _a[1]) / 2;
    const double b = Geo::distance(_b[0], _b[1]) / 2;
    return Geo::Point(center.x + a * std::cos(angle) * std::cos(value) - b * std::sin(angle) * std::sin(value),
                      center.y + a * std::sin(angle) * std::cos(value) + b * std::cos(angle) * std::sin(value));
}

Geo::Point Ellipse::angle_point(const double value) const
{
    return param_point(angle_to_param(value));
}


// BSpline
double BSpline::default_step = 0.02;
double BSpline::default_down_sampling_value = 0.02;

BSpline::BSpline()
{
    _shape.shape_fixed = true;
}

BSpline::BSpline(const BSpline &bspline)
    : Geometry(bspline), _shape(bspline._shape), control_points(bspline.control_points), path_points(bspline.path_points),
      _knots(bspline._knots), controls_model(bspline.controls_model), _path_values(bspline._path_values)
{
    _shape.shape_fixed = true;
}

BSpline &BSpline::operator=(const BSpline &bspline)
{
    if (this != &bspline)
    {
        Geometry::operator=(bspline);
        _shape = bspline._shape;
        controls_model = bspline.controls_model;
        control_points = bspline.control_points;
        path_points = bspline.path_points;
        _path_values = bspline._path_values;
        _knots = bspline._knots;
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

void BSpline::set_knots(const std::vector<double>::const_iterator &begin, const std::vector<double>::const_iterator &end)
{
    _knots.assign(begin, end);
}

void BSpline::rbasis(const int order, const double t, const size_t npts, const std::vector<double> &x, std::vector<double> &output)
{
    const size_t nplusc = npts + order + 1;
    std::vector<double> temp(nplusc, 0);
    // calculate the first order nonrational basis functions n[i]
    for (size_t i = 0; i < nplusc - 1; ++i)
    {
        if ((t >= x[i]) && (t < x[i + 1]))
        {
            temp[i] = 1;
        }
    }

    // calculate the higher order nonrational basis functions
    for (int k = 2; k <= order + 1; ++k)
    {
        for (size_t i = 0; i < nplusc - k; ++i)
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
        temp[npts - 1] = 1;
    }

    // calculate sum for denominator of rational basis functions
    double sum = 0;
    for (size_t i = 0; i < npts; ++i)
    {
        sum += temp[i];
    }

    output.resize(npts, 0);
    // form rational basis functions and put in r vector
    if (sum != 0)
    {
        for (size_t i = 0; i < npts; ++i)
        {
            output[i] = temp[i] / sum;
        }
    }
}

void BSpline::rbspline(const int order, const size_t npts, const size_t p1, const std::vector<double> &knots, const std::vector<Point> &b,
                       std::vector<Point> &p)
{
    const size_t nplusc = npts + order + 1;
    // calculate the points on the rational B-spline curve
    double t = knots[0];
    const double step = (knots[nplusc - 1] - t) / (p1 - 1);

    for (Geo::Point &vp : p)
    {
        if (knots[nplusc - 1] - t < 5e-6)
        {
            t = knots[nplusc - 1];
        }
        // generate the basis function for this value of t
        std::vector<double> nbasis;
        rbasis(order, t, npts, knots, nbasis);
        // generate a point on the curve
        for (size_t i = 0; i < npts; ++i)
        {
            vp += b[i] * nbasis[i];
        }
        t += step;
    }
}


// QuadBSpline
QuadBSpline::QuadBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                         const bool is_path_points)
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
        QuadBSpline::update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        control_points.assign(begin, end);
        knot(control_points.size(), _knots);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(QuadBSpline::at(value));
            }
        }
    }
    QuadBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

QuadBSpline::QuadBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                         const std::vector<double> &knots, const bool is_path_points)
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
        QuadBSpline::update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        _knots.assign(knots.begin(), knots.end());
        control_points.assign(begin, end);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(QuadBSpline::at(value));
            }
        }
    }
    QuadBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
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
        QuadBSpline::update_control_points();
        knot(control_points.size(), _knots);
    }
    else
    {
        control_points.assign(points.begin(), points.end());
        knot(control_points.size(), _knots);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(QuadBSpline::at(value));
            }
        }
    }
    QuadBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

const Type QuadBSpline::type() const
{
    return Type::BSPLINE;
}

void QuadBSpline::update_control_points() // 从LibreCAD抄的
{
    control_points.clear();
    const size_t n = path_points.size();
    if (n < 4)
    {
        _path_values.clear();
        if (n > 0)
        {
            control_points.emplace_back(path_points[0]);
            _path_values.push_back(0);
        }
        if (Point control; n > 2 && get_three_points_control(path_points[0], path_points[1], path_points[2], control))
        {
            control_points.emplace_back(control);
            const double dl1 = Geo::distance(path_points[1], path_points[0]);
            const double dl2 = Geo::distance(path_points[2], path_points[1]);
            _path_values.push_back(dl1 / (dl1 + dl2));
        }
        if (n > 1)
        {
            control_points.emplace_back(path_points[n - 1]);
            _path_values.push_back(1);
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
        但对于首段，如果直接用 dt[0] = dl1 / (dl1 +
        dl2)，会导致首段的参数分布只考虑了前两段的长度，忽略了后续曲线的“延续性”，容易在端点产生不自然的曲率变化。 让首段的参数分布不仅考虑
        P0-P1 的长度，还适当考虑了 P1-P2 的影响，但权重减半（即只取一半）。
        这样做可以让首段的插值更平滑，首端的切线方向和曲率更自然，减少端点处的异常。*/
        for (int i = 1; i < dim - 1; ++i)
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

    {
        _path_values.clear();
        _path_values.push_back(0);
        _path_values.insert(_path_values.end(), dt.begin(), dt.end());
        _path_values.push_back(1);
        std::sort(_path_values.begin(), _path_values.end());
        const double value = path_points.size() - 2;
        for (size_t i = 0, count = _path_values.size(); i < count; ++i)
        {
            _path_values[i] *= value;
        }
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
        dx[dim - 1] =
            ((path_points[dim].x - path_points[dim + 1].x * std::pow(dt[n - 3], 2)) - pdDiag2[dim - 2] * dx[dim - 2]) / pdDiag[dim - 1];
        dy[dim - 1] =
            ((path_points[dim].y - path_points[dim + 1].y * std::pow(dt[n - 3], 2)) - pdDiag2[dim - 2] * dy[dim - 2]) / pdDiag[dim - 1];

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

bool QuadBSpline::get_three_points_control(const Point &point0, const Point &point1, const Point &point2, Point &output) // 从LibreCAD抄的
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

void QuadBSpline::get_matrix(const size_t count, const std::vector<double> &dt, std::vector<double> &output) // 从LibreCAD抄的
{
    if (count < 4 || dt.size() != count - 2)
    {
        return;
    }

    size_t dim = 3 * count - 8;    // 主对角线元素(n-2) + 上对角线元素(n-3) + 下对角线元素(n-3)
    double *res = new double[dim]; // 三对角矩阵
    {
        double *pdDiag = res;                  // 主对角线元素
        double *pdDiag1 = &res[count - 2];     // 上对角线元素
        double *pdDiag2 = &res[2 * count - 5]; // 下对角线元素

        double x3 = std::pow(dt[0], 2) / 2.0; // 首部点使用非均匀B样条曲线公式计算
        double x2 = 2.0 * dt[0] * (1.0 - dt[0]) + x3;
        pdDiag[0] = std::sqrt(x2);   // L矩阵主对角线第一个元素,使用平方根分解而非追赶分解
        pdDiag1[0] = x3 / pdDiag[0]; // U矩阵上对角线第一个元素

        for (size_t i = 1; i < count - 3; ++i)
        {
            double x1 = std::pow(1.0 - dt[i], 2) / 2.0;
            x3 = std::pow(dt[i], 2) / 2.0; // 除去首尾两点,中间部分的点按照均匀B样条曲线公式计算
            x2 = x1 + 2.0 * dt[i] * (1.0 - dt[i]) + x3;

            pdDiag2[i - 1] = x1 / pdDiag[i - 1];                         // L矩阵下对角线元素
            pdDiag[i] = std::sqrt(x2 - pdDiag1[i - 1] * pdDiag2[i - 1]); // L矩阵主对角线元素,使用平方根分解而非追赶分解
            pdDiag1[i] = x3 / pdDiag[i];                                 // U矩阵上对角线元素
        }

        double x1 = std::pow(1.0 - dt[count - 3], 2) / 2.0; // 尾部点使用非均匀B样条曲线公式计算
        x2 = x1 + 2.0 * dt[count - 3] * (1.0 - dt[count - 3]);
        pdDiag2[count - 4] = x1 / pdDiag[count - 4];                                 // U矩阵下对角线最后一个元素
        pdDiag[count - 3] = std::sqrt(x2 - pdDiag1[count - 4] * pdDiag2[count - 4]); // L矩阵主对角线最后一个元素,使用平方根分解而非追赶分解
    }
    output.assign(res, res + dim);
    delete[] res;
}

void QuadBSpline::knot(const size_t num, std::vector<double> &output) // 从LibreCAD抄的
{
    output.resize(num + 3, 0);
    // use uniform knots
    std::iota(output.begin() + 3, output.begin() + num + 1, 1);
    std::fill(output.begin() + num + 1, output.end(), output[num]);
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
    const size_t points_count = std::max(npts * 8.0, length / (step * 5));

    std::vector<Point> points(points_count, Point(0, 0));
    rbspline(2, npts, points_count, _knots, control_points, points);

    _shape.clear();
    _shape.append(path_points.empty() ? control_points.front() : path_points.front());
    _shape.append(points.begin(), points.end());
    _shape.append(path_points.empty() ? control_points.back() : path_points.back());
    Geo::down_sampling(_shape, down_sampling_value);

    if (controls_model)
    {
        path_points.clear();
        for (const double t : _path_values)
        {
            path_points.emplace_back(QuadBSpline::at(t));
        }
    }
}

QuadBSpline *QuadBSpline::clone() const
{
    return new QuadBSpline(*this);
}

void QuadBSpline::insert(const double t)
{
    // if (std::find(_knots.begin(), _knots.end(), t) != _knots.end())
    // {
    //     return;
    // }
    size_t k = 0;
    for (size_t i = 1, count = _knots.size(); i < count; ++i)
    {
        if (_knots[i - 1] <= t && t <= _knots[i])
        {
            k = --i;
            break;
        }
    }

    const size_t npts = control_points.size();
    std::vector<double> nbasis;
    rbasis(2, t, npts, _knots, nbasis);
    Geo::Point anchor;
    for (size_t i = 0; i < npts; ++i)
    {
        anchor += control_points[i] * nbasis[i];
    }

    Geo::Point array[2];
    for (size_t i = k, j = 1; i > k - 2; --i, --j)
    {
        double alpha = (t - _knots[i]);
        double dev = (_knots[i + 2] - _knots[i]);
        alpha = (dev == 0) ? 0 : alpha / dev;
        array[j] = control_points[i - 1] * (1 - alpha) + control_points[i] * alpha;
    }
    for (size_t i = k - 1, j = 0; i < k; ++i, ++j)
    {
        control_points[i] = array[j];
    }
    control_points.insert(control_points.begin() + k, array[1]);
    _knots.insert(_knots.begin() + k + 1, t);

    std::vector<double> lenghts({0});
    for (size_t i = 1, count = _shape.size(); i < count; ++i)
    {
        lenghts.push_back(lenghts.back() + Geo::distance(_shape[i - 1], _shape[i]));
    }
    std::vector<double> distances;
    for (const Geo::Point &point : path_points)
    {
        size_t index = 0;
        double min_dis = DBL_MAX;
        for (size_t i = 0, count = _shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(_shape[i], point); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        distances.push_back(lenghts[index]);
    }

    double anchor_dis = 0;
    {
        size_t index = 0;
        double min_dis = DBL_MAX;
        for (size_t i = 0, count = _shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(_shape[i], anchor); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        anchor_dis = lenghts[index];
    }

    for (size_t i = 1, count = path_points.size(); i < count; ++i)
    {
        if (distances[i - 1] <= anchor_dis && anchor_dis <= distances[i])
        {
            path_points.insert(path_points.begin() + i, anchor);
            _path_values.insert(_path_values.begin() + i, t);
            break;
        }
    }
}

Geo::Point QuadBSpline::at(const double t) const
{
    if (t <= _knots.front())
    {
        return control_points.front();
    }
    else if (t >= _knots.back())
    {
        return control_points.back();
    }
    const size_t npts = control_points.size();
    std::vector<double> nbasis;
    rbasis(2, t, npts, _knots, nbasis);
    Geo::Point point;
    for (size_t i = 0; i < npts; ++i)
    {
        point += control_points[i] * nbasis[i];
    }
    return point;
}

Geo::Point QuadBSpline::tangent(const double t) const
{
    if (t == _knots.front())
    {
        return control_points[1] - control_points.front();
    }
    else if (t == _knots.back())
    {
        return control_points.back() - control_points[control_points.size() - 2];
    }
    const size_t npts = control_points.size() - 1;
    std::vector<double> nbasis;
    const std::vector<double> knots(_knots.begin() + 1, _knots.end() - 1); // 一阶导数的节点矢量
    rbasis(1, t, npts, knots, nbasis);
    std::vector<Geo::Point> points;
    for (size_t i = 0; i < npts; ++i)
    {
        const double denom = _knots[i + 3] - _knots[i + 1];
        points.emplace_back((control_points[i + 1] - control_points[i]) * 2 / denom);
    }
    Geo::Point tang;
    for (size_t i = 0; i < npts; ++i)
    {
        tang += points[i] * nbasis[i];
    }
    return tang;
}

Geo::Point QuadBSpline::vertical(const double t) const
{
    const Geo::Point tan = tangent(t);
    return Geo::Point(-tan.y, tan.x);
}

void QuadBSpline::extend_front(const Geo::Point &expoint)
{
    control_points.insert(control_points.begin(), (control_points.front() + expoint) / 2);
    control_points.insert(control_points.begin(), expoint);

    path_points.insert(path_points.begin(), expoint);

    if (_knots.front() -= 1; _knots.front() < 0)
    {
        const double v = -_knots.front();
        std::for_each(_knots.begin(), _knots.end(), [=](double &k) { k += v; });
        std::for_each(_path_values.begin(), _path_values.end(), [=](double &k) { k += v; });
    }
    _knots.insert(_knots.begin(), _knots.front());
    _knots.insert(_knots.begin(), _knots.front());
    _path_values.insert(_path_values.begin(), _knots.front());
}

void QuadBSpline::extend_back(const Geo::Point &expoint)
{
    control_points.emplace_back((control_points.back() + expoint) / 2);
    control_points.emplace_back(expoint);

    path_points.emplace_back(expoint);

    _knots.back() += 1;
    _knots.push_back(_knots.back());
    _knots.push_back(_knots.back());
    _path_values.push_back(_knots.back());
}


// CubicBSpline
CubicBSpline::CubicBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                           const bool is_path_points)
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
        CubicBSpline::update_control_points();
    }
    else
    {
        control_points.assign(begin, end);
        const size_t num = control_points.size();
        _knots.resize(num + 4, 0);
        std::iota(_knots.begin() + 4, _knots.begin() + num + 1, 1);
        std::fill(_knots.begin() + num + 1, _knots.end(), _knots[num]);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(CubicBSpline::at(value));
            }
        }
    }
    CubicBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
}

CubicBSpline::CubicBSpline(const std::vector<Point>::const_iterator &begin, const std::vector<Point>::const_iterator &end,
                           const std::vector<double> &knots, const bool is_path_points)
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
        CubicBSpline::update_control_points();
    }
    else
    {
        _knots.assign(knots.begin(), knots.end());
        control_points.assign(begin, end);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(CubicBSpline::at(value));
            }
        }
    }
    CubicBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
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
        CubicBSpline::update_control_points();
    }
    else
    {
        control_points.assign(points.begin(), points.end());
        const size_t num = control_points.size();
        _knots.resize(num + 4, 0);
        std::iota(_knots.begin() + 4, _knots.begin() + num + 1, 1);
        std::fill(_knots.begin() + num + 1, _knots.end(), _knots[num]);
        for (const double value : _knots)
        {
            if (std::find(_path_values.begin(), _path_values.end(), value) == _path_values.end())
            {
                _path_values.push_back(value);
                path_points.emplace_back(CubicBSpline::at(value));
            }
        }
    }
    CubicBSpline::update_shape(BSpline::default_step, BSpline::default_down_sampling_value);
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
        _path_values.clear();
        if (n > 0)
        {
            control_points.emplace_back(path_points.front());
            _path_values.push_back(0);
        }
        if (n > 1)
        {
            control_points.emplace_back(path_points.back());
            _path_values.push_back(1);
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
        _path_values.clear();
        _path_values.push_back(0);
        for (size_t i = 4; i < n + 2; ++i)
        {
            double l2 = 0;
            for (size_t j = 0; j < i - 3; ++j)
            {
                l2 += l[j];
            }
            _knots[i] = l2 / l1;
            _path_values.push_back(_knots[i]);
        }
        _path_values.push_back(1);
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
    b[n - 1] = delta[n + 1] / (delta[n] + delta[n + 1]) *
               (delta[n + 1] / (delta[n] + delta[n] + delta[n + 1]) - delta[n] / (delta[n] + delta[n + 1]));
    c[n - 1] = delta[n] * delta[n + 1] / std::pow(delta[n] + delta[n + 1], 2) - 1;
    e[n - 1] = -(path_points[n - 1].x + 2 * path_points[n - 2].x) / 3;
    f[n - 1] = -(path_points[n - 1].y + 2 * path_points[n - 2].y) / 3;
    for (size_t i = 1; i < n - 1; ++i)
    {
        a[i] = std::pow(delta[i + 3], 2) / (delta[i + 1] + delta[i + 2] + delta[i + 3]);
        b[i] = delta[i + 3] * (delta[i + 1] + delta[i + 2]) / (delta[i + 1] + delta[i + 2] + delta[i + 3]) +
               delta[i + 2] * (delta[i + 3] + delta[i + 4]) / (delta[i + 2] + delta[i + 3] + delta[i + 4]);
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

void CubicBSpline::update_shape(const double step, const double down_sampling_value)
{
    const size_t npts = control_points.size();

    if (npts == 2)
    {
        _shape.clear();
        _shape.append(control_points.front());
        _shape.append(control_points.back());
        if (controls_model)
        {
            path_points.assign(control_points.begin(), control_points.end());
        }
        return;
    }

    double length = 0;
    for (size_t i = 1, count = control_points.size(); i < count; ++i)
    {
        length += Geo::distance(control_points[i - 1], control_points[i]);
    }

    // resolution:
    const size_t points_count = std::max(npts * 8.0, length / (step * 5));

    std::vector<Point> points(points_count, Point(0, 0));
    rbspline(3, npts, points_count, _knots, control_points, points);

    _shape.clear();
    _shape.append(path_points.empty() ? control_points.front() : path_points.front());
    _shape.append(points.begin(), points.end());
    _shape.append(path_points.empty() ? control_points.back() : path_points.back());
    Geo::down_sampling(_shape, down_sampling_value);

    if (controls_model)
    {
        path_points.clear();
        for (const double t : _path_values)
        {
            path_points.emplace_back(CubicBSpline::at(t));
        }
    }
}

CubicBSpline *CubicBSpline::clone() const
{
    return new CubicBSpline(*this);
}

void CubicBSpline::insert(const double t)
{
    // if (std::find(_knots.begin(), _knots.end(), t) != _knots.end())
    // {
    //     return;
    // }
    size_t k = 0;
    for (size_t i = 1, count = _knots.size(); i < count; ++i)
    {
        if (_knots[i - 1] <= t && t <= _knots[i])
        {
            k = --i;
            break;
        }
    }

    const size_t npts = control_points.size();
    std::vector<double> nbasis;
    rbasis(3, t, npts, _knots, nbasis);
    Geo::Point anchor;
    for (size_t i = 0; i < npts; ++i)
    {
        anchor += control_points[i] * nbasis[i];
    }

    Geo::Point array[3];
    for (size_t i = k, j = 2; i > k - 3; --i, --j)
    {
        double alpha = (t - _knots[i]);
        double dev = (_knots[i + 3] - _knots[i]);
        alpha = (dev == 0) ? 0 : alpha / dev;
        array[j] = control_points[i - 1] * (1 - alpha) + control_points[i] * alpha;
    }
    for (size_t i = k - 2, j = 0; i < k; ++i, ++j)
    {
        control_points[i] = array[j];
    }
    control_points.insert(control_points.begin() + k, array[2]);
    _knots.insert(_knots.begin() + k + 1, t);

    std::vector<double> lenghts({0});
    for (size_t i = 1, count = _shape.size(); i < count; ++i)
    {
        lenghts.push_back(lenghts.back() + Geo::distance(_shape[i - 1], _shape[i]));
    }
    std::vector<double> distances;
    for (const Geo::Point &point : path_points)
    {
        size_t index = 0;
        double min_dis = DBL_MAX;
        for (size_t i = 0, count = _shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(_shape[i], point); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        distances.push_back(lenghts[index]);
    }

    double anchor_dis = 0;
    {
        size_t index = 0;
        double min_dis = DBL_MAX;
        for (size_t i = 0, count = _shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(_shape[i], anchor); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        anchor_dis = lenghts[index];
    }

    for (size_t i = 1, count = path_points.size(); i < count; ++i)
    {
        if (distances[i - 1] <= anchor_dis && anchor_dis <= distances[i])
        {
            path_points.insert(path_points.begin() + i, anchor);
            _path_values.insert(_path_values.begin() + i, t);
            break;
        }
    }
}

Geo::Point CubicBSpline::at(const double t) const
{
    if (t <= _knots.front())
    {
        return control_points.front();
    }
    else if (t >= _knots.back())
    {
        return control_points.back();
    }
    const size_t npts = control_points.size();
    std::vector<double> nbasis;
    rbasis(3, t, npts, _knots, nbasis);
    Geo::Point point;
    for (size_t i = 0; i < npts; ++i)
    {
        point += control_points[i] * nbasis[i];
    }
    return point;
}

Geo::Point CubicBSpline::tangent(const double t) const
{
    if (t == _knots.front())
    {
        return control_points[1] - control_points.front();
    }
    else if (t == _knots.back())
    {
        return control_points.back() - control_points[control_points.size() - 2];
    }
    const size_t npts = control_points.size() - 1;
    std::vector<double> nbasis;
    const std::vector<double> knots(_knots.begin() + 1, _knots.end() - 1); // 二阶导数的节点矢量
    rbasis(2, t, npts, knots, nbasis);
    std::vector<Geo::Point> points;
    for (size_t i = 0; i < npts; ++i)
    {
        const double denom = _knots[i + 4] - _knots[i + 1];
        points.emplace_back((control_points[i + 1] - control_points[i]) * 3 / denom);
    }
    Geo::Point tang;
    for (size_t i = 0; i < npts; ++i)
    {
        tang += points[i] * nbasis[i];
    }
    return tang;
}

Geo::Point CubicBSpline::vertical(const double t) const
{
    const Geo::Point tan = tangent(t);
    return Geo::Point(-tan.y, tan.x);
}

void CubicBSpline::extend_front(const Geo::Point &expoint)
{
    const Geo::Point head(control_points.front());
    control_points.insert(control_points.begin(), (head + expoint * 2) / 3);
    control_points.insert(control_points.begin(), (head * 2 + expoint) / 3);
    control_points.insert(control_points.begin(), expoint);

    path_points.insert(path_points.begin(), expoint);

    if (_knots.front() -= 1; _knots.front() < 0)
    {
        const double v = -_knots.front();
        std::for_each(_knots.begin(), _knots.end(), [=](double &k) { k += v; });
        std::for_each(_path_values.begin(), _path_values.end(), [=](double &k) { k += v; });
    }
    _knots.insert(_knots.begin(), _knots.front());
    _knots.insert(_knots.begin(), _knots.front());
    _knots.insert(_knots.begin(), _knots.front());
    _path_values.insert(_path_values.begin(), _knots.front());
}

void CubicBSpline::extend_back(const Geo::Point &expoint)
{
    const Geo::Point head(control_points.back());
    control_points.emplace_back((head + expoint * 2) / 3);
    control_points.emplace_back((head * 2 + expoint) / 3);
    control_points.emplace_back(expoint);

    path_points.emplace_back(expoint);

    _knots.back() += 1;
    _knots.push_back(_knots.back());
    _knots.push_back(_knots.back());
    _knots.push_back(_knots.back());
    _path_values.push_back(_knots.back());
}


// Arc
Arc::Arc(const double x0, const double y0, const double x1, const double y1, const double x2, const double y2)
{
    assert(x0 != x2 || y0 != y2);
    control_points[0].x = x0, control_points[0].y = y0;
    control_points[2].x = x2, control_points[2].y = y2;
    const double a = x0 - x1, b = y0 - y1, c = x0 - x2, d = y0 - y2;
    const double e = (x0 * x0 - x1 * x1 + y0 * y0 - y1 * y1) / 2;
    const double f = (x0 * x0 - x2 * x2 + y0 * y0 - y2 * y2) / 2;
    const double t = b * c - a * d;
    // assert(t != 0);
    x = (b * f - d * e) / t, y = (c * e - a * f) / t;
    radius = (std::hypot(x - x0, y - y0) + std::hypot(x - x1, y - y1) + std::hypot(x - x2, y - y2)) / 3;
    if ((x1 - x0) * (y2 - y1) > (y1 - y0) * (x2 - x1)) // 逆时针
    {
        control_points[1] = Geo::Point(x, y) + (control_points[0] - control_points[2]).vertical().normalize() * radius;
    }
    else // 顺时针
    {
        control_points[1] = Geo::Point(x, y) + (control_points[2] - control_points[0]).vertical().normalize() * radius;
    }
    update_shape(Circle::default_down_sampling_value);
}

Arc::Arc(const Point &point0, const Point &point1, const Point &point2) : Arc(point0.x, point0.y, point1.x, point1.y, point2.x, point2.y)
{
}

Arc::Arc(const double x0, const double y0, const double x1, const double y1, const double param, const ParameterType type,
         const bool counterclockwise)
{
    switch (type)
    {
    case ParameterType::StartCenterAngle:
        {
            control_points[0].x = control_points[2].x = x0;
            control_points[0].y = control_points[2].y = y0;
            x = x1, y = y1, radius = std::hypot(x1 - x0, y1 - y0);
            assert(radius != 0 && param != 0);
            control_points[2].rotate(x, y, param);
            if (counterclockwise)
            {
                Geo::Vector vec = (control_points[0] - control_points[2]).vertical().normalize() * radius;
                control_points[1].x = x + vec.x, control_points[1].y = y + vec.y;
            }
            else
            {
                Geo::Vector vec = (control_points[2] - control_points[0]).vertical().normalize() * radius;
                control_points[1].x = x + vec.x, control_points[1].y = y + vec.y;
            }
        }
        break;
    case ParameterType::StartEndAngle:
        {
            control_points[0].x = x0, control_points[0].y = y0;
            control_points[2].x = x1, control_points[2].y = y1;
            radius = param == Geo::PI ? std::hypot(x0 - x1, y0 - y1) / 2
                                      : std::hypot(x0 - x1, y0 - y1) / std::sin(param) * std::sin((Geo::PI - param) / 2);
            assert(radius > 0);
            const Geo::Point temp = (control_points[0] + control_points[2]) / 2;
            if (counterclockwise)
            {
                if (param <= Geo::PI)
                {
                    Geo::Vector vec = (control_points[0] - control_points[2]).vertical().normalize() * radius;
                    x = temp.x + vec.x * std::cos(param / 2), y = temp.y + vec.y * std::cos(param / 2);
                    control_points[1].x = x - vec.x, control_points[1].y = y - vec.y;
                }
                else
                {
                    Geo::Vector vec = (control_points[2] - control_points[0]).vertical().normalize() * radius;
                    x = temp.x + vec.x * std::cos(param / 2), y = temp.y + vec.y * std::cos(param / 2);
                    control_points[1].x = x + vec.x, control_points[1].y = y + vec.y;
                }
            }
            else
            {
                if (param <= Geo::PI)
                {
                    Geo::Vector vec = (control_points[2] - control_points[0]).vertical().normalize() * radius;
                    x = temp.x + vec.x * std::cos(param / 2), y = temp.y + vec.y * std::cos(param / 2);
                    control_points[1].x = x - vec.x, control_points[1].y = y - vec.y;
                }
                else
                {
                    Geo::Vector vec = (control_points[0] - control_points[2]).vertical().normalize() * radius;
                    x = temp.x + vec.x * std::cos(param / 2), y = temp.y + vec.y * std::cos(param / 2);
                    control_points[1].x = x + vec.x, control_points[1].y = y + vec.y;
                }
            }
        }
        break;
    case ParameterType::StartEndRadius:
        {
            control_points[0].x = x0, control_points[0].y = y0;
            control_points[2].x = x1, control_points[2].y = y1;
            radius = param;
            assert(radius > 0);
            const double a = std::hypot(x1 - x0, y1 - y0) / 2;
            const double b = std::sqrt(radius * radius - a * a);
            const Geo::Point temp = (control_points[0] + control_points[2]) / 2;
            if (counterclockwise)
            {
                Geo::Vector vec = (control_points[0] - control_points[2]).vertical().normalize();
                x = temp.x + vec.x * b, y = temp.y + vec.y * b;
                control_points[1].x = x - vec.x * radius, control_points[1].y = y - vec.y * radius;
            }
            else
            {
                Geo::Vector vec = (control_points[2] - control_points[0]).vertical().normalize();
                x = temp.x + vec.x * b, y = temp.y + vec.y * b;
                control_points[1].x = x - vec.x * radius, control_points[1].y = y - vec.y * radius;
            }
        }
        break;
    default:
        assert(type == ParameterType::StartCenterAngle || type == ParameterType::StartEndAngle || type == ParameterType::StartEndRadius);
        break;
    }
    update_shape(Circle::default_down_sampling_value);
}

Arc::Arc(const Point &point0, const Point &point1, const double param, const ParameterType type, const bool counterclockwise)
    : Arc(point0.x, point0.y, point1.x, point1.y, param, type, counterclockwise)
{
}

Arc::Arc(const double x_, const double y_, const double radius_, const double start_angle, const double end_angle,
         const bool counterclockwise)
    : x(x_), y(y_), radius(radius_)
{
    control_points[2].x = control_points[0].x = x_ + radius_;
    control_points[2].y = control_points[0].y = y_;
    control_points[0].rotate(x_, y_, start_angle);
    control_points[2].rotate(x_, y_, end_angle);
    if (counterclockwise)
    {
        control_points[1] = Geo::Point(x_, y_) + (control_points[0] - control_points[2]).vertical().normalize() * radius_;
    }
    else
    {
        control_points[1] = Geo::Point(x_, y_) + (control_points[2] - control_points[0]).vertical().normalize() * radius_;
    }
    update_shape(Circle::default_down_sampling_value);
}

Arc::Arc(const double x0, const double y0, const double x1, const double y1, const double bulge)
{
    assert(x0 != x1 || y0 != y1);
    assert(-1 <= bulge && bulge <= 1 && bulge != 0);
    control_points[0].x = x0, control_points[0].y = y0;
    control_points[2].x = x1, control_points[2].y = y1;
    radius = (Geo::distance(x0, y0, x1, y1) * (1 + std::pow(bulge, 2))) / (4 * std::abs(bulge));
    {
        const Geo::Point line(x1 - x0, y1 - y0);
        const Geo::Point half_line = line / 2;
        const double line_to_center_angle = Geo::PI / 2 - std::atan(bulge) * 2;
        if (std::abs(line_to_center_angle) < Geo::EPSILON)
        {
            x = half_line.x + x0;
            y = half_line.y + y0;
        }
        else
        {
            const Geo::Point point = half_line + Geo::Point(-half_line.y, half_line.x) * std::tan(line_to_center_angle);
            x = point.x + x0;
            y = point.y + y0;
        }
    }
    if (bulge < 0)
    {
        control_points[1] = Geo::Point(x, y) + (control_points[2] - control_points[0]).vertical().normalize() * radius;
    }
    else
    {
        control_points[1] = Geo::Point(x, y) + (control_points[0] - control_points[2]).vertical().normalize() * radius;
    }
}

Arc::Arc(const Point &point0, const Point &point1, const double bulge) : Arc(point0.x, point0.y, point1.x, point1.y, bulge)
{
}

Arc::Arc(const Arc &arc) : Geometry(arc), x(arc.x), y(arc.y), radius(arc.radius), _shape(arc._shape)
{
    control_points[0] = arc.control_points[0];
    control_points[1] = arc.control_points[1];
    control_points[2] = arc.control_points[2];
}

Arc &Arc::operator=(const Arc &arc)
{
    if (this != &arc)
    {
        Geometry::operator=(arc);
        x = arc.x;
        y = arc.y;
        radius = arc.radius;
        _shape = arc._shape;
        control_points[0] = arc.control_points[0];
        control_points[1] = arc.control_points[1];
        control_points[2] = arc.control_points[2];
    }
    return *this;
}

const Type Arc::type() const
{
    return Type::ARC;
}

const double Arc::area() const
{
    double angle = Geo::angle(control_points[0], Geo::Point(x, y), control_points[2]);
    if (is_cw())
    {
        if (angle > 0)
        {
            angle -= Geo::PI * 2;
        }
    }
    else
    {
        if (angle < 0)
        {
            angle += Geo::PI * 2;
        }
    }
    return radius * radius * std::abs(angle) / 2;
}

const double Arc::length() const
{
    double angle = Geo::angle(control_points[0], Geo::Point(x, y), control_points[2]);
    if (is_cw())
    {
        if (angle > 0)
        {
            angle -= Geo::PI * 2;
        }
    }
    else
    {
        if (angle < 0)
        {
            angle += Geo::PI * 2;
        }
    }
    return radius * std::abs(angle);
}

const bool Arc::empty() const
{
    return radius == 0 || control_points[0] == control_points[2];
}

void Arc::clear()
{
    x = y = radius = 0;
    control_points[0].x = control_points[1].x = control_points[2].x = 0;
    control_points[0].y = control_points[1].y = control_points[2].y = 0;
}

Arc *Arc::clone() const
{
    return new Arc(*this);
}

void Arc::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    const double x_ = x, y_ = y;
    x = a * x_ + b * y_ + c;
    y = d * x_ + e * y_ + f;
    for (Point &point : control_points)
    {
        point.transform(a, b, c, d, e, f);
    }
    radius =
        (Geo::distance(x, y, control_points[0].x, control_points[0].y) + Geo::distance(x, y, control_points[1].x, control_points[1].y) +
         Geo::distance(x, y, control_points[2].x, control_points[2].y)) /
        3;
    if (std::abs(a) == 1 && std::abs(e) == 1)
    {
        _shape.transform(a, b, c, d, e, f);
    }
    else
    {
        update_shape(Geo::Circle::default_down_sampling_value);
    }
}

void Arc::transform(const double mat[6])
{
    const double x_ = x, y_ = y;
    x = mat[0] * x_ + mat[1] * y_ + mat[2];
    y = mat[3] * x_ + mat[4] * y_ + mat[5];
    for (Point &point : control_points)
    {
        point.transform(mat);
    }
    radius =
        (Geo::distance(x, y, control_points[0].x, control_points[0].y) + Geo::distance(x, y, control_points[1].x, control_points[1].y) +
         Geo::distance(x, y, control_points[2].x, control_points[2].y)) /
        3;
    if (std::abs(mat[0]) == 1 && std::abs(mat[4]) == 1)
    {
        _shape.transform(mat);
    }
    else
    {
        update_shape(Geo::Circle::default_down_sampling_value);
    }
}

void Arc::translate(const double tx, const double ty)
{
    x += tx;
    y += ty;
    _shape.translate(tx, ty);
    for (Point &point : control_points)
    {
        point.translate(tx, ty);
    }
}

void Arc::scale(const double x_, const double y_, const double k)
{
    const double x1 = x, y1 = y;
    x = k * x1 + x_ * (1 - k);
    y = k * y1 + y_ * (1 - k);
    radius *= k;
    for (Point &point : control_points)
    {
        point.scale(x_, y_, k);
    }
    update_shape(Geo::Circle::default_down_sampling_value);
}

void Arc::rotate(const double x_, const double y_, const double rad)
{
    x -= x_;
    y -= y_;
    const double x1 = x, y1 = y;
    x = x1 * std::cos(rad) - y1 * std::sin(rad);
    y = x1 * std::sin(rad) + y1 * std::cos(rad);
    x += x_;
    y += y_;
    for (Point &point : control_points)
    {
        point.rotate(x_, y_, rad);
    }
    _shape.rotate(x_, y_, rad);
}

Polygon Arc::convex_hull() const
{
    return mini_bounding_rect();
}

AABBRect Arc::bounding_rect() const
{
    double left = std::min({control_points[0].x, control_points[1].x, control_points[2].x});
    if (Geo::distance(Geo::Point(x - radius, y), *this) < Geo::EPSILON)
    {
        left = x - radius;
    }
    double right = std::max({control_points[0].x, control_points[1].x, control_points[2].x});
    if (Geo::distance(Geo::Point(x + radius, y), *this) < Geo::EPSILON)
    {
        right = x + radius;
    }
    double top = std::max({control_points[0].y, control_points[1].y, control_points[2].y});
    if (Geo::distance(Geo::Point(x, y + radius), *this) < Geo::EPSILON)
    {
        top = y + radius;
    }
    double bottom = std::min({control_points[0].y, control_points[1].y, control_points[2].y});
    if (Geo::distance(Geo::Point(x, y - radius), *this) < Geo::EPSILON)
    {
        bottom = y - radius;
    }
    return AABBRect(left, top, right, bottom);
}

Polygon Arc::mini_bounding_rect() const
{
    const double a = Geo::distance(control_points[0], control_points[2]) / 2;
    const double b = std::sqrt(radius * radius - a * a);
    const Geo::Vector vec = (control_points[1] - Geo::Point(x, y)).normalize();
    if (Geo::distance(control_points[1], control_points[0], control_points[2]) <= radius)
    {
        return AABBRect(control_points[0] + vec * b, control_points[2]);
    }
    else
    {
        return AABBRect(control_points[0] + vec * (b + radius), control_points[2]);
    }
}

void Arc::update_shape(const double down_sampling_value)
{
    const Point center(x, y);
    const double angle0 = Geo::angle(center, control_points[0]);
    const double angle1 = Geo::angle(center, control_points[2]);
    _shape = Geo::arc_to_polyline(center, radius, angle0, angle1, is_cw(), down_sampling_value);
}

const Polyline &Arc::shape() const
{
    return _shape;
}

bool Arc::is_cw() const
{
    return (control_points[1].x - control_points[0].x) * (control_points[2].y - control_points[1].y) <
           (control_points[1].y - control_points[0].y) * (control_points[2].x - control_points[1].x);
}

double Arc::angle() const
{
    double angle = Geo::angle(control_points[0], Geo::Point(x, y), control_points[2]);
    if (is_cw())
    {
        if (angle > 0)
        {
            angle -= Geo::PI * 2;
        }
    }
    else
    {
        if (angle < 0)
        {
            angle += Geo::PI * 2;
        }
    }
    return angle;
}