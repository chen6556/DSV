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
        _objects.push_back(object);
        return true;
    }
    else
    {
        return false;
    }
}

bool Collision::GridNode::remove(Geo::Geometry *object)
{   
    std::vector<Geo::Geometry *>::iterator it = std::find(_objects.begin(), _objects.end(), object);
    if (it == _objects.end())
    {
        return false;
    }
    else
    {
        _objects.erase(it);
        return true;
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
    if (group.empty())
    {
        _left = _bottom = 0;
        _right = _top = 100;
        _rects.clear();
        _objects.clear();
        _grids.clear();
        _grids.emplace_back(_left, _top, _right, _bottom);
        return;
    }

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
    if (objects.empty())
    {
        _left = _bottom = 0;
        _right = _top = 100;
        _rects.clear();
        _objects.clear();
        _grids.clear();
        _grids.emplace_back(_left, _top, _right, _bottom);
        return;
    }

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
    if (objects.empty())
    {
        _left = _bottom = 0;
        _right = _top = 100;
        _rects.clear();
        _objects.clear();
        _grids.clear();
        _grids.emplace_back(_left, _top, _right, _bottom);
        return;
    }

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

bool Collision::GridMap::select(const Geo::Point &pos, std::vector<Geo::Geometry *> &objects) const
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

bool Collision::GridMap::select(const Geo::AABBRect &rect, std::vector<Geo::Geometry *> &objects) const
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

bool Collision::GridMap::find_collision_objects(const Geo::Geometry *object, std::vector<Geo::Geometry *> &objects) const
{
    if (object == nullptr)
    {
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

bool Collision::GridMap::find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs) const
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


Collision::QuadTreeNode::QuadTreeNode()
{

}

Collision::QuadTreeNode::QuadTreeNode(const Geo::AABBRect &rect)
    : Collision::GridNode(rect)
{

}

Collision::QuadTreeNode::QuadTreeNode(const double left, const double top, const double right, const double bottom)
    : Collision::GridNode(left, top, right, bottom)
{

}

Collision::QuadTreeNode::~QuadTreeNode()
{
    if (_nodes[0] != nullptr)
    {
        Collision::QuadTreeNode *node;
        std::vector<Collision::QuadTreeNode*> nodes({_nodes[0], _nodes[1], _nodes[2], _nodes[3]});
        while (!nodes.empty())
        {
            node = nodes.back();
            nodes.pop_back();
            if (node->_nodes[0] != nullptr)
            {
                nodes.push_back(node->_nodes[0]);
                nodes.push_back(node->_nodes[1]);
                nodes.push_back(node->_nodes[2]);
                nodes.push_back(node->_nodes[3]);
            }
            node->_nodes[0] = nullptr;
            delete node;
        }   
    }
}

void Collision::QuadTreeNode::split()
{
    const double left = _rect.left(), top = _rect.top(),
        right = _rect.right(), bottom = _rect.bottom();
    _nodes[0] = new QuadTreeNode(left, top, (left + right) / 2, (top + bottom) / 2);
    _nodes[1] = new QuadTreeNode((left + right) / 2, top, right, (top + bottom) / 2);
    _nodes[2] = new QuadTreeNode(left, (top + bottom) / 2, (left + right) / 2, bottom);
    _nodes[3] = new QuadTreeNode((left + right) / 2, (top + bottom) / 2, right, bottom);

    for (Geo::Geometry *object : _objects)
    {
        const Geo::AABBRect rect(object->bounding_rect());
        for (size_t i = 0; i < 4; ++i)
        {
            if (Geo::is_intersected(_nodes[i]->_rect, rect))
            {
                _nodes[i]->append(object);
            }
        }
    }
    _objects.clear();
}

bool Collision::QuadTreeNode::is_tail_node() const
{
    return _nodes[0] == nullptr;
}

bool Collision::QuadTreeNode::merge()
{
    if (_nodes[0] == nullptr)
    {
        return true;
    }
    else if (_nodes[0]->is_tail_node() && _nodes[1]->is_tail_node()
        && _nodes[2]->is_tail_node() && _nodes[3]->is_tail_node())
    {
        size_t count = _nodes[0]->_objects.size();
        count += _nodes[1]->_objects.size();
        count += _nodes[2]->_objects.size();
        count += _nodes[3]->_objects.size();
        if (count < 4 || (count <= 40 && _rect.width() <= 200))
        {
            for (size_t i = 0; i < 4; ++i)
            {
                _objects.insert(_objects.end(), _nodes[i]->_objects.begin(), _nodes[i]->_objects.end());
                delete _nodes[i];
                _nodes[i] = nullptr;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        bool flag0 = _nodes[0]->merge();
        bool flag1 = _nodes[1]->merge();
        bool flag2 = _nodes[2]->merge();
        bool flag3 = _nodes[3]->merge();
        if (flag0 && flag1 && flag2 && flag3)
        {
            size_t count = _nodes[0]->_objects.size();
            count += _nodes[1]->_objects.size();
            count += _nodes[2]->_objects.size();
            count += _nodes[3]->_objects.size();
            if (count < 4 || (count <= 40 && _rect.width() <= 200))
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    _objects.insert(_objects.end(), _nodes[i]->_objects.begin(), _nodes[i]->_objects.end());
                    delete _nodes[i];
                    _nodes[i] = nullptr;
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
}

bool Collision::QuadTreeNode::append(Geo::Geometry *object)
{
    if (_nodes[0] == nullptr && _nodes[1] == nullptr && _nodes[2] == nullptr && _nodes[3] == nullptr)
    {
        if (std::find(_objects.begin(), _objects.end(), object) == _objects.end())
        {
            _objects.push_back(object);
            if (_objects.size() > 40 || (_objects.size() > 4 && _rect.width() > 200))
            {
                split();
            }
            return true;
        }
        return false;
    }
    else
    {
        const Geo::AABBRect rect(object->bounding_rect());
        bool flag = false;
        for (size_t i = 0; i < 4; ++i)
        {
            if (_nodes[i] != nullptr && Geo::is_intersected(_nodes[i]->_rect, rect))
            {
                if (_nodes[i]->append(object))
                {
                    flag = true;
                }
            }
        }
        return flag;
    }
}

void Collision::QuadTreeNode::append_node(const size_t index, Collision::QuadTreeNode *node)
{
    assert(index < 4);
    if (_nodes[index] != nullptr)
    {
        delete _nodes[index];
    }
    _nodes[index] = node;
}

bool Collision::QuadTreeNode::remove(Geo::Geometry *object)
{
    if (_nodes[0] == nullptr)
    {
        std::vector<Geo::Geometry *>::iterator it = std::find(_objects.begin(), _objects.end(), object);
        if (it == _objects.end())
        {
            return false;
        }
        _objects.erase(it);
        merge();
        return true;
    }
    else
    {
        bool flag = false;
        for (size_t i = 0; i < 4; ++i)
        {
            if (_nodes[i]->remove(object))
            {
                flag = true;
            }
        }
        merge();
        return flag;
    }
}

bool Collision::QuadTreeNode::has(Geo::Geometry *object) const
{
    if (_nodes[0] == nullptr)
    {
        return std::find(_objects.begin(), _objects.end(), object) != _objects.end();
    }
    else
    {
        return _nodes[0]->has(object) || _nodes[1]->has(object) || _nodes[2]->has(object) || _nodes[3]->has(object);
    }
}

void Collision::QuadTreeNode::clear()
{
    if (_nodes[0] == nullptr)
    {
        _objects.clear();
    }
    else
    {
        for (size_t i = 0; i < 4; ++i)
        {
            delete _nodes[i];
            _nodes[i] = nullptr;
        }
    }
}

bool Collision::QuadTreeNode::select(const Geo::Point &pos, std::vector<Geo::Geometry *> &objects) const
{
    if (_nodes[0] == nullptr)
    {
        return Collision::GridNode::select(pos, objects);
    }
    else
    {
        const size_t size = objects.size();
        _nodes[0]->select(pos, objects);
        _nodes[1]->select(pos, objects);
        _nodes[2]->select(pos, objects);
        _nodes[3]->select(pos, objects);
        std::set<Geo::Geometry *> temp(objects.begin(), objects.end());
        objects.assign(temp.begin(), temp.end());
        return objects.size() > size;
    }
}

bool Collision::QuadTreeNode::select(const Geo::AABBRect &rect, std::vector<Geo::Geometry *> &objects) const
{
    if (_nodes[0] == nullptr)
    {
        return Collision::GridNode::select(rect, objects);
    }
    else
    {
        const size_t size = objects.size();
        _nodes[0]->select(rect, objects);
        _nodes[1]->select(rect, objects);
        _nodes[2]->select(rect, objects);
        _nodes[3]->select(rect, objects);
        std::set<Geo::Geometry *> temp(objects.begin(), objects.end());
        objects.assign(temp.begin(), temp.end());
        return objects.size() > size;
    }
}

bool Collision::QuadTreeNode::find_collision_objects(const Geo::Geometry *object, std::vector<Geo::Geometry *> &objects) const
{
    if (_nodes[0] == nullptr)
    {
        return Collision::GridNode::find_collision_objects(object, objects);
    }
    else
    {
        const size_t size = objects.size();
        _nodes[0]->find_collision_objects(object, objects);
        _nodes[1]->find_collision_objects(object, objects);
        _nodes[2]->find_collision_objects(object, objects);
        _nodes[3]->find_collision_objects(object, objects);
        return objects.size() > size;
    }
}

bool Collision::QuadTreeNode::find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs) const
{
    if (_nodes[0] == nullptr)
    {
        return Collision::GridNode::find_collision_pairs(pairs);
    }
    else
    {
        const size_t size = pairs.size();
        _nodes[0]->find_collision_pairs(pairs);
        _nodes[1]->find_collision_pairs(pairs);
        _nodes[2]->find_collision_pairs(pairs);
        _nodes[3]->find_collision_pairs(pairs);

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
}


Collision::QuadTree::QuadTree()
{

}

Collision::QuadTree::QuadTree(const ContainerGroup &group)
    : _objects(group.cbegin(), _objects.cend())
{
    build(_objects);
}

Collision::QuadTree::QuadTree(const std::vector<Geo::Geometry *> &objects)
    : _objects(objects)
{
    build(objects);
}

Collision::QuadTree::QuadTree(const std::initializer_list<Geo::Geometry *> &objects)
    : _objects(objects)
{
    build(_objects);
}

Collision::QuadTree::~QuadTree()
{
    if (_root != nullptr)
    {
        delete _root;
    }
}

void Collision::QuadTree::build(const ContainerGroup &group)
{
    if (_root != nullptr)
    {
        delete _root;
    }

    if (group.empty())
    {
        _left = _bottom = 0;
        _top = _right = 100;
        _root = new QuadTreeNode(_left, _top, _right, _bottom);
        _objects.clear();
        _rects.clear();
        return;
    }

    _left = _bottom = DBL_MAX;
    _top = _right = -DBL_MAX;
    _rects.clear();
    for (const Geo::Geometry *object : group)
    {
        _rects.emplace_back(object->bounding_rect());
        _left = std::min(_left, _rects.back().left());
        _top = std::max(_top, _rects.back().top());
        _right = std::max(_right, _rects.back().right());
        _bottom = std::min(_bottom, _rects.back().bottom());
    }
    _objects.assign(group.cbegin(), group.cend());

    _root = new QuadTreeNode(_left, _top, _right, _bottom);
    for (Geo::Geometry *object : group)
    {
        _root->append(object);
    }
}

void Collision::QuadTree::build(const std::vector<Geo::Geometry *> &objects)
{
    if (_root != nullptr)
    {
        delete _root;
    }

    if (objects.empty())
    {
        _left = _bottom = 0;
        _top = _right = 100;
        _root = new QuadTreeNode(_left, _top, _right, _bottom);
        _rects.clear();
        _objects.clear();
        return;
    }

    _left = _bottom = DBL_MAX;
    _top = _right = -DBL_MAX;
    _rects.clear();
    for (const Geo::Geometry *object : objects)
    {
        _rects.emplace_back(object->bounding_rect());
        _left = std::min(_left, _rects.back().left());
        _top = std::max(_top, _rects.back().top());
        _right = std::max(_right, _rects.back().right());
        _bottom = std::min(_bottom, _rects.back().bottom());
    }
    if (&objects != &_objects)
    {
        _objects.assign(objects.cbegin(), objects.cend());
    }

    _root = new QuadTreeNode(_left, _top, _right, _bottom);
    for (Geo::Geometry *object : objects)
    {
        _root->append(object);
    }
}

void Collision::QuadTree::build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects)
{
    if (_root != nullptr)
    {
        delete _root;
    }

    if (objects.empty())
    {
        _left = _bottom = 0;
        _top = _right = 100;
        _root = new QuadTreeNode(_left, _top, _right, _bottom);
        _rects.clear();
        _objects.clear();
        return;
    }

    _left = _bottom = DBL_MAX;
    _top = _right = -DBL_MAX;
    for (const Geo::AABBRect &rect : rects)
    {
        _left = std::min(_left, rect.left());
        _top = std::max(_top, rect.top());
        _right = std::max(_right, rect.right());
        _bottom = std::min(_bottom, rect.bottom());
    }
    if (&objects != &_objects)
    {
        _objects.assign(objects.cbegin(), objects.cend());
        _rects.assign(rects.cbegin(), rects.cend());
    }

    _root = new QuadTreeNode(_left, _top, _right, _bottom);
    for (Geo::Geometry *object : objects)
    {
        _root->append(object);
    }
}

void Collision::QuadTree::append(Geo::Geometry *object)
{
    if (std::find(_objects.begin(), _objects.end(), object) != _objects.end())
    {
        return;
    }
    _objects.push_back(object);

    const Geo::AABBRect rect(object->bounding_rect());
    if (rect.left() >= _left && rect.right() <= _right && rect.top() <= _top && rect.bottom() >= _bottom)
    {
        _root->append(object);
    }
    else
    {
        Collision::QuadTreeNode *node;
        double left_sapce = _left - rect.left(), top_space = rect.top() > _top,
            right_space = rect.right() - _right, bottom_sapce = _bottom - rect.bottom();
        while (left_sapce > 0 || top_space > 0 || right_space > 0 || bottom_sapce > 0)
        {
            node = _root;
            if (left_sapce > right_space)
            {
                _left -= (_right - _left);
                if (bottom_sapce > top_space)
                {
                    _bottom -= (_top - _bottom);
                    _root = new QuadTreeNode(_left, _top, _right, _bottom);
                    _root->append_node(0, new QuadTreeNode(_left, _top, (_left + _right) / 2, (_top + _bottom) / 2));
                    _root->append_node(1, node);
                    _root->append_node(2, new QuadTreeNode(_left, (_top + _bottom) / 2, (_left + _right) / 2, _bottom));
                    _root->append_node(3, new QuadTreeNode((_left + _right) / 2, (_top + _bottom) / 2, _right, _bottom));
                }
                else
                {
                    _top += (_top - _bottom);
                    _root = new QuadTreeNode(_left, _top, _right, _bottom);
                    _root->append_node(0, new QuadTreeNode(_left, _top, (_left + _right) / 2, (_top + _bottom) / 2));
                    _root->append_node(1, new QuadTreeNode((_left + _right) / 2, _top, _right, (_top + _bottom) / 2));
                    _root->append_node(2, new QuadTreeNode(_left, (_top + _bottom) / 2, (_left + _right) / 2, _bottom));
                    _root->append_node(3, node);
                }
            }
            else
            {
                _right += (_right - _left);
                if (bottom_sapce > top_space)
                {
                    _bottom -= (_top - _bottom);
                    _root = new QuadTreeNode(_left, _top, _right, _bottom);
                    _root->append_node(0, node);
                    _root->append_node(1, new QuadTreeNode((_left + _right) / 2, _top, _right, (_top + _bottom) / 2));
                    _root->append_node(2, new QuadTreeNode(_left, (_top + _bottom) / 2, (_left + _right) / 2, _bottom));
                    _root->append_node(3, new QuadTreeNode((_left + _right) / 2, (_top + _bottom) / 2, _right, _bottom));
                }
                else
                {
                    _top += (_top - _bottom);
                    _root = new QuadTreeNode(_left, _top, _right, _bottom);
                    _root->append_node(0, new QuadTreeNode(_left, _top, (_left + _right) / 2, (_top + _bottom) / 2));
                    _root->append_node(1, new QuadTreeNode((_left + _right) / 2, _top, _right, (_top + _bottom) / 2));
                    _root->append_node(2, node);
                    _root->append_node(3, new QuadTreeNode((_left + _right) / 2, (_top + _bottom) / 2, _right, _bottom));
                }
            }

            left_sapce = _left - rect.left();
            top_space = rect.top() > _top;
            right_space = rect.right() - _right;
            bottom_sapce = _bottom - rect.bottom();
        }
    }
}

void Collision::QuadTree::remove(Geo::Geometry *object)
{
    if (std::find(_objects.begin(), _objects.end(), object) != _objects.end())
    {
        _root->remove(object);
    }
}

void Collision::QuadTree::update(Geo::Geometry *object)
{
    if (std::find(_objects.begin(), _objects.end(), object) != _objects.end())
    {
        _root->remove(object);
        _root->append(object);
    }
}

void Collision::QuadTree::update()
{
    delete _root;
    _root = nullptr;
    build(_objects, _rects);
}

bool Collision::QuadTree::has(Geo::Geometry *object) const
{
    return std::find(_objects.begin(), _objects.end(), object) != _objects.end();
}

void Collision::QuadTree::clear()
{
    _rects.clear();
    _objects.clear();
    delete _root;
    _root = nullptr;
}

bool Collision::QuadTree::select(const Geo::Point &pos, std::vector<Geo::Geometry *> &objects) const
{
    return _root != nullptr && _root->select(pos, objects);
}

bool Collision::QuadTree::select(const Geo::AABBRect &rect, std::vector<Geo::Geometry *> &objects) const
{
    return _root != nullptr && _root->select(rect, objects);
}

bool Collision::QuadTree::find_collision_objects(const Geo::Geometry *object, std::vector<Geo::Geometry *> &objects) const
{
    return _root != nullptr && _root->find_collision_objects(object, objects);
}

bool Collision::QuadTree::find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs) const
{
    return _root != nullptr && _root->find_collision_pairs(pairs);
}


void Collision::gjk_furthest_point(const Geo::Polygon &polygon, const Geo::Point &start, const Geo::Point &end, Geo::Point &result)
{
    result = polygon.front();
    Geo::Point point;
    Geo::foot_point(start, end, polygon.front(), point, true);
    const Geo::Vector vec = end - start;
    double value, max_value = vec * (point - start);
    for (size_t i = 1, count = polygon.size() - 1; i < count; ++i)
    {
        Geo::foot_point(start, end, polygon[i], point, true);
        value = vec * (point - start);
        if (value > max_value)
        {
            max_value = value;
            result = polygon[i];
        }
    }
}

void Collision::gjk_furthest_point(const Geo::AABBRect &rect, const Geo::Point &start, const Geo::Point &end, Geo::Point &result)
{
    result = rect[0];
    Geo::Point point;
    Geo::foot_point(start, end, rect[0], point, true);
    const Geo::Vector vec = end - start;
    double value, max_value = vec * (point - start);
    for (size_t i = 1; i < 4; ++i)
    {
        Geo::foot_point(start, end, rect[i], point, true);
        value = vec * (point - start);
        if (value > max_value)
        {
            max_value = value;
            result = rect[i];
        }
    }
}

bool Collision::gjk(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1)
{
    Geo::Point start = polygon0.average_point(), end = polygon1.average_point();
    Geo::Point point0, point1;
    Geo::Triangle triangle, last_triangle;
    double distance[3];

    Collision::gjk_furthest_point(polygon0, start, end, point0);
    Collision::gjk_furthest_point(polygon1, end, start, point1);
    triangle[0] = point0 - point1;
    Collision::gjk_furthest_point(polygon0, end, start, point0);
    Collision::gjk_furthest_point(polygon1, start, end, point1);
    triangle[1] = point0 - point1;
    end.clear();
    Geo::foot_point(triangle[0], triangle[1], end, start, true);

    while (true)
    {
        Collision::gjk_furthest_point(polygon0, start, end, point0);
        Collision::gjk_furthest_point(polygon1, end, start, point1);
        triangle[2] = point0 - point1;

        if (triangle[2] * (point0 - point1) < 0)
        {
            return false;
        }

        if (Geo::is_inside(end, triangle, true))
        {
            return true;
        }
        else if (last_triangle[0] == triangle[0] && last_triangle[1] == triangle[1]
            && last_triangle[2] == triangle[2])
        {
            return false;
        }

        end.clear();
        distance[0] = Geo::distance(end, triangle[0], triangle[1], true);
        distance[1] = Geo::distance(end, triangle[1], triangle[2], true);
        distance[2] = Geo::distance(end, triangle[0], triangle[2], true);
        last_triangle = triangle;
        if (distance[0] <= distance[1])
        {
            if (distance[0] > distance[2])
            {
                triangle[1] = triangle[2];
            }
        }
        else
        {
            if (distance[1] <= distance[2])
            {
                triangle[0] = triangle[2];
            }
            else
            {
                triangle[1] = triangle[2];
            }
        }
        Geo::foot_point(triangle[0], triangle[1], end, start, true);
    }
}

bool Collision::gjk(const Geo::AABBRect &rect, const Geo::Polygon &polygon)
{
    Geo::Point start = rect.center(), end = polygon.average_point();
    Geo::Point point0, point1;
    Geo::Triangle triangle, last_triangle;
    double distance[3];

    Collision::gjk_furthest_point(rect, start, end, point0);
    Collision::gjk_furthest_point(polygon, end, start, point1);
    triangle[0] = point0 - point1;
    Collision::gjk_furthest_point(rect, end, start, point0);
    Collision::gjk_furthest_point(polygon, start, end, point1);
    triangle[1] = point0 - point1;
    end.clear();
    Geo::foot_point(triangle[0], triangle[1], end, start, true);

    while (true)
    {
        Collision::gjk_furthest_point(rect, start, end, point0);
        Collision::gjk_furthest_point(polygon, end, start, point1);
        triangle[2] = point0 - point1;

        if (triangle[2] * (point0 - point1) < 0)
        {
            return false;
        }

        if (Geo::is_inside(end, triangle, true))
        {
            return true;
        }
        else if (last_triangle[0] == triangle[0] && last_triangle[1] == triangle[1]
            && last_triangle[2] == triangle[2])
        {
            return false;
        }

        distance[0] = Geo::distance(end, triangle[0], triangle[1], true);
        distance[1] = Geo::distance(end, triangle[1], triangle[2], true);
        distance[2] = Geo::distance(end, triangle[0], triangle[2], true);
        last_triangle = triangle;
        if (distance[0] <= distance[1])
        {
            if (distance[0] > distance[2])
            {
                triangle[1] = triangle[2];
            }
        }
        else
        {
            if (distance[1] <= distance[2])
            {
                triangle[0] = triangle[2];
            }
            else
            {
                triangle[1] = triangle[2];
            }
        }
        Geo::foot_point(triangle[0], triangle[1], end, start, true);
    }
}

double Collision::epa(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, Geo::Vector &vec)
{
    Geo::Point start = polygon0.average_point(), end = polygon1.average_point();
    Geo::Point point0, point1, point2;
    Geo::Triangle triangle, last_triangle;
    double distance[3] = {DBL_MAX, DBL_MAX, DBL_MAX};

    Collision::gjk_furthest_point(polygon0, start, end, point0);
    Collision::gjk_furthest_point(polygon1, end, start, point1);
    triangle[0] = point0 - point1;
    Collision::gjk_furthest_point(polygon0, end, start, point0);
    Collision::gjk_furthest_point(polygon1, start, end, point1);
    triangle[1] = point0 - point1;
    end.clear();
    Geo::foot_point(triangle[0], triangle[1], end, start, true);

    while (true)
    {
        Collision::gjk_furthest_point(polygon0, start, end, point0);
        Collision::gjk_furthest_point(polygon1, end, start, point1);
        triangle[2] = point0 - point1;

        if (triangle[2] * (point0 - point1) < 0)
        {
            return -1;
        }

        if (Geo::is_inside(end, triangle, true))
        {
            break;
        }
        else if (last_triangle[0] == triangle[0] && last_triangle[1] == triangle[1]
            && last_triangle[2] == triangle[2])
        {
            return -1;
        }

        distance[0] = Geo::distance_square(end, triangle[0], triangle[1], true);
        distance[1] = Geo::distance_square(end, triangle[1], triangle[2], true);
        distance[2] = Geo::distance_square(end, triangle[0], triangle[2], true);
        last_triangle = triangle;
        if (distance[0] <= distance[1])
        {
            if (distance[0] > distance[2])
            {
                triangle[1] = triangle[2];
            }
        }
        else
        {
            if (distance[1] <= distance[2])
            {
                triangle[0] = triangle[2];
            }
            else
            {
                triangle[1] = triangle[2];
            }
        }
        Geo::foot_point(triangle[0], triangle[1], end, start, true);
    }

    std::vector<Geo::Point> points;
    points.emplace_back(triangle[0]);
    points.emplace_back(triangle[1]);
    points.emplace_back(triangle[2]);

    size_t index;
    while (true)
    {
        distance[0] = Geo::distance_square(end, points.front(), points.back(), true);
        index = points.size();
        for (size_t i = 1, count = points.size(); i < count; ++i)
        {
            distance[1] = Geo::distance_square(end, points[i - 1], points[i], true);
            if (distance[1] < distance[0])
            {
                distance[0] = distance[1];
                index = i;
            }
        }

        Geo::foot_point(points[index - 1], points[index % points.size()], end, vec, true);
        Collision::gjk_furthest_point(polygon0, end, vec, point0);
        Collision::gjk_furthest_point(polygon1, vec, end, point1);
        point2 = point0 - point1;
        if (vec * point2 <= 0 || points[index - 1] == point2 || points[index % points.size()] == point2)
        {
            return vec.length();
        }
        else
        {
            if (index < points.size())
            {
                points.insert(points.begin() + index, point2);
            }
            else
            {
                points.emplace_back(point2);
            }
        }
    }
}

double Collision::epa(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, Geo::Point &start0, Geo::Point &end0)
{
    Geo::Point start = polygon0.average_point(), end = polygon1.average_point();
    Geo::Point point0, point1, point2;
    Geo::Triangle triangle, last_triangle;
    double distance[3] = {DBL_MAX, DBL_MAX, DBL_MAX};

    std::vector<std::tuple<Geo::Point, Geo::Point>> point_pairs;

    Collision::gjk_furthest_point(polygon0, start, end, point0);
    Collision::gjk_furthest_point(polygon1, end, start, point1);
    point_pairs.emplace_back(point0, point1);
    triangle[0] = point0 - point1;
    Collision::gjk_furthest_point(polygon0, end, start, point0);
    Collision::gjk_furthest_point(polygon1, start, end, point1);
    point_pairs.emplace_back(point0, point1);
    triangle[1] = point0 - point1;
    end.clear();
    Geo::foot_point(triangle[0], triangle[1], end, start, true);

    while (true)
    {
        Collision::gjk_furthest_point(polygon0, start, end, point0);
        Collision::gjk_furthest_point(polygon1, end, start, point1);
        point_pairs.emplace_back(point0, point1);
        triangle[2] = point0 - point1;

        if (triangle[2] * (point0 - point1) < 0)
        {
            return -1;
        }

        if (Geo::is_inside(end, triangle, true))
        {
            break;
        }
        else if (last_triangle[0] == triangle[0] && last_triangle[1] == triangle[1]
            && last_triangle[2] == triangle[2])
        {
            return -1;
        }

        distance[0] = Geo::distance_square(end, triangle[0], triangle[1], true);
        distance[1] = Geo::distance_square(end, triangle[1], triangle[2], true);
        distance[2] = Geo::distance_square(end, triangle[0], triangle[2], true);
        last_triangle = triangle;
        if (distance[0] <= distance[1])
        {
            if (distance[0] > distance[2])
            {
                triangle[1] = triangle[2];
                point_pairs[1] = point_pairs[2];
            }
        }
        else
        {
            if (distance[1] <= distance[2])
            {
                triangle[0] = triangle[2];
                point_pairs[0] = point_pairs[2];
            }
            else
            {
                triangle[1] = triangle[2];
                point_pairs[1] = point_pairs[2];
            }
        }
        Geo::foot_point(triangle[0], triangle[1], end, start, true);
        point_pairs.pop_back();
    }

    std::vector<Geo::Point> points;
    points.emplace_back(triangle[0]);
    points.emplace_back(triangle[1]);
    points.emplace_back(triangle[2]);

    size_t index;
    while (true)
    {
        distance[0] = Geo::distance_square(end, points.front(), points.back(), true);
        index = points.size();
        for (size_t i = 1, count = points.size(); i < count; ++i)
        {
            distance[1] = Geo::distance_square(end, points[i - 1], points[i], true);
            if (distance[1] < distance[0])
            {
                distance[0] = distance[1];
                index = i;
            }
        }

        Geo::foot_point(points[index - 1], points[index % points.size()], end, start0, true);
        Collision::gjk_furthest_point(polygon0, end, start0, point0);
        Collision::gjk_furthest_point(polygon1, start0, end, point1);
        point2 = point0 - point1;
        if (start0 * point2 <= 0 || points[index - 1] == point2 || points[index % points.size()] == point2)
        {
            return Geo::distance(std::get<0>(point_pairs[index - 1]), std::get<0>(point_pairs[index % points.size()]),
                std::get<1>(point_pairs[index - 1]), std::get<1>(point_pairs[index % points.size()]), start0, end0);
        }
        else
        {
            if (index < points.size())
            {
                points.insert(points.begin() + index, point2);
                point_pairs.insert(point_pairs.begin() + index, std::make_tuple(point0, point1));
            }
            else
            {
                points.emplace_back(point2);
                point_pairs.emplace_back(point0, point1);
            }
        }
    }
}

