#include <QFontMetrics>
#include <QStringList>

#include "draw/Container.hpp"


// Text

Text::Text(const double x, const double y, const int size, const QString &text)
    : _text(text), _text_size(size)
{
    QFont font("SimSun");
    font.setPixelSize(size);
    const QFontMetrics font_metrics(font);
    long long width = 0;
    for (const QString &s : text.split('\n'))
    {
        width = std::max(width, font.pixelSize() * s.length());
    }
    if (width == 0)
    {
        width = font.pixelSize() * text.length();
    }
    width = std::max(20ll, width);
    long long height = std::max(font_metrics.lineSpacing() * (text.count('\n') + 1), 20ll);
    set_left(x - 1 - width / 2);
    set_right(x + 1 + width / 2);
    set_top(y + height / 2);
    set_bottom(y - height / 2);
}

Text::Text(const Text &text)
    : Geo::AABBRect(text), _text(text._text), _text_size(text._text_size),
    text_index(text.text_index), text_count(text.text_count)
{}

const Geo::Type Text::type() const
{
    return Geo::Type::TEXT;
}

Text &Text::operator=(const Text &text)
{
    if (&text != this)
    {
        Geo::AABBRect::operator=(text);
        _text = text._text;
        _text_size = text._text_size;
        text_index = text.text_index;
        text_count = text.text_count;
    }
    return *this;
}

void Text::set_text(const QString &str, const int size)
{
    _text = str;
    _text_size = size;
    const Geo::Point coord(center());
    QFont font("SimSun");
    font.setPixelSize(size);
    const QFontMetrics font_metrics(font);
    long long width = 0;
    for (const QString &s : str.split('\n'))
    {
        width = std::max(width, font.pixelSize() * s.length());
    }
    if (width == 0)
    {
        width = font.pixelSize() * str.length();
    }
    width = std::max(20ll, width);
    long long height = std::max(font_metrics.lineSpacing() * (str.count('\n') + 1), 20ll);
    set_left(coord.x - 1 - width / 2);
    set_right(coord.x + 1 + width / 2);
    set_top(coord.y + height / 2);
    set_bottom(coord.y - height / 2);
}

void Text::update_size(const int size)
{
    if (_text_size == size)
    {
        return;
    }
    const Geo::Point coord(center());
    QFont font("SimSun");
    font.setPixelSize(size);
    const QFontMetrics font_metrics(font);
    long long width = 0;
    for (const QString &s : _text.split('\n'))
    {
        width = std::max(width, font.pixelSize() * s.length());
    }
    if (width == 0)
    {
        width = font.pixelSize() * _text.length();
    }
    width = std::max(20ll, width);
    long long height = std::max(font_metrics.lineSpacing() * (_text.count('\n') + 1), 20ll);
    set_left(coord.x - 1 - width / 2);
    set_right(coord.x + 1 + width / 2);
    set_top(coord.y + height / 2);
    set_bottom(coord.y- height / 2);
}

int Text::text_size() const
{
    return _text_size;
}

const QString &Text::text() const
{
    return _text;
}

Geo::AABBRect &Text::shape()
{
    return *dynamic_cast<Geo::AABBRect *>(this);
}

const Geo::AABBRect &Text::shape() const
{
    return *dynamic_cast<const Geo::AABBRect *>(this);
}

void Text::clear()
{
    _text.clear();
    const Geo::Point coord(center());
    set_left(coord.x - 10);
    set_left(coord.x + 10);
    set_top(coord.y + 10);
    set_bottom(coord.y + 10);
}

Text *Text::clone() const
{
    return new Text(*this);
}

// Container

Container::Container(const Geo::Polygon &shape)
    : Geo::Polygon(shape)
{}

Container::Container(const QString &txt, const Geo::Polygon &shape)
    : Geo::Polygon(shape), _txt(txt)
{}

Container::Container(const Container &container)
    : Geo::Polygon(container), _txt(container._txt),
    text_index(container.text_index),
    text_count(container.text_count)
{}

const Geo::Type Container::type() const
{
    return Geo::Type::CONTAINER;
}

