#include "draw/Graph.hpp"
#include <cfloat>

Graph::Graph(const Graph &graph)
    : Geo::Geometry(graph), _ratio(graph._ratio)
{
    for (const ContainerGroup &group : graph._container_groups)
    {
        _container_groups.emplace_back(group);
    }
}

Graph::Graph(const Graph &&graph)
    : Geo::Geometry(graph), _ratio(std::move(graph._ratio))
{
    for (const ContainerGroup &group : graph._container_groups)
    {
        _container_groups.emplace_back(group);
    }
}

Graph *Graph::clone() const
{
    return new Graph(*this);
}

void Graph::transfer(Graph &graph)
{
    graph.clear();
    graph._memo = _memo;
    for (ContainerGroup &group : _container_groups)
    {
        graph.append_group();
        group.transfer(graph.back());
    }
    clear();
}

Graph &Graph::operator=(const Graph &graph)
{
    if (this != &graph)
    {
        Geo::Geometry::operator=(graph);
        _container_groups.clear();
        for (const ContainerGroup &group : graph._container_groups)
        {
            _container_groups.emplace_back(group);
        }
        _ratio = graph._ratio;
    }
    return *this;
}

Graph &Graph::operator=(const Graph &&graph)
{
    if (this != &graph)
    {
        Geo::Geometry::operator=(graph);
        _container_groups.clear();
        for (const ContainerGroup &group : graph._container_groups)
        {
            _container_groups.emplace_back(group);
        }
        _ratio = std::move(graph._ratio);
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
    _ratio *= k;
}

void Graph::rescale(const double x, const double y)
{
    for (ContainerGroup &group : _container_groups)
    {
        group.rescale(x, y);
    }
    _ratio = 1;
}

double Graph::ratio() const
{
    return _ratio;
}

Geo::Rectangle Graph::bounding_rect() const
{
    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const ContainerGroup &group : _container_groups)
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.coord().x);
            y0 = std::min(y0, point.coord().y);
            x1 = std::max(x1, point.coord().x);
            y1 = std::max(y1, point.coord().y);
        }
    }
    return Geo::Rectangle(x0, y0, x1, y1);
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



void Graph::append(Container *container, const size_t index)
{
    assert(index < _container_groups.size());
    if (container->shape().size() > 3)
    {
        container_group(index).append(container);
        container->memo()["is_selected"] = false;
    }
}

void Graph::append(CircleContainer *container, const size_t index)
{
    assert(index < _container_groups.size());
    if (!container->shape().empty())
    {
        container_group(index).append(container);
        container->memo()["is_selected"] = false;
    }
}

void Graph::append(Geo::Polyline *polyline, const size_t index)
{
    assert(index < _container_groups.size());
    if (polyline->size() > 1)
    {
        container_group(index).append(polyline);
        polyline->memo()["is_selected"] = false;
    }
}

void Graph::append(Geo::Bezier *bezier, const size_t index)
{
    assert(index < _container_groups.size());
    container_group(index).append(bezier);
    bezier->memo()["is_selected"] = false;
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
    _container_groups.push_back(std::move(group));
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
    _container_groups.insert(it, std::move(group));
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