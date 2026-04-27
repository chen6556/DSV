#include <QDebug>
#include "Dimension.hpp"
#include "Algorithm.hpp"


using namespace Dim;


void Dimension::clear()
{
    this->txt.clear();
}

const Geo::Type Dimension::type() const
{
    return Geo::Type::DIMENSION;
}

const bool Dimension::empty() const
{
    return this->txt.isEmpty();
}

void Dimension::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    this->anchor[0].transform(a, b, c, d, e, f);
    this->anchor[1].transform(a, b, c, d, e, f);
    this->label.transform(a, b, c, d, e, f);
}

void Dimension::transform(const double mat[6])
{
    this->anchor[0].transform(mat);
    this->anchor[1].transform(mat);
    this->label.transform(mat);
}

void Dimension::translate(const double tx, const double ty)
{
    this->anchor[0].translate(tx, ty);
    this->anchor[1].translate(tx, ty);
    this->label.translate(tx, ty);
}

void Dimension::rotate(const double x, const double y, const double rad)
{
    this->anchor[0].rotate(x, y, rad);
    this->anchor[1].rotate(x, y, rad);
    this->label.rotate(x, y, rad);
}

void Dimension::scale(const double x, const double y, const double k)
{
    this->anchor[0].scale(x, y, k);
    this->anchor[1].scale(x, y, k);
    this->label.scale(x, y, k);
}

void Dimension::paint(QPainter &painter) const
{
    double rad = Geo::angle(this->anchor[0], this->anchor[1]);
    if (rad > Geo::PI / 2)
    {
        rad -= Geo::PI;
    }
    else if (rad < -Geo::PI / 2)
    {
        rad += Geo::PI;
    }
    QFont font = painter.font();
    font.setPointSize(this->font_size);
    painter.setFont(font);
    painter.rotate(-Geo::rad_to_degree(rad));
    painter.drawText(0, -1, this->txt);
}

void Dimension::arrow(const Geo::Point &anchor, const Geo::Point &direction, const double size, double data[6])
{
    data[0] = anchor.x, data[1] = anchor.y;
    Geo::Vector vec = direction.normalized() * 3 * size;
    vec.x = -vec.x, vec.y = -vec.y;
    const Geo::Point base = anchor + vec;
    Geo::Point point = base + vec.vertical().normalize() * size;
    data[2] = point.x, data[3] = point.y;
    point = base - vec.vertical().normalize() * size;
    data[4] = point.x, data[5] = point.y;
}


DimAligned::DimAligned(const Geo::Point &start, const Geo::Point &end, const double height)
{
    this->anchor[0] = start, this->anchor[1] = end;
    this->txt = QString::number(Geo::distance(start, end), 'f', 4);
    set_height(height);
}

void DimAligned::set_height(const double height)
{
    _height = height;
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    this->label = (this->anchor[0] + this->anchor[1]) / 2 + vec;
    qDebug() << this->label.x << ',' << this->label.y;
}

const Type DimAligned::dim_type() const
{
    return Type::ALIGNED;
}

DimAligned *DimAligned::clone() const
{
    return new DimAligned(*this);
}

Geo::Polygon DimAligned::convex_hull() const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    return Geo::Polygon({this->anchor[0], this->anchor[0] + vec, this->anchor[1] + vec, this->anchor[1]});
}

Geo::AABBRect DimAligned::bounding_rect() const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    vec.normalize();
    vec *= (std::abs(_height) + 5);
    if (Geo::distance(point0, point1) <= 10)
    {
        const Geo::Vector dir(point1 - point0);
        point0 -= dir.normalized() * 7 * this->arrow_size;
        point1 += dir.normalized() * 7 * this->arrow_size;
    }
    const double left =
        std::min({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x + vec.x, this->anchor[1].x + vec.x, point0.x, point1.x});
    const double top =
        std::max({this->anchor[0].y, this->anchor[1].y, this->anchor[0].y + vec.y, this->anchor[1].y + vec.y, point0.y, point1.y});
    const double right =
        std::max({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x + vec.x, this->anchor[1].x + vec.x, point0.x, point1.x});
    const double bottom =
        std::min({this->anchor[0].y, this->anchor[1].y, this->anchor[0].y + vec.y, this->anchor[1].y + vec.y, point0.y, point1.y});
    return Geo::AABBRect(left, top, right, bottom);
}

Geo::Polygon DimAligned::mini_bounding_rect() const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    return Geo::Polygon({this->anchor[1], this->anchor[0], this->anchor[0] + vec, this->anchor[1] + vec});
}

Geo::AABBRectParams DimAligned::aabbrect_params() const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    vec.normalize();
    vec *= (std::abs(_height) + 5);
    if (Geo::distance(point0, point1) <= 10)
    {
        const Geo::Vector dir(point1 - point0);
        point0 -= dir.normalized() * 7 * this->arrow_size;
        point1 += dir.normalized() * 7 * this->arrow_size;
    }

    Geo::AABBRectParams param;
    param.left = std::min({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x + vec.x, this->anchor[1].x + vec.x, point0.x, point1.x});
    param.top = std::max({this->anchor[0].y, this->anchor[1].y, this->anchor[0].y + vec.y, this->anchor[1].y + vec.y, point0.y, point1.y});
    param.right =
        std::max({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x + vec.x, this->anchor[1].x + vec.x, point0.x, point1.x});
    param.bottom =
        std::min({this->anchor[0].y, this->anchor[1].y, this->anchor[0].y + vec.y, this->anchor[1].y + vec.y, point0.y, point1.y});
    return param;
}

void DimAligned::set_anchor(const int index, const double x, const double y)
{
    assert(index == 0 || index == 1);
    if (index == 0)
    {
        this->anchor[0].x = x, this->anchor[0].y = y;
    }
    else
    {
        this->anchor[1].x = x, this->anchor[1].y = y;
    }
    this->txt = QString::number(Geo::distance(this->anchor[0], this->anchor[1]), 'f', 4);
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    this->label = (this->anchor[0] + this->anchor[1]) / 2 + vec;
    qDebug() << this->label.x << ',' << this->label.y;
}