Container &Container::operator=(const Container &container)
{
    if (this != &container)
    {
        Geo::Polygon::operator=(container);
        _txt = container._txt;
        text_index = container.text_index;
        text_count = container.text_count;
    }
    return *this;
}

Geo::Polygon &Container::shape()
{
    return *dynamic_cast<Geo::Polygon *>(this);
}

const Geo::Polygon &Container::shape() const
{
    return *dynamic_cast<const Geo::Polygon *>(this);
}

const QString &Container::text() const
{
    return _txt;
}

void Container::set_text(const QString &txt)
{
    _txt = txt;
}

void Container::clear_text()
{
    _txt.clear();
}

const Geo::Point Container::center() const
{
    return bounding_rect().center();
}

const bool Container::empty() const
{
    return Geo::Polygon::empty() && _txt.isEmpty();
}

Container *Container::clone() const
{
    return new Container(*this);
}

// CircleContainer

CircleContainer::CircleContainer(const Geo::Circle &shape)
    : Geo::Circle(shape)
{}

CircleContainer::CircleContainer(const QString &txt, const Geo::Circle &shape)
    : Geo::Circle(shape), _txt(txt)
{}

CircleContainer::CircleContainer(const QString &txt, const double x, const double y, const double r)
    : Geo::Circle(x, y, r), _txt(txt)
{}

CircleContainer::CircleContainer(const CircleContainer &container)
    : Geo::Circle(container), _txt(container._txt),
    text_index(container.text_index), text_count(container.text_count)
{}

const Geo::Type CircleContainer::type() const
{
    return Geo::Type::CIRCLECONTAINER;
}

CircleContainer &CircleContainer::operator=(const CircleContainer &container)
{
    if (this != &container)
    {
        Geo::Circle::operator=(container);
        _txt = container._txt;
        text_index = container.text_index;
        text_count = container.text_count;
    }
    return *this;
}

Geo::Circle &CircleContainer::shape()
{
    return *dynamic_cast<Geo::Circle *>(this);
}

const Geo::Circle &CircleContainer::shape() const
{
    return *dynamic_cast<const Geo::Circle *>(this);
}

const QString &CircleContainer::text() const
{
    return _txt;
}

void CircleContainer::set_text(const QString &txt)
{
    _txt = txt;
}

void CircleContainer::clear_text()
{
    _txt.clear();
}

const bool CircleContainer::empty() const
{
    return radius <= 0 && _txt.isEmpty();
}

CircleContainer *CircleContainer::clone() const
{
    return new CircleContainer(*this);
}

// ContainerGroup

ContainerGroup::ContainerGroup(const ContainerGroup &containers)
    : Geo::Geometry(containers), _ratio(containers._ratio), _visible(containers._visible),
    name(containers.name)
{
    for (const Geo::Geometry *geo : containers)
    {
        switch (geo->type())
        {
        case Geo::Type::TEXT:
            _containers.push_back(dynamic_cast<const Text *>(geo)->clone());
            break;
        case Geo::Type::CONTAINER:
            _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
            break;
        case Geo::Type::CIRCLECONTAINER:
            _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
            break;
        case Geo::Type::COMBINATION:
            _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
            break;
        case Geo::Type::POLYLINE:
            _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
            break;
        case Geo::Type::BEZIER:
            _containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
    }
}

ContainerGroup::ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers)
    : _containers(containers.begin(), containers.end())
{}

ContainerGroup::ContainerGroup(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end)
    : _containers(begin, end)
{}

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
    std::vector<Geo::Geometry *> containers;
    for (const Geo::Geometry *geo : _containers)
    {
        switch (geo->type())
        {
        case Geo::Type::TEXT:
            containers.push_back(dynamic_cast<const Text *>(geo)->clone());
            break;
        case Geo::Type::CONTAINER:
            containers.push_back(dynamic_cast<const Container *>(geo)->clone());
            break;
        case Geo::Type::CIRCLECONTAINER:
            containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
            break;
        case Geo::Type::COMBINATION:
            containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
            break;
        case Geo::Type::POLYLINE:
            containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
            break;
        case Geo::Type::BEZIER:
            containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
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
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                _containers.push_back(dynamic_cast<const Text *>(geo)->clone());
                break;
            case Geo::Type::CONTAINER:
                _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
                break;
            case Geo::Type::CIRCLECONTAINER:
                _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
                break;
            case Geo::Type::COMBINATION:
                _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
                break;
            case Geo::Type::POLYLINE:
                _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
                break;
            case Geo::Type::BEZIER:
                _containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
                break;
            default:
                break;
            }
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
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container)
                  { container->transform(a, b, c, d, e, f); });
}

