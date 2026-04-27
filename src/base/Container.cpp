#include <QFontMetrics>
#include <QStringList>
#include <utility>

#include "base/Container.hpp"
#include "base/Dimension.hpp"


// Text

Text::Text(const double x, const double y, const QFont &font, QString text, const int anchor_index)
    : _text(std::move(text)), _font(font), _anchor_index(anchor_index)
{
    const QFontMetricsF font_metrics(_font);
    double width = 2;
    double height = -font_metrics.leading();
    const double base_width = font_metrics.boundingRect("0").width();
    for (const QString &txt : _text.split('\n'))
    {
        const QRectF rect = font_metrics.boundingRect(txt);
        if (std::any_of(txt.begin(), txt.end(), [](QChar c) { return c > QChar(256); }))
        {
            width = std::max(width, rect.width() + base_width);
        }
        else
        {
            width = std::max(width, rect.width());
        }
        height += (rect.height() + font_metrics.leading());
    }
    if (width <= 2)
    {
        width = 20;
    }
    if (height <= 2)
    {
        height = 20;
    }
    switch (anchor_index)
    {
    case 0:
        _shape[0].x = x, _shape[0].y = y;
        _shape[1].x = x + width, _shape[1].y = y;
        _shape[2].x = x + width, _shape[2].y = y - height;
        _shape[3].x = x, _shape[3].y = y - height;
        break;
    case 1:
        _shape[0].x = x - width, _shape[0].y = y;
        _shape[1].x = x, _shape[1].y = y;
        _shape[2].x = x, _shape[2].y = y - height;
        _shape[3].x = x - width, _shape[3].y = y - height;
        break;
    case 2:
        _shape[0].x = x - width, _shape[0].y = y + height;
        _shape[1].x = x, _shape[1].y = y + height;
        _shape[2].x = x, _shape[2].y = y;
        _shape[3].x = x - width, _shape[3].y = y;
        break;
    default:
        _shape[0].x = x, _shape[0].y = y + height;
        _shape[1].x = x + width, _shape[1].y = y + height;
        _shape[2].x = x + width, _shape[2].y = y;
        _shape[3].x = x, _shape[3].y = y;
        break;
    }
}

const Geo::Type Text::type() const
{
    return Geo::Type::TEXT;
}

const double Text::length() const
{
    double result = std::hypot(_shape[0].x - _shape[3].x, _shape[0].y - _shape[3].y);
    for (int i = 1; i < 4; ++i)
    {
        result += std::hypot(_shape[i].x - _shape[i - 1].x, _shape[i].y - _shape[i - 1].y);
    }
    return result;
}

const bool Text::empty() const
{
    return _text.isEmpty();
}

void Text::set_text(const QString &str)
{
    _text = str;
    const QFontMetricsF font_metrics(_font);
    double width = 2;
    double height = -font_metrics.leading();
    const double base_width = font_metrics.boundingRect("0").width();
    for (const QString &txt : _text.split('\n'))
    {
        const QRectF rect = font_metrics.boundingRect(txt);
        if (std::any_of(txt.begin(), txt.end(), [](QChar c) { return c > QChar(256); }))
        {
            width = std::max(width, rect.width() + base_width);
        }
        else
        {
            width = std::max(width, rect.width());
        }
        height += (rect.height() + font_metrics.leading());
    }
    if (width <= 2)
    {
        width = 20;
    }
    if (height <= 2)
    {
        height = 20;
    }
    const double rad = angle();
    switch (_anchor_index)
    {
    case 0:
        _shape[1].x = _shape[0].x + width, _shape[1].y = _shape[0].y;
        _shape[2].x = _shape[0].x + width, _shape[2].y = _shape[0].y - height;
        _shape[3].x = _shape[0].x, _shape[3].y = _shape[0].y - height;
        break;
    case 1:
        _shape[0].x = _shape[1].x - width, _shape[0].y = _shape[1].y;
        _shape[2].x = _shape[1].x, _shape[2].y = _shape[1].y - height;
        _shape[3].x = _shape[1].x - width, _shape[3].y = _shape[1].y - height;
        break;
    case 2:
        _shape[0].x = _shape[2].x - width, _shape[0].y = _shape[2].y + height;
        _shape[1].x = _shape[2].x, _shape[1].y = _shape[2].y + height;
        _shape[3].x = _shape[2].x - width, _shape[3].y = _shape[2].y;
        break;
    default:
        _shape[0].x = _shape[3].x, _shape[0].y = _shape[3].y + height;
        _shape[1].x = _shape[3].x + width, _shape[1].y = _shape[3].y + height;
        _shape[2].x = _shape[3].x + width, _shape[2].y = _shape[3].y;
        break;
    }
    for (int i = 0; i < 4; ++i)
    {
        if (i != _anchor_index)
        {
            _shape[i].rotate(_shape[_anchor_index].x, _shape[_anchor_index].y, rad);
        }
    }
}

