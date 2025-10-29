#include "base/Graph.hpp"


Graph::Graph(const Graph &graph)
    : Geo::Geometry(graph), modified(graph.modified)
{
    for (const ContainerGroup &group : graph._container_groups)
    {
        _container_groups.emplace_back(group);
    }
}

const Geo::Type Graph::type() const
{
    return Geo::Type::GRAPH;
}

Graph *Graph::clone() const
{
    Graph *g = new Graph(*this);
    g->modified = modified;
    return g;
}

void Graph::transfer(Graph &graph)
{
    graph.clear();
    graph.modified = modified;
    for (ContainerGroup &group : _container_groups)
    {
        graph.append_group();
        group.transfer(graph.back());
    }
    clear();
}

void Graph::merge(Graph &graph)
{
    for (size_t i = _container_groups.size(), count = graph._container_groups.size(); i < count; ++i)
    {
        _container_groups.emplace_back();
    }
    std::list<ContainerGroup>::iterator it0 = _container_groups.begin(), it1 = graph._container_groups.begin();
    for (size_t i = 0, count = graph._container_groups.size(); i < count; ++i)
    {
        (it0++)->append(*(it1++));
    }
}

Graph &Graph::operator=(const Graph &graph)
{
    if (this != &graph)
    {
        Geo::Geometry::operator=(graph);
        modified = graph.modified;
        _container_groups.clear();
        for (const ContainerGroup &group : graph._container_groups)
        {
            _container_groups.emplace_back(group);
        }
    }
    return *this;
}

ContainerGroup &Graph::container_group(const size_t index)
{
    assert(index < _container_groups.size());
    const size_t count = _container_groups.size();
    if (index <= count / 2)
    {
        std::list<ContainerGroup>::iterator it = _container_groups.begin();
        for (size_t i = 0; i < index; ++i)
        {
            ++it;
        }
        return *it;
    }
    else
    {
        std::list<ContainerGroup>::reverse_iterator it = _container_groups.rbegin();
        for (size_t i = 1, end = count - index; i < end; ++i)
        {
            ++it;
        }
        return *it;
    }
}

const ContainerGroup &Graph::container_group(const size_t index) const
{
    assert(index < _container_groups.size());
    const size_t count = _container_groups.size();
    if (index <= count / 2)
    {
        std::list<ContainerGroup>::const_iterator it = _container_groups.cbegin();
        for (size_t i = 0; i < index; ++i)
        {
            ++it;
        }
        return *it;
    }
    else
    {
        std::list<ContainerGroup>::const_reverse_iterator it = _container_groups.crbegin();
        for (size_t i = 1, end = count - index; i < end; ++i)
        {
            ++it;
        }
        return *it;
    }
}

std::list<ContainerGroup> &Graph::container_groups()
{
    return _container_groups;
}

const std::list<ContainerGroup> &Graph::container_groups() const
{
    return _container_groups;
}

ContainerGroup &Graph::operator[](const size_t index)
{
    assert(index < _container_groups.size());
    const size_t count = _container_groups.size();
    if (index <= count / 2)
    {
        std::list<ContainerGroup>::iterator it = _container_groups.begin();
        for (size_t i = 0; i < index; ++i)
        {
            ++it;
        }
        return *it;
    }
    else
    {
        std::list<ContainerGroup>::reverse_iterator it = _container_groups.rbegin();
        for (size_t i = 1, end = count - index; i < end; ++i)
        {
            ++it;
        }
        return *it;
    }
}

const ContainerGroup &Graph::operator[](const size_t index) const
{
    assert(index < _container_groups.size());
    const size_t count = _container_groups.size();
    if (index <= count / 2)
    {
        std::list<ContainerGroup>::const_iterator it = _container_groups.cbegin();
        for (size_t i = 0; i < index; ++i)
        {
            ++it;
        }
        return *it;
    }
    else
    {
        std::list<ContainerGroup>::const_reverse_iterator it = _container_groups.crbegin();
        for (size_t i = 1, end = count - index; i < end; ++i)
        {
            ++it;
        }
        return *it;
    }
}

const bool Graph::empty() const
{
    return _container_groups.empty() || std::all_of(_container_groups.begin(), _container_groups.end(), [](const ContainerGroup &g)
                                                    { return g.empty(); });
}

const bool Graph::empty(const size_t index) const
{
    assert(index < _container_groups.size());
    return container_group(index).empty();
}

const size_t Graph::size() const
{
    return _container_groups.size();
}

const size_t Graph::count(const Geo::Type type, const bool include_combinated) const
{
    size_t num = 0;
    for (const ContainerGroup &group : _container_groups)
    {
        num += group.count(type, include_combinated);
    }
    return num;
}

void Graph::clear()
{
    _container_groups.clear();
}

void Graph::clear(const size_t index)
{
    assert(index < _container_groups.size());
    container_group(index).clear();
}

void Graph::transform(const double a, const double b, const double c, const double d, const double e, const double f)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.transform(a, b, c, d, e, f);
    }
}

void Graph::transform(const double mat[6])
{
    for (ContainerGroup &group : _container_groups)
    {
        group.transform(mat);
    }
}

void Graph::translate(const double tx, const double ty)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.translate(tx, ty);
    }
}

void Graph::rotate(const double x, const double y, const double rad)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.rotate(x, y, rad);
    }
}

void Graph::scale(const double x, const double y, const double k)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.scale(x, y, k);
    }
}

void Graph::rescale(const double x, const double y)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.rescale(x, y);
    }
}