void DimAligned::paintable_lines(std::vector<double> &data) const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    const Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    vec.normalize();
    vec *= (std::abs(_height) + 5);

    data.push_back(this->anchor[0].x);
    data.push_back(this->anchor[0].y);
    data.push_back(this->anchor[0].x + vec.x);
    data.push_back(this->anchor[0].y + vec.y);

    if (Geo::distance(point0, point1) > 10)
    {
        data.push_back(point0.x);
        data.push_back(point0.y);
        data.push_back(point1.x);
        data.push_back(point1.y);
    }
    else
    {
        const Geo::Vector dir(point1 - point0);
        const Geo::Point start = point0 - dir.normalized() * 7 * this->arrow_size;
        const Geo::Point end = point1 + dir.normalized() * 7 * this->arrow_size;
        data.push_back(start.x);
        data.push_back(start.y);
        data.push_back(end.x);
        data.push_back(end.y);
    }

    data.push_back(this->anchor[1].x);
    data.push_back(this->anchor[1].y);
    data.push_back(this->anchor[1].x + vec.x);
    data.push_back(this->anchor[1].y + vec.y);
}

void DimAligned::paintable_arrows(std::vector<double> &data) const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    const Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    if (double temp[6]; Geo::distance(point0, point1) > 10)
    {
        arrow(point0, point0 - point1, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
        arrow(point1, point1 - point0, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
    }
    else
    {
        arrow(point0, point1 - point0, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
        arrow(point1, point0 - point1, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
    }
}

bool DimAligned::select(const Geo::Point &point, const double distance) const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    const Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    vec.normalize();
    vec *= (std::abs(_height) + 5);

    if (Geo::distance(point0, point1) > 10)
    {
        if (Geo::distance(point, point0, point1, false) <= distance)
        {
            return true;
        }
    }
    else
    {
        const Geo::Vector dir(point1 - point0);
        const Geo::Point start = point0 - dir.normalized() * 7 * this->arrow_size;
        const Geo::Point end = point1 + dir.normalized() * 7 * this->arrow_size;
        if (Geo::distance(point, start, end, false) <= distance)
        {
            return true;
        }
    }

    if (Geo::Point point2(this->anchor[0].x + vec.x, this->anchor[0].y + vec.y);
        Geo::distance(point, this->anchor[0], point2, false) <= distance)
    {
        return true;
    }
    else if (Geo::Point point2(this->anchor[1].x + vec.x, this->anchor[1].y + vec.y);
             Geo::distance(point, this->anchor[1], point2, false) <= distance)
    {
        return true;
    }

    return false;
}

bool DimAligned::select(const Geo::AABBRect &rect) const
{
    Geo::Vector vec((this->anchor[1] - this->anchor[0]).vertical());
    vec.normalize();
    vec *= _height;
    const Geo::Point point0(this->anchor[0] + vec), point1(this->anchor[1] + vec);
    vec.normalize();
    vec *= (std::abs(_height) + 5);

    if (Geo::distance(point0, point1) > 10)
    {
        if (Geo::is_intersected(rect, point0, point1))
        {
            return true;
        }
    }
    else
    {
        const Geo::Vector dir(point1 - point0);
        const Geo::Point start = point0 - dir.normalized() * 7 * this->arrow_size;
        const Geo::Point end = point1 + dir.normalized() * 7 * this->arrow_size;
        if (Geo::is_intersected(rect, start, end))
        {
            return true;
        }
    }

    if (Geo::Point point(this->anchor[0].x + vec.x, this->anchor[0].y + vec.y); Geo::is_intersected(rect, this->anchor[0], point))
    {
        return true;
    }
    else if (Geo::Point point(this->anchor[1].x + vec.x, this->anchor[1].y + vec.y); Geo::is_intersected(rect, this->anchor[1], point))
    {
        return true;
    }

    return false;
}

double DimAligned::height() const
{
    return _height;
}


DimLinear::DimLinear(const Geo::Point &start, const Geo::Point &end, const bool horizontal, const double dis)
{
    this->anchor[0] = start, this->anchor[1] = end;
    this->txt = QString::number(std::abs(horizontal ? start.x - end.x : start.y - end.y), 'f', 4);
    set_distance(dis);
    set_horizontal(horizontal);
}

void DimLinear::set_distance(const double dis)
{
    _distance = dis;
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        this->label.x = mid.x;
        this->label.y = mid.y + dis;
    }
    else
    {
        this->label.x = mid.x + dis;
        this->label.y = mid.y;
    }
}

const Type DimLinear::dim_type() const
{
    return Type::LINEAR;
}

DimLinear *DimLinear::clone() const
{
    return new DimLinear(*this);
}

Geo::Polygon DimLinear::convex_hull() const
{
    std::vector<Geo::Point> points;
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        points.emplace_back(this->anchor[0]);
        points.emplace_back(this->anchor[0].x, mid.y + _distance);
        points.emplace_back(this->anchor[1]);
        points.emplace_back(this->anchor[1].x, mid.y + _distance);
    }
    else
    {
        points.emplace_back(this->anchor[0]);
        points.emplace_back(mid.x + _distance, this->anchor[0].y);
        points.emplace_back(this->anchor[1]);
        points.emplace_back(mid.x + _distance, this->anchor[1].y);
    }
    return Geo::Polygon(points.begin(), points.end());
}

Geo::AABBRect DimLinear::bounding_rect() const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        const int width = this->anchor[0].x < this->anchor[1].x ? 7 : -7;
        const double left = std::min({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x - width * this->arrow_size,
                                      this->anchor[1].x + width * this->arrow_size});
        const double top = std::max({this->anchor[0].y, this->anchor[1].y, mid.y + _distance,
                                     mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5),
                                     mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5)});
        const double right = std::max({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x - width * this->arrow_size,
                                       this->anchor[1].x + width * this->arrow_size});
        const double bottom = std::min({this->anchor[0].y, this->anchor[1].y, mid.y + _distance,
                                        mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5),
                                        mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5)});
        return Geo::AABBRect(left, top, right, bottom);
    }
    else
    {
        const int width = this->anchor[0].y < this->anchor[1].y ? 7 : -7;
        const double left = std::min({this->anchor[0].x, this->anchor[1].x, mid.x + _distance,
                                      mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5)});
        const double top = std::max({this->anchor[0].y, this->anchor[1].y, mid.y + _distance, this->anchor[0].y - width * this->arrow_size,
                                     this->anchor[1].y + width * this->arrow_size});
        const double right = std::max({this->anchor[0].x, this->anchor[1].x, mid.x + _distance,
                                       mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5)});
        const double bottom = std::min({this->anchor[0].y, this->anchor[1].y, mid.y + _distance,
                                        this->anchor[0].y - width * this->arrow_size, this->anchor[1].y + width * this->arrow_size});
        return Geo::AABBRect(left, top, right, bottom);
    }
}