void ContainerGroup::transform(const double mat[6])
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container)
                  { container->transform(mat); });
}

void ContainerGroup::translate(const double tx, const double ty)
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container)
                  { container->translate(tx, ty); });
}

void ContainerGroup::rotate(const double x, const double y, const double rad)
{
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container)
                  { container->rotate(x, y, rad); });
}

void ContainerGroup::scale(const double x, const double y, const double k)
{
    _ratio *= k;
    std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *container)
                  { container->scale(x, y, k); });
}

void ContainerGroup::rescale(const double x, const double y)
{
    if (_ratio != 1)
    {
        std::for_each(_containers.begin(), _containers.end(), [=](Geo::Geometry *c)
                      { c->scale(x, y, 1.0 / _ratio); });
        _ratio = 1;
    }
}

Geo::AABBRect ContainerGroup::bounding_rect() const
{
    if (_containers.empty())
    {
        return Geo::AABBRect();
    }
    double r, x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    Geo::Point coord;
    for (const Geo::Geometry *continer : _containers)
    {
        switch (continer->type())
        {
        case Geo::Type::TEXT:
            x0 = std::min(x0, dynamic_cast<const Text *>(continer)->left());
            y0 = std::min(y0, dynamic_cast<const Text *>(continer)->bottom());
            x1 = std::max(x1, dynamic_cast<const Text *>(continer)->right());
            y1 = std::max(y1, dynamic_cast<const Text *>(continer)->top());
            break;
        case Geo::Type::CONTAINER:
            for (const Geo::Point &point : dynamic_cast<const Container *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::CIRCLECONTAINER:
            r = dynamic_cast<const CircleContainer *>(continer)->radius;
            coord = *dynamic_cast<const CircleContainer *>(continer);
            x0 = std::min(x0, coord.x - r);
            y0 = std::min(y0, coord.y - r);
            x1 = std::max(x1, coord.x + r);
            y1 = std::max(y1, coord.y + r);
            break;
        case Geo::Type::COMBINATION:
            {
                const Geo::AABBRect rect(dynamic_cast<const Combination *>(continer)->bounding_rect());
                x0 = std::min(x0, rect.left());
                y0 = std::min(y0, rect.top());
                x1 = std::max(x1, rect.right());
                y1 = std::max(y1, rect.bottom());
            }
            break;
        case Geo::Type::POLYLINE:
            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(continer))
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(continer)->shape())
            {
                x0 = std::min(x0, point.x);
                y0 = std::min(y0, point.y);
                x1 = std::max(x1, point.x);
                y1 = std::max(y1, point.y);
            }
            break;
        default:
            break;
        }
    }
    return Geo::AABBRect(x0, y1, x1, y0);
}

const size_t ContainerGroup::size() const
{
    return _containers.size();
}

void ContainerGroup::append(Container *container)
{
    _containers.push_back(container);
}

void ContainerGroup::append(CircleContainer *container)
{
    _containers.push_back(container);
}

void ContainerGroup::append(Geo::Polyline *polyline)
{
    _containers.push_back(polyline);
}

void ContainerGroup::append(Geo::Bezier *bezier)
{
    _containers.push_back(bezier);
}

void ContainerGroup::append(Combination *combination)
{
    _containers.push_back(combination);
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

void ContainerGroup::append(Text *text)
{
    _containers.push_back(text);
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

Combination::Combination(const Combination &combination)
    : ContainerGroup(combination), _border(combination._border)
{}

Combination::Combination(const std::initializer_list<Geo::Geometry *> &containers)
    : ContainerGroup(containers)
{
    update_border();
}

Combination::Combination(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end)
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

