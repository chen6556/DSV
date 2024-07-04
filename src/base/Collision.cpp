#include <set>
#include "base/Collision.hpp"
#include "base/Algorithm.hpp"


using namespace Geo;

Collision::GridNode::GridNode()
{

}

Collision::GridNode::GridNode(const Geo::AABBRect &rect)
    : _rect(rect)
{
  
}

Collision::GridNode::GridNode(const double left, const double top, const double right, const double bottom)
    : _rect(left, top, right, bottom)
{

}

void Collision::GridNode::set_rect(const Geo::AABBRect &rect)
{
    _rect = rect;
}

const Geo::AABBRect &Collision::GridNode::rect() const
{
    return _rect;
}

bool Collision::GridNode::append(Geo::Geometry *object)
{
    if (std::find(_objects.begin(), _objects.end(), object) == _objects.end())
    {
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
        case Geo::Type::AABBRECT:
        case Geo::Type::POLYGON:
        case Geo::Type::CIRCLE:
        case Geo::Type::LINE:
        case Geo::Type::BEZIER:
        case Geo::Type::CONTAINER:
        case Geo::Type::CIRCLECONTAINER:
            _objects.push_back(object);
            return true;
        default:
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool Collision::GridNode::remove(Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::CIRCLE:
    case Geo::Type::LINE:
    case Geo::Type::BEZIER:
    case Geo::Type::CONTAINER:
    case Geo::Type::CIRCLECONTAINER:
        {
            std::vector<Geo::Geometry *>::iterator it = std::find(_objects.begin(), _objects.end(), object);
            if (it == _objects.end())
            {
                return true;
            }
            else
            {
                _objects.erase(it);
                return true;
            }
        }
    default:
        return false;
    }
}

bool Collision::GridNode::has(Geo::Geometry *object) const
{
    return std::find(_objects.begin(), _objects.end(), object) != _objects.end(); 
}

void Collision::GridNode::clear()
{
    _objects.clear();
}

bool Collision::GridNode::select(const Geo::Point &pos, std::vector<Geo::Geometry *> &objects) const
{
    const size_t size = objects.size();
    for (Geo::Geometry *object : _objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            if (Geo::is_inside(pos, *static_cast<Geo::Polyline *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::AABBRECT:
            if (Geo::is_inside(pos, *static_cast<Geo::AABBRect *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::POLYGON:
        case Geo::Type::CONTAINER:
            if (Geo::is_inside(pos, *static_cast<Geo::Polygon *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::CIRCLECONTAINER:
            if (Geo::is_inside(pos, *static_cast<Geo::Circle *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::LINE:
            if (Geo::is_inside(pos, *static_cast<Geo::Line *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_inside(pos, static_cast<Geo::Bezier *>(object)->shape()))
            {
                objects.push_back(object);
            }
            break;
        default:
            break;
        }
    }
    return objects.size() > size;
}

bool Collision::GridNode::select(const Geo::AABBRect &rect, std::vector<Geo::Geometry *> &objects) const
{
    const size_t size = objects.size();
    for (Geo::Geometry *object : _objects)
    {
        switch (object->type())
        {
        case Geo::Type::POINT:
            if (Geo::is_inside(*static_cast<Geo::Point *>(object), rect))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polyline *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::AABBRECT:
            if (Geo::is_intersected(rect, *static_cast<Geo::AABBRect *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::POLYGON:
        case Geo::Type::CONTAINER:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polygon *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::CIRCLECONTAINER:
            if (Geo::is_intersected(rect, *static_cast<Geo::Circle *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::LINE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Line *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_intersected(rect, static_cast<Geo::Bezier *>(object)->shape()))
            {
                objects.push_back(object);
            }
            break;
        default:
            break;
        }
    }
    return objects.size() > size;
}

bool Collision::GridNode::find_collision_objects(const Geo::Geometry *object, std::vector<Geo::Geometry *> &objects) const
{
    const size_t size = objects.size();
    for (Geo::Geometry *obj : _objects)
    {
        if (obj != object && Geo::is_intersected(object, obj))
        {
            objects.push_back(obj);
        }
    }
    return objects.size() > size;
}

bool Collision::GridNode::find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs) const
{
    const size_t size = pairs.size();
    for (size_t i = 0, count = _objects.size(); i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            if (Geo::is_intersected(_objects[i], _objects[j]))
            {
                pairs.emplace_back(_objects[i], _objects[j]);
            }
        }
    }
    return pairs.size() > size;
}


Collision::GridMap::GridMap()
{

}

Collision::GridMap::GridMap(const ContainerGroup &group)
    : _objects(group.cbegin(), group.cend())
{
    build(_objects);
}

Collision::GridMap::GridMap(const std::vector<Geo::Geometry *> &objects)
    : _objects(objects)
{
    build(_objects);
}

Collision::GridMap::GridMap(const std::initializer_list<Geo::Geometry *> &objects)
    : _objects(objects.begin(), objects.end())
{
    build(_objects);
}

void Collision::GridMap::build(const ContainerGroup &group)
{
    _left = _bottom = DBL_MAX;
    _right = _top = -DBL_MAX;
    _rects.clear();
    _objects.clear();
    _grids.clear();
    for (Geo::Geometry *object : group)
    {
        _objects.push_back(object);
        _rects.emplace_back(object->bounding_rect());
        _left = std::min(_rects.back().left(), _left);
        _top = std::max(_rects.back().top(), _top);
        _right = std::max(_rects.back().right(), _right);
        _bottom = std::min(_rects.back().bottom(), _bottom);
    }

    if (_objects.size() > 40 || (_right - _left) > 800)
    {
        double x_step = (_right - _left) / 8, y_step = (_top - _bottom) / 4;
        for (size_t i = 0; i < 8; ++i)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                _grids.emplace_back(_left + x_step * i, _top - y_step * j,
                    _left + x_step * i + x_step, _top - y_step * j - y_step);

                for (size_t k = 0, count = _rects.size(); k < count; ++k)
                {
                    if (Geo::is_intersected(_grids.back().rect(), _rects[k]))
                    {
                        _grids.back().append(_objects[k]);
                    }
                }
            }
        }
    }
    else
    {
        _grids.emplace_back(_left, _top, _right, _bottom);
        for (Geo::Geometry *object : _objects)
        {
            _grids.back().append(object);
        }
    }
}

void Collision::GridMap::build(const std::vector<Geo::Geometry *> &objects)
{
    _left = _bottom = DBL_MAX;
    _right = _top = -DBL_MAX;
    _rects.clear();
    _grids.clear();
    for (Geo::Geometry *object : objects)
    {
        _rects.emplace_back(object->bounding_rect());
        _left = std::min(_rects.back().left(), _left);
        _top = std::max(_rects.back().top(), _top);
        _right = std::max(_rects.back().right(), _right);
        _bottom = std::min(_rects.back().bottom(), _bottom);
    }
    if (&objects != &_objects)
    {
        _objects.assign(objects.begin(), objects.end());
    }

    if (_objects.size() > 40 || (_right - _left) > 800)
    {
        double x_step = (_right - _left) / 8, y_step = (_top - _bottom) / 4;
        for (size_t i = 0; i < 8; ++i)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                _grids.emplace_back(_left + x_step * i, _top - y_step * j,
                    _left + x_step * i + x_step, _top - y_step * j - y_step);

                for (size_t k = 0, count = _rects.size(); k < count; ++k)
                {
                    if (Geo::is_intersected(_grids.back().rect(), _rects[k]))
                    {
                        _grids.back().append(objects[k]);
                    }
                }
            }
        }
    }
    else
    {
        _grids.emplace_back(_left, _top, _right, _bottom);
        for (Geo::Geometry *object : _objects)
        {
            _grids.back().append(object);
        }
    }
}

void Collision::GridMap::build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects)
{
    _left = _bottom = DBL_MAX;
    _right = _top = -DBL_MAX;
    _grids.clear();
    for (const Geo::AABBRect &rect : rects)
    {
        _left = std::min(rect.left(), _left);
        _top = std::max(rect.top(), _top);
        _right = std::max(rect.right(), _right);
        _bottom = std::min(rect.bottom(), _bottom);
    }
    if (&objects != &_objects)
    {
        _objects.assign(objects.begin(), objects.end());
        _rects.assign(rects.begin(), rects.end());
    }

    if (objects.size() > 40 || (_right - _left) > 800)
    {
        double x_step = (_right - _left) / 8, y_step = (_top - _bottom) / 4;
        for (size_t i = 0; i < 8; ++i)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                _grids.emplace_back(_left + x_step * i, _top - y_step * j,
                    _left + x_step * i + x_step, _top - y_step * j - y_step);

                for (size_t k = 0, count = rects.size(); k < count; ++k)
                {
                    if (Geo::is_intersected(_grids.back().rect(), rects[k]))
                    {
                        _grids.back().append(objects[k]);
                    }
                }
            }
        }
    }
    else
    {
        _grids.emplace_back(_left, _top, _right, _bottom);
        for (Geo::Geometry *object : objects)
        {
            _grids.back().append(object);
        }
    }
}

void Collision::GridMap::append(Geo::Geometry *object)
{
    if (std::find(_objects.begin(), _objects.end(), object) != _objects.end())
    {
        return;
    }
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::CIRCLE:
    case Geo::Type::LINE:
    case Geo::Type::BEZIER:
    case Geo::Type::CONTAINER:
    case Geo::Type::CIRCLECONTAINER:
        break;
    default:
        return;
    }

    _objects.push_back(object);
    _rects.emplace_back(object->bounding_rect());
    if (_rects.back().left() >= _left && _rects.back().right() <= _right
        && _rects.back().top() <= _top && _rects.back().bottom() >= _bottom)
    {
        for (GridNode &grid : _grids)
        {
            if (Geo::is_intersected(grid.rect(), _rects.back()))
            {
                grid.append(object);
            }
        }
    }
    else
    {
        _grids.clear();
        _left = std::min(_left, _rects.back().left());
        _top = std::max(_top, _rects.back().top());
        _right = std::max(_right, _rects.back().right());
        _bottom = std::min(_bottom, _rects.back().bottom());

        if (_objects.size() > 40 || (_right - _left) > 800)
        {
            double x_step = (_right - _left) / 8, y_step = (_top - _bottom) / 4;
            for (size_t i = 0; i < 8; ++i)
            {
                for (size_t j = 0; j < 4; ++j)
                {
                    _grids.emplace_back(_left + x_step * i, _top - y_step * j,
                        _left + x_step * i + x_step, _top - y_step * j - y_step);

                    for (size_t k = 0, count = _rects.size(); k < count; ++k)
                    {
                        if (Geo::is_intersected(_grids.back().rect(), _rects[k]))
                        {
                            _grids.back().append(_objects[k]);
                        }
                    }
                }
            }
        }
        else
        {
            _grids.emplace_back(_left, _top, _right, _bottom);
            for (Geo::Geometry *object : _objects)
            {
                _grids.back().append(object);
            }
        }
    }
}

void Collision::GridMap::remove(Geo::Geometry *object)
{
    std::vector<Geo::Geometry *>::iterator it = std::find(_objects.begin(), _objects.end(), object);
    if (it == _objects.end())
    {
        return;
    }
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::CIRCLE:
    case Geo::Type::LINE:
    case Geo::Type::BEZIER:
    case Geo::Type::CONTAINER:
    case Geo::Type::CIRCLECONTAINER:
        break;
    default:
        return;
    }

    for (GridNode &grid : _grids)
    {
        grid.remove(object);
    }
    _rects.erase(_rects.begin() + std::distance(_objects.begin(), it));
    _objects.erase(it);
}

void Collision::GridMap::update(Geo::Geometry *object)
{
    std::vector<Geo::Geometry *>::iterator it = std::find(_objects.begin(), _objects.end(), object);
    if (it == _objects.end())
    {
        return;
    }
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::CIRCLE:
    case Geo::Type::LINE:
    case Geo::Type::BEZIER:
    case Geo::Type::CONTAINER:
    case Geo::Type::CIRCLECONTAINER:
        break;
    default:
        return;
    }

    const size_t index = std::distance(_objects.begin(), it);
    _rects[index] = object->bounding_rect();
    if (_rects[index].left() >= _left && _rects[index].right() <= _right
        && _rects[index].top() <= _top && _rects[index].bottom() >= _bottom)
    {
        for (GridNode &grid : _grids)
        {
            if (Geo::is_intersected(grid.rect(), _rects[index]))
            {
                grid.append(object);
            }
            else
            {
                grid.remove(object);
            }
        }
    }
    else
    {
        _grids.clear();
        build(_objects, _rects);
    }
}

