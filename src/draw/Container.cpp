#include "draw/Container.hpp"
#include <cfloat>


Container::Container(const Geo::Polygon &shape)
    : Geo::Polygon(shape)
{
    _memo["Type"] = 0;
}

Container::Container(const QString &txt, const Geo::Polygon &shape)
    : Geo::Polygon(shape), _txt(txt)
{
    _memo["Type"] = 0;
}

Container::Container(const Container &container)
    : Geo::Polygon(container), _txt(container._txt)
{
    _memo["Type"] = 0;
}

Container::Container(const Container &&container)
    : Geo::Polygon(std::move(container)), _txt(std::move(container._txt))
{
    _memo["Type"] = 0;
}

Container &Container::operator=(const Container &container)
{
    if (this != &container)
    {
        Geo::Polygon::operator=(container);
        _txt = container._txt;
    }
    return *this;
}

Container &Container::operator=(const Container &&container)
{
    if (this != &container)
    {
        Geo::Polygon::operator=(container);
        _txt = std::move(container._txt);
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
{
    _memo["Type"] = 1;
}

CircleContainer::CircleContainer(const QString &txt, const Geo::Circle &shape)
    : Geo::Circle(shape), _txt(txt)
{
    _memo["Type"] = 1;
}

CircleContainer::CircleContainer(const QString &txt, const double x, const double y, const double r)
    : Geo::Circle(x, y, r), _txt(txt)
{
    _memo["Type"] = 1;
}

CircleContainer::CircleContainer(const CircleContainer &container)
    : Geo::Circle(container), _txt(container._txt)
{
    _memo["Type"] = 1;
}

CircleContainer::CircleContainer(const CircleContainer &&container)
    : Geo::Circle(std::move(container)), _txt(std::move(container._txt))
{
    _memo["Type"] = 1;
}

CircleContainer &CircleContainer::operator=(const CircleContainer &container)
{
    if (this != &container)
    {
        Geo::Circle::operator=(container);
        _txt = container._txt;
    }
    return *this;
}

CircleContainer &CircleContainer::operator=(const CircleContainer &&container)
{
    if (this != &container)
    {
        Geo::Circle::operator=(std::move(container));
        _txt = std::move(container._txt);
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
    return radius() <= 0 && _txt.isEmpty();
}

CircleContainer *CircleContainer::clone() const
{
    return new CircleContainer(*this);
}

// ContainerGroup

ContainerGroup::ContainerGroup(const ContainerGroup &containers)
    : Geo::Geometry(containers), _ratio(containers._ratio), _visible(containers._visible)
{
    for (const Geo::Geometry *geo : containers)
    {
        switch (geo->memo()["Type"].to_int())
        {
        case 0:
            _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
            break;
        case 1:
            _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
            break;
        case 3:
            _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
            break;
        case 20:
            _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
            break;
        case 21:
            _containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
    }
}

ContainerGroup::ContainerGroup(const ContainerGroup &&containers)
    : Geo::Geometry(containers), _ratio(std::move(containers._ratio)), _visible(containers._visible)
{
    for (const Geo::Geometry *geo : containers)
    {
        switch (geo->memo()["Type"].to_int())
        {
        case 0:
            _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
            break;
        case 1:
            _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
            break;
        case 3:
            _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
            break;
        case 20:
            _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
            break;
        case 21:
            _containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
    }
}

ContainerGroup::ContainerGroup(const std::initializer_list<Geo::Geometry *> &containers)
    : _containers(containers.begin(), containers.end())
{
}

ContainerGroup::ContainerGroup(std::vector<Geo::Geometry *>::const_iterator begin, std::vector<Geo::Geometry *>::const_iterator end)
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

const bool &ContainerGroup::visible() const
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
        switch (geo->memo()["Type"].to_int())
        {
        case 0:
            containers.push_back(dynamic_cast<const Container *>(geo)->clone());
            break;
        case 1:
            containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
            break;
        case 3:
            containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
            break;
        case 20:
            containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
            break;
        case 21:
            containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
    }

    return new ContainerGroup(containers.cbegin(), containers.cend());
}

void ContainerGroup::transfer(ContainerGroup &group)
{
    group.clear();
    group._containers.assign(_containers.begin(), _containers.end());
    group._memo = _memo;
    _containers.clear();
}

ContainerGroup &ContainerGroup::operator=(const ContainerGroup &group)
{
    if (this != &group)
    {
        Geo::Geometry::operator=(group);
        for (size_t i = 0, count = _containers.size(); i < count; ++i)
        {
            delete _containers[i];
        }
        _containers.clear();
        _containers.shrink_to_fit();
        for (const Geo::Geometry *geo : group._containers)
        {
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
                break;
            case 1:
                _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
                break;
            case 3:
                _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
                break;
            case 20:
                _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
                break;
            case 21:
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

ContainerGroup &ContainerGroup::operator=(const ContainerGroup &&group)
{
    if (this != &group)
    {
        Geo::Geometry::operator=(group);
        for (size_t i = 0, count = _containers.size(); i < count; ++i)
        {
            delete _containers[i];
        }
        _containers.clear();
        _containers.shrink_to_fit();
        for (const Geo::Geometry *geo : group._containers)
        {
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                _containers.push_back(dynamic_cast<const Container *>(geo)->clone());
                break;
            case 1:
                _containers.push_back(dynamic_cast<const CircleContainer *>(geo)->clone());
                break;
            case 3:
                _containers.push_back(dynamic_cast<const Combination *>(geo)->clone());
                break;
            case 20:
                _containers.push_back(dynamic_cast<const Geo::Polyline *>(geo)->clone());
                break;
            case 21:
                _containers.push_back(dynamic_cast<const Geo::Bezier *>(geo)->clone());
                break;
            default:
                break;
            }
        }
        _ratio = std::move(group._ratio);
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
    std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *container)
                  { container->transform(a, b, c, d, e, f); });
}

void ContainerGroup::transform(const double mat[6])
{
    std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *container)
                  { container->transform(mat); });
}

void ContainerGroup::translate(const double tx, const double ty)
{
    std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *container)
                  { container->translate(tx, ty); });
}

void ContainerGroup::rotate(const double x, const double y, const double rad)
{
    std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *container)
                  { container->rotate(x, y, rad); });
}

