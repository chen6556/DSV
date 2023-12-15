#include "base/Geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

using Geo::AxisAlignedBoundingBox;

AxisAlignedBoundingBox::AxisAlignedBoundingBox()
    : Rectangle(), _topLeft(), _width(0), _height(0)
{
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const double x0, const double y0, const double x1, const double y1)
    : Rectangle(x0, y0, x1, y1), _topLeft(std::min(x0, x1), std::min(y0, y1)), _width(std::abs(x1 - x0)), _height(std::abs(y1 - y0))
{
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const Point &point0, const Point &point1)
    : AxisAlignedBoundingBox(point0.coord().x, point0.coord().y, point1.coord().x, point1.coord().y)
{
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const AxisAlignedBoundingBox &rect)
    : Rectangle(rect), _topLeft(rect._topLeft), _width(rect._width), _height(rect._height)
{
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const AxisAlignedBoundingBox &&rect)
    : _topLeft(rect._topLeft), _width(rect._width), _height(rect._height), Rectangle(std::move(rect))
{
}
AxisAlignedBoundingBox &AxisAlignedBoundingBox::operator=(const AxisAlignedBoundingBox &rect)
{
    if (this != &rect)
    {
        new (this) AxisAlignedBoundingBox(rect);
    }
    return *this;
}

AxisAlignedBoundingBox &AxisAlignedBoundingBox::operator=(const AxisAlignedBoundingBox &&rect)
{
    if (this != &rect)
    {
        new (this) AxisAlignedBoundingBox(rect);
    }
    return *this;
}
Geo::AxisAlignedBoundingBox *AxisAlignedBoundingBox::clone() const
{
    return new AxisAlignedBoundingBox(*this);
}

const double AxisAlignedBoundingBox::left() const
{
    return _topLeft.coord().x;
}

const double AxisAlignedBoundingBox::top() const
{
    return _topLeft.coord().y;
}

const double AxisAlignedBoundingBox::right() const
{
    return left() + width();
}

const double AxisAlignedBoundingBox::bottom() const
{
    return top() + height();
}

const double AxisAlignedBoundingBox::width() const
{
    return _width;
}

const double AxisAlignedBoundingBox::height() const
{
    return _height;
}

const double AxisAlignedBoundingBox::length() const
{
    return (width() + height()) * 2;
}

const double AxisAlignedBoundingBox::area() const
{
    return width() * height();
}

inline bool doubleEq(double value, double target)
{
    //------------+------+------+----------------
    //           -e    target   +e
    // -e = target - Geo::EPSILON
    // +e = target + Geo::EPSILON
    // value == target => -e < value < e
    // value>=target => value > -e
    // value<=target => value < +e
    return std::abs(target - value) < Geo::EPSILON;
}
inline bool doubleGE(double value, double target)
{
    return value > target - Geo::EPSILON;
}
inline bool doubleLE(double value, double target)
{
    return value < target + Geo::EPSILON;
}

inline bool inRange(double value, double left, double right, bool leftClose = true, bool rightClose = true)
{
    return (leftClose ? doubleGE(value, left) : value > left) && (rightClose ? doubleLE(value, right) : value < right);
}
inline bool inCloseRange(double value, double left, double right)
{
    return inRange(value, left, right, true, true);
}
inline bool inOpenRange(double value, double left, double right)
{
    return inRange(value, left, right, false, false);
}

const bool AxisAlignedBoundingBox::isPointInside(const double x, const double y, bool edgeConsider) const
{
    if(this->empty())
        return false;
    return inRange(x, left(), right(), edgeConsider, edgeConsider) && inRange(y, top(), bottom(), edgeConsider, edgeConsider);
}

const bool AxisAlignedBoundingBox::isInside(const Point &point, bool edgeConsider) const
{
    if(this->empty())
        return false;
    return this->isPointInside(point.coord().x, point.coord().y, edgeConsider);
}
const bool AxisAlignedBoundingBox::isInside(const Line &line, bool edgeConsider) const
{
    if(this->empty())
        return false;
    return this->isInside(line.front()) && this->isInside(line.back());
}

