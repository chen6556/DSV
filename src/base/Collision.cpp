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

void Collision::GridNode::set_rect(const Geo::AABBRect &rect)
{
    _rect = rect;
}

bool Collision::GridNode::append(Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::TRIANGLE:
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

bool Collision::GridNode::remove(Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
    case Geo::Type::AABBRECT:
    case Geo::Type::POLYGON:
    case Geo::Type::TRIANGLE:
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
        case Geo::Type::TRIANGLE:
            if (Geo::is_inside(pos, *static_cast<Geo::Triangle *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::CIRCLECONTAINER:
            if (Geo::is_inside(pos, *static_cast<Geo::Circle *>(object)))
            {
                _objects.push_back(object);
            }
            break;
        case Geo::Type::LINE:
            if (Geo::is_inside(pos, *static_cast<Geo::Line *>(object)))
            {
                _objects.push_back(object);
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_inside(pos, static_cast<Geo::Bezier *>(object)->shape()))
            {
                _objects.push_back(object);
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
            if (Geo::is_intersected(pos, *static_cast<Geo::AABBRect *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::POLYGON:
        case Geo::Type::CONTAINER:
            if (Geo::is_intersected(pos, *static_cast<Geo::Polygon *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::TRIANGLE:
            if (Geo::is_intersected(pos, *static_cast<Geo::Triangle *>(object)))
            {
                objects.push_back(object);
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::CIRCLECONTAINER:
            if (Geo::is_intersected(pos, *static_cast<Geo::Circle *>(object)))
            {
                _objects.push_back(object);
            }
            break;
        case Geo::Type::LINE:
            if (Geo::is_intersected(pos, *static_cast<Geo::Line *>(object)))
            {
                _objects.push_back(object);
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_intersected(pos, static_cast<Geo::Bezier *>(object)->shape()))
            {
                _objects.push_back(object);
            }
            break;
        default:
            break;
        }
    }
    return objects.size() > size;
}

bool Collision::GridNode::find_collision_objects(std::vector<Geo::Geometry *> &objects) const
{
    const size_t size = objects.size();
    // TODO
    return objects.size() > size;
}

bool Collision::GridNode::find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs) const
{
    const size_t size = pairs.size();
    // TODO
    return pairs.size() > size;
}