void Collision::GridMap::update()
{
    _grids.clear();
    build(_objects, _rects);
}

bool Collision::GridMap::has(Geo::Geometry *object) const
{
    return std::find(_objects.begin(), _objects.end(), object) != _objects.end();
}

void Collision::GridMap::clear()
{
    _objects.clear();
    _rects.clear();
    _grids.clear();
}

bool Collision::GridMap::select(const Point &pos, std::vector<Geometry *> &objects) const
{
    if (pos.x >= _left && pos.x <= _right && pos.y >= _bottom && pos.y <= _top)
    {
        for (const GridNode &grid : _grids)
        {
            if (Geo::is_inside(pos, grid.rect()))
            {
                return grid.select(pos, objects);
            }
        }
    }
    return false;
}

bool Collision::GridMap::select(const AABBRect &rect, std::vector<Geometry *> &objects) const
{
    if (rect.right() < _left || rect.left() > _right || rect.bottom() > _top || rect.top() < _bottom)
    {
        return false;
    }

    const size_t size = objects.size();
    for (const GridNode &grid : _grids)
    {
        if (Geo::is_intersected(rect, grid.rect()))
        {
            grid.select(rect, objects);
        }
    }
    std::set<Geo::Geometry *> temp(objects.begin(), objects.end());
    objects.assign(temp.begin(), temp.end());
    return objects.size() > size;
}