Geo::Polygon DimLinear::mini_bounding_rect() const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        const double left = std::min(this->anchor[0].x, this->anchor[1].x);
        const double top = std::max({this->anchor[0].y, this->anchor[1].y, mid.y + _distance});
        const double right = std::max(this->anchor[0].x, this->anchor[1].x);
        const double bottom = std::min({this->anchor[0].y, this->anchor[1].y, mid.y + _distance});
        return Geo::AABBRect(left, top, right, bottom);
    }
    else
    {
        const double left = std::min({this->anchor[0].x, this->anchor[1].x, mid.x + _distance});
        const double top = std::max(this->anchor[0].y, this->anchor[1].y);
        const double right = std::max({this->anchor[0].x, this->anchor[1].x, mid.x + _distance});
        const double bottom = std::min(this->anchor[0].y, this->anchor[1].y);
        return Geo::AABBRect(left, top, right, bottom);
    }
}

Geo::AABBRectParams DimLinear::aabbrect_params() const
{
    Geo::AABBRectParams param;
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        const int width = this->anchor[0].x < this->anchor[1].x ? 7 : -7;
        param.left = std::min({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x - width * this->arrow_size,
                               this->anchor[1].x + width * this->arrow_size});
        param.top = std::max({this->anchor[0].y, this->anchor[1].y, mid.y + _distance,
                              mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5),
                              mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5)});
        param.right = std::max({this->anchor[0].x, this->anchor[1].x, this->anchor[0].x - width * this->arrow_size,
                                this->anchor[1].x + width * this->arrow_size});
        param.bottom = std::min({this->anchor[0].y, this->anchor[1].y, mid.y + _distance,
                                 mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5),
                                 mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5)});
    }
    else
    {
        const int width = this->anchor[0].y < this->anchor[1].y ? 7 : -7;
        param.left = std::min({this->anchor[0].x, this->anchor[1].x, mid.x + _distance,
                               mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5)});
        param.top = std::max({this->anchor[0].y, this->anchor[1].y, mid.y + _distance, this->anchor[0].y - width * this->arrow_size,
                              this->anchor[1].y + width * this->arrow_size});
        param.right = std::max({this->anchor[0].x, this->anchor[1].x, mid.x + _distance,
                                mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5)});
        param.bottom = std::min({this->anchor[0].y, this->anchor[1].y, mid.y + _distance, this->anchor[0].y - width * this->arrow_size,
                                 this->anchor[1].y + width * this->arrow_size});
    }
    return param;
}

void DimLinear::set_anchor(const int index, const double x, const double y)
{
    assert(index == 0 || index == 1);
    if (index == 0)
    {
        this->anchor[0].x = x;
        this->anchor[0].y = y;
    }
    else
    {
        this->anchor[1].x = x;
        this->anchor[1].y = y;
    }
    this->txt =
        QString::number(std::abs(_horizontal ? this->anchor[0].x - this->anchor[1].x : this->anchor[0].y - this->anchor[1].y), 'f', 4);
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        _distance = this->label.x - mid.x;
    }
    else
    {
        _distance = this->label.y - mid.y;
    }
}

void DimLinear::set_horizontal(const bool horizontal)
{
    _horizontal = horizontal;
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); horizontal)
    {
        this->label.x = mid.x;
        this->label.y = mid.y + _distance;
        this->txt = QString::number(std::abs(this->anchor[0].x - this->anchor[1].x), 'f', 4);
    }
    else
    {
        this->label.x = mid.x + _distance;
        this->label.y = mid.y;
        this->txt = QString::number(std::abs(this->anchor[0].y - this->anchor[1].y), 'f', 4);
    }
}

bool DimLinear::is_horizontal() const
{
    return _horizontal;
}

void DimLinear::paint(QPainter &painter) const
{
    QFont font = painter.font();
    font.setPointSize(this->font_size);
    painter.setFont(font);
    if (_horizontal)
    {
        painter.drawText(0, -1, this->txt);
    }
    else
    {
        painter.drawText(1, 0, this->txt);
    }
}

void DimLinear::paintable_lines(std::vector<double> &data) const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        data.push_back(this->anchor[0].x);
        data.push_back(this->anchor[0].y);
        data.push_back(this->anchor[0].x);
        data.push_back(mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5));

        if (std::abs(this->anchor[0].x - this->anchor[1].x) > 10)
        {
            data.push_back(this->anchor[0].x);
            data.push_back(mid.y + _distance);
            data.push_back(this->anchor[1].x);
            data.push_back(mid.y + _distance);
        }
        else
        {
            const int width = this->anchor[0].x < this->anchor[1].x ? 7 : -7;
            data.push_back(this->anchor[0].x - width * this->arrow_size);
            data.push_back(mid.y + _distance);
            data.push_back(this->anchor[1].x + width * this->arrow_size);
            data.push_back(mid.y + _distance);
        }

        data.push_back(this->anchor[1].x);
        data.push_back(this->anchor[1].y);
        data.push_back(this->anchor[1].x);
        data.push_back(mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5));
    }
    else
    {
        data.push_back(this->anchor[0].x);
        data.push_back(this->anchor[0].y);
        data.push_back(mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5));
        data.push_back(this->anchor[0].y);

        if (std::abs(this->anchor[0].y - this->anchor[1].y) > 10)
        {
            data.push_back(mid.x + _distance);
            data.push_back(this->anchor[0].y);
            data.push_back(mid.x + _distance);
            data.push_back(this->anchor[1].y);
        }
        else
        {
            const int width = this->anchor[0].y < this->anchor[1].y ? 7 : -7;
            data.push_back(mid.x + _distance);
            data.push_back(this->anchor[0].y - width * this->arrow_size);
            data.push_back(mid.x + _distance);
            data.push_back(this->anchor[1].y + width * this->arrow_size);
        }

        data.push_back(this->anchor[1].x);
        data.push_back(this->anchor[1].y);
        data.push_back(mid.x + _distance + (this->anchor[1].x <= mid.x + _distance ? 5 : -5));
        data.push_back(this->anchor[1].y);
    }
}