void Text::set_font(const QFont &font)
{
    if (_font == font)
    {
        return;
    }
    _font = font;
    const QFontMetricsF font_metrics(_font);
    double width = 2;
    double height = -font_metrics.leading();
    const double base_width = font_metrics.boundingRect("0").width();
    for (const QString &txt : _text.split('\n'))
    {
        const QRectF rect = font_metrics.boundingRect(txt);
        if (std::any_of(txt.begin(), txt.end(), [](QChar c) { return c > QChar(256); }))
        {
            width = std::max(width, rect.width() + base_width);
        }
        else
        {
            width = std::max(width, rect.width());
        }
        height += (rect.height() + font_metrics.leading());
    }
    if (width <= 2)
    {
        width = 20;
    }
    if (height <= 2)
    {
        height = 20;
    }
    const double rad = angle();
    switch (_anchor_index)
    {
    case 0:
        _shape[1].x = _shape[0].x + width, _shape[1].y = _shape[0].y;
        _shape[2].x = _shape[0].x + width, _shape[2].y = _shape[0].y - height;
        _shape[3].x = _shape[0].x, _shape[3].y = _shape[0].y - height;
        break;
    case 1:
        _shape[0].x = _shape[1].x - width, _shape[0].y = _shape[1].y;
        _shape[2].x = _shape[1].x, _shape[2].y = _shape[1].y - height;
        _shape[3].x = _shape[1].x - width, _shape[3].y = _shape[1].y - height;
        break;
    case 2:
        _shape[0].x = _shape[2].x - width, _shape[0].y = _shape[2].y + height;
        _shape[1].x = _shape[2].x, _shape[1].y = _shape[2].y + height;
        _shape[3].x = _shape[2].x - width, _shape[3].y = _shape[2].y;
        break;
    default:
        _shape[0].x = _shape[3].x, _shape[0].y = _shape[3].y + height;
        _shape[1].x = _shape[3].x + width, _shape[1].y = _shape[3].y + height;
        _shape[2].x = _shape[3].x + width, _shape[2].y = _shape[3].y;
        break;
    }
    for (int i = 0; i < 4; ++i)
    {
        if (i != _anchor_index)
        {
            _shape[i].rotate(_shape[_anchor_index].x, _shape[_anchor_index].y, rad);
        }
    }
}

const QFont &Text::font() const
{
    return _font;
}

QFont &Text::font()
{
    return _font;
}

const QString &Text::text() const
{
    return _text;
}

const Geo::Point &Text::anchor() const
{
    return _shape[_anchor_index];
}

Geo::Point Text::center() const
{
    return (_shape[0] + _shape[1] + _shape[2] + _shape[3]) / 4;
}

const Geo::Point &Text::shape(const int index) const
{
    assert(0 <= index && index < 4);
    return _shape[index];
}

const double Text::width() const
{
    return std::hypot(_shape[0].x - _shape[1].x, _shape[0].y - _shape[1].y);
}

const double Text::height() const
{
    return std::hypot(_shape[0].x - _shape[3].x, _shape[0].y - _shape[3].y);
}

const double Text::angle() const
{
    return std::atan2(_shape[1].y - _shape[0].y, _shape[1].x - _shape[0].x);
}

