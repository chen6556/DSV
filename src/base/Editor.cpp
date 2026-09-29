#include <thread>
#include <algorithm>
#include <unordered_map>
#include "base/Editor.hpp"
#include "base/Math.hpp"
#include "io/GlobalSetting.hpp"
#include "io/SHXReader.hpp"
#include "io/TextEncoding.hpp"


Editor::~Editor()
{
    delete _graph;
    _graph = nullptr;
    _backup.clear();
    for (Geo::DObject *geo : _paste_table)
    {
        delete geo;
    }
}

void Editor::init()
{
    if (_graph != nullptr)
    {
        if (_graph->container_groups().empty())
        {
            _graph->append_group();
        }
        _view_tree.build(_graph);
        _backup.clear();
        _backup.set_graph(_graph);
    }
}

void Editor::load_graph(Graph *graph, const QString &path)
{
    if (graph != nullptr)
    {
        _graph = graph;
        _current_group = 0;
        init();
        _file_path = path;
    }
    else
    {
        _graph = nullptr;
        _view_tree.clear();
    }
}

void Editor::load_graph(Graph *graph)
{
    if (graph != nullptr)
    {
        _graph = graph;
        _current_group = 0;
        init();
        _file_path.clear();
    }
    else
    {
        _graph = nullptr;
        _view_tree.clear();
    }
}

void Editor::delete_graph()
{
    if (_graph != nullptr)
    {
        delete _graph;
        _graph = nullptr;
        _current_group = 0;
        _file_path.clear();
        _backup.clear();
    }
}

const QString &Editor::path() const
{
    return _file_path;
}

void Editor::set_path(const QString &path)
{
    _file_path = path;
}

Graph *Editor::graph()
{
    return _graph;
}

const Graph *Editor::graph() const
{
    return _graph;
}

void Editor::refresh_visible_objects(const Geo::AABBRect &rect)
{
    return _view_tree.find_visible_objects(rect);
}

const std::vector<Geo::DObject *> &Editor::visible_objects() const
{
    return _view_tree.visible_objects();
}

std::vector<Geo::Point> &Editor::point_cache()
{
    return _point_cache;
}

const std::vector<Geo::Point> &Editor::point_cache() const
{
    return _point_cache;
}

const size_t Editor::current_group() const
{
    return _current_group;
}

void Editor::set_current_group(const size_t index)
{
    if (index < _graph->container_groups().size() && index != _current_group)
    {
        _current_group = index;
        for (ContainerGroup &group : *_graph)
        {
            for (Geo::DObject *object : group)
            {
                object->is_selected = false;
            }
        }
    }
}

const size_t Editor::groups_count() const
{
    return _graph->container_groups().size();
}

void Editor::set_view_ratio(const double value)
{
    _view_ratio = value;
}

Geo::DObject *Editor::select(const Geo::Point &point, const bool reset_others, const bool visible_only)
{
    if (_graph == nullptr || _graph->empty())
    {
        return nullptr;
    }
    if (reset_others)
    {
        reset_selected_mark();
    }

    const double catch_distance = 2 / _view_ratio;
    Text *t = nullptr;
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    Geo::Ellipse *ellipse = nullptr;
    Geo::Polyline *p = nullptr;
    Geo::CubicBezier *b = nullptr;
    Geo::BSpline *bs = nullptr;
    Geo::Arc *arc = nullptr;
    Combination *cb = nullptr;

    std::vector<Geo::DObject *> objects;
    if (visible_only)
    {
        std::vector<Geo::DObject *> current_group_objects(_graph->container_group(_current_group).begin(),
                                                          _graph->container_group(_current_group).end());
        std::sort(current_group_objects.begin(), current_group_objects.end());
        std::vector<Geo::DObject *> visible_objects(_view_tree.visible_objects());
        std::set_intersection(visible_objects.begin(), visible_objects.end(), current_group_objects.begin(), current_group_objects.end(),
                              std::back_inserter(objects));
        objects.erase(std::remove_if(objects.begin(), objects.end(), [&](Geo::DObject *object)
                                     { return object->type() != Geo::Type::BEZIER && object->type() != Geo::Type::BSPLINE; }),
                      objects.end());
        Geo::AABBRect rect;
        rect.left = point.x - catch_distance - 1;
        rect.right = point.x + catch_distance + 1;
        rect.bottom = point.y - catch_distance - 1;
        rect.top = point.y + catch_distance + 1;
        visible_objects.clear();
        _view_tree.find_visible_objects(rect, visible_objects);
        std::set_intersection(visible_objects.begin(), visible_objects.end(), current_group_objects.begin(), current_group_objects.end(),
                              std::back_inserter(objects));
        std::unordered_map<const Geo::DObject *, size_t> orders;
        for (Geo::DObject *object : objects)
        {
            orders.insert_or_assign(object, std::distance(_graph->container_group(_current_group).begin(),
                                                          std::find(_graph->container_group(_current_group).begin(),
                                                                    _graph->container_group(_current_group).end(), object)));
        }
        std::sort(objects.begin(), objects.end(), [&](const Geo::DObject *a, const Geo::DObject *b) { return orders[a] > orders[b]; });
    }
    else
    {
        objects.assign(_graph->container_group(_current_group).rbegin(), _graph->container_group(_current_group).rend());
    }

    for (Geo::DObject *it : objects)
    {
        switch (it->type())
        {
        case Geo::Type::TEXT:
            t = static_cast<Text *>(it);
            if (Geo::is_inside(point, t->shape(0), t->shape(1), t->shape(2), t->shape(3), false))
            {
                t->is_selected = true;
                return t;
            }
            t = nullptr;
            break;
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(it);
            for (size_t i = 1, count = polygon->size(); i < count; ++i)
            {
                if (Geo::distance_square(point, (*polygon)[i], (*polygon)[i - 1]) <= catch_distance * catch_distance)
                {
                    polygon->is_selected = true;
                    return polygon;
                }
            }
            polygon = nullptr;
            break;
        case Geo::Type::CIRCLE:
            circle = static_cast<Geo::Circle *>(it);
            if (std::abs(Geo::distance(point, *circle) - circle->radius) <= catch_distance ||
                Geo::distance(point, *circle) <= catch_distance)
            {
                circle->is_selected = true;
                return circle;
            }
            circle = nullptr;
            break;
        case Geo::Type::ELLIPSE:
            ellipse = static_cast<Geo::Ellipse *>(it);
            if (ellipse->is_arc())
            {
                if (Geo::distance(point, *ellipse) <= catch_distance)
                {
                    ellipse->is_selected = true;
                    return ellipse;
                }
            }
            else
            {
                if (std::abs(Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point) -
                             std::max(ellipse->lengtha(), ellipse->lengthb()) * 2) <= catch_distance ||
                    Geo::distance_square(point, (ellipse->c0() + ellipse->c1()) / 2) <= catch_distance)
                {
                    ellipse->is_selected = true;
                    return ellipse;
                }
            }
            ellipse = nullptr;
            break;
        case Geo::Type::COMBINATION:
            cb = static_cast<Combination *>(it);
            if (Geo::is_inside(point, cb->border(), true))
            {
                for (Geo::DObject *item : *cb)
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        t = static_cast<Text *>(item);
                        if (Geo::is_inside(point, t->shape(0), t->shape(1), t->shape(2), t->shape(3), false))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        t = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        polygon = static_cast<Geo::Polygon *>(item);
                        for (size_t i = 1, count = polygon->size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, (*polygon)[i], (*polygon)[i - 1]) <= catch_distance * catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        polygon = nullptr;
                        break;
                    case Geo::Type::CIRCLE:
                        circle = static_cast<Geo::Circle *>(item);
                        if (std::abs(Geo::distance(point, *circle) - circle->radius) <= catch_distance ||
                            Geo::distance(point, *circle) <= catch_distance)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<Geo::Ellipse *>(item);
                        if (std::abs(Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point) -
                                     std::max(ellipse->lengtha(), ellipse->lengthb()) * 2) <= catch_distance ||
                            Geo::distance(point, (ellipse->c0() + ellipse->c1()) / 2) <= catch_distance)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        ellipse = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        p = static_cast<Geo::Polyline *>(item);
                        for (size_t i = 1, count = p->size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        p = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        b = static_cast<Geo::CubicBezier *>(item);
                        for (size_t i = 1, count = b->shape().size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        b = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bs = static_cast<Geo::BSpline *>(item);
                        for (size_t i = 1, count = bs->shape().size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, bs->shape()[i - 1], bs->shape()[i]) <= catch_distance * catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        bs = nullptr;
                        break;
                    case Geo::Type::ARC:
                        arc = static_cast<Geo::Arc *>(item);
                        if (Geo::distance(point, *arc) <= catch_distance)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        arc = nullptr;
                        break;
                    case Geo::Type::POINT:
                        if (Geo::Point *pt = static_cast<Geo::Point *>(item);
                            Geo::distance_square(point, *pt) <= catch_distance * catch_distance)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            cb = nullptr;
            break;
        case Geo::Type::POLYLINE:
            p = static_cast<Geo::Polyline *>(it);
            for (size_t i = 1, count = p->size(); i < count; ++i)
            {
                if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
                {
                    p->is_selected = true;
                    return p;
                }
            }
            p = nullptr;
            break;
        case Geo::Type::BEZIER:
            b = static_cast<Geo::CubicBezier *>(it);
            if (b->is_selected)
            {
                for (const Geo::Point &inner_point : b->control_points)
                {
                    if (Geo::distance_square(point, inner_point) <= catch_distance * catch_distance * 2.25)
                    {
                        return b;
                    }
                }
            }
            for (size_t i = 1, count = b->shape().size(); i < count; ++i)
            {
                if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
                {
                    b->is_selected = true;
                    return b;
                }
            }
            b = nullptr;
            break;
        case Geo::Type::BSPLINE:
            bs = static_cast<Geo::BSpline *>(it);
            if (bs->is_selected)
            {
                for (const Geo::Point &inner_point : (bs->controls_model ? bs->control_points : bs->path_points))
                {
                    if (Geo::distance_square(point, inner_point) <= catch_distance * catch_distance * 2.25)
                    {
                        return bs;
                    }
                }
            }
            for (size_t i = 1, count = bs->shape().size(); i < count; ++i)
            {
                if (Geo::distance_square(point, bs->shape()[i - 1], bs->shape()[i]) <= catch_distance * catch_distance)
                {
                    bs->is_selected = true;
                    return bs;
                }
            }
            bs = nullptr;
            break;
        case Geo::Type::ARC:
            arc = static_cast<Geo::Arc *>(it);
            if (Geo::distance(point, *arc) <= catch_distance)
            {
                arc->is_selected = true;
                return arc;
            }
            arc = nullptr;
            break;
        case Geo::Type::POINT:
            if (Geo::Point *pt = static_cast<Geo::Point *>(it); Geo::distance_square(point, *pt) <= catch_distance * catch_distance)
            {
                pt->is_selected = true;
                return pt;
            }
            break;
        case Geo::Type::DIMENSION:
            if (static_cast<Dim::Dimension *>(it)->select(point, catch_distance))
            {
                it->is_selected = true;
                return it;
            }
            break;
        default:
            break;
        }
    }

    return nullptr;
}

Geo::DObject *Editor::select(const double x, const double y, const bool reset_others, const bool visible_only)
{
    return select(Geo::Point(x, y), reset_others, visible_only);
}

std::tuple<Geo::DObject *, bool> Editor::select_with_state(const Geo::Point &point, const bool reset_others)
{
    if (_graph == nullptr || _graph->empty())
    {
        return std::make_tuple(nullptr, false);
    }
    if (reset_others)
    {
        reset_selected_mark();
    }

    const double catch_distance = 2 / _view_ratio;
    Text *t = nullptr;
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    Geo::Ellipse *ellipse = nullptr;
    Geo::Polyline *p = nullptr;
    Geo::CubicBezier *b = nullptr;
    Geo::BSpline *bs = nullptr;
    Combination *cb = nullptr;
    for (std::vector<Geo::DObject *>::reverse_iterator it = _graph->container_group(_current_group).rbegin(),
                                                       end = _graph->container_group(_current_group).rend();
         it != end; ++it)
    {
        switch ((*it)->type())
        {
        case Geo::Type::TEXT:
            t = static_cast<Text *>(*it);
            if (Geo::is_inside(point, t->shape(0), t->shape(1), t->shape(2), t->shape(3), false))
            {
                bool state = t->is_selected;
                t->is_selected = true;
                return std::make_tuple(t, state);
            }
            t = nullptr;
            break;
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(*it);
            if (Geo::is_inside(point, *polygon, true))
            {
                bool state = polygon->is_selected;
                polygon->is_selected = true;
                return std::make_tuple(polygon, state);
            }
            for (size_t i = 1, count = polygon->size(); i < count; ++i)
            {
                if (Geo::distance_square(point, (*polygon)[i], (*polygon)[i - 1]) <= catch_distance * catch_distance)
                {
                    bool state = polygon->is_selected;
                    polygon->is_selected = true;
                    return std::make_tuple(polygon, state);
                }
            }
            polygon = nullptr;
            break;
        case Geo::Type::CIRCLE:
            circle = static_cast<Geo::Circle *>(*it);
            if (Geo::distance_square(point, *circle) <= std::pow(catch_distance + circle->radius, 2))
            {
                bool state = circle->is_selected;
                circle->is_selected = true;
                return std::make_tuple(circle, state);
            }
            circle = nullptr;
            break;
        case Geo::Type::ELLIPSE:
            ellipse = static_cast<Geo::Ellipse *>(*it);
            if (Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point) <=
                catch_distance + std::max(ellipse->lengtha(), ellipse->lengthb()) * 2)
            {
                bool state = ellipse->is_selected;
                ellipse->is_selected = true;
                return std::make_tuple(ellipse, state);
            }
            ellipse = nullptr;
            break;
        case Geo::Type::COMBINATION:
            cb = static_cast<Combination *>(*it);
            if (Geo::is_inside(point, cb->border(), true))
            {
                for (Geo::DObject *item : *cb)
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        t = static_cast<Text *>(item);
                        if (Geo::is_inside(point, t->shape(0), t->shape(1), t->shape(2), t->shape(3), false))
                        {
                            bool state = cb->is_selected;
                            cb->is_selected = true;
                            return std::make_tuple(cb, state);
                        }
                        t = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        if (Geo::is_inside(point, *static_cast<Geo::Polygon *>(item), true))
                        {
                            bool state = cb->is_selected;
                            cb->is_selected = true;
                            return std::make_tuple(cb, state);
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_inside(point, *static_cast<Geo::Circle *>(item), true))
                        {
                            bool state = cb->is_selected;
                            cb->is_selected = true;
                            return std::make_tuple(cb, state);
                        }
                        break;
                    case Geo::Type::ELLIPSE:
                        if (Geo::is_inside(point, *static_cast<Geo::Ellipse *>(item), true))
                        {
                            bool state = cb->is_selected;
                            cb->is_selected = true;
                            return std::make_tuple(cb, state);
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        p = static_cast<Geo::Polyline *>(item);
                        for (size_t i = 1, count = p->size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
                            {
                                bool state = cb->is_selected;
                                cb->is_selected = true;
                                return std::make_tuple(cb, state);
                            }
                        }
                        p = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        b = static_cast<Geo::CubicBezier *>(item);
                        for (size_t i = 1, count = b->shape().size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
                            {
                                bool state = cb->is_selected;
                                cb->is_selected = true;
                                return std::make_tuple(cb, state);
                            }
                        }
                        b = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bs = static_cast<Geo::BSpline *>(item);
                        for (size_t i = 1, count = bs->shape().size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, bs->shape()[i - 1], bs->shape()[i]) <= catch_distance * catch_distance)
                            {
                                bool state = cb->is_selected;
                                cb->is_selected = true;
                                return std::make_tuple(cb, state);
                            }
                        }
                        bs = nullptr;
                        break;
                    default:
                        break;
                    }
                }
            }
            cb = nullptr;
            break;
        case Geo::Type::POLYLINE:
            p = static_cast<Geo::Polyline *>(*it);
            for (size_t i = 1, count = p->size(); i < count; ++i)
            {
                if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
                {
                    bool state = p->is_selected;
                    p->is_selected = true;
                    return std::make_tuple(p, state);
                }
            }
            p = nullptr;
            break;
        case Geo::Type::BEZIER:
            b = static_cast<Geo::CubicBezier *>(*it);
            if (b->is_selected)
            {
                for (const Geo::Point &inner_point : b->control_points)
                {
                    if (Geo::distance_square(point, inner_point) <= catch_distance * catch_distance * 2.25)
                    {
                        return std::make_tuple(b, true);
                    }
                }
            }
            for (size_t i = 1, count = b->shape().size(); i < count; ++i)
            {
                if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
                {
                    bool state = b->is_selected;
                    b->is_selected = true;
                    return std::make_tuple(b, state);
                }
            }
            b = nullptr;
            break;
        case Geo::Type::BSPLINE:
            bs = static_cast<Geo::BSpline *>(*it);
            if (bs->is_selected)
            {
                for (const Geo::Point &inner_point : bs->path_points)
                {
                    if (Geo::distance_square(point, inner_point) <= catch_distance * catch_distance * 2.25)
                    {
                        return std::make_tuple(bs, true);
                    }
                }
            }
            for (size_t i = 1, count = bs->shape().size(); i < count; ++i)
            {
                if (Geo::distance_square(point, bs->shape()[i - 1], bs->shape()[i]) <= catch_distance * catch_distance)
                {
                    bool state = bs->is_selected;
                    bs->is_selected = true;
                    return std::make_tuple(bs, state);
                }
            }
            bs = nullptr;
            break;
        default:
            break;
        }
    }

    return std::make_tuple(nullptr, false);
}