void DimLinear::paintable_arrows(std::vector<double> &data) const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        const Geo::Point point0(this->anchor[0].x, mid.y + _distance), point1(this->anchor[1].x, mid.y + _distance);
        if (double temp[6]; std::abs(this->anchor[0].x - this->anchor[1].x) > 10)
        {
            arrow(point0, Geo::Point(this->anchor[0].x - this->anchor[1].x, 0), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
            arrow(point1, Geo::Point(this->anchor[1].x - this->anchor[0].x, 0), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
        }
        else
        {
            arrow(point0, Geo::Point(this->anchor[1].x - this->anchor[0].x, 0), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
            arrow(point1, Geo::Point(this->anchor[0].x - this->anchor[1].x, 0), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
        }
    }
    else
    {
        const Geo::Point point0(mid.x + _distance, this->anchor[0].y), point1(mid.x + _distance, this->anchor[1].y);
        if (double temp[6]; std::abs(this->anchor[0].y - this->anchor[1].y) > 10)
        {
            arrow(point0, Geo::Point(0, this->anchor[0].y - this->anchor[1].y), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
            arrow(point1, Geo::Point(0, this->anchor[1].y - this->anchor[0].y), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
        }
        else
        {
            arrow(point0, Geo::Point(0, this->anchor[1].y - this->anchor[0].y), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
            arrow(point1, Geo::Point(0, this->anchor[0].y - this->anchor[1].y), this->arrow_size, temp);
            data.insert(data.end(), temp, temp + 6);
        }
    }
}

bool DimLinear::select(const Geo::Point &point, const double distance) const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        if (std::abs(this->anchor[0].x - this->anchor[1].x) > 10)
        {
            const Geo::Point point0(this->anchor[0].x, mid.y + _distance);
            const Geo::Point point1(this->anchor[1].x, mid.y + _distance);
            if (Geo::distance(point, point0, point1, false) <= distance)
            {
                return true;
            }
        }
        else
        {
            const int width = this->anchor[0].x < this->anchor[1].x ? 7 : -7;
            const Geo::Point point0(this->anchor[0].x - width * this->arrow_size, mid.y + _distance);
            const Geo::Point point1(this->anchor[1].x + width * this->arrow_size, mid.y + _distance);
            if (Geo::distance(point, point0, point1, false) <= distance)
            {
                return true;
            }
        }

        if (const Geo::Point point0(this->anchor[0].x, mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5));
            Geo::distance(point, this->anchor[0], point0, false) <= distance)
        {
            return true;
        }
        else if (const Geo::Point point0(this->anchor[1].x, mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5));
                 Geo::distance(point, this->anchor[1], point0, false) <= distance)
        {
            return true;
        }
    }
    else
    {
        if (std::abs(this->anchor[0].y - this->anchor[1].y) > 10)
        {
            const Geo::Point point0(mid.x + _distance, this->anchor[0].y);
            const Geo::Point point1(mid.x + _distance, this->anchor[1].y);
            if (Geo::distance(point, point0, point1, false) <= distance)
            {
                return true;
            }
        }
        else
        {
            const int width = this->anchor[0].y < this->anchor[1].y ? 7 : -7;
            const Geo::Point point0(mid.x + _distance, this->anchor[0].y - width * this->arrow_size);
            const Geo::Point point1(mid.x + _distance, this->anchor[1].y + width * this->arrow_size);
            if (Geo::distance(point, point0, point1, false) <= distance)
            {
                return true;
            }
        }

        if (const Geo::Point point0(mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5), this->anchor[0].y);
            Geo::distance(point, this->anchor[0], point0, false) <= distance)
        {
            return true;
        }
        else if (const Geo::Point point0(mid.x + _distance + (this->anchor[1].x <= mid.x + _distance ? 5 : -5), this->anchor[1].y);
                 Geo::distance(point, this->anchor[1], point0, false) <= distance)
        {
            return true;
        }
    }
    return false;
}

bool DimLinear::select(const Geo::AABBRect &rect) const
{
    if (const Geo::Point mid((this->anchor[0] + this->anchor[1]) / 2); _horizontal)
    {
        if (std::abs(this->anchor[0].x - this->anchor[1].x) > 10)
        {
            const Geo::Point point0(this->anchor[0].x, mid.y + _distance);
            const Geo::Point point1(this->anchor[1].x, mid.y + _distance);
            if (Geo::is_intersected(rect, point0, point1))
            {
                return true;
            }
        }
        else
        {
            const int width = this->anchor[0].x < this->anchor[1].x ? 7 : -7;
            const Geo::Point point0(this->anchor[0].x - width * this->arrow_size, mid.y + _distance);
            const Geo::Point point1(this->anchor[1].x + width * this->arrow_size, mid.y + _distance);
            if (Geo::is_intersected(rect, point0, point1))
            {
                return true;
            }
        }

        if (const Geo::Point point(this->anchor[0].x, mid.y + _distance + (this->anchor[0].y <= mid.y + _distance ? 5 : -5));
            Geo::is_intersected(rect, this->anchor[0], point))
        {
            return true;
        }
        else if (const Geo::Point point(this->anchor[1].x, mid.y + _distance + (this->anchor[1].y <= mid.y + _distance ? 5 : -5));
                 Geo::is_intersected(rect, this->anchor[1], point))
        {
            return true;
        }
    }
    else
    {
        if (std::abs(this->anchor[0].y - this->anchor[1].y) > 10)
        {
            const Geo::Point point0(mid.x + _distance, this->anchor[0].y);
            const Geo::Point point1(mid.x + _distance, this->anchor[1].y);
            if (Geo::is_intersected(rect, point0, point1))
            {
                return true;
            }
        }
        else
        {
            const int width = this->anchor[0].y < this->anchor[1].y ? 7 : -7;
            const Geo::Point point0(mid.x + _distance, this->anchor[0].y - width * this->arrow_size);
            const Geo::Point point1(mid.x + _distance, this->anchor[1].y + width * this->arrow_size);
            if (Geo::is_intersected(rect, point0, point1))
            {
                return true;
            }
        }

        if (const Geo::Point point(mid.x + _distance + (this->anchor[0].x <= mid.x + _distance ? 5 : -5), this->anchor[0].y);
            Geo::is_intersected(rect, this->anchor[0], point))
        {
            return true;
        }
        else if (const Geo::Point point(mid.x + _distance + (this->anchor[1].x <= mid.x + _distance ? 5 : -5), this->anchor[1].y);
                 Geo::is_intersected(rect, this->anchor[1], point))
        {
            return true;
        }
    }
    return false;
}