void Text::clear()
{
    _text.clear();
    const double rad = angle();
    switch (_anchor_index)
    {
    case 0:
        _shape[1].x = _shape[0].x + 20, _shape[1].y = _shape[0].y;
        _shape[2].x = _shape[0].x + 20, _shape[2].y = _shape[0].y - 20;
        _shape[3].x = _shape[0].x, _shape[3].y = _shape[0].y - 20;
        break;
    case 1:
        _shape[0].x = _shape[1].x - 20, _shape[0].y = _shape[1].y;
        _shape[2].x = _shape[1].x, _shape[2].y = _shape[1].y - 20;
        _shape[3].x = _shape[1].x - 20, _shape[3].y = _shape[1].y - 20;
        break;
    case 2:
        _shape[0].x = _shape[2].x - 20, _shape[0].y = _shape[2].y + 20;
        _shape[1].x = _shape[2].x, _shape[1].y = _shape[2].y + 20;
        _shape[3].x = _shape[2].x - 20, _shape[3].y = _shape[2].y;
        break;
    default:
        _shape[0].x = _shape[3].x, _shape[0].y = _shape[3].y + 20;
        _shape[1].x = _shape[3].x + 20, _shape[1].y = _shape[3].y + 20;
        _shape[2].x = _shape[3].x + 20, _shape[2].y = _shape[3].y;
        break;
    }
    for (int i = 0; i < 4; ++i)
    {
        if (i != _anchor_index)
        {
            _shape[i].rotate(_shape[_anchor_index].x, _shape[_anchor_index].y, rad);
        }
    }
}

Text *Text::clone() const
{
    return new Text(*this);
}

void Text::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    Geo::Point point = center();
    point.transform(a, b, c, d, e, f);
    const double tx = point.x - center().x, ty = point.y - center().y;
    for (int i = 0; i < 4; ++i)
    {
        _shape[i].translate(tx, ty);
    }
}

void Text::transform(const double mat[6])
{
    return transform(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
}

void Text::translate(const double tx, const double ty)
{
    for (int i = 0; i < 4; ++i)
    {
        _shape[i].translate(tx, ty);
    }
}

void Text::rotate(const double x, const double y, const double rad)
{
    for (int i = 0; i < 4; ++i)
    {
        _shape[i].rotate(x, y, rad);
    }
}

void Text::scale(const double x, const double y, const double k)
{
    QFont font(_font);
    font.setPointSizeF(std::max(_font.pointSizeF() * k, 1.0));
    set_font(font);
}

Geo::Polygon Text::convex_hull() const
{
    return Geo::Polygon({_shape[0], _shape[1], _shape[2], _shape[3], _shape[0]});
}

Geo::AABBRect Text::bounding_rect() const
{
    double left = _shape[0].x, right = _shape[2].x;
    double top = _shape[0].y, bottom = _shape[2].y;
    for (int i = 0; i < 4; ++i)
    {
        left = std::min(_shape[i].x, left);
        right = std::max(_shape[i].x, right);
        top = std::max(_shape[i].y, top);
        bottom = std::min(_shape[i].y, bottom);
    }
    return Geo::AABBRect(left, top, right, bottom);
}

Geo::Polygon Text::mini_bounding_rect() const
{
    return Geo::Polygon({_shape[0], _shape[1], _shape[2], _shape[3], _shape[0]});
}

Geo::AABBRectParams Text::aabbrect_params() const
{
    Geo::AABBRectParams param{_shape[0].x, _shape[0].y, _shape[2].x, _shape[2].y};
    for (int i = 0; i < 4; ++i)
    {
        param.left = std::min(_shape[i].x, param.left);
        param.right = std::max(_shape[i].x, param.right);
        param.top = std::max(_shape[i].y, param.top);
        param.bottom = std::min(_shape[i].y, param.bottom);
    }
    return param;
}

void Text::paint(QPainter &painter) const
{
    painter.rotate(-angle() * 180 / Geo::PI);
    painter.setFont(_font);
    // const double h = -QFontMetricsF(_font).height();
    // const QStringList txts = _text.split('\n');
    // int index = txts.size();
    // for (const QString &txt : txts)
    // {
    //     painter.drawText(0, h * --index, txt);
    // }
    const QRectF rect(0, -height(), width() + 2, height());
    painter.drawText(rect, _text);
}

// ContainerGroup

ContainerGroup::ContainerGroup(const ContainerGroup &containers)
    : Geo::Geometry(containers), _ratio(containers._ratio), _visible(containers._visible)
{
    for (const Geo::Geometry *geo : containers)
    {
        _containers.push_back(geo->clone());
    }
}

ContainerGroup::ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers) : _containers(containers.begin(), containers.end())
{
}

ContainerGroup::ContainerGroup(const std::vector<Geo::Geometry *>::const_iterator &begin,
                               const std::vector<Geo::Geometry *>::const_iterator &end)
    : _containers(begin, end)
{
}