const bool AxisAlignedBoundingBox::isInside(const Circle &circle, bool edgeConsider) const
{
    if(this->empty())
        return false;
    Point point = circle.center();
    if (!this->isInside(point, edgeConsider))
    {
        return false;
    }
    double distance_x = point.coord().x - left();
    double distance_y = point.coord().y - top();
    double distance_limit = std::min({distance_x, distance_y, width() - distance_x, height() - distance_y});
    return inRange(circle.radius(), 0, distance_limit, edgeConsider, edgeConsider);
}
const bool AxisAlignedBoundingBox::isIntersected(const Point &point, bool insideConsider) const
{
    if(this->empty())
        return false;
    // 点与AABB相交仅考虑边框情况，除非设置insideConsider
    double x = point.coord().x, y = point.coord().y;
    bool result = doubleEq(x, left()) || doubleEq(x, right()) || doubleEq(y, top()) || doubleEq(y, bottom());
    if (insideConsider)
    {
        result |= this->isPointInside(x, y);
    }
    return result;
}
// 将线段投影到轴axis上,并计算两个端点对应的模长，从而得到端点在轴上的位置
inline double projectOntoAxis(const Geo::Point &vec, const Geo::Point &axis)
{
    // 点投影到轴b上 proj = (a*b/|b|^2 ) * b; 这里我们只关心系数，即括号内的部分
    // proj_x =  (a_x*b_x + a_y*b_y) / (b_x^2+b_y^2)^(1/2)
    Geo::Coord a = vec.coord();
    Geo::Coord b = axis.coord();
    return (a.x * b.x + a.y * b.y) / (b.x * b.x + b.y * b.y);
}
inline void projectOntoAxis(const Geo::Line &line, const Geo::Point &axis, double &minProj, double &maxProj)
{
    double proj1 = projectOntoAxis(line.front(), axis);
    double proj2 = projectOntoAxis(line.back(), axis);
    minProj = std::min(proj1, proj2);
    maxProj = std::max(proj1, proj2);
}

bool isOverLap(double begin0, double end0, double begin1, double end1)
{
    return begin0 <= end1 && end0 >= begin1;
}
const bool AxisAlignedBoundingBox::isIntersected(const Line &line, bool edgeConsider) const
{
    if(this->empty())
        return false;
    // 分离轴定理
    // 计算线段法向量
    Point dir = line.back() - line.front();      // 方向向量
    Point normal(-dir.coord().y, dir.coord().x); // 法向量

    Point project_axis[3] = {{1, 0}, {0, 1}, normal};
    // 线段投影
    double lineProject[3][2];
    for (int i = 0; i < 3; i++)
    {
        projectOntoAxis(line, project_axis[i], lineProject[i][0], lineProject[i][0]);
    }
    // box投影
    // clang-format off
    double boxProject[3][2] = {{left(), right()},
                               {top(), bottom()},
                               {std::numeric_limits<double>::max(), std::numeric_limits<double>::min()}};
    for (int i = 0; i < 3; i++)
    {
        double proj = projectOntoAxis(_points[i], project_axis[i]);
        boxProject[2][0] = std::min(boxProject[2][0], proj);
        boxProject[2][1] = std::max(boxProject[2][1], proj);
    }
    return isOverLap(boxProject[0][0], boxProject[0][1], lineProject[0][0], lineProject[0][1])
        && isOverLap(boxProject[1][0], boxProject[1][1], lineProject[1][0], lineProject[1][1])
        && isOverLap(boxProject[2][0], boxProject[2][1], lineProject[2][0], lineProject[2][1]);
    // clang-format on
}
const bool AxisAlignedBoundingBox::isIntersected(const Circle &circle, bool insideConsider) const
{
    if (this->empty())
        return false;
    // AABB上最接近圆心的坐标
    double x, y;
    Coord c = circle.center().coord();
    x = c.x < left() ? left() : (c.x > right() ? right() : c.x);
    y = c.y < top() ? top() : (c.y > bottom() ? bottom() : c.y);
    double distance = Geo::distance(Point(x, y), Point(c));
    return doubleLE(distance, circle.radius());
}

const bool AxisAlignedBoundingBox::isIntersected(const AxisAlignedBoundingBox &box, bool edgeConsider) const{
    if(this->empty()||box.empty())
        return false;
    Vector v = box.center()-this->center();
    double d_x = std::abs(v.coord().x);
    double d_y = std::abs(v.coord().y);
    return inRange(d_x, 0, (width() + box.width()) / 2,false, edgeConsider) 
    && inRange(d_y, 0, (height() + box.height()) / 2, false, edgeConsider);
}