double DimLinear::height() const
{
    return _distance;
}


DimRadius::DimRadius(const Geo::Point &start, const Geo::Point &end, const double dis)
{
    this->anchor[0] = start, this->anchor[1] = end;
    this->txt = 'R' + QString::number(Geo::distance(start, end), 'f', 4);
    set_distance(dis);
}

void DimRadius::set_distance(const double dis)
{
    Geo::Vector vec(this->anchor[1] - this->anchor[0]);
    this->label = this->anchor[0] + vec.normalize() * dis;
    _distance = dis;
}

const Type DimRadius::dim_type() const
{
    return Type::RADIUS;
}

DimRadius *DimRadius::clone() const
{
    return new DimRadius(*this);
}

Geo::Polygon DimRadius::convex_hull() const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        return Geo::Polygon({this->label, this->anchor[1], this->label});
    }
    else
    {
        return Geo::Polygon({this->anchor[0], this->label, this->anchor[1]});
    }
}

Geo::AABBRect DimRadius::bounding_rect() const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        const double left = std::min(this->label.x, this->anchor[1].x);
        const double top = std::max(this->label.y, this->anchor[1].y);
        const double right = std::max(this->label.x, this->anchor[1].x);
        const double bottom = std::min(this->label.y, this->anchor[1].y);
        return Geo::AABBRect(left, top, right, bottom);
    }
    else
    {
        const double left = std::min(this->label.x, this->anchor[0].x);
        const double top = std::max(this->label.y, this->anchor[0].y);
        const double right = std::max(this->label.x, this->anchor[0].x);
        const double bottom = std::min(this->label.y, this->anchor[0].y);
        return Geo::AABBRect(left, top, right, bottom);
    }
}

Geo::Polygon DimRadius::mini_bounding_rect() const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        const double left = std::min(this->label.x, this->anchor[1].x);
        const double top = std::max(this->label.y, this->anchor[1].y);
        const double right = std::max(this->label.x, this->anchor[1].x);
        const double bottom = std::min(this->label.y, this->anchor[1].y);
        return Geo::AABBRect(left, top, right, bottom);
    }
    else
    {
        const double left = std::min(this->label.x, this->anchor[0].x);
        const double top = std::max(this->label.y, this->anchor[0].y);
        const double right = std::max(this->label.x, this->anchor[0].x);
        const double bottom = std::min(this->label.y, this->anchor[0].y);
        return Geo::AABBRect(left, top, right, bottom);
    }
}

Geo::AABBRectParams DimRadius::aabbrect_params() const
{
    Geo::AABBRectParams param;
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        param.left = std::min(this->label.x, this->anchor[1].x);
        param.top = std::max(this->label.y, this->anchor[1].y);
        param.right = std::max(this->label.x, this->anchor[1].x);
        param.bottom = std::min(this->label.y, this->anchor[1].y);
    }
    else
    {
        param.left = std::min(this->label.x, this->anchor[0].x);
        param.top = std::max(this->label.y, this->anchor[0].y);
        param.right = std::max(this->label.x, this->anchor[0].x);
        param.bottom = std::min(this->label.y, this->anchor[0].y);
    }
    return param;
}

void DimRadius::set_anchor(const int index, const double x, const double y)
{
    assert(index == 0 || index == 1);
    if (index == 0)
    {
        this->anchor[0].x = x;
        this->anchor[0].y = y;
    }
    else
    {
        this->anchor[1].x = x;
        this->anchor[1].y = y;
    }
    this->txt = 'R' + QString::number(Geo::distance(this->anchor[0], this->anchor[1]), 'f', 4);
}

void DimRadius::set_label(const double x, const double y)
{
    this->label.x = x, this->label.y = y;
    if (this->label != this->anchor[0])
    {
        const double rad = Geo::angle(this->anchor[1], this->anchor[0], this->label);
        this->anchor[1].rotate(this->anchor[0].x, this->anchor[0].y, rad);
        _distance = Geo::distance(this->anchor[0], this->label);
    }
}

void DimRadius::paintable_lines(std::vector<double> &data) const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        data.push_back(this->label.x);
        data.push_back(this->label.y);
        data.push_back(this->anchor[1].x);
        data.push_back(this->anchor[1].y);
    }
    else
    {
        data.push_back(this->anchor[0].x);
        data.push_back(this->anchor[0].y);
        data.push_back(this->label.x);
        data.push_back(this->label.y);
    }
}

void DimRadius::paintable_arrows(std::vector<double> &data) const
{
    double temp[6];
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        arrow(this->anchor[1], this->anchor[1] - this->anchor[0], this->arrow_size, temp);
    }
    else
    {
        arrow(this->anchor[1], this->anchor[0] - this->anchor[1], this->arrow_size, temp);
    }
    data.insert(data.end(), temp, temp + 6);
}

bool DimRadius::select(const Geo::Point &point, const double distance) const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        return Geo::distance(point, this->anchor[1], this->label, false) <= distance;
    }
    else
    {
        return Geo::distance(point, this->anchor[0], this->label, false) <= distance;
    }
}

bool DimRadius::select(const Geo::AABBRect &rect) const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        return Geo::is_intersected(rect, this->anchor[1], this->label);
    }
    else
    {
        return Geo::is_intersected(rect, this->anchor[0], this->label);
    }
}

double DimRadius::distance() const
{
    return _distance;
}


DimDiameter::DimDiameter(const Geo::Point &start, const Geo::Point &end, const double dis) : DimRadius(start, end, dis)
{
    this->txt = "⌀" + QString::number(Geo::distance(start, end) * 2, 'f', 4);
}

const Type DimDiameter::dim_type() const
{
    return Type::DIAMETER;
}

DimDiameter *DimDiameter::clone() const
{
    return new DimDiameter(*this);
}

void DimDiameter::paintable_lines(std::vector<double> &data) const
{
    if (_distance <= Geo::distance(this->anchor[0], this->anchor[1]))
    {
        data.push_back(this->label.x);
        data.push_back(this->label.y);
        data.push_back(this->anchor[1].x);
        data.push_back(this->anchor[1].y);
    }
    else
    {
        data.push_back(this->anchor[0].x * 2 - this->anchor[1].x);
        data.push_back(this->anchor[0].y * 2 - this->anchor[1].y);
        data.push_back(this->label.x);
        data.push_back(this->label.y);
    }
}