ContainerGroup::~ContainerGroup()
{
    for (size_t i = 0, count = _containers.size(); i < count; ++i)
    {
        delete _containers[i];
    }
}

const Geo::Type ContainerGroup::type() const
{
    return Geo::Type::CONTAINERGROUP;
}

const bool ContainerGroup::visible() const
{
    return _visible;
}

void ContainerGroup::show()
{
    _visible = true;
}

void ContainerGroup::hide()
{
    _visible = false;
}

ContainerGroup *ContainerGroup::clone() const
{
    std::vector<Geo::Geometry *> containers(_containers.size());
    for (const Geo::Geometry *geo : _containers)
    {
        containers.push_back(geo->clone());
    }

    ContainerGroup *g = new ContainerGroup(containers.cbegin(), containers.cend());
    g->name = name;
    return g;
}

void ContainerGroup::transfer(ContainerGroup &group)
{
    group.clear();
    group._containers.assign(_containers.begin(), _containers.end());
    group.name = name;
    group._visible = _visible;
    _containers.clear();
}

ContainerGroup &ContainerGroup::operator=(const ContainerGroup &group)
{
    if (this != &group)
    {
        Geo::Geometry::operator=(group);
        name = group.name;
        for (size_t i = 0, count = _containers.size(); i < count; ++i)
        {
            delete _containers[i];
        }
        _containers.clear();
        _containers.shrink_to_fit();
        for (const Geo::Geometry *geo : group._containers)
        {
            _containers.push_back(geo->clone());
        }
        _ratio = group._ratio;
        _visible = group._visible;
    }
    return *this;
}

std::vector<Geo::Geometry *>::iterator ContainerGroup::begin()
{
    return _containers.begin();
}

std::vector<Geo::Geometry *>::const_iterator ContainerGroup::begin() const
{
    return _containers.begin();
}

std::vector<Geo::Geometry *>::const_iterator ContainerGroup::cbegin() const
{
    return _containers.cbegin();
}

std::vector<Geo::Geometry *>::iterator ContainerGroup::end()
{
    return _containers.end();
}

std::vector<Geo::Geometry *>::const_iterator ContainerGroup::end() const
{
    return _containers.end();
}

std::vector<Geo::Geometry *>::const_iterator ContainerGroup::cend() const
{
    return _containers.cend();
}

std::vector<Geo::Geometry *>::reverse_iterator ContainerGroup::rbegin()
{
    return _containers.rbegin();
}

std::vector<Geo::Geometry *>::const_reverse_iterator ContainerGroup::rbegin() const
{
    return _containers.rbegin();
}

std::vector<Geo::Geometry *>::const_reverse_iterator ContainerGroup::crbegin() const
{
    return _containers.crbegin();
}

std::vector<Geo::Geometry *>::reverse_iterator ContainerGroup::rend()
{
    return _containers.rend();
}

std::vector<Geo::Geometry *>::const_reverse_iterator ContainerGroup::rend() const
{
    return _containers.rend();
}

std::vector<Geo::Geometry *>::const_reverse_iterator ContainerGroup::crend() const
{
    return _containers.crend();
}

Geo::Geometry *ContainerGroup::operator[](const size_t index)
{
    assert(index < _containers.size());
    return _containers[index];
}

const Geo::Geometry *ContainerGroup::operator[](const size_t index) const
{
    assert(index < _containers.size());
    return _containers[index];
}

Geo::Geometry *ContainerGroup::at(const size_t index)
{
    assert(index < _containers.size());
    return _containers[index];
}

const Geo::Geometry *ContainerGroup::at(const size_t index) const
{
    assert(index < _containers.size());
    return _containers[index];
}

void ContainerGroup::clear()
{
    for (size_t i = 0, count = _containers.size(); i < count; ++i)
    {
        delete _containers[i];
    }
    _containers.clear();
    _containers.shrink_to_fit();
}

void ContainerGroup::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container) { container->transform(a, b, c, d, e, f); });
}

void ContainerGroup::transform(const double mat[6])
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container) { container->transform(mat); });
}

void ContainerGroup::translate(const double tx, const double ty)
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container) { container->translate(tx, ty); });
}

void ContainerGroup::rotate(const double x, const double y, const double rad)
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container) { container->rotate(x, y, rad); });
}