bool Collision::GridMap::find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const
{
    if (object == nullptr)
    {
        return false;
    }
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::CIRCLE:
    case Geo::Type::LINE:
    case Geo::Type::BEZIER:
    case Geo::Type::CONTAINER:
    case Geo::Type::CIRCLECONTAINER:
        break;
    default:
        return false;
    }

    const size_t size = objects.size();
    const Geo::AABBRect rect(object->bounding_rect());
    for (const GridNode &grid : _grids)
    {
        if (Geo::is_intersected(rect, grid.rect()))
        {
            grid.find_collision_objects(object, objects);
        }
    }
    std::set<Geo::Geometry *> temp(objects.begin(), objects.end());
    objects.assign(temp.begin(), temp.end());
    return objects.size() > size;
}

bool Collision::GridMap::find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const
{
    const size_t size = pairs.size();
    for (const GridNode &grid : _grids)
    {
        grid.find_collision_pairs(pairs);
    }

    for (size_t i = 0, count = pairs.size(); i < count; ++i)
    {
        for (size_t j = count - 1; j > i; --j)
        {
            if ((pairs[i].first == pairs[j].first && pairs[i].second == pairs[j].second)
                || (pairs[i].first == pairs[j].second && pairs[i].second == pairs[j].first))
            {
                pairs.erase(pairs.begin() + j);
                --count;
            }
        }
    }

    return pairs.size() > size;
}