void DimDiameter::set_anchor(const int index, const double x, const double y)
{
    assert(index == 0 || index == 1);
    if (index == 0)
    {
        this->anchor[0].x = x;
        this->anchor[0].y = y;
    }
    else
    {
        this->anchor[1].x = x;
        this->anchor[1].y = y;
    }
    this->txt = "⌀" + QString::number(Geo::distance(this->anchor[0], this->anchor[1]) * 2, 'f', 4);
}


DimAngle::DimAngle(const Geo::Point &start, const Geo::Point &center, const Geo::Point &end, const double dis, const Geo::Point &root0,
                   const Geo::Point &root1)
    : _center(center)
{
    _root[0] = root0, _root[1] = root1;
    _lines[0] = start, _lines[1] = _lines[2] = center, _lines[3] = end;
    this->anchor[0] = start, this->anchor[1] = end;
    this->txt = QString::number(Geo::rad_to_degree(std::abs(Geo::angle(start, center, end))), 'f', 4) + "°";
    set_distance(dis);
}

DimAngle::DimAngle(const Geo::Point &line0_head, const Geo::Point &line0_tail, const Geo::Point &line1_head, const Geo::Point &line1_tail,
                   const Geo::Point &arc_anchor)
{
    _lines[0] = line0_head, _lines[1] = line0_tail, _lines[2] = line1_head, _lines[3] = line1_tail;
    Geo::is_intersected(line0_head, line0_tail, line1_head, line1_tail, _center, true);
    set_arc_pos(arc_anchor);
}

void DimAngle::set_distance(const double dis)
{
    if (dis > 0)
    {
        this->anchor[0] = _center + (this->anchor[0] - _center).normalize() * dis;
        this->anchor[1] = _center + (this->anchor[1] - _center).normalize() * dis;
        if (_minor_arc)
        {
            this->label = _center + ((this->anchor[0] + this->anchor[1]) / 2 - _center).normalize() * dis;
        }
        else
        {
            this->label = _center + (_center - (this->anchor[0] + this->anchor[1]) / 2).normalize() * dis;
        }
        _distance = dis;
    }
}

void DimAngle::set_minor_arc(const bool value)
{
    if (value == _minor_arc)
    {
        return;
    }
    _minor_arc = value;
    if (const double rad = std::abs(Geo::angle(_root[0], _center, _root[1])); value)
    {
        this->label = _center + ((this->anchor[0] + this->anchor[1]) / 2 - _center).normalize() * _distance;
        this->txt = QString::number(Geo::rad_to_degree(rad), 'f', 4) + "°";
    }
    else
    {
        this->label = _center + ((_center - this->anchor[0] + this->anchor[1]) / 2).normalize() * _distance;
        this->txt = QString::number(Geo::rad_to_degree(Geo::PI * 2 - rad), 'f', 4) + "°";
    }
    if (this->txt == "180.0000°")
    {
        this->label = _center + (this->anchor[0] - _center).vertical().normalize() * _distance;
    }
}

void DimAngle::set_arc_pos(const Geo::Point &pos)
{
    const double radius = Geo::distance(_center, pos);
    const Geo::Point tails[4] = {
        _center + (_lines[0] - _lines[1]).normalize() * radius, _center + (_lines[1] - _lines[0]).normalize() * radius,
        _center + (_lines[2] - _lines[3]).normalize() * radius, _center + (_lines[3] - _lines[2]).normalize() * radius};
    const double extend_ratio =
        1.2 / std::min(std::cos(std::abs(Geo::angle(_lines[0], _lines[1], _lines[2], _lines[3])) / 2.0),
                       std::cos((Geo::PI - std::abs(Geo::angle(_lines[0], _lines[1], _lines[2], _lines[3]))) / 2.0));
    const Geo::Point extend_tails[4] = {_center + (tails[0] - _center) * extend_ratio, _center + (tails[1] - _center) * extend_ratio,
                                        _center + (tails[2] - _center) * extend_ratio, _center + (tails[3] - _center) * extend_ratio};
    Geo::Point start = tails[1], end = tails[3];
    if (Geo::is_inside(pos, _center, extend_tails[0], extend_tails[2]))
    {
        start = tails[0];
        end = tails[2];
    }
    else if (Geo::is_inside(pos, _center, extend_tails[0], extend_tails[3]))
    {
        start = tails[0];
        end = tails[3];
    }
    else if (Geo::is_inside(pos, _center, extend_tails[1], extend_tails[2]))
    {
        start = tails[1];
        end = tails[2];
    }

    this->anchor[0] = start, this->anchor[1] = end;
    this->txt = QString::number(Geo::rad_to_degree(std::abs(Geo::angle(start, _center, end))), 'f', 4) + "°";
    set_distance(radius);

    if (Geo::distance(start, _lines[0]) + Geo::distance(start, _lines[1]) <= Geo::distance(_lines[0], _lines[1]))
    {
        _root[0] = start + (_center - start).normalize() * std::min(8.0, radius);
    }
    else
    {
        if (const double dis[3] = {Geo::distance(start, _lines[0]), Geo::distance(start, _lines[1]), Geo::distance(start, _center)};
            dis[2] < dis[0] && dis[2] < dis[1])
        {
            _root[0] = _center;
        }
        else
        {
            _root[0] = dis[0] < dis[1] ? _lines[0] : _lines[1];
        }
    }

    if (Geo::distance(end, _lines[2]) + Geo::distance(end, _lines[3]) <= Geo::distance(_lines[2], _lines[3]))
    {
        _root[1] = end + (_center - end).normalize() * std::min(8.0, radius);
    }
    else
    {
        if (const double dis[3] = {Geo::distance(end, _lines[2]), Geo::distance(end, _lines[3]), Geo::distance(end, _center)};
            dis[2] < dis[0] && dis[2] < dis[1])
        {
            _root[1] = _center;
        }
        else
        {
            _root[1] = dis[0] < dis[1] ? _lines[2] : _lines[3];
        }
    }
}

const Type DimAngle::dim_type() const
{
    return Type::ANGLE;
}

DimAngle *DimAngle::clone() const
{
    return new DimAngle(*this);
}

Geo::Polygon DimAngle::convex_hull() const
{
    return Geo::Polygon({_center, this->anchor[0], this->label, this->anchor[1]});
}