void ContainerGroup::scale(const double x, const double y, const double k)
{
    _ratio *= k;
    std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *container)
                  { container->scale(x, y, k); });
}

void ContainerGroup::rescale(const double x, const double y)
{
    if (_ratio != 1)
    {
        std::for_each(_containers.begin(), _containers.end(), [&](Geo::Geometry *c)
                      { c->scale(x, y, 1.0 / _ratio); });
        _ratio = 1;
    }
}

Geo::Rectangle ContainerGroup::bounding_rect(const bool orthogonality) const
{
    if (_containers.empty())
    {
        return Geo::Rectangle();
    }
    double r, x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    Geo::Coord coord;
    for (const Geo::Geometry *continer : _containers)
    {
        switch (continer->memo()["Type"].to_int())
        {
        case 0:
            for (const Geo::Point &point : dynamic_cast<const Container *>(continer)->shape())
            {
                x0 = std::min(x0, point.coord().x);
                y0 = std::min(y0, point.coord().y);
                x1 = std::max(x1, point.coord().x);
                y1 = std::max(y1, point.coord().y);
            }
            break;
        case 1:
            r = dynamic_cast<const CircleContainer *>(continer)->radius();
            coord = dynamic_cast<const CircleContainer *>(continer)->center().coord();
            x0 = std::min(x0, coord.x - r);
            y0 = std::min(y0, coord.y - r);
            x1 = std::max(x1, coord.x + r);
            y1 = std::max(y1, coord.y + r);
            break;
        case 3:
            {
                const Geo::Rectangle rect(dynamic_cast<const Combination *>(continer)->bounding_rect());
                x0 = std::min(x0, rect.left());
                y0 = std::min(y0, rect.top());
                x1 = std::max(x1, rect.right());
                y1 = std::max(y1, rect.bottom());
            }
            break;
        case 2:
        case 20:
            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(continer))
            {
                x0 = std::min(x0, point.coord().x);
                y0 = std::min(y0, point.coord().y);
                x1 = std::max(x1, point.coord().x);
                y1 = std::max(y1, point.coord().y);
            }
            break;
        case 21:
            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(continer)->shape())
            {
                x0 = std::min(x0, point.coord().x);
                y0 = std::min(y0, point.coord().y);
                x1 = std::max(x1, point.coord().x);
                y1 = std::max(y1, point.coord().y);
            }
            break;
        default:
            break;
        }
    }
    return Geo::Rectangle(x0, y0, x1, y1);
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

void ContainerGroup::append(Geo::Polyline *container)
{
    _containers.push_back(container);
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

void ContainerGroup::insert(const size_t index, Container *container)
{
    _containers.insert(_containers.begin() + index, container);
}

void ContainerGroup::insert(const size_t index, CircleContainer *container)
{
    _containers.insert(_containers.begin() + index, container);
}

void ContainerGroup::insert(const size_t index, Geo::Polyline *container)
{
    _containers.insert(_containers.begin() + index, container);
}

void ContainerGroup::insert(const size_t index, Geo::Bezier *bezier)
{
    _containers.insert(_containers.begin() + index, bezier);
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

Combination::Combination()
{
    _memo["Type"] = 3;
}

Combination::Combination(const Combination &combination)
    : ContainerGroup(combination), _border(combination._border)
{
    _memo["Type"] = 3;
}

Combination::Combination(const Combination &&combination)
    : ContainerGroup(combination), _border(std::move(combination._border))
{
    _memo["Type"] = 3;
}

void Combination::append(Combination *combination)
{
    while (!combination->empty())
    {
        append(combination->pop_back());
    }
}

void Combination::append(Geo::Geometry *geo)
{
    switch (geo->memo()["Type"].to_int())
    {
    case 0:
        ContainerGroup::append(dynamic_cast<Container *>(geo));
        break;
    case 1:
        ContainerGroup::append(dynamic_cast<CircleContainer *>(geo));
        break;
    case 3:
        append(dynamic_cast<Combination *>(geo));
        break;
    case 20:
        ContainerGroup::append(dynamic_cast<Geo::Polyline *>(geo));
        break;
    case 21:
        ContainerGroup::append(dynamic_cast<Geo::Bezier *>(geo));
        break;
    default:
        break;
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

const Geo::Rectangle &Combination::border() const
{
    return _border;
}