std::vector<Geo::DObject *> Editor::select(const Geo::AABBRect &rect, const bool reset_others, const bool visible_only)
{
    std::vector<Geo::DObject *> result;
    if (_graph == nullptr || _graph->empty())
    {
        return result;
    }

    if (reset_others)
    {
        reset_selected_mark();
    }

    std::vector<Geo::DObject *> objects;
    if (visible_only)
    {
        std::vector<Geo::DObject *> current_group_objects(_graph->container_group(_current_group).begin(),
                                                          _graph->container_group(_current_group).end());
        std::sort(current_group_objects.begin(), current_group_objects.end());
        std::set_intersection(_view_tree.visible_objects().begin(), _view_tree.visible_objects().end(), current_group_objects.begin(),
                              current_group_objects.end(), std::back_inserter(objects));
    }
    else
    {
        objects.assign(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end());
    }

    for (Geo::DObject *container : objects)
    {
        if (container->is_selected)
        {
            result.push_back(container);
            continue;
        }
        switch (container->type())
        {
        case Geo::Type::TEXT:
            if (const Text *text = static_cast<const Text *>(container);
                Geo::is_intersected(rect, text->shape(0), text->shape(1), text->shape(2), text->shape(3)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::POLYGON:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polygon *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::CIRCLE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Circle *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::ELLIPSE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Ellipse *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::COMBINATION:
            if (Geo::is_intersected(rect, static_cast<Combination *>(container)->border(), true))
            {
                bool end = false;
                for (Geo::DObject *item : *static_cast<Combination *>(container))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (const Text *text = static_cast<const Text *>(item);
                            Geo::is_intersected(rect, text->shape(0), text->shape(1), text->shape(2), text->shape(3)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::POLYGON:
                        if (Geo::is_intersected(rect, *static_cast<Geo::Polygon *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_intersected(rect, *static_cast<Geo::Circle *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::ELLIPSE:
                        if (Geo::is_intersected(rect, *static_cast<Geo::Ellipse *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        if (Geo::is_intersected(rect, *static_cast<Geo::Polyline *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(rect, static_cast<Geo::CubicBezier *>(item)->shape()))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::BSPLINE:
                        if (Geo::is_intersected(rect, static_cast<Geo::BSpline *>(item)->shape()))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::ARC:
                        if (Geo::is_intersected(rect, *static_cast<Geo::Arc *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(item), rect, true))
                        {
                            end = true;
                        }
                        break;
                    default:
                        break;
                    }
                    if (end)
                    {
                        break;
                    }
                }
                if (end)
                {
                    container->is_selected = true;
                    result.push_back(container);
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polyline *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_intersected(rect, static_cast<Geo::CubicBezier *>(container)->shape()))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::BSPLINE:
            if (Geo::is_intersected(rect, static_cast<Geo::BSpline *>(container)->shape()))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::ARC:
            if (Geo::is_intersected(rect, *static_cast<Geo::Arc *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::POINT:
            if (Geo::is_inside(*static_cast<Geo::Point *>(container), rect, true))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        case Geo::Type::DIMENSION:
            if (static_cast<const Dim::Dimension *>(container)->select(rect))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            break;
        default:
            break;
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Geo::DObject *> Editor::selected(const bool visible_only) const
{
    std::vector<Geo::DObject *> result;
    if (_graph == nullptr)
    {
        return result;
    }
    if (visible_only)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (!group.visible())
            {
                continue;
            }
            for (Geo::DObject *object : group)
            {
                if (object->is_selected)
                {
                    result.push_back(object);
                }
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::DObject *object : group)
            {
                if (object->is_selected)
                {
                    result.push_back(object);
                }
            }
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

const size_t Editor::selected_count() const
{
    if (_graph == nullptr)
    {
        return 0;
    }
    size_t count = 0;
    for (ContainerGroup &group : _graph->container_groups())
    {
        for (Geo::DObject *object : group)
        {
            if (object->is_selected)
            {
                ++count;
            }
        }
    }
    return count;
}

void Editor::reset_selected_mark(const bool value)
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }
    for (Geo::DObject *container : _graph->container_group(_current_group))
    {
        container->is_selected = value;
    }
}

const std::vector<Geo::DObject *> &Editor::paste_table() const
{
    return _paste_table;
}

void Editor::undo()
{
    _backup.undo();
    _view_tree.remove(_backup.removed);
    _view_tree.append(_backup.appended);
    _view_tree.update(_backup.updated);
}

void Editor::set_backup_count(const size_t count)
{
    _backup.set_count(count);
}

void Editor::push_backup_command(UndoStack::Command *command)
{
    _view_tree.remove(command->removed);
    _view_tree.append(command->appended);
    _view_tree.update(command->updated);
    command->removed.clear();
    command->appended.clear();
    command->updated.clear();
    return _backup.push_command(command);
}

void Editor::moved_objects(const std::vector<Geo::DObject *> &objects, const double dx, const double dy)
{
    _view_tree.update(objects);
    if (this->edited_shape.empty() || objects.size() > 1)
    {
        _backup.push_command(new UndoStack::TranslateCommand(objects, dx, dy));
    }
    else
    {
        if (objects.front()->type() == Geo::Type::BSPLINE)
        {
            _backup.push_command(new UndoStack::ChangeShapeCommand(static_cast<Geo::BSpline *>(objects.front()), this->edited_shape,
                                                                   this->edited_path, this->edited_knots));
            this->edited_path.clear();
            this->edited_knots.clear();
        }
        else
        {
            _backup.push_command(new UndoStack::ChangeShapeCommand(objects.front(), this->edited_shape));
        }
        this->edited_shape.clear();
    }
}


void Editor::remove_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    if (_current_group >= index && _current_group > 0)
    {
        _current_group--;
    }
    _view_tree.remove(std::vector<Geo::DObject *>(_graph->container_group(index).begin(), _graph->container_group(index).end()));
    _backup.push_command(new UndoStack::GroupCommand(index, false, _graph->container_group(index)));
    _graph->remove_group(index);
}

void Editor::append_group(const size_t index)
{
    if (index >= _graph->container_groups().size())
    {
        _graph->append_group();
        _backup.push_command(new UndoStack::GroupCommand(_graph->size() - 1, true));
    }
    else
    {
        _graph->insert_group(index);
        _backup.push_command(new UndoStack::GroupCommand(index, true));
    }
}

void Editor::reorder_group(size_t from, size_t to)
{
    if (from < to)
    {
        _backup.push_command(new UndoStack::ReorderGroupCommand(from, to++));
    }
    else if (from > to)
    {
        _backup.push_command(new UndoStack::ReorderGroupCommand(from++, to));
    }
    else
    {
        return;
    }

    if (to >= _graph->size())
    {
        _graph->append_group();
    }
    else
    {
        _graph->insert_group(to);
    }
    _graph->container_group(from).transfer(_graph->container_group(to));
    _graph->remove_group(from);
}

bool Editor::group_is_visible(const size_t index) const
{
    return _graph->container_group(index).visible();
}

void Editor::show_group(const size_t index)
{
    _graph->container_group(index).show();
    _view_tree.build(_graph);
}

void Editor::hide_group(const size_t index)
{
    _graph->container_group(index).hide();
    _view_tree.build(_graph);
}

std::string Editor::group_name(const size_t index) const
{
    return _graph->container_group(index).name;
}

void Editor::set_group_name(const size_t index, const std::string &name)
{
    _backup.push_command(new UndoStack::RenameGroupCommand(index, _graph->container_group(index).name));
    _graph->container_group(index).name = name;
}


void Editor::append(Geo::DObject *object)
{
    if (object->type() != Geo::Type::POINT && object->empty())
    {
        return;
    }
    _graph->append(object, _current_group);
    _graph->modified = true;
    _view_tree.append(object);
    _backup.push_command(new UndoStack::ObjectCommand(object, _current_group, _graph->container_group(_current_group).size(), true));
}

void Editor::append(const std::vector<Geo::DObject *> &objects)
{
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    for (Geo::DObject *object : objects)
    {
        _graph->append(object, _current_group);
        items.emplace_back(object, _current_group, _graph->container_group(_current_group).size());
    }
    _graph->modified = true;
    _view_tree.append(objects);
    _backup.push_command(new UndoStack::ObjectCommand(items, true));
}

void Editor::translate_points(Geo::DObject *points, const double x0, const double y0, const double x1, const double y1,
                              const bool change_shape)
{
    const double catch_distance = std::max(GlobalSetting::setting().catch_distance, std::pow(GlobalSetting::setting().catch_distance, 2));
    GlobalSetting::setting().translated_points = true;
    switch (points->type())
    {
    case Geo::Type::POLYGON:
        if (Geo::Polygon *temp = static_cast<Geo::Polygon *>(points); change_shape)
        {
            size_t count = 0, index = SIZE_MAX;
            double distance = 0, min_distance = DBL_MAX;
            for (Geo::Point &point : *temp)
            {
                distance = std::min(Geo::distance_square(x0, y0, point.x, point.y), Geo::distance_square(x1, y1, point.x, point.y));
                if (distance <= catch_distance * catch_distance && distance < min_distance)
                {
                    index = count;
                    min_distance = distance;
                }
                ++count;
            }
            if (index < SIZE_MAX)
            {
                if (edited_shape.empty())
                {
                    for (const Geo::Point &point : *temp)
                    {
                        edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->at(index).translate(x1 - x0, y1 - y0);
                temp->back() = temp->front();
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::CIRCLE:
        if (Geo::Circle *temp = static_cast<Geo::Circle *>(points); change_shape)
        {
            if (std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x0, y0))) <= catch_distance ||
                std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x1, y1))) <= catch_distance)
            {
                if (edited_shape.empty())
                {
                    edited_shape.emplace_back(temp->x, temp->y);
                    edited_shape.emplace_back(temp->radius, 0);
                }
                temp->radius = Geo::distance(*temp, Geo::Point(x1, y1));
                temp->update_shape(Geo::Circle::default_down_sampling_value);
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::ELLIPSE:
        if (Geo::Ellipse *temp = static_cast<Geo::Ellipse *>(points); change_shape)
        {
            const Geo::Point point0(x0, y0), point1(x1, y1);
            const double disa[4] = {Geo::distance(temp->a0(), point0), Geo::distance(temp->a0(), point1), Geo::distance(temp->a1(), point0),
                                    Geo::distance(temp->a1(), point1)};
            const double min_disa = *std::min_element(disa, disa + 4);
            const double disb[4] = {Geo::distance(temp->b0(), point0), Geo::distance(temp->b0(), point1), Geo::distance(temp->b1(), point0),
                                    Geo::distance(temp->b1(), point1)};
            const double min_disb = *std::min_element(disb, disb + 4);
            if (min_disa < min_disb && min_disa <= catch_distance)
            {
                if (edited_shape.empty())
                {
                    edited_shape.emplace_back(temp->a0().x, temp->a0().y);
                    edited_shape.emplace_back(temp->a1().x, temp->a1().y);
                    edited_shape.emplace_back(temp->b0().x, temp->b0().y);
                    edited_shape.emplace_back(temp->b1().x, temp->b1().y);
                    edited_shape.emplace_back(temp->arc_angle0(), temp->arc_angle1());
                }
                temp->set_lengtha(Geo::distance(point1, temp->center()));
                temp->update_shape(Geo::Ellipse::default_down_sampling_value);
            }
            else if (min_disb < min_disa && min_disb <= catch_distance)
            {
                if (edited_shape.empty())
                {
                    edited_shape.emplace_back(temp->a0().x, temp->a0().y);
                    edited_shape.emplace_back(temp->a1().x, temp->a1().y);
                    edited_shape.emplace_back(temp->b0().x, temp->b0().y);
                    edited_shape.emplace_back(temp->b1().x, temp->b1().y);
                    edited_shape.emplace_back(temp->arc_angle0(), temp->arc_angle1());
                }
                temp->set_lengthb(Geo::distance(point1, temp->center()));
                temp->update_shape(Geo::Ellipse::default_down_sampling_value);
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::ARC:
        if (Geo::Arc *temp = static_cast<Geo::Arc *>(points); change_shape)
        {
            const Geo::Point point0(x0, y0), point1(x1, y1);
            int index = 0;
            double dis = std::min(Geo::distance(point0, temp->control_points[0]), Geo::distance(point1, temp->control_points[0]));
            for (int i = 1; i < 3; ++i)
            {
                if (const double dis2 =
                        std::min(Geo::distance(point0, temp->control_points[i]), Geo::distance(point1, temp->control_points[i]));
                    dis2 < dis)
                {
                    dis = dis2;
                    index = i;
                }
            }
            if (dis <= catch_distance)
            {
                if (edited_shape.empty())
                {
                    edited_shape.emplace_back(temp->control_points[0].x, temp->control_points[0].y);
                    edited_shape.emplace_back(temp->control_points[1].x, temp->control_points[1].y);
                    edited_shape.emplace_back(temp->control_points[2].x, temp->control_points[2].y);
                }
                temp->control_points[index].translate(x1 - x0, y1 - y0);
                double parameters[6] = {std::get<0>(edited_shape[0]), std::get<1>(edited_shape[0]), std::get<0>(edited_shape[1]),
                                        std::get<1>(edited_shape[1]), std::get<0>(edited_shape[2]), std::get<1>(edited_shape[2])};
                parameters[index * 2] = temp->control_points[index].x;
                parameters[index * 2 + 1] = temp->control_points[index].y;
                temp->control_points[0].x = parameters[0], temp->control_points[0].y = parameters[1];
                temp->control_points[2].x = parameters[4], temp->control_points[2].y = parameters[5];
                const double a = parameters[0] - parameters[2], b = parameters[1] - parameters[3], c = parameters[0] - parameters[4],
                             d = parameters[1] - parameters[5];
                const double e = (parameters[0] * parameters[0] - parameters[2] * parameters[2] + parameters[1] * parameters[1] -
                                  parameters[3] * parameters[3]) /
                                 2;
                const double f = (parameters[0] * parameters[0] - parameters[4] * parameters[4] + parameters[1] * parameters[1] -
                                  parameters[5] * parameters[5]) /
                                 2;
                const double t = b * c - a * d;
                temp->x = (b * f - d * e) / t, temp->y = (c * e - a * f) / t;
                temp->radius = (std::hypot(temp->x - parameters[0], temp->y - parameters[1]) +
                                std::hypot(temp->x - parameters[2], temp->y - parameters[3]) +
                                std::hypot(temp->x - parameters[4], temp->y - parameters[5])) /
                               3;
                temp->update_shape(Geo::Circle::default_down_sampling_value);
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::TEXT:
    case Geo::Type::COMBINATION:
    case Geo::Type::POINT:
        points->translate(x1 - x0, y1 - y0);
        break;
    case Geo::Type::POLYLINE:
        if (Geo::Polyline *temp = static_cast<Geo::Polyline *>(points); change_shape)
        {
            size_t count = 0, index = SIZE_MAX;
            double distance = 0, min_distance = DBL_MAX;
            for (Geo::Point &point : *temp)
            {
                distance = std::min(Geo::distance_square(x0, y0, point.x, point.y), Geo::distance_square(x1, y1, point.x, point.y));
                if (distance <= catch_distance * catch_distance && distance < min_distance)
                {
                    index = count;
                    min_distance = distance;
                }
                ++count;
            }
            if (index < SIZE_MAX)
            {
                if (edited_shape.empty())
                {
                    for (const Geo::Point &point : *temp)
                    {
                        edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->at(index).translate(x1 - x0, y1 - y0);
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::BEZIER:
        if (Geo::CubicBezier *temp = static_cast<Geo::CubicBezier *>(points); change_shape)
        {
            size_t count = temp->control_points.size(), index = SIZE_MAX;
            double distance = 0, min_distance = DBL_MAX;
            for (size_t i = 0; i < count; ++i)
            {
                distance = std::min(Geo::distance_square(x0, y0, temp->control_points[i].x, temp->control_points[i].y),
                                    Geo::distance_square(x1, y1, temp->control_points[i].x, temp->control_points[i].y));
                if (distance <= catch_distance * catch_distance && distance < min_distance)
                {
                    index = i;
                    min_distance = distance;
                }
            }
            if (index < SIZE_MAX)
            {
                if (edited_shape.empty())
                {
                    for (const Geo::Point &point : temp->control_points)
                    {
                        edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->control_points.at(index).translate(x1 - x0, y1 - y0);
                if (const int order = 3; index > 2 && index % order == 1)
                {
                    temp->control_points[index - 2] = temp->control_points[index - 1] +
                                                      (temp->control_points[index - 1] - temp->control_points[index]).normalize() *
                                                          Geo::distance(temp->control_points[index - 2], temp->control_points[index - 1]);
                }
                else if (index + 2 < temp->control_points.size() && index % order == order - 1)
                {
                    temp->control_points[index + 2] = temp->control_points[index + 1] +
                                                      (temp->control_points[index + 1] - temp->control_points[index]).normalize() *
                                                          Geo::distance(temp->control_points[index + 1], temp->control_points[index + 2]);
                }
                else if (index % order == 0 && index > 0 && index < count - 1)
                {
                    temp->control_points[index - 1].translate(x1 - x0, y1 - y0);
                    temp->control_points[index + 1].translate(x1 - x0, y1 - y0);
                }
                temp->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::BSPLINE:
        if (Geo::BSpline *temp = static_cast<Geo::BSpline *>(points); change_shape)
        {
            if (temp->controls_model)
            {
                size_t count = temp->control_points.size(), index = SIZE_MAX;
                double distance = 0, min_distance = DBL_MAX;
                for (size_t i = 0; i < count; ++i)
                {
                    distance = std::min(Geo::distance_square(x0, y0, temp->control_points[i].x, temp->control_points[i].y),
                                        Geo::distance_square(x1, y1, temp->control_points[i].x, temp->control_points[i].y));
                    if (distance <= catch_distance * catch_distance && distance < min_distance)
                    {
                        index = i;
                        min_distance = distance;
                    }
                }
                if (index < SIZE_MAX)
                {
                    if (edited_shape.empty())
                    {
                        for (const Geo::Point &point : temp->path_points)
                        {
                            edited_path.emplace_back(point.x, point.y);
                        }
                        for (const Geo::Point &point : temp->control_points)
                        {
                            edited_shape.emplace_back(point.x, point.y);
                        }
                        edited_knots.assign(temp->knots().begin(), temp->knots().end());
                    }

                    temp->control_points[index].translate(x1 - x0, y1 - y0);
                    temp->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
                }
            }
            else
            {
                size_t count = temp->path_points.size(), index = SIZE_MAX;
                double distance = 0, min_distance = DBL_MAX;
                for (size_t i = 0; i < count; ++i)
                {
                    distance = std::min(Geo::distance_square(x0, y0, temp->path_points[i].x, temp->path_points[i].y),
                                        Geo::distance_square(x1, y1, temp->path_points[i].x, temp->path_points[i].y));
                    if (distance <= catch_distance * catch_distance && distance < min_distance)
                    {
                        index = i;
                        min_distance = distance;
                    }
                }
                if (index < SIZE_MAX)
                {
                    if (edited_shape.empty())
                    {
                        for (const Geo::Point &point : temp->path_points)
                        {
                            edited_path.emplace_back(point.x, point.y);
                        }
                        for (const Geo::Point &point : temp->control_points)
                        {
                            edited_shape.emplace_back(point.x, point.y);
                        }
                        edited_knots.assign(temp->knots().begin(), temp->knots().end());
                    }

                    temp->path_points[index].translate(x1 - x0, y1 - y0);
                    temp->update_control_points();
                    temp->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
                }
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    default:
        break;
    }
    _graph->modified = true;
    _view_tree.update(points);
}

bool Editor::remove_selected()
{
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected)
        {
            _view_tree.remove(_graph->container_group(_current_group)[i]);
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        _view_tree.remove(_graph->container_group(_current_group).front());
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));
    return true;
}

bool Editor::copy_selected()
{
    while (!_paste_table.empty())
    {
        delete _paste_table.back();
        _paste_table.pop_back();
    }
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }

    for (const Geo::DObject *container : _graph->container_group(_current_group))
    {
        if (container->is_selected && container->type() != Geo::Type::DIMENSION)
        {
            _paste_table.push_back(container->clone());
        }
    }

    reset_selected_mark();
    return true;
}

bool Editor::cut_selected()
{
    while (!_paste_table.empty())
    {
        delete _paste_table.back();
        _paste_table.pop_back();
    }
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected &&
            _graph->container_group(_current_group)[i]->type() != Geo::Type::DIMENSION)
        {
            _view_tree.remove(_graph->container_group(_current_group)[i]);
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _paste_table.push_back(std::get<0>(items.back())->clone());
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        _view_tree.remove(_graph->container_group(_current_group).front());
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
        _paste_table.push_back(std::get<0>(items.back())->clone());
    }
    std::reverse(_paste_table.begin(), _paste_table.end());

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));

    return true;
}

bool Editor::paste(const double tx, const double ty)
{
    if (_paste_table.empty() || _graph == nullptr)
    {
        return false;
    }

    reset_selected_mark();
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (Geo::DObject *geo : _paste_table)
    {
        _graph->container_group(_current_group).append(geo->clone());
        _graph->container_group(_current_group).back()->translate(tx, ty);
        items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        _view_tree.append(_graph->container_group(_current_group).back());
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

bool Editor::connect(const std::vector<Geo::DObject *> &objects, const double connect_distance)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<Geo::Polyline *> polylines;
    std::vector<size_t> indices;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::DObject *object : objects)
    {
        if (object->type() == Geo::Type::POLYLINE)
        {
            polylines.push_back(static_cast<Geo::Polyline *>(object));
            indices.push_back(std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }
    std::vector<bool> merged(polylines.size(), false);

    Geo::Polyline *polyline = nullptr;
    size_t index = 0;
    for (size_t i = 0, count = indices.size(); i < count; ++i)
    {
        const Geo::Point front_i = polylines[i]->front();
        const Geo::Point back_i = polylines[i]->back();
        for (size_t j = i + 1; j < count; ++j)
        {
            const Geo::Point front_j = polylines[j]->front();
            const Geo::Point back_j = polylines[j]->back();
            if (Geo::distance_square(front_i, front_j) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline(polylines[i]->begin(), polylines[i]->end());
                std::reverse(polyline->begin(), polyline->end());
                polyline->append(polylines[j]->begin(), polylines[j]->end());

                merged[i] = merged[j] = true;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(front_i, back_j) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline(polylines[j]->begin(), polylines[j]->end());
                polyline->append(polylines[i]->begin(), polylines[i]->end());

                merged[i] = merged[j] = true;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(back_i, front_j) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline(polylines[i]->begin(), polylines[i]->end());
                polyline->append(polylines[j]->begin(), polylines[j]->end());

                merged[i] = merged[j] = true;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(back_i, back_j) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline(polylines[i]->begin(), polylines[i]->end());
                polyline->append(polylines[j]->rbegin(), polylines[j]->rend());

                merged[i] = merged[j] = true;
                index = i;
                i = count;
                break;
            }
        }
    }

    if (polyline == nullptr)
    {
        return false;
    }

    for (size_t i = 0, count = polylines.size(); i < count;)
    {
        if (merged[i])
        {
            ++i;
            continue;
        }

        Geo::Point front_i = polylines[i]->front();
        Geo::Point back_i = polylines[i]->back();
        if (Geo::distance_square(polyline->front(), front_i) < connect_distance * connect_distance)
        {
            polyline->insert(0, polylines[i]->rbegin(), polylines[i]->rend());
            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->front(), back_i) < connect_distance * connect_distance)
        {
            polyline->insert(0, polylines[i]->begin(), polylines[i]->end());
            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->back(), front_i) < connect_distance * connect_distance)
        {
            polyline->append(polylines[i]->begin(), polylines[i]->end());
            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->back(), back_i) < connect_distance * connect_distance)
        {
            polyline->append(polylines[i]->rbegin(), polylines[i]->rend());
            merged[i] = true;
            i = 0;
        }
        else
        {
            ++i;
        }
    }

    for (size_t i = polylines.size() - 1; i > 0; --i)
    {
        if (!merged[i])
        {
            indices.erase(indices.begin() + i);
        }
    }
    if (!merged.front())
    {
        indices.erase(indices.begin());
    }
    std::sort(indices.begin(), indices.end(), std::greater<>());
    std::vector<std::tuple<Geo::DObject *, size_t>> items;
    for (size_t i : indices)
    {
        _view_tree.remove(group[i]);
        items.emplace_back(group.pop(i), i);
    }
    std::reverse(items.begin(), items.end());

    if (group.size() <= index)
    {
        group.append(polyline);
    }
    else
    {
        group.insert(index, polyline);
    }
    _view_tree.append(polyline);

    _graph->modified = true;
    _backup.push_command(new UndoStack::ConnectCommand(items, polyline, _current_group));
    return true;
}

bool Editor::blend(const Geo::DObject *object0, const Geo::DObject *object1, const Geo::Point &pos0, const Geo::Point &pos1)
{
    Geo::Point pre0, point0, pre1, point1;
    switch (object0->type())
    {
    case Geo::Type::ARC:
        if (const Geo::Arc *arc = static_cast<const Geo::Arc *>(object0);
            Geo::distance_square(arc->control_points[0], pos0) < Geo::distance_square(arc->control_points[2], pos0))
        {
            point0 = arc->control_points[0];
            if (Geo::is_on_left(arc->control_points[1], arc->control_points[0], arc->control_points[2]))
            {
                pre0 = point0 + (Geo::Point(arc->x, arc->y) - arc->control_points[0]).vertical();
            }
            else
            {
                pre0 = point0 + (arc->control_points[0] - Geo::Point(arc->x, arc->y)).vertical();
            }
        }
        else
        {
            point0 = arc->control_points[2];
            if (Geo::is_on_left(arc->control_points[1], arc->control_points[2], arc->control_points[0]))
            {
                pre0 = point0 + (Geo::Point(arc->x, arc->y) - arc->control_points[2]).vertical();
            }
            else
            {
                pre0 = point0 + (arc->control_points[2] - Geo::Point(arc->x, arc->y)).vertical();
            }
        }
        break;
    case Geo::Type::BEZIER:
        if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object0);
            Geo::distance_square(bezier->front(), pos0) < Geo::distance_square(bezier->back(), pos0))
        {
            point0 = bezier->front();
            pre0 = bezier->control_points.at(1);
        }
        else
        {
            point0 = bezier->back();
            pre0 = bezier->control_points.at(bezier->control_points.size() - 2);
        }
        break;
    case Geo::Type::BSPLINE:
        if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object0);
            Geo::distance_square(bspline->front(), pos0) < Geo::distance_square(bspline->back(), pos0))
        {
            point0 = bspline->front();
            pre0 = bspline->control_points[1];
        }
        else
        {
            point0 = bspline->back();
            pre0 = bspline->control_points[bspline->control_points.size() - 2];
        }
        break;
    case Geo::Type::ELLIPSE:
        {
            const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object0);
            const Geo::Point center = ellipse->center();
            const double angle = Geo::angle(ellipse->a0(), ellipse->a1());
            const double aa = Geo::distance_square(ellipse->a0(), ellipse->a1()) / 4;
            const double bb = Geo::distance_square(ellipse->b0(), ellipse->b1()) / 4;
            if (Geo::distance_square(ellipse->arc_point0(), pos0) < Geo::distance_square(ellipse->arc_point1(), pos0))
            {
                point0 = ellipse->arc_point0();
                Geo::Point coord = Geo::to_coord(point0, center.x, center.y, angle);
                const double a = coord.x / aa, b = coord.y / bb;
                pre0.x = coord.x + 10;
                pre0.y = (1 - a * pre0.x) / b;
                if (Geo::is_on_left(pre0, Geo::Point(0, 0), coord))
                {
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre0 = Geo::to_coord(pre0, anchor.x, anchor.y, -angle);
                }
                else
                {
                    pre0.x = coord.x - 10;
                    pre0.y = (1 - a * pre0.x) / b;
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre0 = Geo::to_coord(pre0, anchor.x, anchor.y, -angle);
                }
            }
            else
            {
                point0 = ellipse->arc_point1();
                Geo::Point coord = Geo::to_coord(point0, center.x, center.y, angle);
                const double a = coord.x / aa, b = coord.y / bb;
                pre0.x = coord.x + 10;
                pre0.y = (1 - a * pre0.x) / b;
                if (Geo::is_on_left(pre0, Geo::Point(0, 0), coord))
                {
                    pre0.x = coord.x - 10;
                    pre0.y = (1 - a * pre0.x) / b;
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre0 = Geo::to_coord(pre0, anchor.x, anchor.y, -angle);
                }
                else
                {
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre0 = Geo::to_coord(pre0, anchor.x, anchor.y, -angle);
                }
            }
        }
        break;
    case Geo::Type::POLYLINE:
        if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object0);
            Geo::distance(polyline->front(), pos0) < Geo::distance(polyline->back(), pos0))
        {
            point0 = polyline->front();
            pre0 = polyline->at(1);
        }
        else
        {
            point0 = polyline->back();
            pre0 = polyline->at(polyline->size() - 2);
        }
        break;
    default:
        break;
    }
    switch (object1->type())
    {
    case Geo::Type::ARC:
        if (const Geo::Arc *arc = static_cast<const Geo::Arc *>(object1);
            Geo::distance_square(arc->control_points[0], pos1) < Geo::distance_square(arc->control_points[2], pos1))
        {
            point1 = arc->control_points[0];
            if (Geo::is_on_left(arc->control_points[1], arc->control_points[0], arc->control_points[2]))
            {
                pre1 = point1 + (Geo::Point(arc->x, arc->y) - arc->control_points[0]).vertical();
            }
            else
            {
                pre1 = point1 + (arc->control_points[0] - Geo::Point(arc->x, arc->y)).vertical();
            }
        }
        else
        {
            point1 = arc->control_points[2];
            if (Geo::is_on_left(arc->control_points[1], arc->control_points[2], arc->control_points[0]))
            {
                pre1 = point1 + (Geo::Point(arc->x, arc->y) - arc->control_points[2]).vertical();
            }
            else
            {
                pre1 = point1 + (arc->control_points[2] - Geo::Point(arc->x, arc->y)).vertical();
            }
        }
        break;
    case Geo::Type::BEZIER:
        if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object1);
            Geo::distance_square(bezier->front(), pos1) < Geo::distance_square(bezier->back(), pos1))
        {
            point1 = bezier->front();
            pre1 = bezier->control_points.at(1);
        }
        else
        {
            point1 = bezier->back();
            pre1 = bezier->control_points.at(bezier->control_points.size() - 2);
        }
        break;
    case Geo::Type::BSPLINE:
        if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object1);
            Geo::distance_square(bspline->front(), pos1) < Geo::distance_square(bspline->back(), pos1))
        {
            point1 = bspline->front();
            pre1 = bspline->control_points[1];
        }
        else
        {
            point1 = bspline->back();
            pre1 = bspline->control_points[bspline->control_points.size() - 2];
        }
        break;
    case Geo::Type::ELLIPSE:
        {
            const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object1);
            const Geo::Point center = ellipse->center();
            const double angle = Geo::angle(ellipse->a0(), ellipse->a1());
            const double aa = Geo::distance_square(ellipse->a0(), ellipse->a1()) / 4;
            const double bb = Geo::distance_square(ellipse->b0(), ellipse->b1()) / 4;
            if (Geo::distance_square(ellipse->arc_point0(), pos1) < Geo::distance_square(ellipse->arc_point1(), pos1))
            {
                point1 = ellipse->arc_point0();
                Geo::Point coord = Geo::to_coord(point1, center.x, center.y, angle);
                const double a = coord.x / aa, b = coord.y / bb;
                pre1.x = coord.x + 10;
                pre1.y = (1 - a * pre1.x) / b;
                if (Geo::is_on_left(pre1, Geo::Point(0, 0), coord))
                {
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre1 = Geo::to_coord(pre1, anchor.x, anchor.y, -angle);
                }
                else
                {
                    pre1.x = coord.x - 10;
                    pre1.y = (1 - a * pre1.x) / b;
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre1 = Geo::to_coord(pre1, anchor.x, anchor.y, -angle);
                }
            }
            else
            {
                point1 = ellipse->arc_point1();
                Geo::Point coord = Geo::to_coord(point1, center.x, center.y, angle);
                const double a = coord.x / aa, b = coord.y / bb;
                pre1.x = coord.x + 10;
                pre1.y = (1 - a * pre1.x) / b;
                if (Geo::is_on_left(pre1, Geo::Point(0, 0), coord))
                {
                    pre1.x = coord.x - 10;
                    pre1.y = (1 - a * pre1.x) / b;
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre1 = Geo::to_coord(pre1, anchor.x, anchor.y, -angle);
                }
                else
                {
                    const Geo::Point anchor = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    pre1 = Geo::to_coord(pre1, anchor.x, anchor.y, -angle);
                }
            }
        }
        break;
    case Geo::Type::POLYLINE:
        if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object1);
            Geo::distance(polyline->front(), pos1) < Geo::distance(polyline->back(), pos1))
        {
            point1 = polyline->front();
            pre1 = polyline->at(1);
        }
        else
        {
            point1 = polyline->back();
            pre1 = polyline->at(polyline->size() - 2);
        }
        break;
    default:
        break;
    }

    if (Geo::CubicBezier *bezier = Geo::blend(pre0, point0, point1, pre1))
    {
        bezier->is_selected = true;
        _graph->container_group(_current_group).append(bezier);
        _view_tree.append(bezier);
        _backup.push_command(
            new UndoStack::ObjectCommand(bezier, _current_group, _graph->container_group(_current_group).size() - 1, true));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editor::close_polyline(const std::vector<Geo::DObject *> &objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    Geo::Polygon *shape = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::DObject *object : objects)
    {
        if (object->type() != Geo::Type::POLYLINE)
        {
            continue;
        }
        if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object); polyline->size() >= 3)
        {
            shape = new Geo::Polygon(*polyline);
            shape->is_selected = true;
            size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            add_items.emplace_back(shape, _current_group, index);
            _view_tree.append(shape);
            _view_tree.remove(group[index]);
            remove_items.emplace_back(group.pop(index), _current_group, index);
            group.insert(index, shape);
        }
    }

    if (shape == nullptr)
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));

        return true;
    }
}

bool Editor::combine(const std::vector<Geo::DObject *> &objects)
{
    if (_graph == nullptr || objects.size() < 2)
    {
        return false;
    }

    Combination *combination = new Combination();
    ContainerGroup &group = _graph->container_group(_current_group);
    std::vector<std::tuple<Combination *, size_t, std::vector<Geo::DObject *>>> items;
    for (Geo::DObject *object : objects)
    {
        if (object->type() == Geo::Type::DIMENSION)
        {
            continue;
        }
        if (object->type() == Geo::Type::COMBINATION)
        {
            std::vector<Geo::DObject *>::iterator it = std::find(group.begin(), group.end(), object);
            size_t index = std::distance(group.begin(), it);
            Combination *temp = static_cast<Combination *>(group.pop(it));
            items.emplace_back(temp, index, std::vector<Geo::DObject *>(temp->begin(), temp->end()));
            combination->append(temp);
        }
        else
        {
            combination->append(group.pop(std::find(group.rbegin(), group.rend(), object)));
        }
        _view_tree.remove(object);
    }

    std::reverse(combination->begin(), combination->end());
    combination->is_selected = true;
    combination->update_border();
    _graph->container_group(_current_group).append(combination);
    _view_tree.append(combination);

    _backup.push_command(new UndoStack::CombineCommand(combination, items, _current_group));
    _graph->modified = true;

    return true;
}

bool Editor::detach(const std::vector<Geo::DObject *> &objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Combination *, size_t>> combiantions;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::DObject *object : objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            combiantions.emplace_back(static_cast<Combination *>(object),
                                      std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }
    if (combiantions.empty())
    {
        return false;
    }

    std::reverse(combiantions.begin(), combiantions.end());
    _backup.push_command(new UndoStack::CombineCommand(combiantions, _current_group));
    for (std::tuple<Combination *, size_t> &combination : combiantions)
    {
        std::reverse(std::get<0>(combination)->begin(), std::get<0>(combination)->end());
        group.pop(std::find(group.rbegin(), group.rend(), std::get<0>(combination)));
        _view_tree.remove(std::get<0>(combination));
        _view_tree.append(std::vector<Geo::DObject *>(std::get<0>(combination)->begin(), std::get<0>(combination)->end()));
        group.append(*static_cast<ContainerGroup *>(std::get<0>(combination)));
    }

    _graph->modified = true;

    return true;
}

bool Editor::mirror(const std::vector<Geo::DObject *> &objects, const Geo::Point &start, const Geo::Point &end, const bool copy)
{
    if (objects.empty() || start == end)
    {
        return false;
    }

    const double a = start.y - end.y;
    const double b = end.x - start.x;
    const double c = start.x * end.y - end.x * start.y;
    const double d = a * a + b * b;
    const double mat[6] = {(b * b - a * a) / d, -2 * a * b / d, -2 * a * c / d, -2 * a * b / d, (a * a - b * b) / d, -2 * b * c / d};

    if (copy)
    {
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
        size_t index = _graph->container_group(_current_group).size();
        for (Geo::DObject *obj : objects)
        {
            if (obj->type() != Geo::Type::DIMENSION)
            {
                _graph->container_group(_current_group).append(obj->clone());
                _graph->container_group(_current_group).back()->transform(mat);
                _graph->container_group(_current_group).back()->is_selected = true;
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
                _view_tree.append(_graph->container_group(_current_group).back());
            }
        }
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
    }
    else
    {
        std::vector<Geo::DObject *> items;
        for (Geo::DObject *obj : objects)
        {
            if (obj->type() != Geo::Type::DIMENSION)
            {
                obj->transform(mat);
                obj->is_selected = true;
                items.push_back(obj);
            }
        }
        _view_tree.update(items);
        _backup.push_command(new UndoStack::TransformCommand(items, mat));
    }

    _graph->modified = true;

    return true;
}

bool Editor::offset(const std::vector<Geo::DObject *> &objects, const double distance, const Geo::Offset::JoinType join_type,
                    const Geo::Offset::EndType end_type)
{
    const size_t count = _graph->container_group(_current_group).size();
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    Geo::Ellipse *ellipse = nullptr;
    Geo::BSpline *bspline = nullptr;
    Geo::CubicBezier *bezier = nullptr;
    Geo::Arc *arc = nullptr;
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    size_t index = count;
    for (Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(object);
            if (std::vector<Geo::Polygon> result; Geo::offset(*polygon, result, distance, join_type, end_type))
            {
                for (const Geo::Polygon &shape0 : result)
                {
                    _graph->append(new Geo::Polygon(shape0), _current_group);
                    items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
                }
            }
            break;
        case Geo::Type::CIRCLE:
            circle = static_cast<Geo::Circle *>(object);
            if (distance >= 0 || -distance < circle->radius)
            {
                _graph->append(new Geo::Circle(circle->x, circle->y, circle->radius + distance), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::ELLIPSE:
            ellipse = static_cast<Geo::Ellipse *>(object);
            if (distance >= 0 || -distance < std::min(ellipse->lengtha(), ellipse->lengthb()))
            {
                _graph->append(new Geo::Ellipse(ellipse->center(), ellipse->lengtha() + distance, ellipse->lengthb() + distance),
                               _current_group);
                _graph->container_group(_current_group).back()->rotate(ellipse->center().x, ellipse->center().y, ellipse->angle());
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::Polyline shape1; Geo::offset(*static_cast<const Geo::Polyline *>(object), shape1, distance))
            {
                _graph->append(shape1.clone(), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::BSPLINE:
            {
                bspline = static_cast<Geo::BSpline *>(object);
                const Geo::CubicBezier beziershape = Geo::bspline_to_bezier(*bspline);
                if (std::vector<Geo::CubicBezier> shapes;
                    Geo::offset(beziershape, shapes, distance, GlobalSetting::setting().offset_tolerance,
                                GlobalSetting::setting().offset_sample_count))
                {
                    for (const Geo::CubicBezier &shape : shapes)
                    {
                        _graph->append(new Geo::CubicBSpline(Geo::bezier_to_bspline(shape)), _current_group);
                        items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
                    }
                }
            }
            break;
        case Geo::Type::BEZIER:
            bezier = static_cast<Geo::CubicBezier *>(object);
            if (std::vector<Geo::CubicBezier> shapes; Geo::offset(*bezier, shapes, distance, GlobalSetting::setting().offset_tolerance,
                                                                  GlobalSetting::setting().offset_sample_count))
            {
                for (Geo::CubicBezier &shape : shapes)
                {
                    _graph->append(new Geo::CubicBezier(shape), _current_group);
                    items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
                }
            }
            break;
        case Geo::Type::ARC:
            arc = static_cast<Geo::Arc *>(object);
            if (distance >= 0 || -distance < arc->radius)
            {
                const Geo::Point center(arc->x, arc->y);
                _graph->append(new Geo::Arc(arc->x, arc->y, arc->radius + distance, Geo::angle(center, arc->control_points[0]),
                                            Geo::angle(center, arc->control_points[2]), !arc->is_cw()),
                               _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        default:
            break;
        }
        object->is_selected = false;
    }

    if (count == _graph->container_group(_current_group).size())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : items)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
        return true;
    }
}

bool Editor::scale(const std::vector<Geo::DObject *> &objects, const bool unitary, const double k)
{
    if (objects.empty() || k == 0 || k == 1)
    {
        return false;
    }

    if (unitary)
    {
        double top = -DBL_MAX, bottom = DBL_MAX, left = DBL_MAX, right = -DBL_MAX;
        std::vector<Geo::DObject *> items;
        for (Geo::DObject *object : objects)
        {
            if (object->type() != Geo::Type::DIMENSION)
            {
                items.push_back(object);
                const Geo::AABBRect rect = object->aabbrect();
                top = std::max(top, rect.top);
                bottom = std::min(bottom, rect.bottom);
                left = std::min(left, rect.left);
                right = std::max(right, rect.right);
            }
        }

        if (items.empty())
        {
            return false;
        }

        const double x = (left + right) / 2, y = (top + bottom) / 2;
        for (Geo::DObject *object : items)
        {
            object->scale(x, y, k);
            _view_tree.update(object);
        }

        _backup.push_command(new UndoStack::ScaleCommand(items, x, y, k, unitary));
    }
    else
    {
        std::vector<Geo::DObject *> items;
        for (Geo::DObject *object : objects)
        {
            if (object->type() != Geo::Type::DIMENSION)
            {
                items.push_back(object);
                const Geo::AABBRect rect = object->aabbrect();
                object->scale((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2, k);
                _view_tree.update(object);
            }
        }

        if (items.empty())
        {
            return false;
        }

        _backup.push_command(new UndoStack::ScaleCommand(items, 0, 0, k, unitary));
    }

    _graph->modified = true;

    return true;
}

bool Editor::shape_union(Geo::DObject *shape0, Geo::DObject *shape1)
{
    if (_graph == nullptr || _graph->empty() || shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    std::vector<Geo::DObject *> result;
    switch (shape0->type())
    {
    case Geo::Type::POLYGON:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            if (std::vector<Geo::Polygon> shapes;
                Geo::polygon_union(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Polygon *>(shape1), shapes))
            {
                for (const Geo::Polygon &polygon : shapes)
                {
                    result.push_back(new Geo::Polygon(polygon));
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_union(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Circle *>(shape1),
                                              shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_union(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                               shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_union(*static_cast<const Geo::Polygon *>(shape1), *static_cast<const Geo::Circle *>(shape0),
                                              shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Arc> shapes;
                Geo::circle_union(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Circle *>(shape1), shapes))
            {
                for (const Geo::Arc &arc : shapes)
                {
                    result.push_back(new Geo::Arc(arc));
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_union(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                              shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::ELLIPSE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_union(*static_cast<const Geo::Polygon *>(shape1), *static_cast<const Geo::Ellipse *>(shape0),
                                               shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_union(*static_cast<const Geo::Circle *>(shape1), *static_cast<const Geo::Ellipse *>(shape0),
                                              shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Ellipse> shapes;
                Geo::ellipse_union(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Ellipse *>(shape1), shapes))
            {
                for (const Geo::Ellipse &ellipse : shapes)
                {
                    result.push_back(new Geo::Ellipse(ellipse));
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (result.empty())
    {
        return false;
    }
    else
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        if (index0 < index1)
        {
            remove_items.emplace_back(shape0, _current_group, index0);
            remove_items.emplace_back(shape1, _current_group, index1);
            _view_tree.remove(group.pop(index1));
            _view_tree.remove(group.pop(index0));
        }
        else
        {
            remove_items.emplace_back(shape1, _current_group, index1);
            remove_items.emplace_back(shape0, _current_group, index0);
            _view_tree.remove(group.pop(index0));
            _view_tree.remove(group.pop(index1));
            index0 = index1;
        }
        _view_tree.append(result);

        if (index0 == group.size())
        {
            group.append(result.front());
        }
        else
        {
            group.insert(index0, result.front());
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            group.append(result[i]);
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

bool Editor::shape_intersection(Geo::DObject *shape0, Geo::DObject *shape1)
{
    if (_graph == nullptr || _graph->empty() || shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    std::vector<Geo::DObject *> result;
    switch (shape0->type())
    {
    case Geo::Type::POLYGON:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            if (std::vector<Geo::Polygon> shapes;
                Geo::polygon_intersection(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Polygon *>(shape1), shapes))
            {
                for (const Geo::Polygon &polygon : shapes)
                {
                    result.push_back(new Geo::Polygon(polygon));
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_intersection(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Circle *>(shape1),
                                                     shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_intersection(*static_cast<const Geo::Polygon *>(shape0),
                                                      *static_cast<const Geo::Ellipse *>(shape1), shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_intersection(*static_cast<const Geo::Polygon *>(shape1), *static_cast<const Geo::Circle *>(shape0),
                                                     shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Arc> shapes;
                Geo::circle_intersection(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Circle *>(shape1), shapes))
            {
                for (const Geo::Arc &arc : shapes)
                {
                    result.push_back(new Geo::Arc(arc));
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_intersection(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                                     shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::ELLIPSE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_intersection(*static_cast<const Geo::Polygon *>(shape1),
                                                      *static_cast<const Geo::Ellipse *>(shape0), shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_intersection(*static_cast<const Geo::Circle *>(shape1), *static_cast<const Geo::Ellipse *>(shape0),
                                                     shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Ellipse> shapes;
                Geo::ellipse_intersection(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Ellipse *>(shape1), shapes))
            {
                for (const Geo::Ellipse &ellipse : shapes)
                {
                    result.push_back(new Geo::Ellipse(ellipse));
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (result.empty())
    {
        return false;
    }
    else
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        if (index0 < index1)
        {
            remove_items.emplace_back(shape0, _current_group, index0);
            remove_items.emplace_back(shape1, _current_group, index1);
            _view_tree.remove(group.pop(index1));
            _view_tree.remove(group.pop(index0));
        }
        else
        {
            remove_items.emplace_back(shape1, _current_group, index1);
            remove_items.emplace_back(shape0, _current_group, index0);
            _view_tree.remove(group.pop(index0));
            _view_tree.remove(group.pop(index1));
            index0 = index1;
        }
        _view_tree.append(result);

        if (index0 == group.size())
        {
            group.append(result.front());
        }
        else
        {
            group.insert(index0, result.front());
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            group.append(result[i]);
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

bool Editor::shape_difference(Geo::DObject *shape0, const Geo::DObject *shape1)
{
    if (shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    std::vector<Geo::DObject *> result;
    switch (shape0->type())
    {
    case Geo::Type::POLYGON:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            if (std::vector<Geo::Polygon> shapes;
                Geo::polygon_difference(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Polygon *>(shape1), shapes))
            {
                for (const Geo::Polygon &polygon : shapes)
                {
                    result.push_back(new Geo::Polygon(polygon));
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_difference(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Circle *>(shape1),
                                                   shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_difference(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                                    shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Polyline> shapes1;
                if (Geo::circle_polygon_difference(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Polygon *>(shape1),
                                                   shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Polyline &polyline : shapes1)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Arc> shapes;
                Geo::circle_difference(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Circle *>(shape1), shapes))
            {
                for (const Geo::Arc &arc : shapes)
                {
                    result.push_back(new Geo::Arc(arc));
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_difference(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                                   shapes0, shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::ELLIPSE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Ellipse> shapes0;
                std::vector<Geo::Polyline> shapes1;
                if (Geo::ellipse_polygon_difference(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Polygon *>(shape1),
                                                    shapes0, shapes1))
                {
                    for (const Geo::Ellipse &ellipse : shapes0)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                    for (const Geo::Polyline &polyline : shapes1)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Ellipse> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::ellipse_circle_difference(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Circle *>(shape1),
                                                   shapes0, shapes1))
                {
                    for (const Geo::Ellipse &ellipse : shapes0)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Ellipse> shapes;
                Geo::ellipse_difference(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Ellipse *>(shape1), shapes))
            {
                for (const Geo::Ellipse &ellipse : shapes)
                {
                    result.push_back(new Geo::Ellipse(ellipse));
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (result.empty())
    {
        return false;
    }
    else
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        remove_items.emplace_back(shape0, _current_group, index0);
        _view_tree.remove(group.pop(index0));
        _view_tree.append(result);

        if (index0 == group.size())
        {
            group.append(result.front());
        }
        else
        {
            group.insert(index0, result.front());
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            group.append(result[i]);
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

bool Editor::shape_xor(Geo::DObject *shape0, Geo::DObject *shape1)
{
    if (shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    std::vector<Geo::DObject *> result;
    switch (shape0->type())
    {
    case Geo::Type::POLYGON:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            if (std::vector<Geo::Polygon> shapes;
                Geo::polygon_xor(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Polygon *>(shape1), shapes))
            {
                for (const Geo::Polygon &polygon : shapes)
                {
                    result.push_back(new Geo::Polygon(polygon));
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_xor(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Circle *>(shape1), shapes0,
                                            shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_xor(*static_cast<const Geo::Polygon *>(shape0), *static_cast<const Geo::Ellipse *>(shape1),
                                             shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Arc> shapes1;
                if (Geo::polygon_circle_xor(*static_cast<const Geo::Polygon *>(shape1), *static_cast<const Geo::Circle *>(shape0), shapes0,
                                            shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Arc &arc : shapes1)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Arc> shapes;
                Geo::circle_xor(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Circle *>(shape1), shapes))
            {
                for (const Geo::Arc &arc : shapes)
                {
                    result.push_back(new Geo::Arc(arc));
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_xor(*static_cast<const Geo::Circle *>(shape0), *static_cast<const Geo::Ellipse *>(shape1), shapes0,
                                            shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::ELLIPSE:
        switch (shape1->type())
        {
        case Geo::Type::POLYGON:
            {
                std::vector<Geo::Polyline> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::polygon_ellipse_xor(*static_cast<const Geo::Polygon *>(shape1), *static_cast<const Geo::Ellipse *>(shape0),
                                             shapes0, shapes1))
                {
                    for (const Geo::Polyline &polyline : shapes0)
                    {
                        result.push_back(new Geo::Polyline(polyline));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                std::vector<Geo::Arc> shapes0;
                std::vector<Geo::Ellipse> shapes1;
                if (Geo::circle_ellipse_xor(*static_cast<const Geo::Circle *>(shape1), *static_cast<const Geo::Ellipse *>(shape0), shapes0,
                                            shapes1))
                {
                    for (const Geo::Arc &arc : shapes0)
                    {
                        result.push_back(new Geo::Arc(arc));
                    }
                    for (const Geo::Ellipse &ellipse : shapes1)
                    {
                        result.push_back(new Geo::Ellipse(ellipse));
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Ellipse> shapes;
                Geo::ellipse_xor(*static_cast<const Geo::Ellipse *>(shape0), *static_cast<const Geo::Ellipse *>(shape1), shapes))
            {
                for (const Geo::Ellipse &ellipse : shapes)
                {
                    result.push_back(new Geo::Ellipse(ellipse));
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (result.empty())
    {
        return false;
    }
    else
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        if (index0 < index1)
        {
            remove_items.emplace_back(shape0, _current_group, index0);
            remove_items.emplace_back(shape1, _current_group, index1);
            _view_tree.remove(group.pop(index1));
            _view_tree.remove(group.pop(index0));
        }
        else
        {
            remove_items.emplace_back(shape1, _current_group, index1);
            remove_items.emplace_back(shape0, _current_group, index0);
            _view_tree.remove(group.pop(index0));
            _view_tree.remove(group.pop(index1));
            index0 = index1;
        }
        _view_tree.append(result);

        if (index0 == group.size())
        {
            group.append(result.front());
        }
        else
        {
            group.insert(index0, result.front());
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            group.append(result[i]);
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

bool Editor::fillet(Geo::Polyline *polyline0, const Geo::Point &point0, Geo::Polyline *polyline1, const Geo::Point &point1,
                    const double radius0, const double radius1)
{
    if (polyline0 == nullptr || polyline1 == nullptr || polyline0 == polyline1)
    {
        return false;
    }

    Geo::Polyline *result0 = new Geo::Polyline();
    Geo::Polyline *result1 = new Geo::Polyline();
    Geo::Polyline *result2 = new Geo::Polyline();
    Geo::Polyline *result3 = new Geo::Polyline();
    Geo::CubicBezier *arc = new Geo::CubicBezier();

    if (!Geo::fillet(*polyline0, point0, *polyline1, point1, radius0, radius1, *arc, *result0, *result1, *result2, *result3))
    {
        delete result0;
        delete result1;
        delete result2;
        delete result3;
        delete arc;
        return false;
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, k = 0, count = _graph->container_group(_current_group).size(); i < count && k < 2; ++i)
    {
        if (_graph->container_group(_current_group)[i] == polyline0)
        {
            ++k;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            polyline0->is_selected = false;
            remove_items.emplace_back(polyline0, _current_group, i);
            size_t j = i;
            if (result2->size() > 1)
            {
                result2->is_selected = false;
                add_items.emplace_back(result2, _current_group, i);
                _graph->container_group(_current_group).insert(i, result2);
                _view_tree.append(result2);
                ++count;
            }
            else
            {
                delete result2;
                --j;
            }
            add_items.emplace_back(result0, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, result0);
            _view_tree.append(result0);
            result0->is_selected = false;
            add_items.emplace_back(arc, _current_group, j + 2);
            _graph->container_group(_current_group).insert(j + 2, arc);
            _view_tree.append(arc);
            arc->is_selected = true;
            ++count;
        }
        else if (_graph->container_group(_current_group)[i] == polyline1)
        {
            ++k;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            polyline1->is_selected = false;
            remove_items.emplace_back(polyline1, _current_group, i);
            size_t j = i;
            if (result3->size() > 1)
            {
                result3->is_selected = false;
                add_items.emplace_back(result3, _current_group, i);
                _graph->container_group(_current_group).insert(i, result3);
                _view_tree.append(result3);
                ++count;
            }
            else
            {
                delete result3;
                --j;
            }
            add_items.emplace_back(result1, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, result1);
            _view_tree.append(result1);
            result1->is_selected = false;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    return true;
}

bool Editor::fillet(Geo::DObject *object0, Geo::DObject *object1, const Geo::Point &start, const Geo::Point &center, const Geo::Point &end,
                    const std::vector<std::tuple<size_t, double, double, double>> &tvalues)
{
    if (object0 == object1 || start == center || center == end || start == end)
    {
        return false;
    }

    if (Geo::CubicBezier curve; Geo::angle_to_arc(start, center, end, curve))
    {
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
        switch (object0->type())
        {
        case Geo::Type::ARC:
            {
                Geo::Arc *arc = static_cast<Geo::Arc *>(object0);
                if (Geo::Arc arc0, arc1; Geo::split(*arc, start, arc0, arc1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), arc));
                    remove.emplace_back(arc, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::is_on_left(arc0.control_points[1], center, start))
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                        }
                        else
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
                        }
                    }
                    else
                    {
                        if (Geo::is_on_left(arc0.control_points[1], center, start))
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
                        }
                        else
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::BEZIER:
            {
                Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(object0);
                if (Geo::CubicBezier bezier0, bezier1;
                    Geo::split(*bezier, std::get<0>(tvalues.front()), std::get<1>(tvalues.front()), bezier0, bezier1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bezier));
                    remove.emplace_back(bezier, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bezier0.front(), start) < Geo::distance(bezier0.back(), start))
                        {
                            if (Geo::is_on_left(bezier0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0.control_points[bezier0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bezier0.front(), start) < Geo::distance(bezier0.back(), start))
                        {
                            if (Geo::is_on_left(bezier0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0.control_points[bezier0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::BSPLINE:
            if (Geo::BSpline *bspline = static_cast<Geo::BSpline *>(object0); dynamic_cast<Geo::CubicBSpline *>(bspline) != nullptr)
            {
                if (Geo::CubicBSpline bspline0, bspline1; Geo::split(*bspline, true, std::get<1>(tvalues.front()), bspline0, bspline1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
                    remove.emplace_back(bspline, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bspline0.control_points.front(), start) < Geo::distance(bspline0.control_points.back(), start))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bspline0.control_points.front(), start) < Geo::distance(bspline0.control_points.back(), start))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
            }
            else
            {
                if (Geo::QuadBSpline bspline0, bspline1; Geo::split(*bspline, false, std::get<1>(tvalues.front()), bspline0, bspline1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
                    remove.emplace_back(bspline, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bspline0.control_points.front(), start) < Geo::distance(bspline0.control_points.back(), start))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bspline0.control_points.front(), start) < Geo::distance(bspline0.control_points.back(), start))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(object0);
                if (Geo::Ellipse ellipse0, ellipse1; Geo::split(*ellipse, start, ellipse0, ellipse1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), ellipse));
                    remove.emplace_back(ellipse, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse1));
                    }
                    else
                    {
                        _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse0));
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::POLYLINE:
            {
                Geo::Polyline *polyline = static_cast<Geo::Polyline *>(object0);
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), polyline));
                remove.emplace_back(polyline, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                const size_t i = std::get<0>(tvalues.front());
                if (Geo::angle(start, center, end) > 0)
                {
                    if (Geo::is_on_left(polyline->at(i), center, start))
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin(), polyline->begin() + i + 1);
                        points->back().x = start.x, points->back().y = start.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                    else
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin() + i - 1, polyline->end());
                        points->front().x = start.x, points->front().y = start.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                }
                else
                {
                    if (Geo::is_on_left(polyline->at(i), center, start))
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin() + i - 1, polyline->end());
                        points->front().x = start.x, points->front().y = start.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                    else
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin(), polyline->begin() + i + 1);
                        points->back().x = start.x, points->back().y = start.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                }
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                break;
            }
        default:
            break;
        }
        switch (object1->type())
        {
        case Geo::Type::ARC:
            {
                Geo::Arc *arc = static_cast<Geo::Arc *>(object1);
                if (Geo::Arc arc0, arc1; Geo::split(*arc, end, arc0, arc1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), arc));
                    remove.emplace_back(arc, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::is_on_left(arc0.control_points[1], center, end))
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
                        }
                        else
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                        }
                    }
                    else
                    {
                        if (Geo::is_on_left(arc0.control_points[1], center, end))
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                        }
                        else
                        {
                            _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::BEZIER:
            {
                Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(object1);
                if (Geo::CubicBezier bezier0, bezier1;
                    Geo::split(*bezier, std::get<0>(tvalues.back()), std::get<1>(tvalues.back()), bezier0, bezier1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bezier));
                    remove.emplace_back(bezier, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bezier0.front(), end) < Geo::distance(bezier0.back(), end))
                        {
                            if (Geo::is_on_left(bezier0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0.control_points[bezier0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bezier0.front(), end) < Geo::distance(bezier0.back(), end))
                        {
                            if (Geo::is_on_left(bezier0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0.control_points[bezier0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::BSPLINE:
            if (Geo::BSpline *bspline = static_cast<Geo::BSpline *>(object1); dynamic_cast<Geo::CubicBSpline *>(bspline) != nullptr)
            {
                if (Geo::CubicBSpline bspline0, bspline1; Geo::split(*bspline, true, std::get<1>(tvalues.back()), bspline0, bspline1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
                    remove.emplace_back(bspline, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bspline0.control_points.front(), end) < Geo::distance(bspline0.control_points.back(), end))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bspline0.control_points.front(), end) < Geo::distance(bspline0.control_points.back(), end))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
            }
            else
            {
                if (Geo::QuadBSpline bspline0, bspline1; Geo::split(*bspline, false, std::get<1>(tvalues.back()), bspline0, bspline1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
                    remove.emplace_back(bspline, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bspline0.control_points.front(), end) < Geo::distance(bspline0.control_points.back(), end))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bspline0.control_points.front(), end) < Geo::distance(bspline0.control_points.back(), end))
                        {
                            if (Geo::is_on_left(bspline0.control_points[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bspline0.control_points[bspline0.control_points.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                            }
                        }
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(object1);
                if (Geo::Ellipse ellipse0, ellipse1; Geo::split(*ellipse, end, ellipse0, ellipse1))
                {
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), ellipse));
                    remove.emplace_back(ellipse, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse0));
                    }
                    else
                    {
                        _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse1));
                    }
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                }
                break;
            }
        case Geo::Type::POLYLINE:
            {
                Geo::Polyline *polyline = static_cast<Geo::Polyline *>(object1);
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), polyline));
                remove.emplace_back(polyline, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                const size_t i = std::get<0>(tvalues.back());
                if (Geo::angle(start, center, end) > 0)
                {
                    if (Geo::is_on_left(polyline->at(i), center, end))
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin() + i - 1, polyline->end());
                        points->front().x = end.x, points->front().y = end.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                    else
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin(), polyline->begin() + i + 1);
                        points->back().x = end.x, points->back().y = end.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                }
                else
                {
                    if (Geo::is_on_left(polyline->at(i), center, end))
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin(), polyline->begin() + i + 1);
                        points->back().x = end.x, points->back().y = end.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                    else
                    {
                        Geo::Polyline *points = new Geo::Polyline(polyline->begin() + i - 1, polyline->end());
                        points->front().x = end.x, points->front().y = end.y;
                        _graph->container_group(_current_group).insert(index, points);
                    }
                }
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                break;
            }
        default:
            break;
        }

        _graph->container_group(_current_group).append(new Geo::CubicBezier(curve));
        append.emplace_back(_graph->container_group(_current_group).back(), _current_group,
                            _graph->container_group(_current_group).size() - 1);
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : remove)
        {
            _view_tree.remove(std::get<0>(item));
        }
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : append)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(append, remove));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editor::fillet(Geo::DObject *object, const Geo::Point &point, const double radius)
{
    std::vector<Geo::DObject *> objects;
    switch (object->type())
    {
    case Geo::Type::POLYGON:
        {
            Geo::Polygon &polygon = *static_cast<Geo::Polygon *>(object);
            std::vector<Geo::Point>::const_iterator it = std::find(polygon.begin(), polygon.end(), point);
            if (it != polygon.end())
            {
                Geo::Arc *arc = new Geo::Arc();
                Geo::Polyline *polyline = new Geo::Polyline();
                const size_t index = std::distance(polygon.cbegin(), it);
                if (Geo::fillet(polygon, index, radius, *arc, *polyline))
                {
                    objects.push_back(arc);
                    objects.push_back(polyline);
                }
                else
                {
                    delete arc;
                    delete polyline;
                }
            }
        }
        break;
    case Geo::Type::POLYLINE:
        {
            Geo::Polyline &polyline = *static_cast<Geo::Polyline *>(object);
            if (point == polyline.front() || point == polyline.back())
            {
                return false;
            }
            std::vector<Geo::Point>::const_iterator it = std::find(polyline.begin(), polyline.end(), point);
            if (it != polyline.end())
            {
                Geo::Arc *arc = new Geo::Arc();
                Geo::Polyline *polyline0 = new Geo::Polyline();
                Geo::Polyline *polyline1 = new Geo::Polyline();
                const size_t index = std::distance(polyline.cbegin(), it);
                if (Geo::fillet(polyline, index, radius, *arc, *polyline0, *polyline1))
                {
                    objects.push_back(arc);
                    objects.push_back(polyline0);
                    objects.push_back(polyline1);
                }
                else
                {
                    delete arc;
                    delete polyline0;
                    delete polyline1;
                }
            }
        }
        break;
    default:
        return false;
    }

    if (objects.empty())
    {
        return false;
    }
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == object)
        {
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            object->is_selected = false;
            remove_items.emplace_back(object, _current_group, i);
            size_t j = i - 1;
            for (Geo::DObject *item : objects)
            {
                add_items.emplace_back(item, _current_group, ++j);
                _graph->container_group(_current_group).insert(i, item);
                _view_tree.append(item);
                item->is_selected = true;
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
            break;
        }
    }
    _graph->modified = true;
    return true;
}

bool Editor::fillet(Geo::DObject *object, const Geo::Point &point, const double radius0, const double radius1)
{
    std::vector<Geo::DObject *> objects;
    switch (object->type())
    {
    case Geo::Type::POLYGON:
        {
            Geo::Polygon &polygon = *static_cast<Geo::Polygon *>(object);
            std::vector<Geo::Point>::const_iterator it = std::find(polygon.begin(), polygon.end(), point);
            if (it != polygon.end())
            {
                Geo::CubicBezier *arc = new Geo::CubicBezier();
                Geo::Polyline *polyline = new Geo::Polyline();
                const size_t index = std::distance(polygon.cbegin(), it);
                if (Geo::fillet(polygon, index, radius0, radius1, *arc, *polyline))
                {
                    objects.push_back(arc);
                    objects.push_back(polyline);
                }
                else
                {
                    delete arc;
                    delete polyline;
                }
            }
        }
        break;
    case Geo::Type::POLYLINE:
        {
            Geo::Polyline &polyline = *static_cast<Geo::Polyline *>(object);
            if (point == polyline.front() || point == polyline.back())
            {
                return false;
            }
            std::vector<Geo::Point>::const_iterator it = std::find(polyline.begin(), polyline.end(), point);
            if (it != polyline.end())
            {
                Geo::CubicBezier *arc = new Geo::CubicBezier();
                Geo::Polyline *polyline0 = new Geo::Polyline();
                Geo::Polyline *polyline1 = new Geo::Polyline();
                const size_t index = std::distance(polyline.cbegin(), it);
                if (Geo::fillet(polyline, index, radius0, radius1, *arc, *polyline0, *polyline1))
                {
                    objects.push_back(arc);
                    objects.push_back(polyline0);
                    objects.push_back(polyline1);
                }
                else
                {
                    delete arc;
                    delete polyline0;
                    delete polyline1;
                }
            }
        }
        break;
    default:
        return false;
    }

    if (objects.empty())
    {
        return false;
    }
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == object)
        {
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            object->is_selected = false;
            remove_items.emplace_back(object, _current_group, i);
            size_t j = i - 1;
            for (Geo::DObject *item : objects)
            {
                add_items.emplace_back(item, _current_group, ++j);
                _graph->container_group(_current_group).insert(j, item);
                _view_tree.append(item);
                item->is_selected = true;
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
            break;
        }
    }
    _graph->modified = true;
    return true;
}

bool Editor::fillet(Geo::DObject *object0, const Geo::Point &point0, Geo::DObject *object1, const Geo::Point &point1, const double radius)
{
    std::vector<Geo::DObject *> objects;
    switch (object0->type())
    {
    case Geo::Type::POLYLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            {
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::Polyline *result2 = new Geo::Polyline();
                Geo::Polyline *result3 = new Geo::Polyline();
                Geo::Arc *arc = new Geo::Arc();
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object0), point0, *static_cast<Geo::Polyline *>(object1), point1, radius,
                                *arc, *result0, *result1, *result2, *result3))
                {
                    objects.push_back(arc);
                    objects.push_back(result0);
                    objects.push_back(result1);
                    if (result2->size() > 1)
                    {
                        objects.push_back(result2);
                    }
                    else
                    {
                        delete result2;
                    }
                    if (result3->size() > 1)
                    {
                        objects.push_back(result3);
                    }
                    else
                    {
                        delete result3;
                    }
                }
                else
                {
                    delete arc;
                    delete result0;
                    delete result1;
                    delete result2;
                    delete result3;
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Arc *result2 = new Geo::Arc();
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object0), point0, *static_cast<Geo::Arc *>(object1), point1, radius,
                                *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result0;
                    delete result1;
                    delete result2;
                    delete result_arc;
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::CubicBezier *result2 = new Geo::CubicBezier();
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object0), point0, *static_cast<Geo::CubicBezier *>(object1), point1, radius,
                                *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result_arc;
                    delete result0;
                    delete result1;
                    delete result2;
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::BSpline *result2 = nullptr;
                const bool is_cubic = dynamic_cast<Geo::CubicBSpline *>(object1) != nullptr;
                if (is_cubic)
                {
                    result2 = new Geo::CubicBSpline();
                }
                else
                {
                    result2 = new Geo::QuadBSpline();
                }
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object0), point0, *static_cast<Geo::BSpline *>(object1), point1, is_cubic,
                                radius, *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result_arc;
                    delete result0;
                    delete result1;
                    delete result2;
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::ARC:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            {
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Arc *result2 = new Geo::Arc();
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object1), point1, *static_cast<Geo::Arc *>(object0), point0, radius,
                                *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result0;
                    delete result1;
                    delete result2;
                    delete result_arc;
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Arc *arc0 = new Geo::Arc();
                Geo::Arc *arc1 = new Geo::Arc();
                Geo::Arc *arc2 = new Geo::Arc();
                if (Geo::fillet(*static_cast<Geo::Arc *>(object0), point0, *static_cast<Geo::Arc *>(object1), point1, radius, *arc0, *arc1,
                                *arc2))
                {
                    objects.push_back(arc2);
                    objects.push_back(arc0);
                    objects.push_back(arc1);
                }
                else
                {
                    delete arc0;
                    delete arc1;
                    delete arc2;
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::BEZIER:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            {
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::CubicBezier *result2 = new Geo::CubicBezier();
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object1), point1, *static_cast<Geo::CubicBezier *>(object0), point0, radius,
                                *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result_arc;
                    delete result0;
                    delete result1;
                    delete result2;
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                Geo::CubicBezier *result0 = new Geo::CubicBezier();
                Geo::CubicBezier *result1 = new Geo::CubicBezier();
                Geo::Arc *arc = new Geo::Arc();
                if (Geo::fillet(*static_cast<Geo::CubicBezier *>(object0), point0, *static_cast<Geo::CubicBezier *>(object1), point1,
                                radius, *arc, *result0, *result1))
                {
                    objects.push_back(arc);
                    objects.push_back(result0);
                    objects.push_back(result1);
                }
                else
                {
                    delete arc;
                    delete result0;
                    delete result1;
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::BSPLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            {
                Geo::Arc *result_arc = new Geo::Arc();
                Geo::Polyline *result0 = new Geo::Polyline();
                Geo::Polyline *result1 = new Geo::Polyline();
                Geo::BSpline *result2 = nullptr;
                const bool is_cubic = dynamic_cast<Geo::CubicBSpline *>(object0) != nullptr;
                if (is_cubic)
                {
                    result2 = new Geo::CubicBSpline();
                }
                else
                {
                    result2 = new Geo::QuadBSpline();
                }
                if (Geo::fillet(*static_cast<Geo::Polyline *>(object1), point1, *static_cast<Geo::BSpline *>(object0), point0, is_cubic,
                                radius, *result_arc, *result0, *result1, *result2))
                {
                    objects.push_back(result_arc);
                    objects.push_back(result0);
                    if (result1->size() > 1)
                    {
                        objects.push_back(result1);
                    }
                    else
                    {
                        delete result1;
                    }
                    objects.push_back(result2);
                }
                else
                {
                    delete result_arc;
                    delete result0;
                    delete result1;
                    delete result2;
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (objects.empty())
    {
        return false;
    }
    bool added = false;
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, k = 0, count = _graph->container_group(_current_group).size(); i < count && k < 2; ++i)
    {
        if (_graph->container_group(_current_group)[i] == object0)
        {
            ++k;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            object0->is_selected = false;
            remove_items.emplace_back(object0, _current_group, i);
            --count;
            size_t j = i - 1;
            while ((added && !objects.empty()) || objects.size() > 1)
            {
                add_items.emplace_back(objects.back(), _current_group, ++j);
                _graph->container_group(_current_group).insert(j, objects.back());
                _view_tree.append(objects.back());
                objects.back()->is_selected = false;
                objects.pop_back();
                ++count;
            }
            added = true;
        }
        else if (_graph->container_group(_current_group)[i] == object1)
        {
            ++k;
            _view_tree.remove(_graph->container_group(_current_group).pop(i));
            object1->is_selected = false;
            remove_items.emplace_back(object1, _current_group, i);
            --count;
            size_t j = i - 1;
            while ((added && !objects.empty()) || objects.size() > 1)
            {
                add_items.emplace_back(objects.back(), _current_group, ++j);
                _graph->container_group(_current_group).insert(j, objects.back());
                _view_tree.append(objects.back());
                objects.back()->is_selected = false;
                objects.pop_back();
                ++count;
            }
            added = true;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    _graph->modified = true;
    return true;
}

bool Editor::chamfer(Geo::Polygon *shape, const Geo::Point &point, const double distance)
{
    if (distance <= 0)
    {
        return false;
    }
    Geo::Polygon &polygon = *shape;
    std::vector<Geo::Point>::const_iterator it = std::find(polygon.begin(), polygon.end(), point);
    if (it == polygon.end())
    {
        return false;
    }
    const size_t index1 = std::distance(polygon.cbegin(), it);
    const size_t index0 = index1 > 0 ? index1 - 1 : polygon.size() - 2;
    const size_t index2 = index1 + 1;

    if (Geo::distance(polygon[index1], polygon[index0]) >= distance && Geo::distance(polygon[index1], polygon[index2]) >= distance)
    {
        std::vector<std::tuple<double, double>> tuple_shape;
        for (const Geo::Point &point : polygon)
        {
            tuple_shape.emplace_back(point.x, point.y);
        }
        _backup.push_command(new UndoStack::ChangeShapeCommand(shape, tuple_shape));

        if (Geo::distance(polygon[index1], polygon[index2]) > distance)
        {
            if (index1 > 0)
            {
                Geo::Point temp = polygon[index1] + (polygon[index2] - polygon[index1]).normalize() * distance;
                polygon.insert(index2, temp);
            }
            else
            {
                polygon.front() = polygon[index1] + (polygon[index2] - polygon[index1]).normalize() * distance;
            }
        }

        if (Geo::distance(polygon[index1], polygon[index0]) > distance)
        {
            if (index1 > 0)
            {
                polygon[index1] += (polygon[index0] - polygon[index1]).normalize() * distance;
            }
            else
            {
                polygon.insert(index0 + 1, polygon.back() + (polygon[index0] - polygon.back()).normalize() * distance);
                polygon.back() = polygon.front();
            }
        }
        else
        {
            polygon.remove(index1);
        }

        _view_tree.update(shape);
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editor::chamfer(Geo::Polyline *polyline, const Geo::Point &point, const double distance)
{
    if (distance <= 0 || point == polyline->front() || point == polyline->back())
    {
        return false;
    }
    std::vector<Geo::Point>::const_iterator it = std::find(polyline->begin(), polyline->end(), point);
    if (it == polyline->end())
    {
        return false;
    }
    const size_t index = std::distance(polyline->cbegin(), it);
    if (Geo::distance((*polyline)[index - 1], (*polyline)[index]) >= distance &&
        Geo::distance((*polyline)[index + 1], (*polyline)[index]) >= distance)
    {
        std::vector<std::tuple<double, double>> tuple_shape;
        for (const Geo::Point &point : *polyline)
        {
            tuple_shape.emplace_back(point.x, point.y);
        }
        _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, tuple_shape));

        if (Geo::distance((*polyline)[index + 1], (*polyline)[index]) > distance)
        {
            Geo::Point temp = (*polyline)[index] + ((*polyline)[index + 1] - (*polyline)[index]).normalize() * distance;
            polyline->insert(index + 1, temp);
        }

        if (Geo::distance((*polyline)[index - 1], (*polyline)[index]) > distance)
        {
            (*polyline)[index] += ((*polyline)[index - 1] - (*polyline)[index]).normalize() * distance;
        }
        else
        {
            polyline->remove(index);
        }

        _view_tree.update(polyline);
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editor::split(Geo::DObject *object, const Geo::Point &pos)
{
    switch (object->type())
    {
    case Geo::Type::POLYLINE:
        {
            Geo::Polyline *polyline = static_cast<Geo::Polyline *>(object);
            std::vector<Geo::Point> coords;
            Geo::closest_point(*polyline, pos, coords);
            if (Geo::Polyline polyline0, polyline1; Geo::split(*polyline, coords.front(), polyline0, polyline1))
            {
                std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _view_tree.remove(_graph->container_group(_current_group).pop(index));
                _graph->container_group(_current_group).insert(index, new Geo::Polyline(polyline1));
                _graph->container_group(_current_group).insert(index, new Geo::Polyline(polyline0));
                _view_tree.append(_graph->container_group(_current_group)[index]);
                _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                return true;
            }
        }
        break;
    case Geo::Type::BEZIER:
        {
            Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(object);
            std::vector<Geo::Point> coords;
            std::vector<std::tuple<size_t, double, double, double>> tvalues;
            Geo::closest_point(*bezier, pos, coords, &tvalues);
            if (Geo::CubicBezier bezier0, bezier1;
                !tvalues.empty() && Geo::split(*bezier, std::get<0>(tvalues.front()), std::get<1>(tvalues.front()), bezier0, bezier1))
            {
                std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _view_tree.remove(_graph->container_group(_current_group).pop(index));
                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier1));
                _graph->container_group(_current_group).insert(index, new Geo::CubicBezier(bezier0));
                _view_tree.append(_graph->container_group(_current_group)[index]);
                _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                return true;
            }
        }
        break;
    case Geo::Type::BSPLINE:
        {
            Geo::CubicBSpline *cubicbspline = dynamic_cast<Geo::CubicBSpline *>(object);
            Geo::QuadBSpline *quadbspline = dynamic_cast<Geo::QuadBSpline *>(object);
            const bool is_cubic = cubicbspline;
            std::vector<Geo::Point> coords;
            std::vector<std::tuple<double, double, double>> tvalues;
            Geo::closest_point(*static_cast<Geo::BSpline *>(object), is_cubic, pos, coords, &tvalues);
            if (is_cubic)
            {
                if (Geo::CubicBSpline bspline0(*cubicbspline), bspline1(*cubicbspline);
                    !tvalues.empty() &&
                    Geo::split(*static_cast<Geo::BSpline *>(object), is_cubic, std::get<0>(tvalues.front()), bspline0, bspline1))
                {
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                    remove.emplace_back(object, _current_group, index);
                    _view_tree.remove(_graph->container_group(_current_group).pop(index));
                    _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                    _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                    _view_tree.append(_graph->container_group(_current_group)[index]);
                    _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                    append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                    _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                    return true;
                }
            }
            else
            {
                if (Geo::QuadBSpline bspline0(*quadbspline), bspline1(*quadbspline);
                    !tvalues.empty() &&
                    Geo::split(*static_cast<Geo::BSpline *>(object), is_cubic, std::get<0>(tvalues.front()), bspline0, bspline1))
                {
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                    size_t index = std::distance(
                        _graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                    remove.emplace_back(object, _current_group, index);
                    _view_tree.remove(_graph->container_group(_current_group).pop(index));
                    _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                    _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
                    _view_tree.append(_graph->container_group(_current_group)[index]);
                    _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                    append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                    _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                    return true;
                }
            }
        }
        break;
    case Geo::Type::ARC:
        {
            Geo::Arc *arc = static_cast<Geo::Arc *>(object);
            Geo::Arc arc0, arc1;
            if (Geo::Point point0, point1;
                Geo::is_intersected(pos, Geo::Point(arc->x, arc->y), *arc, point0, point1, false) && Geo::split(*arc, point0, arc0, arc1))
            {
                std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _view_tree.remove(_graph->container_group(_current_group).pop(index));
                _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
                _view_tree.append(_graph->container_group(_current_group)[index]);
                _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                return true;
            }
        }
        break;
    case Geo::Type::ELLIPSE:
        {
            Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(object);
            Geo::Ellipse ellipse0, ellipse1;
            if (Geo::Point point0, point1; Geo::is_intersected(ellipse->center(), pos, *ellipse, point0, point1, false) &&
                                           Geo::split(*ellipse, point0, ellipse0, ellipse1))
            {
                std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove, append;
                size_t index = std::distance(
                    _graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _view_tree.remove(_graph->container_group(_current_group).pop(index));
                _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse1));
                _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse0));
                _view_tree.append(_graph->container_group(_current_group)[index]);
                _view_tree.append(_graph->container_group(_current_group)[index + 1]);
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                return true;
            }
        }
        break;
    default:
        break;
    }
    return false;
}

bool Editor::line_array(const std::vector<Geo::DObject *> &objects, int x, int y, double x_space, double y_space)
{
    if (objects.empty() || x == 0 || y == 0 || (x == 1 && y == 1))
    {
        return false;
    }

    double left = DBL_MAX, right = -DBL_MAX, top = -DBL_MAX, bottom = DBL_MAX;
    for (const Geo::DObject *object : objects)
    {
        if (object->type() != Geo::Type::DIMENSION)
        {
            const Geo::AABBRect rect = object->aabbrect();
            left = std::min(rect.left, left);
            right = std::max(rect.right, right);
            top = std::max(rect.top, top);
            bottom = std::min(rect.bottom, bottom);
        }
    }

    x_space += (right - left);
    if (x < 0)
    {
        x_space = -x_space;
        x = -x;
    }
    y_space += (top - bottom);
    if (y < 0)
    {
        y_space = -y_space;
        y = -y;
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (int i = 0; i < x; ++i)
    {
        for (int j = 0; j < y; ++j)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }
            for (Geo::DObject *object : objects)
            {
                if (object->type() != Geo::Type::DIMENSION)
                {
                    _graph->container_group(_current_group).append(object->clone());
                    _graph->container_group(_current_group).back()->translate(x_space * i, y_space * j);
                    _graph->container_group(_current_group).back()->is_selected = true;
                    _view_tree.append(_graph->container_group(_current_group).back());
                    items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
                }
            }
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

bool Editor::ring_array(const std::vector<Geo::DObject *> &objects, const double x, const double y, const int n)
{
    if (n <= 1 || objects.empty())
    {
        return false;
    }

    for (Geo::DObject *obj : objects)
    {
        if (obj->type() != Geo::Type::DIMENSION)
        {
            obj->is_selected = true;
        }
    }

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (int i = 1; i < n; ++i)
    {
        for (Geo::DObject *obj : objects)
        {
            if (obj->type() != Geo::Type::DIMENSION)
            {
                _graph->container_group(_current_group).append(obj->clone());
                _graph->container_group(_current_group).back()->rotate(x, y, 2 * Geo::PI * i / n);
                _graph->container_group(_current_group).back()->is_selected = true;
                _view_tree.append(_graph->container_group(_current_group).back());
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

void Editor::up(Geo::DObject *item)
{
    for (size_t i = 0, count = _graph->container_group(_current_group).size() - 1; i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == item)
        {
            _graph->container_group(_current_group).pop(i);
            _graph->container_group(_current_group).append(item);
            break;
        }
    }
}

void Editor::down(Geo::DObject *item)
{
    for (size_t i = 1, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == item)
        {
            _graph->container_group(_current_group).pop(i);
            _graph->container_group(_current_group).insert(0, item);
            break;
        }
    }
}

void Editor::rotate(const std::vector<Geo::DObject *> &objects, const double x, const double y, const double rad)
{
    std::vector<Geo::DObject *> items;
    for (Geo::DObject *geo : objects)
    {
        if (geo->type() != Geo::Type::DIMENSION)
        {
            geo->rotate(x, y, rad);
            items.push_back(geo);
        }
    }
    _view_tree.update(items);
    _graph->modified = true;
    _backup.push_command(new UndoStack::RotateCommand(items, x, y, rad));
}

void Editor::flip(std::vector<Geo::DObject *> objects, const bool direction, const bool unitary, const bool all_layers)
{
    std::vector<Geo::DObject *> items;
    Geo::Point coord;
    if (objects.empty())
    {
        if (unitary)
        {
            if (all_layers)
            {
                {
                    const Geo::AABBRect rect = _graph->aabbrect();
                    coord.x = (rect.left + rect.right) / 2;
                    coord.y = (rect.top + rect.bottom) / 2;
                }
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::DObject *geo : group)
                    {
                        if (geo->type() == Geo::Type::DIMENSION)
                        {
                            continue;
                        }
                        else
                        {
                            items.push_back(geo);
                        }
                        if (direction)
                        {
                            geo->translate(-coord.x, 0);
                            geo->transform(-1, 0, 0, 0, 1, 0);
                            geo->translate(coord.x, 0);
                        }
                        else
                        {
                            geo->translate(0, -coord.y);
                            geo->transform(1, 0, 0, 0, -1, 0);
                            geo->translate(0, coord.y);
                        }
                    }
                }
            }
            else
            {
                {
                    const Geo::AABBRect rect = _graph->container_group(_current_group).aabbrect();
                    coord.x = (rect.left + rect.right) / 2;
                    coord.y = (rect.top + rect.bottom) / 2;
                }
                for (Geo::DObject *geo : _graph->container_group(_current_group))
                {
                    if (geo->type() == Geo::Type::DIMENSION)
                    {
                        continue;
                    }
                    else
                    {
                        items.push_back(geo);
                    }
                    if (direction)
                    {
                        geo->translate(-coord.x, 0);
                        geo->transform(-1, 0, 0, 0, 1, 0);
                        geo->translate(coord.x, 0);
                    }
                    else
                    {
                        geo->translate(0, -coord.y);
                        geo->transform(1, 0, 0, 0, -1, 0);
                        geo->translate(0, coord.y);
                    }
                }
            }
        }
        else
        {
            if (all_layers)
            {
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::DObject *geo : group)
                    {
                        if (geo->type() == Geo::Type::DIMENSION)
                        {
                            continue;
                        }
                        else
                        {
                            items.push_back(geo);
                        }
                        {
                            const Geo::AABBRect rect = geo->aabbrect();
                            coord.x = (rect.left + rect.right) / 2;
                            coord.y = (rect.top + rect.bottom) / 2;
                        }
                        if (direction)
                        {
                            geo->translate(-coord.x, 0);
                            geo->transform(-1, 0, 0, 0, 1, 0);
                            geo->translate(coord.x, 0);
                        }
                        else
                        {
                            geo->translate(0, -coord.y);
                            geo->transform(1, 0, 0, 0, -1, 0);
                            geo->translate(0, coord.y);
                        }
                    }
                    objects.insert(objects.end(), group.begin(), group.end());
                }
            }
            else
            {
                for (Geo::DObject *geo : _graph->container_group(_current_group))
                {
                    if (geo->type() == Geo::Type::DIMENSION)
                    {
                        continue;
                    }
                    else
                    {
                        items.push_back(geo);
                    }
                    {
                        const Geo::AABBRect rect = geo->aabbrect();
                        coord.x = (rect.left + rect.right) / 2;
                        coord.y = (rect.top + rect.bottom) / 2;
                    }
                    if (direction)
                    {
                        geo->translate(-coord.x, 0);
                        geo->transform(-1, 0, 0, 0, 1, 0);
                        geo->translate(coord.x, 0);
                    }
                    else
                    {
                        geo->translate(0, -coord.y);
                        geo->transform(1, 0, 0, 0, -1, 0);
                        geo->translate(0, coord.y);
                    }
                }
                objects.assign(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end());
            }
        }
    }
    else
    {
        if (unitary)
        {
            double left = DBL_MAX, top = -DBL_MAX, right = -DBL_MAX, bottom = DBL_MAX;
            for (Geo::DObject *geo : objects)
            {
                if (geo->type() != Geo::Type::DIMENSION)
                {
                    items.push_back(geo);
                    const Geo::AABBRect rect = geo->aabbrect();
                    left = std::min(left, rect.left);
                    top = std::max(top, rect.top);
                    right = std::max(right, rect.right);
                    bottom = std::min(bottom, rect.bottom);
                }
            }
            coord.x = (left + right) / 2;
            coord.y = (top + bottom) / 2;

            for (Geo::DObject *geo : items)
            {
                if (direction)
                {
                    geo->translate(-coord.x, 0);
                    geo->transform(-1, 0, 0, 0, 1, 0);
                    geo->translate(coord.x, 0);
                }
                else
                {
                    geo->translate(0, -coord.y);
                    geo->transform(1, 0, 0, 0, -1, 0);
                    geo->translate(0, coord.y);
                }
            }
        }
        else
        {
            for (Geo::DObject *geo : objects)
            {
                if (geo->type() == Geo::Type::DIMENSION)
                {
                    continue;
                }
                else
                {
                    items.push_back(geo);
                }
                {
                    const Geo::AABBRect rect = geo->aabbrect();
                    coord.x = (rect.left + rect.right) / 2;
                    coord.y = (rect.top + rect.bottom) / 2;
                }
                if (direction)
                {
                    geo->translate(-coord.x, 0);
                    geo->transform(-1, 0, 0, 0, 1, 0);
                    geo->translate(coord.x, 0);
                }
                else
                {
                    geo->translate(0, -coord.y);
                    geo->transform(1, 0, 0, 0, -1, 0);
                    geo->translate(0, coord.y);
                }
            }
        }
    }

    _view_tree.update(items);
    _graph->modified = true;
    _backup.push_command(new UndoStack::FlipCommand(items, coord.x, coord.y, direction, unitary));
}

void Editor::trim(Geo::Polyline *polyline, const double x, const double y)
{
    Geo::Point anchor(x, y);
    double dis0 = Geo::distance_square(anchor, (*polyline)[0], (*polyline)[1]);
    size_t anchor_index = 1;
    for (size_t i = 2, count = polyline->size(); i < count; ++i)
    {
        if (double d = Geo::distance_square(anchor, (*polyline)[i - 1], (*polyline)[i]); d < dis0)
        {
            dis0 = d;
            anchor_index = i;
        }
    }
    if (dis0 > 4 / (_view_ratio * _view_ratio))
    {
        return; // 修剪位置不在线上
    }

    const Geo::Point head((*polyline)[anchor_index - 1]), tail((*polyline)[anchor_index]);
    Geo::foot_point(head, tail, Geo::Point(anchor), anchor);
    std::vector<Geo::Point> intersections;
    // 选中段两端点也按交点处理
    intersections.emplace_back(head);
    intersections.emplace_back(tail);
    // 找到自身交点
    for (size_t i = 1, count = polyline->size(); i < count; ++i)
    {
        if (i < anchor_index - 1 || i > anchor_index + 1)
        {
            if (Geo::Point point; Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], head, tail, point))
            {
                intersections.emplace_back(point);
            }
        }
    }
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], head, tail, point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (const Geo::Polyline *polyline2 = static_cast<const Geo::Polyline *>(object);
                polyline2 != polyline && Geo::is_intersected(polyline2->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polyline2->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected((*polyline2)[i - 1], (*polyline2)[i], head, tail, point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Circle *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Ellipse *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                Geo::is_intersected(bezier->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail, *bezier, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                Geo::is_intersected(bspline->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points;
                    Geo::is_intersected(head, tail, *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    // 在anchor左侧(认为从head到tail指向右)的点距离记为负值
    dis0 = DBL_MAX;
    double dis1 = DBL_MAX;
    Geo::Point point0, point1; // 要插入的两个点
    for (const Geo::Point &point : intersections)
    {
        const double d = Geo::distance_square(point, anchor);
        if (const double v = (tail - head) * (point - anchor); v > 0) // 在anchor右侧
        {
            if (d < dis1)
            {
                dis1 = d;
                point1 = point;
            }
        }
        else if (v < 0) // 在anchor左侧
        {
            if (d < dis0)
            {
                dis0 = d;
                point0 = point;
            }
        }
    }

    if (point0 == head && point1 == tail)
    {
        if (head == polyline->front() && tail == polyline->back())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polyline);
                    _backup.push_command(new UndoStack::ObjectCommand(remove_items, false));
                    break;
                }
            }
        }
        else if (head == polyline->front())
        {
            std::vector<std::tuple<double, double>> shape;
            for (const Geo::Point &point : *polyline)
            {
                shape.emplace_back(point.x, point.y);
            }
            _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));
            polyline->remove(0);
            _view_tree.update(polyline);
        }
        else if (tail == polyline->back())
        {
            std::vector<std::tuple<double, double>> shape;
            for (const Geo::Point &point : *polyline)
            {
                shape.emplace_back(point.x, point.y);
            }
            _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));
            polyline->remove(polyline->size() - 1);
            _view_tree.update(polyline);
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index, polyline->end());
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polyline);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
                    _view_tree.append(polyline1);
                    _view_tree.append(polyline0);
                    add_items.emplace_back(polyline0, _current_group, i);
                    add_items.emplace_back(polyline1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
    else if (point0 == head)
    {
        if (head == polyline->front())
        {
            std::vector<std::tuple<double, double>> shape;
            for (const Geo::Point &point : *polyline)
            {
                shape.emplace_back(point.x, point.y);
            }
            _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));
            polyline->front() = point1;
            polyline->is_selected = false;
            _view_tree.update(polyline);
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index - 1, polyline->end());
            polyline1->front() = point1;
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polyline);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
                    _view_tree.append(polyline1);
                    _view_tree.append(polyline0);
                    add_items.emplace_back(polyline0, _current_group, i);
                    add_items.emplace_back(polyline1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
    else if (point1 == tail)
    {
        if (tail == polyline->back())
        {
            std::vector<std::tuple<double, double>> shape;
            for (const Geo::Point &point : *polyline)
            {
                shape.emplace_back(point.x, point.y);
            }
            _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));
            polyline->back() = point0;
            polyline->is_selected = false;
            _view_tree.update(polyline);
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index + 1);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index, polyline->end());
            polyline0->back() = point0;
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polyline);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
                    _view_tree.append(polyline1);
                    _view_tree.append(polyline0);
                    add_items.emplace_back(polyline0, _current_group, i);
                    add_items.emplace_back(polyline1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
    else
    {
        Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index + 1);
        Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index - 1, polyline->end());
        polyline0->back() = point0;
        polyline1->front() = point1;
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == polyline)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _view_tree.remove(polyline);
                _graph->container_group(_current_group).insert(i, polyline1);
                _graph->container_group(_current_group).insert(i, polyline0);
                _view_tree.append(polyline1);
                _view_tree.append(polyline0);
                add_items.emplace_back(polyline0, _current_group, i);
                add_items.emplace_back(polyline1, _current_group, i + 1);
                break;
            }
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
    _graph->modified = true;
}

void Editor::trim(Geo::Polygon *polygon, const double x, const double y)
{
    Geo::Point anchor(x, y);
    double dis0 = Geo::distance_square(anchor, (*polygon)[0], (*polygon)[1]);
    size_t anchor_index = 1;
    for (size_t i = 2, count = polygon->size(); i < count; ++i)
    {
        if (double d = Geo::distance_square(anchor, (*polygon)[i - 1], (*polygon)[i]); d < dis0)
        {
            dis0 = d;
            anchor_index = i;
        }
    }
    if (dis0 > 4 / (_view_ratio * _view_ratio))
    {
        return; // 修剪位置不在线上
    }

    const Geo::Point head((*polygon)[anchor_index - 1]), tail((*polygon)[anchor_index]);
    Geo::foot_point(head, tail, Geo::Point(anchor), anchor);
    std::vector<Geo::Point> intersections;
    // 选中段两端点也按交点处理
    intersections.emplace_back(head);
    intersections.emplace_back(tail);
    // 找到自身交点
    for (size_t i = 1, count = polygon->size(); i < count; ++i)
    {
        if (i < anchor_index - 1 || i > anchor_index + 1)
        {
            if (Geo::Point point; Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], head, tail, point))
            {
                intersections.emplace_back(point);
            }
        }
    }
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon2 = static_cast<const Geo::Polygon *>(object);
                polygon2 != polygon && Geo::is_intersected(polygon2->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polygon2->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected((*polygon2)[i - 1], (*polygon2)[i], head, tail, point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (const Geo::Polyline *polyline2 = static_cast<const Geo::Polyline *>(object);
                Geo::is_intersected(polyline2->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polyline2->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected((*polyline2)[i - 1], (*polyline2)[i], head, tail, point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Circle *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Ellipse *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                Geo::is_intersected(bezier->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail, *bezier, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                Geo::is_intersected(bspline->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points;
                    Geo::is_intersected(head, tail, *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    // 在anchor左侧(认为从head到tail指向右)的点距离记为负值
    dis0 = DBL_MAX;
    double dis1 = DBL_MAX;
    Geo::Point point0, point1; // 要插入的两个点
    for (const Geo::Point &point : intersections)
    {
        const double d = Geo::distance_square(point, anchor);
        if (const double v = (tail - head) * (point - anchor); v > 0) // 在anchor右侧
        {
            if (d < dis1)
            {
                dis1 = d;
                point1 = point;
            }
        }
        else if (v < 0) // 在anchor左侧
        {
            if (d < dis0)
            {
                dis0 = d;
                point0 = point;
            }
        }
    }

    if (point0 == head && point1 == tail)
    {
        if (head == polygon->front() && tail == polygon->back())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _backup.push_command(new UndoStack::ObjectCommand(remove_items, false));
                    break;
                }
            }
        }
        else if (head == polygon->front())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin() + 1, polygon->end());
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
        else if (tail == polygon->back())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin(), polygon->end() - 1);
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polygon->begin(), polygon->begin() + anchor_index);
            Geo::Polyline *polyline1 = new Geo::Polyline(polygon->begin() + anchor_index, polygon->end());
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
                    _view_tree.append(polyline1);
                    _view_tree.append(polyline0);
                    add_items.emplace_back(polyline0, _current_group, i);
                    add_items.emplace_back(polyline1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
    else if (point0 == head)
    {
        if (head == polygon->front())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin(), polygon->end());
                    polyline->front() = point1;
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin() + anchor_index - 1, polygon->end());
                    polyline->front() = point1;
                    polyline->append(polygon->begin(), polygon->begin() + anchor_index);
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
    }
    else if (point1 == tail)
    {
        if (tail == polygon->back())
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin(), polygon->end());
                    polyline->back() = point0;
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    Geo::Polyline *polyline = new Geo::Polyline(polygon->begin() + anchor_index, polygon->end());
                    polyline->append(polygon->begin(), polygon->begin() + anchor_index + 1);
                    polyline->back() = point0;
                    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(polygon);
                    _graph->container_group(_current_group).insert(i, polyline);
                    _view_tree.append(polyline);
                    add_items.emplace_back(polyline, _current_group, i);
                    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                    break;
                }
            }
        }
    }
    else
    {
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == polygon)
            {
                Geo::Polyline *polyline = new Geo::Polyline(polygon->begin() + anchor_index - 1, polygon->end());
                polyline->append(polygon->begin(), polygon->begin() + anchor_index + 1);
                polyline->back() = point0;
                polyline->front() = point1;
                std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _view_tree.remove(polygon);
                _graph->container_group(_current_group).insert(i, polyline);
                _view_tree.append(polyline);
                add_items.emplace_back(polyline, _current_group, i);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
    }
    _graph->modified = true;
}

void Editor::trim(Geo::CubicBezier *bezier, const double x, const double y)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    Geo::Point anchor(x, y);
    double anchor_t = 0;
    size_t anchor_index = 0;

    {
        std::vector<std::tuple<size_t, double>> temp;
        for (size_t i = 0, end = bezier->control_points.size() - order; i < end; i += order)
        {
            Geo::Polyline polyline;
            polyline.append(bezier->control_points[i]);
            double t = 0;
            while (t <= 1)
            {
                Geo::Point point;
                for (int j = 0; j <= order; ++j)
                {
                    point += (bezier->control_points[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
                }
                polyline.append(point);
                t += Geo::CubicBezier::default_step;
            }
            polyline.append(bezier->control_points[i + order]);
            Geo::down_sampling(polyline, Geo::CubicBezier::default_down_sampling_value);

            std::vector<Geo::Point> points;
            Geo::closest_point(polyline, anchor, points);
            temp.emplace_back(i, Geo::distance(anchor, points.front()));
        }
        std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
        anchor_index = std::get<0>(temp.front());

        double t = 0;
        double step = 1e-3, lower = 0, upper = 1;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        do
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (int j = 0; j <= order; ++j)
                {
                    coord += (bezier->control_points[j + anchor_index] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                }
                if (double dis = Geo::distance(anchor, coord); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        } while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        lower = std::max(0.0, t - 1e-3), upper = std::min(1.0, t + 1e-3);
        const std::function<double(const double)> f = [&](const double t)
        {
            Geo::Point coord;
            for (int j = 0; j <= order; ++j)
            {
                coord += (bezier->control_points[j + anchor_index] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            return Geo::distance(coord, anchor) * 1e9;
        };
        min_dis[0] = f(lower), min_dis[1] = f(t);
        while (lower > 0 && min_dis[0] < min_dis[1])
        {
            lower -= 0.001;
            min_dis[0] = f(lower);
        }
        lower = std::max(0.0, lower);
        min_dis[0] = f(upper);
        while (upper < 1.0 && min_dis[0] < min_dis[1])
        {
            upper += 0.001;
            min_dis[0] = f(upper);
        }
        upper = std::min(1.0, upper);
        t = Math::min_x_trichotomy(f, lower, upper);

        anchor.clear();
        for (int j = 0; j <= order; ++j)
        {
            anchor += (bezier->control_points[j + anchor_index] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
        }
        anchor_t = t;
    }

    std::vector<std::tuple<size_t, double, double, double>> tvalues; // index, t, x, y
    // 找到自身交点
    const Geo::CubicBezier anchor_bezier(bezier->control_points.begin() + anchor_index,
                                         bezier->control_points.begin() + anchor_index + order + 1, false);
    /*for (size_t i = 0, end = bezier->control_points.size() - order; i < end; i += order)
    {
        if (i == anchor_index)
        {
            continue;
        }
        Geo::Bezier temp_bezier(bezier->control_points.begin() + i, bezier->control_points.begin() + i + order + 1, order, false);
        std::vector<Geo::Point> temp;
        Geo::is_intersected(anchor_bezier, temp_bezier, temp, &tvalues);
    }*/
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], anchor_bezier, temp, false, &tvalues);
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], anchor_bezier, temp, false, &tvalues);
                }
            }
            break;
        case Geo::Type::CIRCLE:
            Geo::is_intersected(*static_cast<const Geo::Circle *>(object), anchor_bezier, temp, &tvalues);
            break;
        case Geo::Type::ELLIPSE:
            Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object), anchor_bezier, temp, &tvalues);
            break;
        case Geo::Type::BEZIER:
            if (object != bezier)
            {
                Geo::is_intersected(anchor_bezier, *static_cast<const Geo::CubicBezier *>(object), temp, &tvalues);
            }
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(anchor_bezier, *static_cast<const Geo::BSpline *>(object), dynamic_cast<const Geo::CubicBSpline *>(object),
                                temp, &tvalues);
            break;
        case Geo::Type::ARC:
            Geo::is_intersected(*static_cast<const Geo::Arc *>(object), anchor_bezier, temp, &tvalues);
            break;
        default:
            break;
        }
    }

    std::sort(tvalues.begin(), tvalues.end(), [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    while (!tvalues.empty() && std::get<1>(tvalues.back()) == 1)
    {
        tvalues.pop_back();
    }
    while (!tvalues.empty() && std::get<1>(tvalues.front()) == 0)
    {
        tvalues.erase(tvalues.begin());
    }

    if (tvalues.empty())
    {
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bezier)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _view_tree.remove(bezier);
                Geo::CubicBezier *bezier0 =
                    new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
                Geo::CubicBezier *bezier1 =
                    new Geo::CubicBezier(bezier->control_points.begin() + anchor_index + order, bezier->control_points.end(), false);
                if (bezier0->control_points.size() > order)
                {
                    _graph->container_group(_current_group).insert(i, bezier0);
                    _view_tree.append(bezier0);
                    add_items.emplace_back(bezier0, _current_group, i++);
                }
                else
                {
                    delete bezier0;
                }
                if (bezier1->control_points.size() > order)
                {
                    _graph->container_group(_current_group).insert(i, bezier1);
                    _view_tree.append(bezier1);
                    add_items.emplace_back(bezier1, _current_group, i);
                }
                else
                {
                    delete bezier1;
                }
                break;
            }
        }
        return _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }

    double left_t = std::get<1>(tvalues.front()), right_t = std::get<1>(tvalues.back());
    for (size_t i = 1, count = tvalues.size(); i < count; ++i)
    {
        if (std::get<1>(tvalues[i - 1]) < anchor_t && anchor_t < std::get<1>(tvalues[i]))
        {
            left_t = std::get<1>(tvalues[i - 1]);
            right_t = std::get<1>(tvalues[i]);
            break;
        }
    }

    if (left_t == right_t)
    {
        Geo::CubicBezier bezier_left, bezier_right;
        Geo::split(anchor_bezier, 0, left_t, bezier_left, bezier_right);
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        Geo::CubicBezier *bezier0 = nullptr, *bezier1 = nullptr;
        if (anchor_t < left_t)
        {
            bezier0 = new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
            bezier1 = new Geo::CubicBezier(bezier_right);
            bezier1->control_points.insert(bezier1->control_points.end(), bezier->control_points.begin() + anchor_index + order + 1,
                                           bezier->control_points.end());
            bezier1->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
        }
        else
        {
            bezier0 = new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
            bezier0->control_points.insert(bezier0->control_points.end(), bezier_left.control_points.begin() + 1,
                                           bezier_left.control_points.end());
            bezier0->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
            bezier1 = new Geo::CubicBezier(bezier->control_points.begin() + anchor_index + order, bezier->control_points.end(), false);
        }
        if (bezier0->control_points.size() <= order)
        {
            delete bezier0;
            bezier0 = nullptr;
        }
        if (bezier1->control_points.size() <= order)
        {
            delete bezier1;
            bezier1 = nullptr;
        }
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bezier)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _view_tree.remove(bezier);
                if (bezier0 != nullptr)
                {
                    _graph->container_group(_current_group).insert(i, bezier0);
                    _view_tree.append(bezier0);
                    add_items.emplace_back(bezier0, _current_group, i++);
                }
                if (bezier1 != nullptr)
                {
                    _graph->container_group(_current_group).insert(i, bezier1);
                    _view_tree.append(bezier1);
                    add_items.emplace_back(bezier1, _current_group, i);
                }
                break;
            }
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
    else
    {
        if (anchor_t < left_t)
        {
            Geo::CubicBezier bezier_left, bezier_right;
            Geo::split(anchor_bezier, 0, left_t, bezier_left, bezier_right);
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bezier);
                    Geo::CubicBezier *bezier0 =
                        new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
                    Geo::CubicBezier *bezier1 = new Geo::CubicBezier(bezier_right);
                    bezier1->control_points.insert(bezier1->control_points.end(), bezier->control_points.begin() + anchor_index + order + 1,
                                                   bezier->control_points.end());
                    bezier1->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
                    if (bezier0->control_points.size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier0);
                        _view_tree.append(bezier0);
                        add_items.emplace_back(bezier0, _current_group, i++);
                    }
                    else
                    {
                        delete bezier0;
                    }
                    if (bezier1->control_points.size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier1);
                        _view_tree.append(bezier1);
                        add_items.emplace_back(bezier1, _current_group, i);
                    }
                    else
                    {
                        delete bezier1;
                    }
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
        else if (anchor_t > right_t)
        {
            Geo::CubicBezier bezier_left, bezier_right;
            Geo::split(anchor_bezier, 0, right_t, bezier_left, bezier_right);
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bezier);
                    Geo::CubicBezier *bezier0 =
                        new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
                    bezier0->control_points.insert(bezier0->control_points.end(), bezier_left.control_points.begin() + 1,
                                                   bezier_left.control_points.end());
                    bezier0->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
                    Geo::CubicBezier *bezier1 =
                        new Geo::CubicBezier(bezier->control_points.begin() + anchor_index + order, bezier->control_points.end(), false);
                    if (bezier0->control_points.size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier0);
                        _view_tree.append(bezier0);
                        add_items.emplace_back(bezier0, _current_group, i++);
                    }
                    else
                    {
                        delete bezier0;
                    }
                    if (bezier1->control_points.size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier1);
                        _view_tree.append(bezier1);
                        add_items.emplace_back(bezier1, _current_group, i);
                    }
                    else
                    {
                        delete bezier1;
                    }
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
        else
        {
            Geo::CubicBezier bezier_left, bezier_right, temp_bezier;
            Geo::split(anchor_bezier, 0, left_t, bezier_left, temp_bezier);
            Geo::split(anchor_bezier, 0, right_t, temp_bezier, bezier_right);
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bezier);
                    Geo::CubicBezier *bezier0 =
                        new Geo::CubicBezier(bezier->control_points.begin(), bezier->control_points.begin() + anchor_index + 1, false);
                    bezier0->control_points.insert(bezier0->control_points.end(), bezier_left.control_points.begin() + 1,
                                                   bezier_left.control_points.end());
                    bezier0->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
                    Geo::CubicBezier *bezier1 = new Geo::CubicBezier(bezier_right);
                    bezier1->control_points.insert(bezier1->control_points.end(), bezier->control_points.begin() + anchor_index + order + 1,
                                                   bezier->control_points.end());
                    bezier1->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
                    _graph->container_group(_current_group).insert(i, bezier1);
                    _graph->container_group(_current_group).insert(i, bezier0);
                    _view_tree.append(bezier1);
                    _view_tree.append(bezier0);
                    add_items.emplace_back(bezier0, _current_group, i);
                    add_items.emplace_back(bezier1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
}

void Editor::trim(Geo::BSpline *bspline, const double x, const double y)
{
    if (bspline == nullptr)
    {
        return;
    }
    const bool is_cubic = dynamic_cast<const Geo::CubicBSpline *>(bspline) != nullptr;
    const std::vector<double> &knots = bspline->knots();
    const size_t npts = bspline->control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    const size_t p1 = std::max(npts * 8.0, bspline->shape().length() / Geo::BSpline::default_step);
    Geo::Point anchor(x, y);

    double anchor_t = knots[0];
    {
        double step = (knots[nplusc - 1] - anchor_t) / (p1 - 1);
        std::vector<double> temp;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        while (anchor_t <= knots[nplusc - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, anchor_t, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline->control_points[i] * nbasis[i];
            }
            if (double dis = Geo::distance(coord, anchor); dis < min_dis[0])
            {
                temp.clear();
                min_dis[0] = dis;
                temp.push_back(anchor_t);
            }
            else if (dis == min_dis[0])
            {
                temp.push_back(anchor_t);
            }
            anchor_t += step;
        }

        std::vector<std::tuple<double, double, Geo::Point>> result; // dis, t, point
        for (double v : temp)
        {
            step = 1e-3;
            double lower = knots[0], upper = knots[nplusc - 1];
            min_dis[0] = min_dis[1] = DBL_MAX;
            do
            {
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    std::vector<double> nbasis;
                    Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                    Geo::Point coord;
                    for (size_t i = 0; i < npts; ++i)
                    {
                        coord += bspline->control_points[i] * nbasis[i];
                    }
                    if (double dis = Geo::distance(anchor, coord); dis < min_dis[1])
                    {
                        min_dis[1] = dis;
                        v = x;
                    }
                }
                lower = std::max(0.0, v - step);
                upper = std::min(1.0, v + step);
                step = (upper - lower) / 100;
                if (min_dis[0] > min_dis[1])
                {
                    min_dis[0] = min_dis[1];
                }
            } while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

            lower = std::max(knots[0], v - 1e-3), upper = std::min(knots[nplusc - 1], v + 1e-3);
            const std::function<double(const double)> f = [&](const double t)
            {
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline->control_points[i] * nbasis[i];
                }
                return Geo::distance(coord, anchor) * 1e9;
            };
            min_dis[0] = f(lower), min_dis[1] = f(v);
            while (lower > knots[0] && min_dis[0] < min_dis[1])
            {
                lower -= 1e-3;
                min_dis[0] = f(lower);
            }
            lower = std::max(knots[0], lower);
            min_dis[0] = f(upper);
            while (upper < knots[nplusc - 1] && min_dis[0] < min_dis[1])
            {
                upper += 1e-3;
                min_dis[0] = f(upper);
            }
            upper = std::min(knots[nplusc - 1], upper);
            v = Math::min_x_trichotomy(f, lower, upper);

            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, v, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline->control_points[i] * nbasis[i];
            }
            result.emplace_back(f(v), v, coord);
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });
        anchor_t = std::get<1>(result.front());
        anchor = std::get<2>(result.front());
    }

    std::vector<std::tuple<double, double, double>> tvalues; // t, x, y
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], *bspline, is_cubic, temp, false, &tvalues);
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], *bspline, is_cubic, temp, false, &tvalues);
                }
            }
            break;
        case Geo::Type::CIRCLE:
            Geo::is_intersected(*static_cast<const Geo::Circle *>(object), *bspline, is_cubic, temp, &tvalues);
            break;
        case Geo::Type::ELLIPSE:
            Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object), *bspline, is_cubic, temp, &tvalues);
            break;
        case Geo::Type::BEZIER:
            Geo::is_intersected(*static_cast<const Geo::CubicBezier *>(object), *bspline, is_cubic, temp, nullptr, &tvalues);
            break;
        case Geo::Type::BSPLINE:
            if (object != bspline)
            {
                Geo::is_intersected(*bspline, is_cubic, *static_cast<const Geo::BSpline *>(object),
                                    dynamic_cast<const Geo::CubicBSpline *>(object), temp, &tvalues, nullptr);
            }
            break;
        case Geo::Type::ARC:
            Geo::is_intersected(*static_cast<const Geo::Arc *>(object), *bspline, is_cubic, temp, &tvalues);
            break;
        default:
            break;
        }
    }

    std::sort(tvalues.begin(), tvalues.end(), [](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });
    while (!tvalues.empty() && std::get<0>(tvalues.back()) == 1)
    {
        tvalues.pop_back();
    }
    while (!tvalues.empty() && std::get<0>(tvalues.front()) == 0)
    {
        tvalues.erase(tvalues.begin());
    }
    double left_t = std::get<0>(tvalues.front()), right_t = std::get<0>(tvalues.back());
    for (size_t i = 1, count = tvalues.size(); i < count; ++i)
    {
        if (std::get<0>(tvalues[i - 1]) <= anchor_t && anchor_t <= std::get<0>(tvalues[i]))
        {
            left_t = std::get<0>(tvalues[i - 1]);
            right_t = std::get<0>(tvalues[i]);
            break;
        }
    }

    if (left_t == right_t)
    {
        Geo::BSpline *result = nullptr;
        if (is_cubic)
        {
            Geo::CubicBSpline bspline_left(*static_cast<const Geo::CubicBSpline *>(bspline)),
                bspline_right(*static_cast<const Geo::CubicBSpline *>(bspline));
            if (Geo::split(*bspline, true, left_t, bspline_left, bspline_right))
            {
                result = new Geo::CubicBSpline(anchor_t < left_t ? bspline_right : bspline_left);
            }
            else
            {
                return;
            }
        }
        else
        {
            Geo::QuadBSpline bspline_left(*static_cast<const Geo::QuadBSpline *>(bspline)),
                bspline_right(*static_cast<const Geo::QuadBSpline *>(bspline));
            if (Geo::split(*bspline, false, left_t, bspline_left, bspline_right))
            {
                result = new Geo::QuadBSpline(anchor_t < left_t ? bspline_right : bspline_left);
            }
            else
            {
                return;
            }
        }
        std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bspline)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _view_tree.remove(bspline);
                _graph->container_group(_current_group).insert(i, result);
                _view_tree.append(result);
                add_items.emplace_back(result, _current_group, i);
                break;
            }
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
    else
    {
        if (anchor_t < left_t)
        {
            Geo::BSpline *result = nullptr;
            if (is_cubic)
            {
                Geo::CubicBSpline bspline_left(*static_cast<const Geo::CubicBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::CubicBSpline *>(bspline));
                if (Geo::split(*bspline, true, left_t, bspline_left, bspline_right))
                {
                    result = new Geo::CubicBSpline(bspline_right);
                }
                else
                {
                    return;
                }
            }
            else
            {
                Geo::QuadBSpline bspline_left(*static_cast<const Geo::QuadBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::QuadBSpline *>(bspline));
                if (Geo::split(*bspline, false, left_t, bspline_left, bspline_right))
                {
                    result = new Geo::QuadBSpline(bspline_right);
                }
                else
                {
                    return;
                }
            }
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bspline);
                    _graph->container_group(_current_group).insert(i, result);
                    _view_tree.append(result);
                    add_items.emplace_back(result, _current_group, i++);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
        else if (anchor_t > right_t)
        {
            Geo::BSpline *result = nullptr;
            if (is_cubic)
            {
                Geo::CubicBSpline bspline_left(*static_cast<const Geo::CubicBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::CubicBSpline *>(bspline));
                Geo::split(*bspline, true, right_t, bspline_left, bspline_right);
                result = new Geo::CubicBSpline(bspline_left);
            }
            else
            {
                Geo::QuadBSpline bspline_left(*static_cast<const Geo::QuadBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::QuadBSpline *>(bspline));
                Geo::split(*bspline, false, right_t, bspline_left, bspline_right);
                result = new Geo::QuadBSpline(bspline_left);
            }
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bspline);
                    _graph->container_group(_current_group).insert(i, result);
                    _view_tree.append(result);
                    add_items.emplace_back(result, _current_group, i++);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
        else
        {
            Geo::BSpline *result_left = nullptr, *result_right = nullptr;
            if (is_cubic)
            {
                Geo::CubicBSpline bspline_left(*static_cast<const Geo::CubicBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::CubicBSpline *>(bspline)),
                    temp_bspline(*static_cast<const Geo::CubicBSpline *>(bspline));
                if (Geo::split(*bspline, true, left_t, bspline_left, temp_bspline) &&
                    Geo::split(*bspline, true, right_t, temp_bspline, bspline_right))
                {
                    result_left = new Geo::CubicBSpline(bspline_left);
                    result_right = new Geo::CubicBSpline(bspline_right);
                }
                else
                {
                    return;
                }
            }
            else
            {
                Geo::QuadBSpline bspline_left(*static_cast<const Geo::QuadBSpline *>(bspline)),
                    bspline_right(*static_cast<const Geo::QuadBSpline *>(bspline)),
                    temp_bspline(*static_cast<const Geo::QuadBSpline *>(bspline));
                if (Geo::split(*bspline, false, left_t, bspline_left, temp_bspline) &&
                    Geo::split(*bspline, false, right_t, temp_bspline, bspline_right))
                {
                    result_left = new Geo::QuadBSpline(bspline_left);
                    result_right = new Geo::QuadBSpline(bspline_right);
                }
                else
                {
                    return;
                }
            }
            std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _view_tree.remove(bspline);
                    _graph->container_group(_current_group).insert(i, result_right);
                    _graph->container_group(_current_group).insert(i, result_left);
                    _view_tree.append(result_left);
                    _view_tree.append(result_right);
                    add_items.emplace_back(result_left, _current_group, i);
                    add_items.emplace_back(result_right, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
}

void Editor::trim(Geo::Circle *circle, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], *circle, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], *circle, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object), *circle, point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*circle, *static_cast<const Geo::Ellipse *>(object), point0, point1, point2, point3))
                {
                case 4:
                    intersections.emplace_back(point3);
                case 3:
                    intersections.emplace_back(point2);
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            Geo::is_intersected(*circle, *static_cast<const Geo::CubicBezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*circle, *static_cast<const Geo::BSpline *>(object), dynamic_cast<const Geo::CubicBSpline *>(object),
                                intersections, nullptr);
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(*circle, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    if (intersections.size() < 2)
    {
        return;
    }

    Geo::Point anchor(x, y);
    double angle0 = -7, angle1 = 7;
    size_t index0 = 0, index1 = intersections.size() - 1;
    for (size_t i = 0, count = intersections.size(); i < count; ++i)
    {
        const double value = Geo::angle(anchor, *circle, intersections[i]);
        if (value < 0 && value > angle0)
        {
            angle0 = value;
            index0 = i;
        }
        if (value > 0 && value < angle1)
        {
            angle1 = value;
            index1 = i;
        }
    }

    angle0 = Geo::angle(*circle, intersections[index0]);
    angle1 = Geo::angle(*circle, intersections[index1]);
    anchor = *circle + (anchor - *circle).normalize() * circle->radius;
    Geo::Arc *arc = new Geo::Arc(circle->x, circle->y, circle->radius, angle0, angle1,
                                 (anchor.x - intersections[index0].x) * (intersections[index1].y - anchor.y) <
                                     (anchor.y - intersections[index0].y) * (intersections[index1].x - anchor.x));

    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == circle)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _view_tree.remove(circle);
            _graph->container_group(_current_group).insert(i, arc);
            _view_tree.append(arc);
            add_items.emplace_back(arc, _current_group, i);
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editor::trim(Geo::Arc *arc, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], *arc, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], *arc, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object), *arc, point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object), *arc, point0, point1, point2, point3))
                {
                case 4:
                    intersections.emplace_back(point3);
                case 3:
                    intersections.emplace_back(point2);
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            Geo::is_intersected(*arc, *static_cast<const Geo::CubicBezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*arc, *static_cast<const Geo::BSpline *>(object), dynamic_cast<const Geo::CubicBSpline *>(object),
                                intersections, nullptr);
            break;
        case Geo::Type::ARC:
            if (arc != object)
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(*arc, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    for (size_t i = 0, count = intersections.size(); i < count; ++i)
    {
        if (Geo::distance(intersections[i], arc->control_points[0]) < Geo::EPSILON ||
            Geo::distance(intersections[i], arc->control_points[2]) < Geo::EPSILON)
        {
            --count;
            intersections.erase(intersections.begin() + i--);
        }
    }

    if (intersections.empty())
    {
        return;
    }

    const Geo::Point anchor(x, y), center(arc->x, arc->y);
    double anchor_angle = Geo::angle(arc->control_points[0], center, anchor);
    if (arc->is_cw()) // 计算圆弧起点沿圆弧方向转动到修剪点所转过的角度
    {
        if (anchor_angle > 0)
        {
            anchor_angle -= Geo::PI * 2;
        }
    }
    else
    {
        if (anchor_angle < 0)
        {
            anchor_angle += Geo::PI * 2;
        }
    }
    double angle0 = 0, angle1 = 7;
    size_t index0 = SIZE_MAX, index1 = SIZE_MAX;
    for (size_t i = 0, count = intersections.size(); i < count; ++i)
    {
        double value = Geo::angle(arc->control_points[0], center, intersections[i]);
        if (arc->is_cw()) // 计算圆弧起点沿圆弧方向转动到交点所转过的角度
        {
            if (value > 0)
            {
                value -= Geo::PI * 2;
            }
        }
        else
        {
            if (value < 0)
            {
                value += Geo::PI * 2;
            }
        }
        if (std::abs(value) > std::abs(angle0) && std::abs(value) < std::abs(anchor_angle))
        { // 转过角度小于修剪点转过角度的最大值
            angle0 = value;
            index0 = i;
        }
        if (std::abs(value) < std::abs(angle1) && std::abs(value) > std::abs(anchor_angle))
        { // 转过角度大于修剪点转过角度的最小值
            angle1 = value;
            index1 = i;
        }
    }
    // 使位置分布为control_points[0],intersections[index0],intersections[index1],control_points[2]
    Geo::Arc *arc0 = nullptr, *arc1 = nullptr;
    if (index0 < SIZE_MAX && index1 < SIZE_MAX)
    {
        angle0 = Geo::angle(center, intersections[index0]);
        angle1 = Geo::angle(center, intersections[index1]);
        const double angle2 = Geo::angle(center, arc->control_points[0]);
        const double angle3 = Geo::angle(center, arc->control_points[2]);
        arc0 = new Geo::Arc(arc->x, arc->y, arc->radius, angle2, angle0, !arc->is_cw());
        arc1 = new Geo::Arc(arc->x, arc->y, arc->radius, angle1, angle3, !arc->is_cw());
    }
    else
    {
        if (index0 < SIZE_MAX)
        {
            angle0 = Geo::angle(center, intersections[index0]);
            arc0 = new Geo::Arc(arc->x, arc->y, arc->radius, Geo::angle(center, arc->control_points[0]), angle0, !arc->is_cw());
        }
        if (index1 < SIZE_MAX)
        {
            angle1 = Geo::angle(center, intersections[index1]);
            arc1 = new Geo::Arc(arc->x, arc->y, arc->radius, angle1, Geo::angle(center, arc->control_points[2]), !arc->is_cw());
        }
    }

    if (arc0 == nullptr && arc1 == nullptr)
    {
        return;
    }
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == arc)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _view_tree.remove(arc);
            if (arc0 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, arc0);
                _view_tree.append(arc0);
                add_items.emplace_back(arc0, _current_group, i++);
            }
            if (arc1 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, arc1);
                _view_tree.append(arc1);
                add_items.emplace_back(arc1, _current_group, i);
            }
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editor::trim(Geo::Ellipse *ellipse, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polygon)[i - 1], (*polygon)[i], *ellipse, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                Geo::Point point0, point1;
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    switch (Geo::is_intersected((*polyline)[i - 1], (*polyline)[i], *ellipse, point0, point1))
                    {
                    case 2:
                        intersections.emplace_back(point1);
                    case 1:
                        intersections.emplace_back(point0);
                        break;
                    default:
                        break;
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object), *ellipse, point0, point1, point2, point3))
                {
                case 4:
                    intersections.emplace_back(point3);
                case 3:
                    intersections.emplace_back(point2);
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (ellipse != object)
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object), *ellipse, point0, point1, point2, point3))
                {
                case 4:
                    intersections.emplace_back(point3);
                case 3:
                    intersections.emplace_back(point2);
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            Geo::is_intersected(*ellipse, *static_cast<const Geo::CubicBezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*ellipse, *static_cast<const Geo::BSpline *>(object), dynamic_cast<const Geo::CubicBSpline *>(object),
                                intersections, nullptr);
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*ellipse, *static_cast<const Geo::Arc *>(object), point0, point1, point2, point3))
                {
                case 4:
                    intersections.emplace_back(point3);
                case 3:
                    intersections.emplace_back(point2);
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    if (ellipse->is_arc())
    {
        const Geo::Point point0(ellipse->arc_point0()), point1(ellipse->arc_point1());
        for (size_t i = 0, count = intersections.size(); i < count; ++i)
        {
            if (Geo::distance(intersections[i], point0) < Geo::EPSILON || Geo::distance(intersections[i], point1) < Geo::EPSILON)
            {
                --count;
                intersections.erase(intersections.begin() + i--);
            }
        }
    }

    if (intersections.empty() || (!ellipse->is_arc() && intersections.size() < 2))
    {
        return;
    }

    const Geo::Point start(ellipse->is_arc() ? ellipse->arc_point0() : ellipse->a1());
    const Geo::Point anchor(x, y), center(ellipse->center());
    double anchor_angle = Geo::angle(start, center, anchor);
    if (anchor_angle < 0) // 计算椭圆或椭圆弧起点逆时针转动到修剪点所转过的角度
    {
        anchor_angle += Geo::PI * 2;
    }
    double angle0 = 0, angle1 = 7;
    size_t index0 = SIZE_MAX, index1 = SIZE_MAX;
    for (size_t i = 0, count = intersections.size(); i < count; ++i)
    {
        double value = Geo::angle(start, center, intersections[i]);
        if (value < 0) // 计算椭圆或椭圆弧起点逆时针转动到交点所转过的角度
        {
            value += Geo::PI * 2;
        }
        if (value > angle0 && value < anchor_angle)
        { // 转过角度小于修剪点转过角度的最大值
            angle0 = value;
            index0 = i;
        }
        if (value < angle1 && value > anchor_angle)
        { // 转过角度大于修剪点转过角度的最小值
            angle1 = value;
            index1 = i;
        }
    }
    if (index0 == SIZE_MAX && index1 == SIZE_MAX)
    {
        return;
    }
    if (!ellipse->is_arc())
    {
        if (index0 == SIZE_MAX)
        { // 所有交点在修剪点之后,那么要保留的椭圆弧终止圆心角是交点最大圆心角
            for (size_t i = 0, count = intersections.size(); i < count; ++i)
            {
                double value = Geo::angle(start, center, intersections[i]);
                if (value < 0) // 计算椭圆或椭圆弧起点逆时针转动到交点所转过的角度
                {
                    value += Geo::PI * 2;
                }
                if (value > angle0)
                { // 转过角度的最大值
                    angle0 = value;
                    index0 = i;
                }
            }
        }
        else if (index1 == SIZE_MAX)
        { // 所有交点在修剪点之前,那么要保留的椭圆弧起始圆心角是交点最小圆心角
            for (size_t i = 0, count = intersections.size(); i < count; ++i)
            {
                double value = Geo::angle(start, center, intersections[i]);
                if (value < 0) // 计算椭圆或椭圆弧起点逆时针转动到交点所转过的角度
                {
                    value += Geo::PI * 2;
                }
                if (value < angle1)
                { // 转过角度的最小值
                    angle1 = value;
                    index1 = i;
                }
            }
        }
    }

    // 使位置分布为start,intersections[index0],intersections[index1],end
    Geo::Ellipse *ellipse0 = nullptr, *ellipse1 = nullptr;
    const Geo::Point end(ellipse->is_arc() ? ellipse->arc_point1() : ellipse->a1());
    const double a = ellipse->lengtha(), b = ellipse->lengthb();
    if (index0 < SIZE_MAX && index1 < SIZE_MAX)
    {
        angle0 = Geo::angle(ellipse->a1(), center, intersections[index0]);
        angle1 = Geo::angle(ellipse->a1(), center, intersections[index1]);
        if (ellipse->is_arc())
        {
            const double angle2 = ellipse->is_arc() ? Geo::angle(ellipse->a1(), center, start) : 0;
            const double angle3 = ellipse->is_arc() ? Geo::angle(ellipse->a1(), center, end) : Geo::PI * 2;
            ellipse0 = new Geo::Ellipse(center.x, center.y, a, b, angle2, angle0, false);
            ellipse1 = new Geo::Ellipse(center.x, center.y, a, b, angle1, angle3, false);
            ellipse0->rotate(center.x, center.y, ellipse->angle());
            ellipse1->rotate(center.x, center.y, ellipse->angle());
        }
        else
        {
            ellipse0 = new Geo::Ellipse(center.x, center.y, a, b, angle1, angle0, false);
            ellipse0->rotate(center.x, center.y, ellipse->angle());
        }
    }
    else if (ellipse->is_arc())
    {
        if (index0 < SIZE_MAX)
        {
            angle0 = Geo::angle(ellipse->a1(), center, intersections[index0]);
            ellipse0 = new Geo::Ellipse(center.x, center.y, a, b, Geo::angle(ellipse->a1(), center, start), angle0, false);
            ellipse0->rotate(center.x, center.y, ellipse->angle());
        }
        if (index1 < SIZE_MAX)
        {
            angle1 = Geo::angle(ellipse->a1(), center, intersections[index1]);
            ellipse1 = new Geo::Ellipse(center.x, center.y, a, b, angle1, Geo::angle(ellipse->a1(), center, end), false);
            ellipse1->rotate(center.x, center.y, ellipse->angle());
        }
    }

    if (ellipse0 == nullptr && ellipse1 == nullptr)
    {
        return;
    }
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == ellipse)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _view_tree.remove(ellipse);
            if (ellipse0 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, ellipse0);
                _view_tree.append(ellipse0);
                add_items.emplace_back(ellipse0, _current_group, i++);
            }
            if (ellipse1 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, ellipse1);
                _view_tree.append(ellipse1);
                add_items.emplace_back(ellipse1, _current_group, i);
            }
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editor::extend(Geo::Polyline *polyline, const double x, const double y)
{
    Geo::Point head, tail;
    if (Geo::distance_square(polyline->front().x, polyline->front().y, x, y) <=
        Geo::distance_square(polyline->back().x, polyline->back().y, x, y)) // 延长头
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).aabbrect();
        head = polyline->front();
        tail = head + (head - (*polyline)[1]).normalize() * std::hypot(rect.right - rect.left, rect.top - rect.bottom);
    }
    else // 延长尾
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).aabbrect();
        head = polyline->back();
        tail = head + (head - (*polyline)[polyline->size() - 2]).normalize() * std::hypot(rect.right - rect.left, rect.top - rect.bottom);
    }

    std::vector<Geo::Point> intersections;
    for (size_t count = (head == polyline->front() ? polyline->size() : polyline->size() - 2),
                i = (head == polyline->front() ? 2 : 1);
         i < count; ++i) // 找到自身交点
    {
        if (Geo::Point point; Geo::is_intersected(head, tail, (*polyline)[i - 1], (*polyline)[i], point))
        {
            intersections.emplace_back(point);
        }
    }
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polygon)[i - 1], (*polygon)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (const Geo::Polyline *polyline2 = static_cast<const Geo::Polyline *>(object);
                polyline2 != polyline && Geo::is_intersected(polyline2->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polyline2->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polyline2)[i - 1], (*polyline2)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Circle *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Ellipse *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                Geo::is_intersected(bezier->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail, *bezier, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                Geo::is_intersected(bspline->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points;
                    Geo::is_intersected(head, tail, *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    if (intersections.empty())
    {
        return;
    }

    double dis = DBL_MAX;
    Geo::Point expoint;
    for (const Geo::Point &point : intersections)
    {
        if ((point - head) * (tail - head) > 0)
        {
            const double d = Geo::distance_square(point, head);
            if (d > 0 && d < dis)
            {
                dis = d;
                expoint = point;
            }
        }
    }
    if (dis == DBL_MAX)
    {
        return;
    }

    std::vector<std::tuple<double, double>> shape;
    for (const Geo::Point &point : *polyline)
    {
        shape.emplace_back(point.x, point.y);
    }
    _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));
    if (head == polyline->front()) // 延长头
    {
        polyline->front() = expoint;
    }
    else // 延长尾
    {
        polyline->back() = expoint;
    }
    _view_tree.update(polyline);
}

void Editor::extend(Geo::CubicBezier *bezier, const double x, const double y)
{
    Geo::Point head, tail;
    if (Geo::distance_square(bezier->front().x, bezier->front().y, x, y) <=
        Geo::distance_square(bezier->back().x, bezier->back().y, x, y)) // 延长头
    {
        const Geo::AABBRect rect(_graph->container_group(_current_group).aabbrect());
        head = bezier->front();
        tail = head + (head - bezier->control_points[1]).normalize() * std::hypot(rect.right - rect.left, rect.bottom - rect.top);
    }
    else // 延长尾
    {
        const Geo::AABBRect rect(_graph->container_group(_current_group).aabbrect());
        head = bezier->back();
        tail = head + (head - bezier->control_points[bezier->control_points.size() - 2]).normalize() *
                          std::hypot(rect.right - rect.left, rect.bottom - rect.top);
    }

    std::vector<Geo::Point> intersections;
    if (Geo::is_intersected(head, tail, *bezier, intersections, true)) // 找到自身交点
    {
        while (std::find(intersections.begin(), intersections.end(), head) != intersections.end())
        {
            intersections.erase(std::remove(intersections.begin(), intersections.end(), head), intersections.end());
        }
    }
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polygon)[i - 1], (*polygon)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                Geo::is_intersected(polyline->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polyline)[i - 1], (*polyline)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Circle *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Ellipse *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            if (const Geo::CubicBezier *bezier2 = static_cast<const Geo::CubicBezier *>(object);
                bezier2 != bezier && Geo::is_intersected(bezier2->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail, *bezier2, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            if (const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                Geo::is_intersected(bspline->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points;
                    Geo::is_intersected(head, tail, *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    if (intersections.empty())
    {
        return;
    }

    double dis = DBL_MAX;
    Geo::Point expoint;
    for (const Geo::Point &point : intersections)
    {
        if ((point - head) * (tail - head) > 0)
        {
            const double d = Geo::distance_square(point, head);
            if (d > 0 && d < dis)
            {
                dis = d;
                expoint = point;
            }
        }
    }
    if (dis == DBL_MAX)
    {
        return;
    }

    std::vector<std::tuple<double, double>> shape;
    for (const Geo::Point &point : bezier->control_points)
    {
        shape.emplace_back(point.x, point.y);
    }
    _backup.push_command(new UndoStack::ChangeShapeCommand(bezier, shape));

    if (head == bezier->front()) // 延长头
    {
        const Geo::Point point0((bezier->front() + expoint * 2) / 3);
        const Geo::Point point1((bezier->front() * 2 + expoint) / 3);
        bezier->control_points.insert(bezier->control_points.cbegin(), point0);
        bezier->control_points.insert(bezier->control_points.cbegin(), point1);
        bezier->control_points.insert(bezier->control_points.cbegin(), expoint);
    }
    else // 延长尾
    {
        const Geo::Point point0((bezier->back() + expoint * 2) / 3);
        const Geo::Point point1((bezier->back() * 2 + expoint) / 3);
        bezier->control_points.push_back(point0);
        bezier->control_points.push_back(point1);
        bezier->control_points.push_back(expoint);
    }
    bezier->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    _view_tree.update(bezier);
}

void Editor::extend(Geo::BSpline *bspline, const double x, const double y)
{
    Geo::Point head, tail;
    if (Geo::distance_square(bspline->control_points.front().x, bspline->control_points.front().y, x, y) <=
        Geo::distance_square(bspline->control_points.back().x, bspline->control_points.back().y, x, y)) // 延长头
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).aabbrect();
        head = bspline->front();
        tail = head + (head - bspline->control_points[1]).normalize() * std::hypot(rect.right - rect.left, rect.top - rect.bottom);
    }
    else // 延长尾
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).aabbrect();
        head = bspline->back();
        tail = head + (head - bspline->control_points[bspline->control_points.size() - 2]).normalize() *
                          std::hypot(rect.right - rect.left, rect.top - rect.bottom);
    }

    std::vector<Geo::Point> intersections;
    if (Geo::is_intersected(head, tail, *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), intersections,
                            true)) // 找到自身交点
    {
        while (std::find(intersections.begin(), intersections.end(), head) != intersections.end())
        {
            intersections.erase(std::remove(intersections.begin(), intersections.end(), head), intersections.end());
        }
    }
    for (const Geo::DObject *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polygon->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polygon)[i - 1], (*polygon)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                Geo::is_intersected(polyline->aabbrect(), head, tail))
            {
                for (size_t i = 1, count = polyline->size(); i < count; ++i)
                {
                    if (Geo::Point point; Geo::is_intersected(head, tail, (*polyline)[i - 1], (*polyline)[i], point))
                    {
                        intersections.emplace_back(point);
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Circle *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Ellipse *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::BEZIER:
            if (const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                Geo::is_intersected(bezier->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail, *bezier, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            if (const Geo::BSpline *bspline2 = static_cast<const Geo::BSpline *>(object);
                bspline2 != bspline && Geo::is_intersected(bspline2->aabbrect(), head, tail))
            {
                if (std::vector<Geo::Point> points;
                    Geo::is_intersected(head, tail, *bspline2, dynamic_cast<const Geo::CubicBSpline *>(bspline2), points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (Geo::Point coord; Geo::foot_point(head, tail, point, coord, false))
                        {
                            intersections.emplace_back(coord);
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1;
                switch (Geo::is_intersected(head, tail, *static_cast<const Geo::Arc *>(object), point0, point1))
                {
                case 2:
                    intersections.emplace_back(point1);
                case 1:
                    intersections.emplace_back(point0);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    if (intersections.empty())
    {
        return;
    }

    double dis = DBL_MAX;
    Geo::Point expoint;
    for (const Geo::Point &point : intersections)
    {
        if ((point - head) * (tail - head) > 0)
        {
            const double d = Geo::distance_square(point, head);
            if (d > 0 && d < dis)
            {
                dis = d;
                expoint = point;
            }
        }
    }
    if (dis == DBL_MAX)
    {
        return;
    }

    {
        std::vector<std::tuple<double, double>> shape, path;
        std::vector<double> knots(bspline->knots());
        for (const Geo::Point &point : bspline->path_points)
        {
            path.emplace_back(point.x, point.y);
        }
        for (const Geo::Point &point : bspline->control_points)
        {
            shape.emplace_back(point.x, point.y);
        }
        _backup.push_command(new UndoStack::ChangeShapeCommand(bspline, shape, path, knots));
    }

    if (head == bspline->front()) // 延长头
    {
        bspline->extend_front(expoint);
    }
    else // 延长尾
    {
        bspline->extend_back(expoint);
    }
    bspline->is_selected = true;
    bspline->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
    _view_tree.update(bspline);
}

bool Editor::divide_points_n(const std::vector<Geo::DObject *> &objects, const size_t n)
{
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (const Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::ARC:
            {
                const Geo::Arc *arc = static_cast<const Geo::Arc *>(object);
                for (size_t i = 1; i < n; ++i)
                {
                    Geo::Point *p = new Geo::Point(arc->shape_point(i * 1.0 / n));
                    add_items.emplace_back(p, _current_group, group.size());
                    group.append(p);
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*bezier, n, pos))
                {
                    for (const auto [index, t] : pos)
                    {
                        Geo::Point *p = new Geo::Point(bezier->shape_point(index, t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                if (std::vector<double> pos; Geo::split(*bspline, n, pos))
                {
                    for (const double t : pos)
                    {
                        Geo::Point *p = new Geo::Point(bspline->at(t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object); ellipse->is_arc())
            {
                if (std::vector<double> pos; Geo::split(*ellipse, n, pos))
                {
                    for (const double t : pos)
                    {
                        Geo::Point *p = new Geo::Point(ellipse->param_point(t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*polyline, n, pos))
                {
                    for (const auto [index, t] : pos)
                    {
                        Geo::Point *p = new Geo::Point(polyline->shape_point(index, t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    if (add_items.empty())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : add_items)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, true));
        return true;
    }
}

bool Editor::divide_parts_n(const std::vector<Geo::DObject *> &objects, const size_t n)
{
    ContainerGroup &group = _graph->container_group(_current_group);
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::ARC:
            {
                const Geo::Arc *arc = static_cast<const Geo::Arc *>(object);
                const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                remove_items.emplace_back(group.pop(index), _current_group, index);
                if (Geo::Arc *p = arc->range((n - 1) * 1.0 / n, 1))
                {
                    add_items.emplace_back(p, _current_group, index);
                    group.insert(index, p);
                }
                for (size_t i = 1; i < n; ++i)
                {
                    if (Geo::Arc *p = arc->range((i - 1) * 1.0 / n, i * 1.0 / n))
                    {
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*bezier, n, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::CubicBezier *p =
                            bezier->range(std::get<0>(pos.back()), std::get<1>(pos.back()), bezier->control_points.size() / 3 - 1, 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    size_t index0 = 0;
                    double t0 = 0;
                    for (const auto [index1, t1] : pos)
                    {
                        if (Geo::CubicBezier *p = bezier->range(index0, t0, index1, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        index0 = index1;
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                if (std::vector<double> pos; Geo::split(*bspline, n, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::BSpline *p = bspline->range(pos.back(), 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    double t0 = 0;
                    for (const double t1 : pos)
                    {
                        if (Geo::BSpline *p = bspline->range(t0, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object); ellipse->is_arc())
            {
                if (std::vector<double> pos; Geo::split(*ellipse, n, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::Ellipse *p = ellipse->range(pos.back(), ellipse->arc_param1()))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    double t0 = ellipse->arc_param0();
                    for (const double t1 : pos)
                    {
                        if (Geo::Ellipse *p = ellipse->range(t0, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*polyline, n, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::Polyline *p = polyline->range(std::get<0>(pos.back()), std::get<1>(pos.back()), polyline->size() - 2, 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    size_t index0 = 0;
                    double t0 = 0;
                    for (const auto [index1, t1] : pos)
                    {
                        if (Geo::Polyline *p = polyline->range(index0, t0, index1, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        index0 = index1;
                        t0 = t1;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    if (add_items.empty())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : remove_items)
        {
            _view_tree.remove(std::get<0>(item));
        }
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : add_items)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

bool Editor::divide_points_measure(const std::vector<Geo::DObject *> &objects, const double length)
{
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::ARC:
            {
                const Geo::Arc *arc = static_cast<const Geo::Arc *>(object);
                if (std::vector<double> pos; Geo::split(*arc, length, pos))
                {
                    for (const double t : pos)
                    {
                        Geo::Point *p = new Geo::Point(arc->shape_point(t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*bezier, length, pos))
                {
                    for (const auto [index, t] : pos)
                    {
                        Geo::Point *p = new Geo::Point(bezier->shape_point(index, t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                if (std::vector<double> pos; Geo::split(*bspline, length, pos))
                {
                    for (const double t : pos)
                    {
                        Geo::Point *p = new Geo::Point(bspline->at(t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object); ellipse->is_arc())
            {
                if (std::vector<double> pos; Geo::split(*ellipse, length, pos))
                {
                    for (const double t : pos)
                    {
                        Geo::Point *p = new Geo::Point(ellipse->param_point(t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*polyline, length, pos))
                {
                    for (const auto [index, t] : pos)
                    {
                        Geo::Point *p = new Geo::Point(polyline->shape_point(index, t));
                        add_items.emplace_back(p, _current_group, group.size());
                        group.append(p);
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    if (add_items.empty())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : add_items)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, true));
        return true;
    }
}

bool Editor::divide_parts_measure(const std::vector<Geo::DObject *> &objects, const double length)
{
    ContainerGroup &group = _graph->container_group(_current_group);
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    for (Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::ARC:
            {
                const Geo::Arc *arc = static_cast<const Geo::Arc *>(object);
                if (std::vector<double> pos; Geo::split(*arc, length, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::Arc *p = arc->range(pos.back(), 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    double t0 = 0;
                    for (const double t1 : pos)
                    {
                        if (Geo::Arc *p = arc->range(t0, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                const Geo::CubicBezier *bezier = static_cast<const Geo::CubicBezier *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*bezier, length, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::CubicBezier *p =
                            bezier->range(std::get<0>(pos.back()), std::get<1>(pos.back()), bezier->control_points.size() / 3 - 1, 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    size_t index0 = 0;
                    double t0 = 0;
                    for (const auto [index1, t1] : pos)
                    {
                        if (Geo::CubicBezier *p = bezier->range(index0, t0, index1, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        index0 = index1;
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(object);
                if (std::vector<double> pos; Geo::split(*bspline, length, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::BSpline *p = bspline->range(pos.back(), 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    double t0 = 0;
                    for (const double t1 : pos)
                    {
                        if (Geo::BSpline *p = bspline->range(t0, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            if (const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(object); ellipse->is_arc())
            {
                if (std::vector<double> pos; Geo::split(*ellipse, length, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::Ellipse *p = ellipse->range(pos.back(), ellipse->arc_param1()))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    double t0 = ellipse->arc_param0();
                    for (const double t1 : pos)
                    {
                        if (Geo::Ellipse *p = ellipse->range(t0, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        t0 = t1;
                    }
                }
            }
            break;
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
                if (std::vector<std::tuple<size_t, double>> pos; Geo::split(*polyline, length, pos))
                {
                    const size_t index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
                    remove_items.emplace_back(group.pop(index), _current_group, index);
                    if (Geo::Polyline *p = polyline->range(std::get<0>(pos.back()), std::get<1>(pos.back()), polyline->size() - 2, 1))
                    {
                        add_items.emplace_back(p, _current_group, index);
                        group.insert(index, p);
                    }
                    size_t index0 = 0;
                    double t0 = 0;
                    for (const auto [index1, t1] : pos)
                    {
                        if (Geo::Polyline *p = polyline->range(index0, t0, index1, t1))
                        {
                            add_items.emplace_back(p, _current_group, group.size());
                            group.append(p);
                        }
                        index0 = index1;
                        t0 = t1;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    if (add_items.empty())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : remove_items)
        {
            _view_tree.remove(std::get<0>(item));
        }
        for (const std::tuple<Geo::DObject *, size_t, size_t> &item : add_items)
        {
            _view_tree.append(std::get<0>(item));
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
}

void Editor::reverse(const std::vector<Geo::DObject *> &objects)
{
    std::vector<Geo::DObject *> reversed;
    for (Geo::DObject *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::ARC:
            {
                Geo::Arc *arc = static_cast<Geo::Arc *>(object);
                std::swap(arc->control_points[0], arc->control_points[2]);
                arc->update_shape(Geo::Circle::default_down_sampling_value);
            }
            break;
        case Geo::Type::BEZIER:
            {
                Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(object);
                std::reverse(bezier->control_points.begin(), bezier->control_points.end());
                bezier->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
            }
            break;
        case Geo::Type::BSPLINE:
            static_cast<Geo::BSpline *>(object)->reverse();
            break;
        case Geo::Type::POLYGON:
            std::reverse(static_cast<Geo::Polygon *>(object)->begin(), static_cast<Geo::Polygon *>(object)->end());
            break;
        case Geo::Type::POLYLINE:
            std::reverse(static_cast<Geo::Polyline *>(object)->begin(), static_cast<Geo::Polyline *>(object)->end());
            break;
        default:
            continue;
        }
        reversed.push_back(object);
    }
    if (!reversed.empty())
    {
        _graph->modified = true;
        _backup.push_command(new UndoStack::ReverseCommand(reversed));
    }
}


void Editor::auto_combine()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::DObject *> all_containers, all_polylines;
    std::unordered_map<const Geo::DObject *, double> areas, lengths;
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->type())
            {
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
            case Geo::Type::BSPLINE:
            case Geo::Type::TEXT:
            case Geo::Type::ARC:
                lengths.insert_or_assign(group.back(), group.back()->length());
                all_polylines.push_back(group.pop_back());
                break;
            case Geo::Type::POINT:
                lengths.insert_or_assign(group.back(), 0);
                all_polylines.push_back(group.pop_back());
                break;
            case Geo::Type::POLYGON:
                areas.insert_or_assign(group.back(), static_cast<const Geo::Polygon *>(group.back())->area());
                all_containers.push_back(group.pop_back());
                break;
            case Geo::Type::CIRCLE:
                areas.insert_or_assign(group.back(), static_cast<const Geo::Circle *>(group.back())->area());
                all_containers.push_back(group.pop_back());
                break;
            case Geo::Type::ELLIPSE:
                areas.insert_or_assign(group.back(), static_cast<const Geo::Ellipse *>(group.back())->area());
                all_containers.push_back(group.pop_back());
                break;
            default:
                break;
            }
        }
    }
    _graph->clear();

    if (all_containers.empty())
    {
        _graph->append_group();
        for (Geo::DObject *item : all_polylines)
        {
            _graph->back().append(item);
        }
        _view_tree.build(_graph);
        return;
    }

    std::sort(all_containers.begin(), all_containers.end(),
              [&](const Geo::DObject *a, const Geo::DObject *b) { return areas[a] > areas[b]; });
    std::sort(all_polylines.begin(), all_polylines.end(),
              [&](const Geo::DObject *a, const Geo::DObject *b) { return lengths[a] > lengths[b]; });

    std::unordered_map<const Geo::DObject *, Geo::AABBRect> container_rects, polyline_rects;
    for (const Geo::DObject *object : all_containers)
    {
        container_rects.insert_or_assign(object, object->aabbrect());
    }
    for (const Geo::DObject *object : all_polylines)
    {
        polyline_rects.insert_or_assign(object, object->aabbrect());
    }

    _graph->append_group();
    for (size_t i = 0, count = all_containers.size(); i < count; ++i)
    {
        Geo::AABBRect current_rect = container_rects[all_containers[i]];
        std::vector<Geo::AABBRect> current_rects({current_rect});
        std::vector<Geo::DObject *> objects({all_containers[i]});
        for (size_t j = i + 1; j < count; ++j)
        {
            if (!Geo::is_intersected(current_rect, container_rects[all_containers[j]]))
            {
                continue;
            }

            const Geo::AABBRect &container_rect = container_rects[all_containers[j]];
            for (size_t k = 0, object_count = objects.size(); k < object_count; ++k)
            {
                if (!Geo::is_intersected(current_rects[k], container_rect))
                {
                    continue;
                }
                switch (objects[k]->type())
                {
                case Geo::Type::POLYGON:
                    {
                        Geo::Polygon *polygon = static_cast<Geo::Polygon *>(objects[k]);
                        switch (all_containers[j]->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::NoAABBTest::is_intersected(*polygon, *static_cast<Geo::Polygon *>(all_containers[j])))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::is_intersected(*polygon, *static_cast<Geo::Circle *>(all_containers[j])))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::is_intersected(*polygon, *static_cast<Geo::Ellipse *>(all_containers[j])))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case Geo::Type::CIRCLE:
                    {
                        Geo::Circle *circle = static_cast<Geo::Circle *>(objects[k]);
                        switch (all_containers[j]->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::is_inside(*circle, *static_cast<Geo::Polygon *>(all_containers[j])))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::is_inside(*circle, *static_cast<Geo::Circle *>(all_containers[j])))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::Point point0, point1, point2, point3; Geo::is_intersected(
                                    *circle, *static_cast<Geo::Ellipse *>(all_containers[j]), point0, point1, point2, point3))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case Geo::Type::ELLIPSE:
                    {
                        Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(objects[k]);
                        switch (all_containers[j]->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::is_intersected(*static_cast<Geo::Polygon *>(all_containers[j]), *ellipse))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::Point point0, point1, point2, point3; Geo::is_intersected(
                                    *static_cast<Geo::Circle *>(all_containers[j]), *ellipse, point0, point1, point2, point3))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::Point point0, point1, point2, point3; Geo::is_intersected(
                                    *ellipse, *static_cast<Geo::Ellipse *>(all_containers[j]), point0, point1, point2, point3))
                            {
                                objects.push_back(all_containers[j]);
                                current_rect += container_rects[all_containers[j]];
                                current_rects.emplace_back(container_rects[all_containers[j]]);
                                all_containers.erase(all_containers.begin() + j);
                                j = i;
                                --count;
                                k = object_count - 1;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }

        for (size_t k = 0, polyline_count = all_polylines.size(), object_count = objects.size(); k < polyline_count; ++k)
        {
            if (!Geo::is_intersected(current_rect, polyline_rects[all_polylines[k]]))
            {
                continue;
            }
            for (size_t j = 0; j < object_count; ++j)
            {
                switch (objects[j]->type())
                {
                case Geo::Type::POLYGON:
                    switch (all_polylines[k]->type())
                    {
                    case Geo::Type::POLYLINE:
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]), *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::CubicBezier *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BSPLINE:
                        if (Geo::is_intersected(static_cast<Geo::BSpline *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::TEXT:
                        if (Geo::is_intersected(static_cast<Text *>(all_polylines[k])->convex_hull(),
                                                *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]), *static_cast<Geo::Polygon *>(objects[j]), true))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case Geo::Type::CIRCLE:
                    switch (all_polylines[k]->type())
                    {
                    case Geo::Type::POLYLINE:
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]), *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::CubicBezier *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BSPLINE:
                        if (Geo::is_intersected(static_cast<Geo::BSpline *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::TEXT:
                        if (Geo::is_intersected(static_cast<Text *>(all_polylines[k])->convex_hull(),
                                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]), *static_cast<Geo::Circle *>(objects[j]), true))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case Geo::Type::ELLIPSE:
                    switch (all_polylines[k]->type())
                    {
                    case Geo::Type::POLYLINE:
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]), *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::CubicBezier *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BSPLINE:
                        if (Geo::is_intersected(static_cast<Geo::BSpline *>(all_polylines[k])->shape(),
                                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::TEXT:
                        if (Geo::is_intersected(static_cast<Text *>(all_polylines[k])->convex_hull(),
                                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]), *static_cast<Geo::Ellipse *>(objects[j]), true))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        if (objects.size() > 1)
        {
            _graph->back().append(new Combination(objects.begin(), objects.end()));
        }
        else
        {
            _graph->back().append(all_containers[i]);
        }
    }

    for (Geo::DObject *polyline : all_polylines)
    {
        _graph->back().append(polyline);
    }
    _view_tree.build(_graph);
}

void Editor::auto_layering()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::DObject *> all_containers, all_polylines;
    std::unordered_map<const Geo::DObject *, Geo::AABBRect> rects;
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->type())
            {
            case Geo::Type::TEXT:
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
            case Geo::Type::BSPLINE:
            case Geo::Type::ARC:
            case Geo::Type::POINT:
                all_polylines.emplace_back(group.pop_back());
                break;
            default:
                all_containers.emplace_back(group.pop_back());
                rects.insert_or_assign(all_containers.back(), all_containers.back()->aabbrect());
                break;
            }
        }
    }
    _graph->clear();

    if (all_containers.empty())
    {
        _graph->append_group();
        for (Geo::DObject *item : all_polylines)
        {
            _graph->back().append(item);
        }
        return;
    }

    {
        std::unordered_map<const Geo::DObject *, double> areas;
        for (const Geo::DObject *object : all_containers)
        {
            switch (object->type())
            {
            case Geo::Type::POLYGON:
                areas.insert_or_assign(object, static_cast<const Geo::Polygon *>(object)->area());
                break;
            case Geo::Type::CIRCLE:
                areas.insert_or_assign(object, static_cast<const Geo::Circle *>(object)->area());
                break;
            case Geo::Type::ELLIPSE:
                areas.insert_or_assign(object, static_cast<const Geo::Ellipse *>(object)->area());
                break;
            default:
                break; // Not sure if only three types needs to be checked.
                       // Remove this and see the error to check if anything that nedds to be checked is missing.
            }
        }
        std::sort(all_containers.begin(), all_containers.end(),
                  [&](const Geo::DObject *a, const Geo::DObject *b) { return areas[a] > areas[b]; });
    }

    _graph->append_group();
    while (!all_containers.empty())
    {
        _graph->back().append(all_containers.front());
        all_containers.erase(all_containers.begin());
        std::unordered_map<const Geo::DObject *, Geo::AABBRect> current_rects;
        current_rects.insert_or_assign(_graph->back().front(), rects[_graph->back().front()]);
        for (size_t i = 0, count = all_containers.size(); i < count; ++i)
        {
            bool flag = true;
            switch (all_containers[i]->type())
            {
            case Geo::Type::POLYGON:
                {
                    const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(all_containers[i]);
                    const Geo::AABBRect &rect = rects[polygon];
                    for (Geo::DObject *geo : _graph->back())
                    {
                        switch (geo->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::is_intersected(rect, current_rects[geo]) &&
                                Geo::NoAABBTest::is_intersected(*polygon, *static_cast<Geo::Polygon *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::is_intersected(*polygon, *static_cast<Geo::Circle *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::is_intersected(*polygon, *static_cast<Geo::Ellipse *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        default:
                            break;
                        }
                        if (!flag)
                        {
                            break;
                        }
                    }
                }
                break;
            case Geo::Type::CIRCLE:
                {
                    const Geo::Circle *circle = static_cast<const Geo::Circle *>(all_containers[i]);
                    for (Geo::DObject *geo : _graph->back())
                    {
                        switch (geo->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::is_inside(*circle, *static_cast<Geo::Polygon *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::is_intersected(*circle, *static_cast<Geo::Circle *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::is_inside(*circle, *static_cast<Geo::Ellipse *>(geo)))
                            {
                                flag = false;
                            }
                            break;
                        default:
                            break;
                        }
                        if (!flag)
                        {
                            break;
                        }
                    }
                }
                break;
            case Geo::Type::ELLIPSE:
                {
                    const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(all_containers[i]);
                    for (Geo::DObject *geo : _graph->back())
                    {
                        switch (geo->type())
                        {
                        case Geo::Type::POLYGON:
                            if (Geo::is_intersected(*static_cast<Geo::Polygon *>(geo), *ellipse))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                            if (Geo::Point point0, point1, point2, point3;
                                Geo::is_intersected(*static_cast<Geo::Circle *>(geo), *ellipse, point0, point1, point2, point3))
                            {
                                flag = false;
                            }
                            break;
                        case Geo::Type::ELLIPSE:
                            if (Geo::Point point0, point1, point2, point3;
                                Geo::is_intersected(*ellipse, *static_cast<Geo::Ellipse *>(geo), point0, point1, point2, point3))
                            {
                                flag = false;
                            }
                            break;
                        default:
                            break;
                        }
                        if (!flag)
                        {
                            break;
                        }
                    }
                }
                break;
            default:
                break;
            }
            if (flag)
            {
                if (dynamic_cast<const Geo::Polygon *>(all_containers[i]) != nullptr)
                {
                    current_rects.insert_or_assign(all_containers[i], rects[all_containers[i]]);
                }
                _graph->back().append(all_containers[i]);
                all_containers.erase(all_containers.begin() + i--);
                --count;
            }
        }
        _graph->append_group();
    }

    for (Geo::DObject *geo : all_polylines)
    {
        _graph->back().append(geo);
    }

    for (size_t i = 0, count = _graph->container_groups().size(); i < count; ++i)
    {
        if (_graph->container_group(i).empty())
        {
            _graph->remove_group(i--);
            --count;
        }
    }
}


void Editor::text_to_polylines(Text *text)
{
    if (text == nullptr)
    {
        return;
    }
    std::ifstream cfile("./fonts/HZFS.SHX", std::ios::binary);
    std::ifstream efile("./fonts/ISO.SHX", std::ios::binary);
    if (!cfile.is_open() || !efile.is_open())
    {
        return;
    }

    SHXReader::SHXFont cfont(&cfile), efont(&efile);
    Combination *combination = new Combination();
    const std::string result = TextEncoding::utf8_to_gbk(text->text().toUtf8().toStdString());
    const int font_size = text->font().pointSize();
    const double init_x = text->aabbrect().left;
    double x = init_x, y = text->aabbrect().top - font_size;
    for (size_t i = 0, count = result.length(); i < count; ++i)
    {
        if (result[i] < 0)
        {
            const int code = ((result[i] & 0xFF) << 8) | (result[i + 1] & 0xFF);
            ++i;
            SHXReader::SHXShape shape = cfont.char_shape(code, font_size);
            for (Geo::Polyline &polyline : shape.polylines)
            {
                polyline.translate(x, y);
                combination->append(new Geo::Polyline(polyline));
            }
            if (shape.polylines.empty())
            {
                x += font_size;
            }
            else
            {
                shape.update_bbox();
                x = shape.bbox.max_x + font_size / 8;
            }
        }
        else
        {
            SHXReader::SHXShape shape = efont.char_shape(result[i], font_size / 2);
            for (Geo::Polyline &polyline : shape.polylines)
            {
                polyline.translate(x, y);
                combination->append(new Geo::Polyline(polyline));
            }
            if (shape.polylines.empty())
            {
                switch (result[i])
                {
                case '\n':
                    y -= font_size;
                    y -= (font_size / 10);
                    x = init_x;
                    break;
                case '\t':
                    x += font_size;
                    break;
                default:
                    x += (font_size / 2);
                    break;
                }
            }
            else
            {
                shape.update_bbox();
                x = shape.bbox.max_x + font_size / 8;
            }
        }
    }
    combination->update_border();
    const size_t index =
        std::distance(_graph->container_group(_current_group).begin(),
                      std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), text));
    _view_tree.remove(_graph->container_group(_current_group).pop(index));
    _graph->container_group(_current_group).insert(index, combination);
    _view_tree.append(combination);
    _graph->modified = true;
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    add_items.emplace_back(combination, _current_group, index);
    remove_items.emplace_back(text, _current_group, index);
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editor::bezier_to_bspline(Geo::CubicBezier *bezier)
{
    if (bezier == nullptr)
    {
        return;
    }
    Geo::CubicBSpline *bspline = new Geo::CubicBSpline(Geo::bezier_to_bspline(*bezier));
    const size_t index =
        std::distance(_graph->container_group(_current_group).begin(),
                      std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bezier));
    _view_tree.remove(_graph->container_group(_current_group).pop(index));
    _graph->container_group(_current_group).insert(index, bspline);
    _view_tree.append(bspline);
    _graph->modified = true;
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    add_items.emplace_back(bspline, _current_group, index);
    remove_items.emplace_back(bezier, _current_group, index);
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editor::bspline_to_bezier(Geo::BSpline *bspline)
{
    if (bspline == nullptr)
    {
        return;
    }
    Geo::CubicBezier *bezier = new Geo::CubicBezier(Geo::bspline_to_bezier(*bspline));
    const size_t index =
        std::distance(_graph->container_group(_current_group).begin(),
                      std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
    _view_tree.remove(_graph->container_group(_current_group).pop(index));
    _graph->container_group(_current_group).insert(index, bezier);
    _view_tree.append(bezier);
    _graph->modified = true;
    std::vector<std::tuple<Geo::DObject *, size_t, size_t>> add_items, remove_items;
    add_items.emplace_back(bezier, _current_group, index);
    remove_items.emplace_back(bspline, _current_group, index);
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}