Geo::AABBRect DimAngle::bounding_rect() const
{
    const Geo::AABBRectParams param(Geo::Arc(this->anchor[0], this->label, this->anchor[1]).aabbrect_params());
    const double left = std::min({param.left, _root[0].x, _root[1].x});
    const double top = std::max({param.top, _root[0].y, _root[1].y});
    const double right = std::max({param.right, _root[0].x, _root[1].x});
    const double bottom = std::min({param.bottom, _root[0].y, _root[1].y});
    return Geo::AABBRect(left, top, right, bottom);
}

Geo::Polygon DimAngle::mini_bounding_rect() const
{
    if (const double dis = Geo::distance(this->anchor[1], _center, this->anchor[0], true);
        Geo::is_on_left(this->anchor[1], this->anchor[0], _center))
    {
        const Geo::Vector vec((_center - this->anchor[0]).vertical().normalized() * dis);
        return Geo::Polygon({this->anchor[0], _center, _center + vec, this->anchor[0] + vec});
    }
    else
    {
        const Geo::Vector vec((this->anchor[0] - _center).vertical().normalized() * dis);
        return Geo::Polygon({this->anchor[0], _center, _center + vec, this->anchor[0] + vec});
    }
}

Geo::AABBRectParams DimAngle::aabbrect_params() const
{
    Geo::AABBRectParams param(Geo::Arc(this->anchor[0], this->label, this->anchor[1]).aabbrect_params());
    param.left = std::min({param.left, _root[0].x, _root[1].x});
    param.top = std::max({param.top, _root[0].y, _root[1].y});
    param.right = std::max({param.right, _root[0].x, _root[1].x});
    param.bottom = std::min({param.bottom, _root[0].y, _root[1].y});
    return param;
}

void DimAngle::paintable_lines(std::vector<double> &data) const
{
    double rad = Geo::angle(this->anchor[0], _center, this->anchor[1]);
    if (!_minor_arc)
    {
        rad = rad > 0 ? rad - Geo::PI * 2 : Geo::PI * 2 + rad;
    }
    const double step = rad / 32.0;
    Geo::Point point(this->anchor[0]);
    for (int i = 0; i < 32; ++i)
    {
        data.push_back(point.x);
        data.push_back(point.y);
        point.rotate(_center.x, _center.y, step);
        data.push_back(point.x);
        data.push_back(point.y);
    }
    for (int i = 0; i < 2; ++i)
    {
        const Geo::Vector vec((this->anchor[i] - _root[i]).normalize() * 4);
        data.push_back(this->anchor[i].x + vec.x);
        data.push_back(this->anchor[i].y + vec.y);
        data.push_back(_root[i].x);
        data.push_back(_root[i].y);
    }
}