void ContainerGroup::scale(const double x, const double y, const double k)
{
    _ratio *= k;
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container) { container->scale(x, y, k); });
}

void ContainerGroup::rescale(const double x, const double y)
{
    if (_ratio != 1)
    {
        std::for_each(_containers.begin(), _containers.end(), [this, x, y](Geo::Geometry *c) { c->scale(x, y, 1.0 / _ratio); });
        _ratio = 1;
    }
}

Geo::AABBRect ContainerGroup::bounding_rect() const
{
    if (_containers.empty())
    {
        return Geo::AABBRect();
    }
    double r = 0, x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
    Geo::Point coord;
    for (const Geo::Geometry *continer : _containers)
    {
        switch (continer->type())
        {
        case Geo::Type::TEXT:
            {
                const Geo::AABBRectParams param = continer->aabbrect_params();
                x0 = std::min(x0, param.left);
                y0 = std::min(y0, param.bottom);
                x1 = std::max(x1, param.right);
                y1 = std::max(y1, param.top);
            }
            break;
        case Geo::Type::POLYGON:
            for (const Geo::Point &point : *static_cast<const Geo::Polygon *>(continer))
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::CIRCLE:
            r = static_cast<const Geo::Circle *>(continer)->radius;
            coord.x = static_cast<const Geo::Circle *>(continer)->x;
            coord.y = static_cast<const Geo::Circle *>(continer)->y;
            x0 = std::min(x0, coord.x - r);
            y0 = std::min(y0, coord.y - r);
            x1 = std::max(x1, coord.x + r);
            y1 = std::max(y1, coord.y + r);
            break;
        case Geo::Type::ELLIPSE:
            {
                const Geo::AABBRect rect(static_cast<const Geo::Ellipse *>(continer)->bounding_rect());
                x0 = std::min(x0, rect.left());
                y0 = std::min(y0, rect.bottom());
                x1 = std::max(x1, rect.right());
                y1 = std::max(y1, rect.top());
            }
            break;
        case Geo::Type::COMBINATION:
            {
                const Geo::AABBRectParams rect = continer->aabbrect_params();
                x0 = std::min(x0, rect.left);
                y0 = std::min(y0, rect.bottom);
                x1 = std::max(x1, rect.right);
                y1 = std::max(y1, rect.top);
            }
            break;
        case Geo::Type::POLYLINE:
            for (const Geo::Point &point : *static_cast<const Geo::Polyline *>(continer))
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : static_cast<const Geo::CubicBezier *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::BSPLINE:
            for (const Geo::Point &point : static_cast<const Geo::BSpline *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::ARC:
            for (const Geo::Point &point : static_cast<const Geo::Arc *>(continer)->bounding_rect())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::POINT:
            coord = *static_cast<const Geo::Point *>(continer);
            x0 = std::min(x0, coord.x);
            y0 = std::min(y0, coord.y);
            x1 = std::max(x1, coord.x);
            y1 = std::max(y1, coord.y);
            break;
        case Geo::Type::DIMENSION:
            {
                const Geo::AABBRectParams param = static_cast<const Dim::Dimension *>(continer)->aabbrect_params();
                x0 = std::min(x0, param.left);
                y0 = std::min(y0, param.bottom);
                x1 = std::max(x1, param.right);
                y1 = std::max(y1, param.top);
            }
            break;
        default:
            break;
        }
    }
    return Geo::AABBRect(x0, y1, x1, y0);
}

Geo::AABBRectParams ContainerGroup::aabbrect_params() const
{
    Geo::AABBRectParams params;
    if (_containers.empty())
    {
        return params;
    }
    double r = 0, x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
    Geo::Point coord;
    for (const Geo::Geometry *continer : _containers)
    {
        switch (continer->type())
        {
        case Geo::Type::TEXT:
            {
                const Geo::AABBRectParams param = continer->aabbrect_params();
                x0 = std::min(x0, param.left);
                y0 = std::min(y0, param.bottom);
                x1 = std::max(x1, param.right);
                y1 = std::max(y1, param.top);
            }
            break;
        case Geo::Type::POLYGON:
            for (const Geo::Point &point : *static_cast<const Geo::Polygon *>(continer))
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::CIRCLE:
            r = static_cast<const Geo::Circle *>(continer)->radius;
            coord.x = static_cast<const Geo::Circle *>(continer)->x;
            coord.y = static_cast<const Geo::Circle *>(continer)->y;
            x0 = std::min(x0, coord.x - r);
            y0 = std::min(y0, coord.y - r);
            x1 = std::max(x1, coord.x + r);
            y1 = std::max(y1, coord.y + r);
            break;
        case Geo::Type::ELLIPSE:
            {
                const Geo::AABBRect rect(static_cast<const Geo::Ellipse *>(continer)->bounding_rect());
                x0 = std::min(x0, rect.left());
                y0 = std::min(y0, rect.bottom());
                x1 = std::max(x1, rect.right());
                y1 = std::max(y1, rect.top());
            }
            break;
        case Geo::Type::COMBINATION:
            {
                const Geo::AABBRect rect(static_cast<const Combination *>(continer)->bounding_rect());
                x0 = std::min(x0, rect.left());
                y0 = std::min(y0, rect.bottom());
                x1 = std::max(x1, rect.right());
                y1 = std::max(y1, rect.top());
            }
            break;
        case Geo::Type::POLYLINE:
            for (const Geo::Point &point : *static_cast<const Geo::Polyline *>(continer))
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : static_cast<const Geo::CubicBezier *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::BSPLINE:
            for (const Geo::Point &point : static_cast<const Geo::BSpline *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::ARC:
            for (const Geo::Point &point : static_cast<const Geo::Arc *>(continer)->bounding_rect())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::POINT:
            coord = *static_cast<const Geo::Point *>(continer);
            x0 = std::min(x0, coord.x);
            y0 = std::min(y0, coord.y);
            x1 = std::max(x1, coord.x);
            y1 = std::max(y1, coord.y);
            break;
        case Geo::Type::DIMENSION:
            {
                const Geo::AABBRectParams param = static_cast<const Dim::Dimension *>(continer)->aabbrect_params();
                x0 = std::min(x0, param.left);
                y0 = std::min(y0, param.bottom);
                x1 = std::max(x1, param.right);
                y1 = std::max(y1, param.top);
            }
            break;
        default:
            break;
        }
    }
    params.left = x0;
    params.bottom = y0;
    params.right = x1;
    params.top = y1;
    return params;
}

const size_t ContainerGroup::size() const
{
    return _containers.size();
}

const size_t ContainerGroup::count(const Geo::Type type, const bool include_combinated) const
{
    if (include_combinated)
    {
        size_t num =
            std::count_if(_containers.begin(), _containers.end(), [=](const Geo::Geometry *object) { return object->type() == type; });
        for (const Geo::Geometry *object : _containers)
        {
            if (const Combination *combination = dynamic_cast<const Combination *>(object))
            {
                num += std::count_if(combination->begin(), combination->end(),
                                     [=](const Geo::Geometry *object) { return object->type() == type; });
            }
        }
        return num;
    }
    else
    {
        return std::count_if(_containers.begin(), _containers.end(), [=](const Geo::Geometry *object) { return object->type() == type; });
    }
}

void ContainerGroup::append(ContainerGroup &group, const bool merge)
{
    if (merge)
    {
        _containers.insert(_containers.end(), group._containers.begin(), group._containers.end());
        group._containers.clear();
    }
    else
    {
        for (Geo::Geometry *geo : group)
        {
            _containers.emplace_back(geo->clone());
        }
    }
}

void ContainerGroup::append(Geo::Geometry *object)
{
    _containers.push_back(object);
}

void ContainerGroup::insert(const size_t index, Geo::Geometry *object)
{
    _containers.insert(_containers.begin() + index, object);
}

void ContainerGroup::insert(const std::vector<Geo::Geometry *>::iterator &it, Geo::Geometry *object)
{
    _containers.insert(it, object);
}

std::vector<Geo::Geometry *>::iterator ContainerGroup::remove(const size_t index)
{
    assert(index < _containers.size());
    delete _containers[index];
    return _containers.erase(_containers.begin() + index);
}

std::vector<Geo::Geometry *>::iterator ContainerGroup::remove(const std::vector<Geo::Geometry *>::iterator &it)
{
    delete *it;
    return _containers.erase(it);
}

std::vector<Geo::Geometry *>::iterator ContainerGroup::remove(const std::vector<Geo::Geometry *>::reverse_iterator &it)
{
    std::vector<Geo::Geometry *>::iterator b = _containers.begin();
    while (*b != *it)
    {
        ++b;
    }
    delete *b;
    return _containers.erase(b);
}

Geo::Geometry *ContainerGroup::pop(const size_t index)
{
    assert(index < _containers.size());
    Geo::Geometry *container = _containers[index];
    _containers.erase(_containers.begin() + index);
    return container;
}

Geo::Geometry *ContainerGroup::pop(const std::vector<Geo::Geometry *>::iterator &it)
{
    Geo::Geometry *container = *it;
    _containers.erase(it);
    return container;
}

Geo::Geometry *ContainerGroup::pop(const std::vector<Geo::Geometry *>::reverse_iterator &it)
{
    std::vector<Geo::Geometry *>::iterator b = _containers.begin();
    while (*b != *it)
    {
        ++b;
    }
    Geo::Geometry *container = *b;
    _containers.erase(b);
    return container;
}

Geo::Geometry *ContainerGroup::pop_front()
{
    assert(!_containers.empty());
    Geo::Geometry *container = _containers.front();
    _containers.erase(_containers.begin());
    return container;
}

Geo::Geometry *ContainerGroup::pop_back()
{
    assert(!_containers.empty());
    Geo::Geometry *container = _containers.back();
    _containers.pop_back();
    return container;
}

const bool ContainerGroup::empty() const
{
    return _containers.empty();
}

Geo::Geometry *ContainerGroup::front()
{
    assert(!_containers.empty());
    return _containers.front();
}

const Geo::Geometry *ContainerGroup::front() const
{
    assert(!_containers.empty());
    return _containers.front();
}

Geo::Geometry *ContainerGroup::back()
{
    assert(!_containers.empty());
    return _containers.back();
}

const Geo::Geometry *ContainerGroup::back() const
{
    assert(!_containers.empty());
    return _containers.back();
}

void ContainerGroup::remove_front()
{
    if (!_containers.empty())
    {
        delete _containers.front();
        _containers.erase(_containers.begin());
    }
}

void ContainerGroup::remove_back()
{
    if (!_containers.empty())
    {
        delete _containers.back();
        _containers.pop_back();
    }
}

// Combination

Combination::Combination(const std::initializer_list<Geo::Geometry *> &containers) : ContainerGroup(containers)
{
    update_border();
}

Combination::Combination(const std::vector<Geo::Geometry *>::const_iterator &begin, const std::vector<Geo::Geometry *>::const_iterator &end)
    : ContainerGroup(begin, end)
{
    update_border();
}

const Geo::Type Combination::type() const
{
    return Geo::Type::COMBINATION;
}

void Combination::append(Combination *combination)
{
    while (!combination->empty())
    {
        ContainerGroup::append(combination->pop_back());
    }
}

void Combination::append(Geo::Geometry *geo)
{
    if (geo->type() == Geo::Type::COMBINATION)
    {
        append(dynamic_cast<Combination *>(geo));
    }
    else
    {
        ContainerGroup::append(geo);
    }
}

Combination *Combination::clone() const
{
    return new Combination(*this);
}

void Combination::transfer(Combination &combination)
{
    ContainerGroup::transfer(combination);
    combination._border = _border;
}

Combination &Combination::operator=(const Combination &combination)
{
    ContainerGroup::operator=(combination);
    return *this;
}

void Combination::clear()
{
    ContainerGroup::clear();
    _border.clear();
}

void Combination::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    ContainerGroup::transform(a, b, c, d, e, f);
    _border.transform(a, b, c, d, e, f);
}

void Combination::transform(const double mat[6])
{
    ContainerGroup::transform(mat);
    _border.transform(mat);
}

void Combination::translate(const double tx, const double ty)
{
    ContainerGroup::translate(tx, ty);
    _border.translate(tx, ty);
}

void Combination::rotate(const double x, const double y, const double rad)
{
    ContainerGroup::rotate(x, y, rad);
    _border.rotate(x, y, rad);
}

void Combination::scale(const double x, const double y, const double k)
{
    ContainerGroup::scale(x, y, k);
    _border.scale(x, y, k);
}

void Combination::update_border()
{
    if (empty())
    {
        _border.clear();
    }
    else
    {
        _border = bounding_rect();
    }
}

const Geo::AABBRect &Combination::border() const
{
    return _border;
}