Geo::AABBRect Graph::bounding_rect() const
{
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const ContainerGroup &group : _container_groups)
    {
        if (group.empty())
        {
            continue;
        }
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.x);
            y0 = std::min(y0, point.y);
            x1 = std::max(x1, point.x);
            y1 = std::max(y1, point.y);
        }
    }
    return Geo::AABBRect(x0, y0, x1, y1);
}




std::list<ContainerGroup>::iterator Graph::begin()
{
    return _container_groups.begin();
}

std::list<ContainerGroup>::iterator Graph::end()
{
    return _container_groups.end();
}

std::list<ContainerGroup>::const_iterator Graph::begin() const
{
    return _container_groups.cbegin();
}

std::list<ContainerGroup>::const_iterator Graph::end() const
{
    return _container_groups.cend();
}

std::list<ContainerGroup>::const_iterator Graph::cbegin() const
{
    return _container_groups.cbegin();
}

std::list<ContainerGroup>::const_iterator Graph::cend() const
{
    return _container_groups.cend();
}

std::list<ContainerGroup>::reverse_iterator Graph::rbegin()
{
    return _container_groups.rbegin();
}

std::list<ContainerGroup>::reverse_iterator Graph::rend()
{
    return _container_groups.rend();
}

std::list<ContainerGroup>::const_reverse_iterator Graph::rbegin() const
{
    return _container_groups.crbegin();
}

std::list<ContainerGroup>::const_reverse_iterator Graph::rend() const
{
    return _container_groups.crend();
}

std::list<ContainerGroup>::const_reverse_iterator Graph::crbegin() const
{
    return _container_groups.crbegin();
}

std::list<ContainerGroup>::const_reverse_iterator Graph::crend() const
{
    return _container_groups.crend();
}

ContainerGroup &Graph::front()
{
    assert(!_container_groups.empty());
    return _container_groups.front();
}

const ContainerGroup &Graph::front() const
{
    assert(!_container_groups.empty());
    return _container_groups.front();
}

ContainerGroup &Graph::back()
{
    assert(!_container_groups.empty());
    return _container_groups.back();
}

const ContainerGroup &Graph::back() const
{
    assert(!_container_groups.empty());
    return _container_groups.back();
}



void Graph::append(Geo::Geometry *object, const size_t index)
{
    assert(index < _container_groups.size());
    container_group(index).append(object);
    object->is_selected = false;
}

void Graph::append_group()
{
    _container_groups.push_back(ContainerGroup());
}

void Graph::append_group(const ContainerGroup &group)
{
    _container_groups.push_back(group);
}

void Graph::append_group(const ContainerGroup &&group)
{
    _container_groups.push_back(group);
}

void Graph::insert_group(const size_t index)
{
    assert(index < _container_groups.size());
    std::list<ContainerGroup>::iterator it = _container_groups.begin();
    for (size_t i = 0; i < index; ++i)
    {
        ++it;
    }
    _container_groups.insert(it, ContainerGroup());
}

void Graph::insert_group(const size_t index, const ContainerGroup &group)
{
    assert(index < _container_groups.size());
    std::list<ContainerGroup>::iterator it = _container_groups.begin();
    for (size_t i = 0; i < index; ++i)
    {
        ++it;
    }
    _container_groups.insert(it, group);
}

void Graph::insert_group(const size_t index, const ContainerGroup &&group)
{
    assert(index < _container_groups.size());
    std::list<ContainerGroup>::iterator it = _container_groups.begin();
    for (size_t i = 0; i < index; ++i)
    {
        ++it;
    }
    _container_groups.insert(it, group);
}

void Graph::remove_group(const size_t index)
{
    assert(index < _container_groups.size());
    std::list<ContainerGroup>::iterator it = _container_groups.begin();
    for (size_t i = 0; i < index; ++i)
    {
        ++it;
    }
    _container_groups.erase(it);
}




bool Graph::has_group(const QString &name) const
{
    for (const ContainerGroup &group : _container_groups)
    {
        if (group.name == name)
        {
            return true;
        }
    }
    return false;
}

bool Graph::has_object(const QString &name) const
{
    for (const ContainerGroup &group : _container_groups)
    {
        for (const Geo::Geometry *object : group)
        {
            if (object->name == name)
            {
                return true;
            }
        }
    }
    return false;
}

bool Graph::remove_object(const Geo::Geometry *object)
{
    for (ContainerGroup &group : _container_groups)
    {
        for (size_t i = 0, count = group.size(); i < count; ++i)
        {
            if (group[i] == object)
            {
                group.remove(i);
                return true;
            }
        }
    }
    return false;
}


void Graph::update_curve_shape(const double step, const double down_sampling_value)
{
    for (ContainerGroup &group : _container_groups)
    {
        for (Geo::Geometry *object : group)
        {
            switch (object->type())
            {
            case Geo::Type::BEZIER:
                static_cast<Geo::Bezier *>(object)->update_shape(step, down_sampling_value);
                break;
            case Geo::Type::BSPLINE:
                static_cast<Geo::BSpline *>(object)->update_shape(step, down_sampling_value);
                break;
            case Geo::Type::CIRCLE:
                static_cast<Geo::Circle *>(object)->update_shape(down_sampling_value);
                break;
            case Geo::Type::ELLIPSE:
                static_cast<Geo::Ellipse *>(object)->update_shape(down_sampling_value);
                break;
            default:
                break;
            }
        }
    }
}