void DimAngle::paintable_arrows(std::vector<double> &data) const
{
    double rad = Geo::angle(this->anchor[0], _center, this->anchor[1]);
    if (!_minor_arc)
    {
        rad = rad > 0 ? rad - Geo::PI * 2 : Geo::PI * 2 + rad;
    }
    rad /= 32;
    double temp[6];
    {
        Geo::Point point(this->anchor[0]);
        point.rotate(_center.x, _center.y, rad);
        arrow(this->anchor[0], this->anchor[0] - point, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
    }
    {
        Geo::Point point(this->anchor[1]);
        point.rotate(_center.x, _center.y, -rad);
        arrow(this->anchor[1], this->anchor[1] - point, this->arrow_size, temp);
        data.insert(data.end(), temp, temp + 6);
    }
}

bool DimAngle::select(const Geo::Point &point, const double distance) const
{
    if (Geo::distance(point, this->anchor[0], _root[0], false) <= distance ||
        Geo::distance(point, this->anchor[1], _root[1], false) <= distance)
    {
        return true;
    }
    else if (std::abs(Geo::distance(point, _center) - _distance) > distance)
    {
        return false;
    }
    if (const double rad0 = Geo::angle(this->anchor[0], _center, this->anchor[1]), rad1 = Geo::angle(this->anchor[0], _center, point);
        rad0 >= 0)
    {
        return _minor_arc ? (rad1 >= 0 && rad1 <= rad0) : !(rad1 >= 0 && rad1 <= rad0);
    }
    else
    {
        return _minor_arc ? (rad1 < 0 && rad1 >= rad0) : !(rad1 < 0 && rad1 >= rad0);
    }
}

bool DimAngle::select(const Geo::AABBRect &rect) const
{
    if (Geo::is_intersected(rect, this->anchor[0], _root[0]) || Geo::is_intersected(rect, this->anchor[1], _root[1]))
    {
        return true;
    }
    double rad = Geo::angle(this->anchor[0], _center, this->anchor[1]);
    if (!_minor_arc)
    {
        rad = rad > 0 ? rad - Geo::PI * 2 : Geo::PI * 2 + rad;
    }
    const double step = rad / 32.0;
    Geo::Point point0(this->anchor[0]), point1(this->anchor[0]);
    for (int i = 0; i < 32; ++i)
    {
        point1.rotate(_center.x, _center.y, step);
        if (Geo::is_intersected(rect, point0, point1))
        {
            return true;
        }
        point0 = point1;
    }
    return false;
}

const Geo::Point &DimAngle::root(const int index) const
{
    assert(index == 0 || index == 1);
    return _root[index];
}

const Geo::Point &DimAngle::center() const
{
    return _center;
}

const Geo::Point &DimAngle::point(const int index) const
{
    assert(index >= 0 && index <= 3);
    switch (index)
    {
    case 0:
        return _lines[0];
    case 1:
        return _lines[1];
    case 2:
        return _lines[2];
    default:
        return _lines[3];
    }
}

double DimAngle::distance() const
{
    return _distance;
}

bool DimAngle::is_minor_arc() const
{
    return _minor_arc;
}


DimArc::DimArc(const Geo::Point &start, const Geo::Point &center, const Geo::Point &end, const double dis, const Geo::Point &root0,
               const Geo::Point &root1, const double radius)
    : DimAngle(start, center, end, dis, root0, root1), _radius(radius)
{
    this->txt = QString::number(radius * std::abs(Geo::angle(start, center, end)), 'f', 4);
}

void DimArc::set_minor_arc(const bool value)
{
    if (value == _minor_arc)
    {
        return;
    }
    _minor_arc = value;
    if (value)
    {
        this->txt = QString::number(_radius * std::abs(Geo::angle(_root[0], _center, _root[1])), 'f', 4);
    }
    else
    {
        this->txt = QString::number(_radius * (Geo::PI * 2 - std::abs(Geo::angle(_root[0], _center, _root[1]))), 'f', 4);
    }
}

const Type DimArc::dim_type() const
{
    return Type::ARC;
}

DimArc *DimArc::clone() const
{
    return new DimArc(*this);
}

double DimArc::radius() const
{
    return _radius;
}


DimOrdinate::DimOrdinate(const Geo::Point &point, const Geo::Point &pos)
{
    _root[1] = _root[0] = point;
    this->anchor[0] = this->anchor[1] = pos;
    if (std::abs(point.x - pos.x) > std::abs(point. y - pos.y))
    {
        this->txt = QString::number(point.y, 'f', 4);
        this->anchor[0].x = this->anchor[1].x - (_root[1].x > this->anchor[1].x ? 10 : -10);
        _root[0].x = this->anchor[1].x + (_root[1].x > this->anchor[1].x ? 10 : -10);
        if (std::abs(_root[1].x - _root[0].x) < 10)
        {
            _root[0].x = _root[1].x - (_root[1].x > this->anchor[1].x ? 10 : -10);
        }
    }
    else
    {
        this->txt = QString::number(point.x, 'f', 4);
        this->anchor[0].y = this->anchor[1].y - (_root[1].y > this->anchor[0].y ? 10 : -10);
        _root[0].y = this->anchor[1].y + (_root[1].y > this->anchor[1].y ? 10 : -10);
        if (std::abs(_root[1].y - _root[0].y) < 10)
        {
            _root[0].y = _root[1].y - (_root[1].y > this->anchor[1].y ? 10 : -10);
        }
    }
    this->label = this->anchor[0];
}

const Type DimOrdinate::dim_type() const
{
    return Type::ORDINATE;
}

DimOrdinate *DimOrdinate::clone() const
{
    return new DimOrdinate(*this);
}

Geo::Polygon DimOrdinate::convex_hull() const
{
    return Geo::Polygon({this->anchor[0], this->anchor[1], _root[1], _root[0]});
}

Geo::AABBRect DimOrdinate::bounding_rect() const
{
    const double left = std::min({this->anchor[0].x, this->anchor[1].x, _root[0].x, _root[1].x});
    const double top = std::max({this->anchor[0].y, this->anchor[1].y, _root[0].y, _root[1].y});
    const double right = std::max({this->anchor[0].x, this->anchor[1].x, _root[0].x, _root[1].x});
    const double bottom = std::min({this->anchor[0].y, this->anchor[1].y, _root[0].y, _root[1].y});
    return Geo::AABBRect(left, top, right, bottom);
}

Geo::Polygon DimOrdinate::mini_bounding_rect() const
{
    return Geo::Polygon({this->anchor[0], this->anchor[1], _root[1], _root[0]});
}

Geo::AABBRectParams DimOrdinate::aabbrect_params() const
{
    Geo::AABBRectParams param;
    param.left = std::min({this->anchor[0].x, this->anchor[1].x, _root[0].x, _root[1].x});
    param.top = std::max({this->anchor[0].y, this->anchor[1].y, _root[0].y, _root[1].y});
    param.right = std::max({this->anchor[0].x, this->anchor[1].x, _root[0].x, _root[1].x});
    param.bottom = std::min({this->anchor[0].y, this->anchor[1].y, _root[0].y, _root[1].y});
    return param;
}

void DimOrdinate::set_label(const double x, const double y)
{
    this->anchor[0].x = this->anchor[1].x = x;
    this->anchor[0].y = this->anchor[1].y = y;
    _root[0] = _root[1];
    if (std::abs(_root[1].x - this->anchor[0].x) > std::abs(_root[1].y - this->anchor[0].y))
    {
        this->txt = QString::number(_root[1].y, 'f', 4);
        this->anchor[0].x = this->anchor[1].x - (_root[1].x > this->anchor[0].x ? 10 : -10);
        _root[0].x = this->anchor[1].x + (_root[1].x > this->anchor[0].x ? 10 : -10);
        if (std::abs(_root[1].x - _root[0].x) < 10)
        {
            _root[0].x = _root[1].x - (_root[1].x > this->anchor[1].x ? 10 : -10);
        }
    }
    else
    {
        this->txt = QString::number(_root[1].x, 'f', 4);
        this->anchor[0].y = this->anchor[1].y - (_root[1].y > this->anchor[0].y ? 10 : -10);
        _root[0].y = this->anchor[1].y + (_root[1].y > this->anchor[0].y ? 10 : -10);
        if (std::abs(_root[1].y - _root[0].y) < 10)
        {
            _root[0].y = _root[1].y - (_root[1].y > this->anchor[1].y ? 10 : -10);
        }
    }
    this->label = this->anchor[0];
}

void DimOrdinate::paint(QPainter &painter) const
{
    QFont font = painter.font();
    font.setPointSize(this->font_size);
    painter.setFont(font);
    if (this->anchor[0].x == this->anchor[1].x)
    {
        painter.rotate(-90);
    }
    painter.drawText(0, -1, this->txt);
}

void DimOrdinate::paintable_lines(std::vector<double> &data) const
{
    data.push_back(this->anchor[0].x);
    data.push_back(this->anchor[0].y);
    data.push_back(this->anchor[1].x);
    data.push_back(this->anchor[1].y);
    data.push_back(this->anchor[1].x);
    data.push_back(this->anchor[1].y);
    data.push_back(_root[0].x);
    data.push_back(_root[0].y);
    data.push_back(_root[0].x);
    data.push_back(_root[0].y);
    data.push_back(_root[1].x);
    data.push_back(_root[1].y);
}

void DimOrdinate::paintable_arrows(std::vector<double> &data) const
{
}

bool DimOrdinate::select(const Geo::Point &point, const double distance) const
{
    return Geo::distance(point, this->anchor[0], this->anchor[1], false) <= distance
        || Geo::distance(point, this->anchor[1], _root[0], false) <= distance
        || Geo::distance(point, _root[0], _root[1], false) <= distance;
}

bool DimOrdinate::select(const Geo::AABBRect &rect) const
{
    return Geo::is_intersected(rect, this->anchor[0], this->anchor[1])
        || Geo::is_intersected(rect, this->anchor[1], _root[0])
        || Geo::is_intersected(rect, _root[0], _root[1]);
}

const Geo::Point &DimOrdinate::root(const int index) const
{
    assert(index == 0 || index == 1);
    return _root[index];
}