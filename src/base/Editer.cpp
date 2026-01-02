#include <thread>
#include "base/Editer.hpp"
#include "io/GlobalSetting.hpp"
#include "io/SHXReader.hpp"
#include "io/TextEncoding.hpp"


Editer::Editer(Graph *graph)
    : _graph(graph)
{
    init();
}

Editer::Editer(Graph *graph, const QString &path)
    : _graph(graph), _file_path(path)
{
    init();
}

Editer::~Editer()
{
    _graph = nullptr;
    _backup.clear();
    for (Geo::Geometry *geo : _paste_table)
    {
        delete geo;
    }
}

void Editer::init()
{
    GlobalSetting::setting().graph = _graph;
    if (_graph != nullptr)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                geo->is_selected = false;
            }
        }
        if (_graph->container_groups().empty())
        {
            _graph->append_group();
        }
        _backup.clear();
        _backup.set_graph(_graph);
    }
}

void Editer::load_graph(Graph *graph, const QString &path)
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
        GlobalSetting::setting().graph = nullptr;
    }
}

void Editer::load_graph(Graph *graph)
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
        GlobalSetting::setting().graph = nullptr;
    }
}

void Editer::delete_graph()
{
    if (_graph != nullptr)
    {
        delete _graph;
        _graph = nullptr;
        GlobalSetting::setting().graph = nullptr;
        _current_group = 0;
        _file_path.clear();
        _backup.clear();
    }
}

const QString &Editer::path() const
{
    return _file_path;
}

void Editer::set_path(const QString &path)
{
    _file_path = path;
}

std::vector<Geo::Point> &Editer::point_cache()
{
    return _point_cache;
}

const std::vector<Geo::Point> &Editer::point_cache() const
{
    return _point_cache;
}

std::vector<std::tuple<double, double>> &Editer::edited_shape()
{
    return _edited_shape;
}

const size_t Editer::current_group() const
{
    return _current_group;
}

void Editer::set_current_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _current_group = index;
}

const size_t Editer::groups_count() const
{
    return _graph->container_groups().size();
}

void Editer::set_view_ratio(const double value)
{
    _view_ratio = value;
}

Geo::Geometry *Editer::select(const Geo::Point &point, const bool reset_others)
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
    Geo::Bezier *b = nullptr;
    Geo::BSpline *bs = nullptr;
    Geo::Arc *arc = nullptr;
    Combination *cb = nullptr;
    for (std::vector<Geo::Geometry *>::reverse_iterator it = _graph->container_group(_current_group).rbegin(),
                                                        end = _graph->container_group(_current_group).rend();
         it != end; ++it)
    {
        switch ((*it)->type())
        {
        case Geo::Type::TEXT:
            t = static_cast<Text *>(*it);
            if (Geo::is_inside(point, *static_cast<const Geo::AABBRect *>(t), true))
            {
                t->is_selected = true;
                return t;
            }
            break;
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(*it);
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
            circle = static_cast<Geo::Circle *>(*it);
            if (std::abs(Geo::distance(point, *circle) - circle->radius) <= catch_distance
                || Geo::distance(point, *circle) <= catch_distance)
            {
                circle->is_selected = true;
                return circle;
            }
            circle = nullptr;
            break;
        case Geo::Type::ELLIPSE:
            ellipse = static_cast<Geo::Ellipse *>(*it);
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
                if (std::abs(Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point)
                    - std::max(ellipse->lengtha(), ellipse->lengthb()) * 2) <= catch_distance
                    || Geo::distance_square(point, (ellipse->c0() + ellipse->c1()) / 2) <= catch_distance)
                {
                    ellipse->is_selected = true;
                    return ellipse;
                }
            }
            ellipse = nullptr;
            break;
        case Geo::Type::COMBINATION:
            cb = static_cast<Combination *>(*it);
            if (Geo::is_inside(point, cb->border(), true))
            {
                for (Geo::Geometry *item : *cb)
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (Geo::is_inside(point, *static_cast<const Geo::AABBRect *>(item), true))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
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
                        if (std::abs(Geo::distance(point, *circle) - circle->radius) <= catch_distance
                            || Geo::distance(point, *circle) <= catch_distance)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<Geo::Ellipse *>(item);
                        if (std::abs(Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point)
                            - std::max(ellipse->lengtha(), ellipse->lengthb()) * 2) <= catch_distance
                            || Geo::distance(point, (ellipse->c0() + ellipse->c1()) / 2) <= catch_distance)
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
                        b = static_cast<Geo::Bezier *>(item);
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
            p = static_cast<Geo::Polyline *>(*it);
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
            b = static_cast<Geo::Bezier *>(*it);
            if (b->is_selected)
            {
                for (const Geo::Point &inner_point : *b)
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
            bs = static_cast<Geo::BSpline *>(*it);
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
            arc = static_cast<Geo::Arc *>(*it);
            if (Geo::distance(point, *arc) <= catch_distance)
            {
                arc->is_selected = true;
                return arc;
            }
            arc = nullptr;
            break;
        case Geo::Type::POINT:
            if (Geo::Point *pt = static_cast<Geo::Point *>(*it);
                Geo::distance_square(point, *pt) <= catch_distance * catch_distance)
            {
                pt->is_selected = true;
                return pt;
            }
            break;
        default:
            break;
        }
    }

    return nullptr;
}

Geo::Geometry *Editer::select(const double x, const double y, const bool reset_others)
{
    return select(Geo::Point(x, y), reset_others);
}

std::tuple<Geo::Geometry *, bool> Editer::select_with_state(const Geo::Point &point, const bool reset_others)
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
    Geo::Bezier *b = nullptr;
    Geo::BSpline *bs = nullptr;
    Combination *cb = nullptr;
    for (std::vector<Geo::Geometry *>::reverse_iterator it = _graph->container_group(_current_group).rbegin(),
                                                        end = _graph->container_group(_current_group).rend();
         it != end; ++it)
    {
        switch ((*it)->type())
        {
        case Geo::Type::TEXT:
            t = static_cast<Text *>(*it);
            if (Geo::is_inside(point, *static_cast<const Geo::AABBRect *>(t), true))
            {
                bool state = t->is_selected;
                t->is_selected = true;
                return std::make_tuple(t, state);
            }
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
            if (Geo::distance(ellipse->c0(), point) + Geo::distance(ellipse->c1(), point)
                    <= catch_distance + std::max(ellipse->lengtha(), ellipse->lengthb()) * 2)
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
                for (Geo::Geometry *item : *cb)
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (Geo::is_inside(point, *static_cast<const Geo::AABBRect *>(item), true))
                        {
                            bool state = cb->is_selected;
                            cb->is_selected = true;
                            return std::make_tuple(cb, state);
                        }
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
                        b = static_cast<Geo::Bezier *>(item);
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
            b = static_cast<Geo::Bezier *>(*it);
            if (b->is_selected)
            {
                for (const Geo::Point &inner_point : *b)
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

std::vector<Geo::Geometry *> Editer::select(const Geo::AABBRect &rect, const bool reset_others)
{
    std::vector<Geo::Geometry *> result;
    if (rect.empty() || _graph == nullptr || _graph->empty())
    {
        return result;
    }

    if (reset_others)
    {
        reset_selected_mark();
    }

    if (const size_t count = _graph->container_group(_current_group).size(); count < 2000)
    {
        select_subfunc(rect, 0, count, &result);
    }
    else if (count < 4000)
    {
        std::vector<Geo::Geometry *> temp0, temp1;
        const size_t step = count / 2;
        std::thread thread0(&Editer::select_subfunc, this, rect, 0, step, &temp0);
        std::thread thread1(&Editer::select_subfunc, this, rect, step, count, &temp1);
        thread0.join();
        thread1.join();
        result.insert(result.end(), temp0.begin(), temp0.end());
        result.insert(result.end(), temp1.begin(), temp1.end());
    }
    else if (count < 6000)
    {
        std::vector<Geo::Geometry *> temp[3];
        const size_t step = count / 3;
        std::thread threads[3] =
        {
            std::thread(&Editer::select_subfunc, this, rect, 0, step, &temp[0]),
            std::thread(&Editer::select_subfunc, this, rect, step, step * 2, &temp[1]),
            std::thread(&Editer::select_subfunc, this, rect, step * 2, count, &temp[2])
        };
        for (int i = 0; i < 3; ++i)
        {
            threads[i].join();
            result.insert(result.end(), temp[i].begin(), temp[i].end());
        }
    }
    else
    {
        std::vector<Geo::Geometry *> temp[4];
        const size_t step = count / 4;
        std::thread threads[4] =
        {
            std::thread(&Editer::select_subfunc, this, rect, 0, step, &temp[0]),
            std::thread(&Editer::select_subfunc, this, rect, step, step * 2, &temp[1]),
            std::thread(&Editer::select_subfunc, this, rect, step * 2, step * 3, &temp[2]),
            std::thread(&Editer::select_subfunc, this, rect, step * 3, count, &temp[3])
        };
        for (int i = 0; i < 4; ++i)
        {
            threads[i].join();
            result.insert(result.end(), temp[i].begin(), temp[i].end());
        }
    }
    return result;
}

std::vector<Geo::Geometry *> Editer::selected(const bool visible_only) const
{
    std::vector<Geo::Geometry *> result;
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
            for (Geo::Geometry *object : group)
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
            for (Geo::Geometry *object : group)
            {
                if (object->is_selected)
                {
                    result.push_back(object);
                }
            }
        }
    }
    return result;
}

const size_t Editer::selected_count() const
{
    if (_graph == nullptr)
    {
        return 0;
    }
    size_t count = 0;
    for (ContainerGroup &group : _graph->container_groups())
    {
        for (Geo::Geometry *object : group)
        {
            if (object->is_selected)
            {
                ++count;
            }
        }
    }
    return count;
}

void Editer::reset_selected_mark(const bool value)
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }
    for (Geo::Geometry *container : _graph->container_group(_current_group))
    {
        container->is_selected = value;
    }
}

const std::vector<Geo::Geometry *> &Editer::paste_table() const
{
    return _paste_table;
}

void Editer::undo()
{
    _backup.undo();
}

void Editer::set_backup_count(const size_t count)
{
    _backup.set_count(count);
}

void Editer::push_backup_command(UndoStack::Command *command)
{
    return _backup.push_command(command);
}


void Editer::remove_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _backup.push_command(new UndoStack::GroupCommand(index, false, _graph->container_group(index)));
    _graph->remove_group(index);
}

void Editer::append_group(const size_t index)
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

void Editer::reorder_group(size_t from, size_t to)
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

bool Editer::group_is_visible(const size_t index) const
{
    return _graph->container_group(index).visible();
}

void Editer::show_group(const size_t index)
{
    _graph->container_group(index).show();
}

void Editer::hide_group(const size_t index)
{
    _graph->container_group(index).hide();
}

QString Editer::group_name(const size_t index) const
{
    return _graph->container_group(index).name;
}

void Editer::set_group_name(const size_t index, const QString &name)
{
    _backup.push_command(new UndoStack::RenameGroupCommand(index, _graph->container_group(index).name));
    _graph->container_group(index).name = name;
}


void Editer::append(Geo::Geometry *object)
{
    if (object->type() != Geo::Type::POINT && object->empty())
    {
        return;
    }
    _graph->append(object, _current_group);
    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(object, 
        _current_group, _graph->container_group(_current_group).size(), true));
}

void Editer::translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape)
{
    const double catch_distance = std::max(GlobalSetting::setting().catch_distance, std::pow(GlobalSetting::setting().catch_distance, 2));
    GlobalSetting::setting().translated_points = true;
    switch (points->type())
    {
    case Geo::Type::POLYGON:
        if (Geo::Polygon *temp = static_cast<Geo::Polygon *>(points); change_shape && !points->shape_fixed)
        {
            size_t count = 0, index = SIZE_MAX;
            double distance, min_distance = DBL_MAX;
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
                if (_edited_shape.empty())
                {
                    for (const Geo::Point &point : *temp)
                    {
                        _edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->at(index).translate(x1 - x0, y1 - y0);
                temp->back() = temp->front();
                _graph->modified = true;
                return;
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::CIRCLE:
        if (Geo::Circle *temp = static_cast<Geo::Circle *>(points); change_shape && !points->shape_fixed)
        {
            if (std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x0, y0))) <= catch_distance ||
                std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x1, y1))) <= catch_distance)
            {
                if (_edited_shape.empty())
                {
                    _edited_shape.emplace_back(temp->x, temp->y);
                    _edited_shape.emplace_back(temp->radius, 0);
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
        if (Geo::Ellipse *temp = static_cast<Geo::Ellipse *>(points); change_shape && !points->shape_fixed)
        {
            const Geo::Point point0(x0, y0), point1(x1, y1);
            const double disa[4] = { Geo::distance(temp->a0(), point0), Geo::distance(temp->a0(), point1), 
                Geo::distance(temp->a1(), point0), Geo::distance(temp->a1(), point1) };
            const double min_disa = *std::min_element(disa, disa + 4);
            const double disb[4] = { Geo::distance(temp->b0(), point0), Geo::distance(temp->b0(), point1),
                Geo::distance(temp->b1(), point0), Geo::distance(temp->b1(), point1) };
            const double min_disb = *std::min_element(disb, disb + 4);
            if (min_disa < min_disb && min_disa <= catch_distance)
            {
                if (_edited_shape.empty())
                {
                    _edited_shape.emplace_back(temp->a0().x, temp->a0().y);
                    _edited_shape.emplace_back(temp->a1().x, temp->a1().y);
                    _edited_shape.emplace_back(temp->b0().x, temp->b0().y);
                    _edited_shape.emplace_back(temp->b1().x, temp->b1().y);
                    _edited_shape.emplace_back(temp->arc_angle0(), temp->arc_angle1());
                }
                temp->set_lengtha(Geo::distance(point1, temp->center()));
                temp->update_shape(Geo::Ellipse::default_down_sampling_value);
            }
            else if (min_disb < min_disa && min_disb <= catch_distance)
            {
                if (_edited_shape.empty())
                {
                    _edited_shape.emplace_back(temp->a0().x, temp->a0().y);
                    _edited_shape.emplace_back(temp->a1().x, temp->a1().y);
                    _edited_shape.emplace_back(temp->b0().x, temp->b0().y);
                    _edited_shape.emplace_back(temp->b1().x, temp->b1().y);
                    _edited_shape.emplace_back(temp->arc_angle0(), temp->arc_angle1());
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
    case Geo::Type::TEXT:
    case Geo::Type::COMBINATION:
    case Geo::Type::ARC:
    case Geo::Type::POINT:
        points->translate(x1 - x0, y1 - y0);
        break;
    case Geo::Type::POLYLINE:
        if (Geo::Polyline *temp = static_cast<Geo::Polyline *>(points); change_shape && !points->shape_fixed)
        {
            size_t count = 0, index = SIZE_MAX;
            double distance, min_distance = DBL_MAX;
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
                if (_edited_shape.empty())
                {
                    for (const Geo::Point &point : *temp)
                    {
                        _edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->at(index).translate(x1 - x0, y1 - y0);
                _graph->modified = true;
                return;
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::BEZIER:
        if ( Geo::Bezier *temp = static_cast<Geo::Bezier *>(points); change_shape && !temp->shape_fixed)
        {
            size_t count = temp->size(), index = SIZE_MAX;
            double distance, min_distance = DBL_MAX;
            for (size_t i = 0; i < count; ++i)
            {
                distance = std::min(Geo::distance_square(x0, y0, (*temp)[i].x, (*temp)[i].y), Geo::distance_square(x1, y1, (*temp)[i].x, (*temp)[i].y));
                if (distance <= catch_distance * catch_distance && distance < min_distance)
                {
                    index = i;
                    min_distance = distance;
                }
            }
            if (index < SIZE_MAX)
            {
                if (_edited_shape.empty())
                {
                    for (const Geo::Point &point : *temp)
                    {
                        _edited_shape.emplace_back(point.x, point.y);
                    }
                }

                temp->at(index).translate(x1 - x0, y1 - y0);
                if (index > 2 && index % temp->order() == 1)
                {
                    (*temp)[index - 2] = (*temp)[index - 1] + ((*temp)[index - 1] - (*temp)[index]).normalize() * Geo::distance((*temp)[index - 2], (*temp)[index - 1]);
                    if (temp->order() == 2)
                    {
                        for (int j = index; j + 2 < count; j += 2)
                        {
                            (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                        }
                        for (int j = index - 2; j > 2; j -= 2)
                        {
                            (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                        }
                    }
                }
                else if (index + 2 < temp->size() && index % temp->order() == temp->order() - 1)
                {
                    (*temp)[index + 2] = (*temp)[index + 1] + ((*temp)[index + 1] - (*temp)[index]).normalize() * Geo::distance((*temp)[index + 1], (*temp)[index + 2]);
                    if (temp->order() == 2)
                    {
                        for (int j = index + 2; j + 2 < count; j += 2)
                        {
                            (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                        }
                        for (int j = index; j > 2; j -= 2)
                        {
                            (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                        }
                    }
                }
                else if (index % temp->order() == 0 && index > 0 && index < count - 1)
                {
                    (*temp)[index - 1].translate(x1 - x0, y1 - y0);
                    (*temp)[index + 1].translate(x1 - x0, y1 - y0);
                    if (temp->order() == 2)
                    {
                        for (int j = index + 1; j + 2 < count; j += 2)
                        {
                            (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                        }
                        for (int j = index - 1; j > 2; j -= 2)
                        {
                            (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                        }
                    }
                }
                temp->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
                _graph->modified = true;
                return;
            }
        }
        else
        {
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::BSPLINE:
        if (Geo::BSpline *temp = static_cast<Geo::BSpline *>(points); change_shape && !temp->shape_fixed)
        {
            if (temp->controls_model)
            {
                size_t count = temp->control_points.size(), index = SIZE_MAX;
                double distance, min_distance = DBL_MAX;
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
                    if (_edited_shape.empty())
                    {
                        _edited_shape.emplace_back(temp->path_points.size(), temp->control_points.size());
                        for (const Geo::Point &point : temp->path_points)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                        for (const Geo::Point &point : temp->control_points)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                    }

                    temp->control_points[index].translate(x1 - x0, y1 - y0);
                    temp->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
                    _graph->modified = true;
                    return;
                }
            }
            else
            {
                size_t count = temp->path_points.size(), index = SIZE_MAX;
                double distance, min_distance = DBL_MAX;
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
                    if (_edited_shape.empty())
                    {
                        _edited_shape.emplace_back(temp->path_points.size(), temp->control_points.size());
                        for (const Geo::Point &point : temp->path_points)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                        for (const Geo::Point &point : temp->control_points)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                    }

                    temp->path_points[index].translate(x1 - x0, y1 - y0);
                    temp->update_control_points();
                    temp->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
                    _graph->modified = true;
                    return;
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
}

bool Editer::remove_selected()
{
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected)
        {
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));
    return true;
}

bool Editer::copy_selected()
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

    for (const Geo::Geometry *container : _graph->container_group(_current_group))
    {
        if (container->is_selected)
        {
            _paste_table.push_back(container->clone());
        }
    }

    reset_selected_mark();
    return true;
}

bool Editer::cut_selected()
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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected)
        {
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _paste_table.push_back(std::get<0>(items.back())->clone());
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
        _paste_table.push_back(std::get<0>(items.back())->clone());
    }
    std::reverse(_paste_table.begin(), _paste_table.end());

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));

    return true;
}

bool Editer::paste(const double tx, const double ty)
{
    if (_paste_table.empty() || _graph == nullptr)
    {
        return false;
    }

    reset_selected_mark();
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (Geo::Geometry *geo : _paste_table)
    {
        _graph->container_group(_current_group).append(geo->clone());
        _graph->container_group(_current_group).back()->translate(tx, ty);
        items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

bool Editer::connect(std::vector<Geo::Geometry *> objects, const double connect_distance)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<Geo::Polyline *> polylines;
    std::vector<size_t> indexs;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::POLYLINE)
        {
            polylines.push_back(static_cast<Geo::Polyline *>(object));
            indexs.push_back(std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }
    std::vector<bool> merged(polylines.size(), false);

    Geo::Polyline *polyline = nullptr;
    size_t index = 0;
    for (size_t i = 0, count = indexs.size(); i < count; ++i)
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
            else if (Geo::distance_square(back_i, front_j) < connect_distance * connect_distance )
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
            indexs.erase(indexs.begin() + i);
        }
    }
    if (!merged.front())
    {
        indexs.erase(indexs.begin());
    }
    std::sort(indexs.begin(), indexs.end(), std::greater<size_t>());
    std::vector<std::tuple<Geo::Geometry *, size_t>> items;
    for (size_t i : indexs)
    {
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

    _graph->modified = true;
    _backup.push_command(new UndoStack::ConnectCommand(items, polyline, _current_group));
    return true;
}

bool Editer::blend(const Geo::Geometry *object0, const Geo::Geometry *object1, const Geo::Point &pos0, const Geo::Point &pos1)
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
        if (const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(object0);
            Geo::distance_square(bezier->front(), pos0) < Geo::distance_square(bezier->back(), pos0))
        {
            point0 = bezier->front();
            pre0 = bezier->at(1);
        }
        else
        {
            point0 = bezier->back();
            pre0 = bezier->at(bezier->size() - 2);
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
        if (const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(object1);
            Geo::distance_square(bezier->front(), pos1) < Geo::distance_square(bezier->back(), pos1))
        {
            point1 = bezier->front();
            pre1 = bezier->at(1);
        }
        else
        {
            point1 = bezier->back();
            pre1 = bezier->at(bezier->size() - 2);
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

    if (Geo::Bezier *bezier = Geo::blend(pre0, point0, point1, pre1))
    {
        bezier->is_selected = true;
        _graph->container_group(_current_group).append(bezier);
        _backup.push_command(new UndoStack::ObjectCommand(bezier, _current_group,
            _graph->container_group(_current_group).size() - 1, true));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::close_polyline(std::vector<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    Geo::Polygon *shape = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    size_t index;
    for (Geo::Geometry *object : objects)
    {
        if (object->type() != Geo::Type::POLYLINE)
        {
            continue;
        }
        if (const Geo::Polyline *polyline = static_cast<const Geo::Polyline *>(object);
            polyline->size() >= 3)
        {
            shape = new Geo::Polygon(*polyline);
            shape->is_selected = true;
            index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            add_items.emplace_back(shape, _current_group, index);
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

bool Editer::combinate(std::vector<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.size() < 2)
    {
        return false;
    }

    size_t count = 0;
    std::vector<Geo::Geometry *> filtered_objects;
    for (Geo::Geometry *object : objects)
    {
        filtered_objects.push_back(object);
    }
    if (filtered_objects.size() < 2)
    {
        return false;
    }

    Combination *combination = new Combination();
    std::reverse(objects.begin(), objects.end());
    Combination *temp = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> items;
    for (Geo::Geometry *object : filtered_objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            std::vector<Geo::Geometry *>::iterator it = std::find(group.begin(), group.end(), object);
            size_t index = std::distance(group.begin(), it);
            temp = static_cast<Combination *>(group.pop(it));
            items.emplace_back(temp, index, std::vector<Geo::Geometry *>(temp->begin(), temp->end()));
            combination->append(temp);
        }
        else
        {
            combination->append(group.pop(std::find(group.rbegin(), group.rend(), object)));
        }
    }

    std::reverse(combination->begin(), combination->end());
    combination->is_selected = true;
    combination->update_border();
    _graph->container_group(_current_group).append(combination);

    _backup.push_command(new UndoStack::CombinateCommand(combination, items, _current_group));
    _graph->modified = true;

    return true;
}

bool Editer::detach(std::vector<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Combination *, size_t>> combiantions;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : objects)
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
    _backup.push_command(new UndoStack::CombinateCommand(combiantions, _current_group));
    for (std::tuple<Combination *, size_t> &combination : combiantions)
    {
        std::reverse(std::get<0>(combination)->begin(), std::get<0>(combination)->end());
        group.pop(std::find(group.rbegin(), group.rend(), std::get<0>(combination)));
        group.append(*static_cast<ContainerGroup *>(std::get<0>(combination)));
    }

    _graph->modified = true;

    return true;
}

bool Editer::mirror(std::vector<Geo::Geometry *> objects, const Geo::Point &start, const Geo::Point &end, const bool copy)
{
    if (objects.empty() || start == end)
    {
        return false;
    }

    const double a = start.y - end.y;
    const double b = end.x - start.x;
    const double c = start.x * end.y - end.x * start.y;
    const double d = a * a + b * b;
    const double mat[6] = {(b * b - a * a) / d, -2 * a * b / d, -2 * a * c / d,
        -2 *a * b / d, (a * a - b * b) / d, -2 * b * c / d};

    if (copy)
    {
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
        size_t index = _graph->container_group(_current_group).size();
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->transform(mat);
            _graph->container_group(_current_group).back()->is_selected = true;
            items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        }
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
    }
    else
    {
        for (Geo::Geometry *obj : objects)
        {
            obj->transform(mat);
            obj->is_selected = true;
        }
        _backup.push_command(new UndoStack::TransformCommand(objects, mat));
    }

    _graph->modified = true;

    return true;
}

bool Editer::offset(std::vector<Geo::Geometry *> objects, const double distance, const Geo::Offset::JoinType join_type, const Geo::Offset::EndType end_type)
{
    const size_t count = _graph->container_group(_current_group).size();
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    Geo::Ellipse *ellipse = nullptr;
    Geo::BSpline *bspline = nullptr;
    Geo::Bezier *bezier = nullptr;
    Geo::Arc *arc = nullptr;
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = count;
    for (Geo::Geometry *object : objects)
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
                _graph->append(new Geo::Ellipse(ellipse->center(), ellipse->lengtha() + distance,
                    ellipse->lengthb() + distance), _current_group);
                _graph->container_group(_current_group).back()->rotate(ellipse->center().x,
                    ellipse->center().y, ellipse->angle());
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
            bspline = static_cast<Geo::BSpline *>(object);
            if (Geo::Polyline shape1, path_points(bspline->path_points.begin(), bspline->path_points.end());
                Geo::offset(path_points, shape1, distance))
            {
                double area = 0;
                for (size_t i = 1, count = path_points.size(); i < count; ++i)
                {
                    area += (path_points[i].x * (path_points[i+1 != count ? i+1 : 0].y - path_points[i-1].y));
                }
                area += (path_points.front().x * (path_points[1].y - path_points.back().y));
                if (area > 0)
                {
                    path_points.flip();
                }
                shape1.front() = (path_points.front() + (path_points[1] - path_points[0]).vertical().normalize() * distance);
                shape1.back() = (path_points.back() + (path_points.back() - path_points[path_points.size() - 2]).vertical().normalize() * distance);

                if (dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr)
                {
                    _graph->append(new Geo::QuadBSpline(shape1.begin(), shape1.end(), true), _current_group);
                }
                else
                {
                    _graph->append(new Geo::CubicBSpline(shape1.begin(), shape1.end(), true), _current_group);
                }
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::BEZIER:
            bezier = static_cast<Geo::Bezier *>(object);
            if (Geo::Polyline shape1, path_points(*static_cast<Geo::Polyline *>(bezier));
                Geo::offset(path_points, shape1, distance))
            {
                double area = 0;
                for (size_t i = 1, count = path_points.size(); i < count; ++i)
                {
                    area += (path_points[i].x * (path_points[i+1 != count ? i+1 : 0].y - path_points[i-1].y));
                }
                area += (path_points.front().x * (path_points[1].y - path_points.back().y));
                if (area > 0)
                {
                    path_points.flip();
                }
                shape1.front() = (path_points.front() + (path_points[1] - path_points[0]).vertical().normalize() * distance);
                shape1.back() = (path_points.back() + (path_points.back() - path_points[path_points.size() - 2]).vertical().normalize() * distance);

                _graph->append(new Geo::Bezier(shape1.begin(), shape1.end(), bezier->order(), true), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::ARC:
            arc = static_cast<Geo::Arc *>(object);
            if (distance >= 0 || -distance < arc->radius)
            {
                const Geo::Point center(arc->x, arc->y);
                _graph->append(new Geo::Arc(arc->x, arc->y, arc->radius + distance, Geo::angle(center, arc->control_points[0]),
                    Geo::angle(center, arc->control_points[2]), !arc->is_cw()), _current_group);
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
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
        return true;
    }
}

bool Editer::scale(std::vector<Geo::Geometry *> objects, const bool unitary, const double k)
{
    if (objects.empty() || k == 0 || k == 1)
    {
        return false;
    }

    const size_t count = _graph->container_group(_current_group).size();

    if (unitary)
    {
        double top = -DBL_MAX, bottom = DBL_MAX, left = DBL_MAX, right = -DBL_MAX;
        bool flag = false;
        Geo::AABBRect rect;
        for (Geo::Geometry *object : objects)
        {
            if (object->type() == Geo::Type::TEXT)
            {
                object->is_selected = false;
            }
            else
            {
                rect = object->bounding_rect();
                top = std::max(top, rect.top());
                bottom = std::min(bottom, rect.bottom());
                left = std::min(left, rect.left());
                right = std::max(right, rect.right());
                flag = true;
            }
        }

        if (!flag)
        {
            return false;
        }

        const double x = (left + right) / 2, y = (top + bottom) / 2;
        for (Geo::Geometry *object : objects)
        {
            if (object->type() != Geo::Type::TEXT)
            {
                object->scale(x, y, k);
            }
        }

        _backup.push_command(new UndoStack::ScaleCommand(objects, x, y, k, unitary));
    }
    else
    {
        bool flag = false;
        Geo::AABBRect rect;
        for (Geo::Geometry *object : objects)
        {
            if (object->type() == Geo::Type::TEXT)
            {
                object->is_selected = false;
            }
            else
            {
                rect = object->bounding_rect();
                object->scale((rect.left() + rect.right()) / 2, (rect.top() + rect.bottom()) / 2, k);
                flag = true;
            }
        }

        if (!flag)
        {
            return false;
        }

        _backup.push_command(new UndoStack::ScaleCommand(objects, 0, 0, k, unitary));
    }
    
    _graph->modified = true;

    return true;
}

bool Editer::polygon_union(Geo::Polygon *shape0, Geo::Polygon *shape1)
{
    if (_graph == nullptr || _graph->empty() || shape0 == nullptr || shape1 == nullptr  || shape0 == shape1)
    {
        return false;
    }

    if (std::vector<Geo::Polygon> shapes; Geo::polygon_union(*shape0, *shape1, shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        remove_items.emplace_back(shape0, _current_group, index0);
        remove_items.emplace_back(shape1, _current_group, index1);
        group.pop(index1);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Geo::Polygon(shapes.front()));
        }
        else
        {
            group.insert(index0, new Geo::Polygon(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Geo::Polygon(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_intersection(Geo::Polygon *shape0, Geo::Polygon *shape1)
{
    if (_graph == nullptr || _graph->empty() || shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    if (std::vector<Geo::Polygon> shapes; Geo::polygon_intersection(*shape0, *shape1, shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        remove_items.emplace_back(shape0, _current_group, index0);
        remove_items.emplace_back(shape1, _current_group, index1);
        group.pop(index1);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Geo::Polygon(shapes.front()));
        }
        else
        {
            group.insert(index0, new Geo::Polygon(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();        
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Geo::Polygon(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_difference(Geo::Polygon *shape0, const Geo::Polygon *shape1)
{
    if (shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    if (std::vector<Geo::Polygon> shapes; Geo::polygon_difference(*shape0, *shape1, shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        remove_items.emplace_back(shape0, _current_group, index0);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Geo::Polygon(shapes.front()));
        }
        else
        {
            group.insert(index0, new Geo::Polygon(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Geo::Polygon(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_xor(Geo::Polygon *shape0, Geo::Polygon *shape1)
{
    if (shape0 == nullptr || shape1 == nullptr || shape0 == shape1)
    {
        return false;
    }

    if (std::vector<Geo::Polygon> shapes; Geo::polygon_xor(*shape0, *shape1, shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), shape1));
        remove_items.emplace_back(shape0, _current_group, index0);
        remove_items.emplace_back(shape1, _current_group, index1);
        group.pop(index1);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Geo::Polygon(shapes.front()));
        }
        else
        {
            group.insert(index0, new Geo::Polygon(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Geo::Polygon(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polygon *shape, const Geo::Point &point, const double radius)
{
    if (radius <= 0)
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
    if (Geo::Arc arc; Geo::angle_to_arc(polygon[index0], polygon[index1], polygon[index2], radius, arc))
    {
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == shape)
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                _graph->container_group(_current_group).pop(i);
                shape->is_selected = false;
                remove_items.emplace_back(shape, _current_group, i);
                std::vector<Geo::Point> points(shape->begin(), shape->end() - 1);
                std::rotate(points.begin(), points.begin() + index1, points.end());
                points.front() = arc.control_points[2];
                points.emplace_back(arc.control_points[0]);
                Geo::Polyline *polyline = new Geo::Polyline(points.begin(), points.end());
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                Geo::Arc *a = new Geo::Arc(arc);
                a->is_selected = true;
                add_items.emplace_back(a, _current_group, i + 1);
                _graph->container_group(_current_group).insert(i + 1, a);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polygon *shape, const Geo::Point &point, const double radius0, const double radius1)
{
    if (radius0 <= 0 || radius1 <= 0)
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
    if (Geo::Bezier arc(3); Geo::angle_to_arc(polygon[index0], polygon[index1], polygon[index2], radius0, radius1, arc))
    {
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == shape)
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                _graph->container_group(_current_group).pop(i);
                shape->is_selected = false;
                remove_items.emplace_back(shape, _current_group, i);
                std::vector<Geo::Point> points(shape->begin(), shape->end() - 1);
                std::rotate(points.begin(), points.begin() + index1, points.end());
                points.front() = arc.back();
                points.emplace_back(arc.front());
                Geo::Polyline *polyline = new Geo::Polyline(points.begin(), points.end());
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                Geo::Bezier *a = new Geo::Bezier(arc);
                a->is_selected = true;
                add_items.emplace_back(a, _current_group, i + 1);
                _graph->container_group(_current_group).insert(i + 1, a);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polyline *polyline, const Geo::Point &point, const double radius)
{
    if (radius <= 0 || point == polyline->front() || point == polyline->back())
    {
        return false;
    }
    std::vector<Geo::Point>::const_iterator it = std::find(polyline->begin(), polyline->end(), point);
    if (it == polyline->end())
    {
        return false;
    }
    const size_t index = std::distance(polyline->cbegin(), it);
    if (Geo::Arc arc; Geo::angle_to_arc((*polyline)[index - 1], (*polyline)[index], (*polyline)[index + 1], radius, arc))
    {
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == polyline)
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                _graph->container_group(_current_group).pop(i);
                polyline->is_selected = false;
                remove_items.emplace_back(polyline, _current_group, i);
                Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + index + 1);
                polyline0->back() = arc.control_points[0];
                add_items.emplace_back(polyline0, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline0);
                Geo::Arc *a = new Geo::Arc(arc);
                a->is_selected = true;
                add_items.emplace_back(a, _current_group, i + 1);
                _graph->container_group(_current_group).insert(i + 1, a);
                Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + index, polyline->end());
                polyline1->front() = arc.control_points[2];
                add_items.emplace_back(polyline1, _current_group, i + 2);
                _graph->container_group(_current_group).insert(i + 2, polyline1);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polyline *polyline, const Geo::Point &point, const double radius0, const double radius1)
{
    if (radius0 <= 0 || radius1 <= 0 || point == polyline->front() || point == polyline->back())
    {
        return false;
    }
    std::vector<Geo::Point>::const_iterator it = std::find(polyline->begin(), polyline->end(), point);
    if (it == polyline->end())
    {
        return false;
    }
    const size_t index = std::distance(polyline->cbegin(), it);
    if (Geo::Bezier arc(3); Geo::angle_to_arc((*polyline)[index - 1], (*polyline)[index], (*polyline)[index + 1], radius0, radius1, arc))
    {
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == polyline)
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                _graph->container_group(_current_group).pop(i);
                polyline->is_selected = false;
                remove_items.emplace_back(polyline, _current_group, i);
                Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + index + 1);
                polyline0->back() = arc.front();
                add_items.emplace_back(polyline0, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline0);
                Geo::Bezier *a = new Geo::Bezier(arc);
                a->is_selected = true;
                add_items.emplace_back(a, _current_group, i + 1);
                _graph->container_group(_current_group).insert(i + 1, a);
                Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + index, polyline->end());
                polyline1->front() = arc.back();
                add_items.emplace_back(polyline1, _current_group, i + 2);
                _graph->container_group(_current_group).insert(i + 2, polyline1);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polyline *polyline0, const Geo::Point &point0, Geo::Polyline *polyline1, const Geo::Point &point1, const double radius)
{
    if (radius <= 0 || polyline0 == polyline1 || polyline0->empty() || polyline1->empty())
    {
        return false;
    }

    Geo::Point head0, tail0, head1, tail1;
    Geo::Polyline polyline0_copy(*polyline0), polyline1_copy(*polyline1);
    if (Geo::distance(polyline0->front(), point0) <= Geo::distance(polyline0->back(), point0))
    {
        head0 = polyline0->front();
        tail0 = polyline0->at(1);
        polyline0_copy.remove(0);
    }
    else
    {
        head0 = polyline0->back();
        tail0 = polyline0->at(polyline0->size() - 2);
        polyline0_copy.remove(polyline0_copy.size() - 1);
    }
    if (Geo::distance(polyline1->front(), point1) <= Geo::distance(polyline1->back(), point1))
    {
        head1 = polyline1->front();
        tail1 = polyline1->at(1);
        polyline1_copy.remove(0);
    }
    else
    {
        head1 = polyline1->back();
        tail1 = polyline1->at(polyline1->size() - 2);
        polyline1_copy.remove(polyline1_copy.size() - 1);
    }

    Geo::Polyline *polyline2 = nullptr, *polyline3 = nullptr;
    Geo::Arc *arc = nullptr;
    if (Geo::Point center; Geo::is_intersected(head0, tail0, head1, tail1, center, true))
    {
        if ((head0 - center) * (tail0 - center) < 0 && (head1 - center) * (tail1 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
            else
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
        }
        else if ((head0 - center) * (tail0 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
        }
        else if ((head1 - center) * (tail1 - center) < 0)
        {
            if ((head1 - center) * (point1 - center) > 0)
            {
                if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
        }
        else
        {
            if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(tail0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, tail1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
                else
                {
                    if (Geo::Arc arc0; Geo::angle_to_arc(head0, center, head1, radius, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.control_points[0]);
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.control_points[2]);
                        arc = new Geo::Arc(arc0);
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }

    if (arc == nullptr)
    {
        return false;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, k = 0, count = _graph->container_group(_current_group).size(); i < count && k < 2; ++i)
    {
        if (_graph->container_group(_current_group)[i] == polyline0)
        {
            ++k;
            _graph->container_group(_current_group).pop(i);
            polyline0->is_selected = false;
            remove_items.emplace_back(polyline0, _current_group, i);
            size_t j = i;
            if (polyline0_copy.size() > 1)
            {
                Geo::Polyline *polyline = new Geo::Polyline(polyline0_copy);
                polyline->is_selected = false;
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                ++count;
            }
            else
            {
                --j;
            }
            add_items.emplace_back(polyline2, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, polyline2);
            polyline2->is_selected = false;
            add_items.emplace_back(arc, _current_group, j + 2);
            _graph->container_group(_current_group).insert(j + 2, arc);
            arc->is_selected = true;
            ++count;
        }
        else if (_graph->container_group(_current_group)[i] == polyline1)
        {
            ++k;
            _graph->container_group(_current_group).pop(i);
            polyline1->is_selected = false;
            remove_items.emplace_back(polyline1, _current_group, i);
            size_t j = i;
            if (polyline1_copy.size() > 1)
            {
                Geo::Polyline *polyline = new Geo::Polyline(polyline1_copy);
                polyline->is_selected = false;
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                ++count;
            }
            else
            {
                --j;
            }
            add_items.emplace_back(polyline3, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, polyline3);
            polyline3->is_selected = false;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    return true;
}

bool Editer::fillet(Geo::Polyline *polyline0, const Geo::Point &point0, Geo::Polyline *polyline1, const Geo::Point &point1, const double radius0, const double radius1)
{
    if (radius0 <= 0 || radius1 <= 0 || polyline0 == polyline1 || polyline0->empty() || polyline1->empty())
    {
        return false;
    }

    Geo::Point head0, tail0, head1, tail1;
    Geo::Polyline polyline0_copy(*polyline0), polyline1_copy(*polyline1);
    if (Geo::distance(polyline0->front(), point0) <= Geo::distance(polyline0->back(), point0))
    {
        head0 = polyline0->front();
        tail0 = polyline0->at(1);
        polyline0_copy.remove(0);
    }
    else
    {
        head0 = polyline0->back();
        tail0 = polyline0->at(polyline0->size() - 2);
        polyline0_copy.remove(polyline0_copy.size() - 1);
    }
    if (Geo::distance(polyline1->front(), point1) <= Geo::distance(polyline1->back(), point1))
    {
        head1 = polyline1->front();
        tail1 = polyline1->at(1);
        polyline1_copy.remove(0);
    }
    else
    {
        head1 = polyline1->back();
        tail1 = polyline1->at(polyline1->size() - 2);
        polyline1_copy.remove(polyline1_copy.size() - 1);
    }

    Geo::Polyline *polyline2 = nullptr, *polyline3 = nullptr;
    Geo::Bezier *arc = nullptr;
    if (Geo::Point center; Geo::is_intersected(head0, tail0, head1, tail1, center, true))
    {
        if ((head0 - center) * (tail0 - center) < 0 && (head1 - center) * (tail1 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
            else
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
        }
        else if ((head0 - center) * (tail0 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
        }
        else if ((head1 - center) * (tail1 - center) < 0)
        {
            if ((head1 - center) * (point1 - center) > 0)
            {
                if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
        }
        else
        {
            if (Geo::distance(center, head0) <= Geo::distance(center, tail0))
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(tail0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
            else
            {
                if (Geo::distance(center, head1) <= Geo::distance(center, tail1))
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(tail1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
                else
                {
                    if (Geo::Bezier arc0(3); Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc0))
                    {
                        polyline2 = new Geo::Polyline();
                        polyline2->append(head0);
                        polyline2->append(arc0.front());
                        polyline3 = new Geo::Polyline();
                        polyline3->append(head1);
                        polyline3->append(arc0.back());
                        arc = new Geo::Bezier(arc0);
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }

    if (arc == nullptr)
    {
        return false;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, k = 0, count = _graph->container_group(_current_group).size(); i < count && k < 2; ++i)
    {
        if (_graph->container_group(_current_group)[i] == polyline0)
        {
            ++k;
            _graph->container_group(_current_group).pop(i);
            polyline0->is_selected = false;
            remove_items.emplace_back(polyline0, _current_group, i);
            size_t j = i;
            if (polyline0_copy.size() > 1)
            {
                Geo::Polyline *polyline = new Geo::Polyline(polyline0_copy);
                polyline->is_selected = false;
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                ++count;
            }
            else
            {
                --j;
            }
            add_items.emplace_back(polyline2, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, polyline2);
            polyline2->is_selected = false;
            add_items.emplace_back(arc, _current_group, j + 2);
            _graph->container_group(_current_group).insert(j + 2, arc);
            arc->is_selected = true;
            ++count;
        }
        else if (_graph->container_group(_current_group)[i] == polyline1)
        {
            ++k;
            _graph->container_group(_current_group).pop(i);
            polyline1->is_selected = false;
            remove_items.emplace_back(polyline1, _current_group, i);
            size_t j = i;
            if (polyline1_copy.size() > 1)
            {
                Geo::Polyline *polyline = new Geo::Polyline(polyline1_copy);
                polyline->is_selected = false;
                add_items.emplace_back(polyline, _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                ++count;
            }
            else
            {
                --j;
            }
            add_items.emplace_back(polyline3, _current_group, j + 1);
            _graph->container_group(_current_group).insert(j + 1, polyline3);
            polyline3->is_selected = false;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    return true;
}

bool Editer::fillet(Geo::Geometry *object0, Geo::Geometry *object1, const Geo::Point &start, const Geo::Point &center,
    const Geo::Point &end, const std::vector<std::tuple<size_t, double, double, double>> &tvalues)
{
    if (object0 == object1 || start == center || center == end || start == end)
    {
        return false;
    }

    if (Geo::Bezier curve(3); Geo::angle_to_arc(start, center, end, curve))
    {
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
        switch (object0->type())
        {
        case Geo::Type::ARC:
            {
                Geo::Arc *arc = static_cast<Geo::Arc *>(object0);
                if (Geo::Arc arc0, arc1; Geo::split(*arc, start, arc0, arc1))
                {
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), arc));
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
                Geo::Bezier *bezier = static_cast<Geo::Bezier *>(object0);
                if (Geo::Bezier bezier0(bezier->order()), bezier1(bezier->order());
                    Geo::split(*bezier, std::get<0>(tvalues.front()), std::get<1>(tvalues.front()), bezier0, bezier1))
                {
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bezier));
                    remove.emplace_back(bezier, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bezier0.front(), start) < Geo::distance(bezier0.back(), start))
                        {
                            if (Geo::is_on_left(bezier0[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0[bezier0.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bezier0.front(), start) < Geo::distance(bezier0.back(), start))
                        {
                            if (Geo::is_on_left(bezier0[1], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0[bezier0.size() - 2], center, start))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bspline));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bspline));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), ellipse));
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
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), polyline));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), arc));
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
                Geo::Bezier *bezier = static_cast<Geo::Bezier *>(object1);
                if (Geo::Bezier bezier0(bezier->order()), bezier1(bezier->order());
                    Geo::split(*bezier, std::get<0>(tvalues.back()), std::get<1>(tvalues.back()), bezier0, bezier1))
                {
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bezier));
                    remove.emplace_back(bezier, _current_group, index);
                    _graph->container_group(_current_group).pop(index);
                    if (Geo::angle(start, center, end) > 0)
                    {
                        if (Geo::distance(bezier0.front(), end) < Geo::distance(bezier0.back(), end))
                        {
                            if (Geo::is_on_left(bezier0[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0[bezier0.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                        }
                    }
                    else
                    {
                        if (Geo::distance(bezier0.front(), end) < Geo::distance(bezier0.back(), end))
                        {
                            if (Geo::is_on_left(bezier0[1], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
                            }
                        }
                        else
                        {
                            if (Geo::is_on_left(bezier0[bezier0.size() - 2], center, end))
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                            }
                            else
                            {
                                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bspline));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), bspline));
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
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), ellipse));
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
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), polyline));
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

        _graph->container_group(_current_group).append(new Geo::Bezier(curve));
        append.emplace_back(_graph->container_group(_current_group).back(), _current_group,
            _graph->container_group(_current_group).size() - 1);
        _backup.push_command(new UndoStack::ObjectCommand(append, remove));
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::chamfer(Geo::Polygon *shape, const Geo::Point &point, const double distance)
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

    if (Geo::distance(polygon[index1], polygon[index0]) >= distance &&
        Geo::distance(polygon[index1], polygon[index2]) >= distance)
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

        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::chamfer(Geo::Polyline *polyline, const Geo::Point &point, const double distance)
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
    if (Geo::distance((*polyline)[index - 1], (*polyline)[index]) >= distance
        && Geo::distance((*polyline)[index + 1], (*polyline)[index]) >= distance)
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

        _graph->modified = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::split(Geo::Geometry *object, const Geo::Point &pos)
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
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                _graph->container_group(_current_group).insert(index, new Geo::Polyline(polyline1));
                _graph->container_group(_current_group).insert(index, new Geo::Polyline(polyline0));
                append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                return true;
            }
        }
        break;
    case Geo::Type::BEZIER:
        {
            Geo::Bezier *bezier = static_cast<Geo::Bezier *>(object);
            std::vector<Geo::Point> coords;
            std::vector<std::tuple<size_t, double, double, double>> tvalues;
            Geo::closest_point(*bezier, pos, coords, &tvalues);
            if (Geo::Bezier bezier0(bezier->order()), bezier1(bezier->order()); !tvalues.empty() &&
                Geo::split(*bezier, std::get<0>(tvalues.front()), std::get<1>(tvalues.front()), bezier0, bezier1))
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier1));
                _graph->container_group(_current_group).insert(index, new Geo::Bezier(bezier0));
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
                if (Geo::CubicBSpline bspline0(*cubicbspline), bspline1(*cubicbspline); !tvalues.empty() &&
                    Geo::split(*static_cast<Geo::BSpline *>(object), is_cubic, std::get<0>(tvalues.front()), bspline0, bspline1))
                {
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                            _graph->container_group(_current_group).end(), object));
                    remove.emplace_back(_graph->container_group(_current_group).pop(index), _current_group, index);
                    _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline1));
                    _graph->container_group(_current_group).insert(index, new Geo::CubicBSpline(bspline0));
                    append.emplace_back(_graph->container_group(_current_group)[index], _current_group, index);
                    append.emplace_back(_graph->container_group(_current_group)[index + 1], _current_group, index + 1);
                    _backup.push_command(new UndoStack::ObjectCommand(append, remove));
                    return true;
                }
            }
            else
            {
                if (Geo::QuadBSpline bspline0(*quadbspline), bspline1(*quadbspline); !tvalues.empty() &&
                    Geo::split(*static_cast<Geo::BSpline *>(object), is_cubic, std::get<0>(tvalues.front()), bspline0, bspline1))
                {
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                    size_t index = std::distance(_graph->container_group(_current_group).begin(),
                        std::find(_graph->container_group(_current_group).begin(),
                            _graph->container_group(_current_group).end(), object));
                    remove.emplace_back(_graph->container_group(_current_group).pop(index), _current_group, index);
                    _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline1));
                    _graph->container_group(_current_group).insert(index, new Geo::QuadBSpline(bspline0));
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
            if (Geo::Point point0, point1; Geo::is_intersected(pos, Geo::Point(arc->x, arc->y),
                *arc, point0, point1, false) && Geo::split(*arc, point0, arc0, arc1))
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                _graph->container_group(_current_group).insert(index, new Geo::Arc(arc1));
                _graph->container_group(_current_group).insert(index, new Geo::Arc(arc0));
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
            if (Geo::Point point0, point1; Geo::is_intersected(ellipse->center(), pos, *ellipse,
                point0, point1, false) && Geo::split(*ellipse, point0, ellipse0, ellipse1))
            {
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove, append;
                size_t index = std::distance(_graph->container_group(_current_group).begin(),
                    std::find(_graph->container_group(_current_group).begin(),
                        _graph->container_group(_current_group).end(), object));
                remove.emplace_back(object, _current_group, index);
                _graph->container_group(_current_group).pop(index);
                _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse1));
                _graph->container_group(_current_group).insert(index, new Geo::Ellipse(ellipse0));
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

bool Editer::line_array(std::vector<Geo::Geometry *> objects, int x, int y, double x_space, double y_space)
{
    if (objects.empty() || x == 0 || y == 0 || (x == 1 && y == 1))
    {
        return false;
    }

    double left = DBL_MAX, right = -DBL_MAX, top = -DBL_MAX, bottom = DBL_MAX;
    for (const Geo::Geometry *object : objects)
    {
        const Geo::AABBRect rect = object->bounding_rect();
        left = std::min(rect.left(), left);
        right = std::max(rect.right(), right);
        top = std::max(rect.top(), top);
        bottom = std::min(rect.bottom(), bottom);
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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (int i = 0; i < x; ++i)
    {
        for (int j = 0; j < y; ++j)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }
            for (Geo::Geometry *object : objects)
            {
                _graph->container_group(_current_group).append(object->clone());
                _graph->container_group(_current_group).back()->translate(x_space * i, y_space * j);
                _graph->container_group(_current_group).back()->is_selected = true;
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

bool Editer::ring_array(std::vector<Geo::Geometry *> objects, const double x, const double y, const int n)
{
    if (n <= 1 || objects.empty())
    {
        return false;
    }

    for (Geo::Geometry *obj : objects)
    {
        obj->is_selected = true;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (int i = 1; i < n; ++i)
    {
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->rotate(
                x, y, 2 * Geo::PI * i / n);
            _graph->container_group(_current_group).back()->is_selected = true;
            items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

void Editer::up(Geo::Geometry *item)
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

void Editer::down(Geo::Geometry *item)
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

void Editer::rotate(std::vector<Geo::Geometry *> objects, const double x, const double y, const double rad)
{
    for (Geo::Geometry *geo : objects)
    {
        geo->rotate(x, y, rad);
    }
    _graph->modified = true;
    _backup.push_command(new UndoStack::RotateCommand(objects, x, y, rad));
}

void Editer::flip(std::vector<Geo::Geometry *> objects, const bool direction, const bool unitary, const bool all_layers)
{
    Geo::Point coord;
    if (objects.empty())
    {
        if (unitary)
        {
            if (all_layers)
            {
                coord = _graph->bounding_rect().center();
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::Geometry *geo : group)
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
                    objects.insert(objects.end(), group.begin(), group.end());
                }
            }
            else
            {
                coord = _graph->container_group(_current_group).bounding_rect().center();
                for (Geo::Geometry *geo : _graph->container_group(_current_group))
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
                objects.assign(_graph->container_group(_current_group).begin(),
                    _graph->container_group(_current_group).end());
            }
        }
        else
        {
            if (all_layers)
            {
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::Geometry *geo : group)
                    {
                        coord = geo->bounding_rect().center();
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
                for (Geo::Geometry *geo : _graph->container_group(_current_group))
                {
                    coord = geo->bounding_rect().center();
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
                objects.assign(_graph->container_group(_current_group).begin(),
                    _graph->container_group(_current_group).end());
            }
        }
    }
    else
    {
        if (unitary)
        {
            Geo::AABBRect rect;
            double left = DBL_MAX, top = -DBL_MAX, right = -DBL_MAX, bottom = DBL_MAX;
            for (Geo::Geometry *geo : objects)
            {
                rect = geo->bounding_rect();
                left = std::min(left, rect.left());
                top = std::max(top, rect.top());
                right = std::max(right, rect.right());
                bottom = std::min(bottom, rect.bottom());
            }
            coord.x = (left + right) / 2;
            coord.y = (top + bottom) / 2;

            for (Geo::Geometry *geo : objects)
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
            for (Geo::Geometry *geo : objects)
            {
                coord = geo->bounding_rect().center();
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

    _graph->modified = true;
    _backup.push_command(new UndoStack::FlipCommand(objects, coord.x, coord.y, direction, unitary));
}

void Editer::trim(Geo::Polyline *polyline, const double x, const double y)
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
            if (Geo::Point point; Geo::is_intersected((*polyline)[i - 1], (*polyline)[i],
                head, tail, point))
            {
                intersections.emplace_back(point);
            }
        }
    }
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->bounding_rect(), head, tail))
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
                polyline2 != polyline && Geo::is_intersected(polyline2->bounding_rect(), head, tail))
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
            if (const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(object);
                Geo::is_intersected(bezier->bounding_rect(), head, tail))
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
                Geo::is_intersected(bspline->bounding_rect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail,
                    *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
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
    std::vector<double> distance_to_anchor;
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
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
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index, polyline->end());
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
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
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index - 1, polyline->end());
            polyline1->front() = point1;
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
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
        }
        else
        {
            Geo::Polyline *polyline0 = new Geo::Polyline(polyline->begin(), polyline->begin() + anchor_index + 1);
            Geo::Polyline *polyline1 = new Geo::Polyline(polyline->begin() + anchor_index, polyline->end());
            polyline0->back() = point0;
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polyline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
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
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == polyline)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline1);
                _graph->container_group(_current_group).insert(i, polyline0);
                add_items.emplace_back(polyline0, _current_group, i);
                add_items.emplace_back(polyline1, _current_group, i + 1);
                break;
            }
        }
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
    _graph->modified = true;
}

void Editer::trim(Geo::Polygon *polygon, const double x, const double y)
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
            if (Geo::Point point; Geo::is_intersected((*polygon)[i - 1], (*polygon)[i],
                head, tail, point))
            {
                intersections.emplace_back(point);
            }
        }
    }
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon2 = static_cast<const Geo::Polygon *>(object);
                polygon2 != polygon && Geo::is_intersected(polygon2->bounding_rect(), head, tail))
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
                Geo::is_intersected(polyline2->bounding_rect(), head, tail))
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
            if (const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(object);
                Geo::is_intersected(bezier->bounding_rect(), head, tail))
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
                Geo::is_intersected(bspline->bounding_rect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail,
                    *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
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
    std::vector<double> distance_to_anchor;
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == polygon)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline1);
                    _graph->container_group(_current_group).insert(i, polyline0);
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
                    polyline->back() = point1;
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
                    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, polyline);
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
                std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _graph->container_group(_current_group).insert(i, polyline);
                add_items.emplace_back(polyline, _current_group, i);
                _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
                break;
            }
        }
    }
    _graph->modified = true;
}

void Editer::trim(Geo::Bezier *bezier, const double x, const double y)
{
    const size_t order = bezier->order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }
    Geo::Point anchor(x, y);
    double anchor_t = 0;
    size_t anchor_index = 0;

    {
        std::vector<std::tuple<size_t, double>> temp;
        for (size_t i = 0, end = bezier->size() - order; i < end; i += order)
        {
            Geo::Polyline polyline;
            polyline.append((*bezier)[i]);
            double t = 0;
            while (t <= 1)
            {
                Geo::Point point;
                for (size_t j = 0; j <= order; ++j)
                {
                    point += ((*bezier)[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
                }
                polyline.append(point);
                t += Geo::Bezier::default_step;
            }
            polyline.append((*bezier)[i + order]);
            Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

            std::vector<Geo::Point> points;
            Geo::closest_point(polyline, anchor, points);
            temp.emplace_back(i, Geo::distance(anchor, points.front()));
        }
        std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b)
            { return std::get<1>(a) < std::get<1>(b); });
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
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += ((*bezier)[j + anchor_index] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j))); 
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
        }
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        lower = std::max(0.0, t - 0.1), upper = std::min(1.0, t + 0.1);
        step = (upper - lower) / 100; 
        min_dis[0] = min_dis[1] = DBL_MAX;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += ((*bezier)[j + anchor_index] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                }
                if (const double dis = Geo::distance(coord, anchor) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1]) // 需要扩大搜索范围
                {
                    flag = -1;
                    break;
                }
                else
                {
                    if (dis == dis0)
                    {
                        if (++flag == 10)
                        {
                            break; // 连续10次相等就退出循环
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                    dis0 = dis;
                }
            }
            if (min_dis[1] < 2e-5)
            {
                break;
            }
            else if (flag == -1) // 需要扩大搜索范围
            {
                if (t - lower < upper - t)
                {
                    lower = std::max(0.0, lower - step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        lower += (upper - lower) / 4;
                    }
                }
                else
                {
                    upper = std::min(1.0, upper + step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        upper += (upper - lower) / 4;
                    }
                }
                stored_t.push_back(t);
                if (stored_t.size() > 4)
                {
                    stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                }
                step = (upper - lower) / 100;
            }
            else
            {
                lower = std::max(0.0, t - step * 2);
                upper = std::min(1.0, t + step * 2);
                step = (upper - lower) / 100;
            }
        }

        anchor.clear();
        for (size_t j = 0; j <= order; ++j)
        {
            anchor += ((*bezier)[j + anchor_index] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
        }
        anchor_t = t;
    }

    std::vector<std::tuple<size_t, double, double, double>> tvalues; // index, t, x, y
    // 找到自身交点
    const Geo::Bezier anchor_bezier(bezier->begin() + anchor_index, bezier->begin() + anchor_index + order + 1, order, false);
    /*for (size_t i = 0, end = bezier->size() - order; i < end; i += order)
    {
        if (i == anchor_index)
        {
            continue;
        }
        Geo::Bezier temp_bezier(bezier->begin() + i, bezier->begin() + i + order + 1, order, false);
        std::vector<Geo::Point> temp;
        Geo::is_intersected(anchor_bezier, temp_bezier, temp, &tvalues);
    }*/
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
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
                Geo::is_intersected(anchor_bezier, *static_cast<const Geo::Bezier *>(object), temp, &tvalues);
            }
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(anchor_bezier, *static_cast<const Geo::BSpline *>(object),
                dynamic_cast<const Geo::CubicBSpline *>(object), temp, &tvalues);
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
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bezier)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                Geo::Bezier *bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
                Geo::Bezier *bezier1 = new Geo::Bezier(bezier->begin() + anchor_index + order, bezier->end(), order, false);
                if (bezier0->size() > order)
                {
                    _graph->container_group(_current_group).insert(i, bezier0);
                    add_items.emplace_back(bezier0, _current_group, i++);
                }
                else
                {
                    delete bezier0;
                }
                if (bezier1->size() > order)
                {
                    _graph->container_group(_current_group).insert(i, bezier1);
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
        Geo::Bezier bezier_left(order), bezier_right(order);
        Geo::split(anchor_bezier, 0, left_t, bezier_left, bezier_right);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        Geo::Bezier *bezier0 = nullptr, *bezier1 = nullptr;
        if (anchor_t < left_t)
        {
            bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
            bezier1 = new Geo::Bezier(bezier_right);
            bezier1->append(bezier->begin() + anchor_index + order, bezier->end());
            bezier1->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
        }
        else
        {
            bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
            bezier0->append(bezier_left);
            bezier0->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
            bezier1 = new Geo::Bezier(bezier->begin() + anchor_index + order, bezier->end(), order, false);
        }
        if (bezier0->size() <= order)
        {
            delete bezier0;
            bezier0 = nullptr;
        }
        if (bezier1->size() <= order)
        {
            delete bezier1;
            bezier1 = nullptr;
        }
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bezier)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                if (bezier0 != nullptr)
                {
                    _graph->container_group(_current_group).insert(i, bezier0);
                    add_items.emplace_back(bezier0, _current_group, i++);
                }
                if (bezier1 != nullptr)
                {
                    _graph->container_group(_current_group).insert(i, bezier1);
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
            Geo::Bezier bezier_left(order), bezier_right(order);
            Geo::split(anchor_bezier, 0, left_t, bezier_left, bezier_right);
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    Geo::Bezier *bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
                    Geo::Bezier *bezier1 = new Geo::Bezier(bezier_right);
                    bezier1->append(bezier->begin() + anchor_index + order, bezier->end());
                    bezier1->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
                    if (bezier0->size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier0);
                        add_items.emplace_back(bezier0, _current_group, i++);
                    }
                    else
                    {
                        delete bezier0;
                    }
                    if (bezier1->size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier1);
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
            Geo::Bezier bezier_left(order), bezier_right(order);
            Geo::split(anchor_bezier, 0, right_t, bezier_left, bezier_right);
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    Geo::Bezier *bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
                    bezier0->append(bezier_left);
                    bezier0->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
                    Geo::Bezier *bezier1 = new Geo::Bezier(bezier->begin() + anchor_index + order, bezier->end(), order, false);
                    if (bezier0->size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier0);
                        add_items.emplace_back(bezier0, _current_group, i++);
                    }
                    else
                    {
                        delete bezier0;
                    }
                    if (bezier1->size() > order)
                    {
                        _graph->container_group(_current_group).insert(i, bezier1);
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
            Geo::Bezier bezier_left(order), bezier_right(order), temp_bezier(order);
            Geo::split(anchor_bezier, 0, left_t, bezier_left, temp_bezier);
            Geo::split(anchor_bezier, 0, right_t, temp_bezier, bezier_right);
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bezier)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    Geo::Bezier *bezier0 = new Geo::Bezier(bezier->begin(), bezier->begin() + anchor_index + 1, order, false);
                    bezier0->append(bezier_left);
                    bezier0->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
                    Geo::Bezier *bezier1 = new Geo::Bezier(bezier_right);
                    bezier1->append(bezier->begin() + anchor_index + order, bezier->end());
                    bezier1->update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
                    _graph->container_group(_current_group).insert(i, bezier1);
                    _graph->container_group(_current_group).insert(i, bezier0);
                    add_items.emplace_back(bezier0, _current_group, i);
                    add_items.emplace_back(bezier1, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    }
}

void Editer::trim(Geo::BSpline *bspline, const double x, const double y)
{
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
            }
            while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

            step = 1e-3, lower = std::max(knots[0], anchor_t - 0.1), upper = std::min(knots[nplusc - 1], anchor_t + 0.1);
            min_dis[0] = min_dis[1] = DBL_MAX;
            std::vector<double> stored_t;
            while ((upper - lower) * 1e15 > 1)
            {
                int flag = 0;
                for (double x = lower, dis0 = 0; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    std::vector<double> nbasis;
                    Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                    Geo::Point coord;
                    for (size_t i = 0; i < npts; ++i)
                    {
                        coord += bspline->control_points[i] * nbasis[i];
                    }
                    if (const double dis = Geo::distance(coord, anchor) * 1e9; dis < min_dis[1])
                    {
                        min_dis[1] = dis;
                        anchor_t = x;
                    }
                    else if (dis == min_dis[1]) // 需要扩大搜索范围
                    {
                        flag = -1;
                        break;
                    }
                    else
                    {
                        if (dis == dis0)
                        {
                            if (++flag == 10)
                            {
                                break; // 连续10次相等就退出循环
                            }
                        }
                        else
                        {
                            flag = 0;
                        }
                        dis0 = dis;
                    }
                }
                if (min_dis[1] < 2e-5)
                {
                    break;
                }
                else if (flag == -1) // 需要扩大搜索范围
                {
                    if (anchor_t - lower < upper - anchor_t)
                    {
                        lower = std::max(0.0, lower - step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            lower += (upper - lower) / 4;
                        }
                    }
                    else
                    {
                        upper = std::min(1.0, upper + step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            upper -= (upper - lower) / 4;
                        }
                    }
                    stored_t.push_back(anchor_t);
                    if (stored_t.size() > 4)
                    {
                        stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                    }
                    step = (upper - lower) / 100;
                }
                else
                {
                    lower = std::max(0.0, anchor_t - step * 2);
                    upper = std::min(1.0, anchor_t + step * 2);
                    step = (upper - lower) / 100;
                }
            }

            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, v, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline->control_points[i] * nbasis[i];
            }
            result.emplace_back(std::min(min_dis[0], min_dis[1]), v, coord);
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
            { return std::get<0>(a) < std::get<0>(b); });
        anchor_t = std::get<1>(result.front());
        anchor = std::get<2>(result.front());
    }

    std::vector<std::tuple<double, double, double>> tvalues; // t, x, y
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
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
            Geo::is_intersected(*static_cast<const Geo::Bezier *>(object), *bspline, is_cubic, temp, nullptr, &tvalues);
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
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
        {
            if (_graph->container_group(_current_group)[i] == bspline)
            {
                remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                _graph->container_group(_current_group).insert(i, result);
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
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, result);
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
                Geo::split(*bspline, false, left_t, bspline_left, bspline_right);
                result = new Geo::QuadBSpline(bspline_left);
            }
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, result);
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
                if (Geo::split(*bspline, true, left_t, bspline_left, temp_bspline)
                    && Geo::split(*bspline, true, right_t, temp_bspline, bspline_right))
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
                if (Geo::split(*bspline, false, left_t, bspline_left, temp_bspline)
                    && Geo::split(*bspline, false, right_t, temp_bspline, bspline_right))
                {
                    result_left = new Geo::QuadBSpline(bspline_left);
                    result_right = new Geo::QuadBSpline(bspline_right);
                }
                else
                {
                    return;
                }
            }
            std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
            for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
            {
                if (_graph->container_group(_current_group)[i] == bspline)
                {
                    remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
                    _graph->container_group(_current_group).insert(i, result_right);
                    _graph->container_group(_current_group).insert(i, result_left);
                    add_items.emplace_back(result_left, _current_group, i);
                    add_items.emplace_back(result_right, _current_group, i + 1);
                    break;
                }
            }
            _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        }
    } 
}

void Editer::trim(Geo::Circle *circle, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
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
            Geo::is_intersected(*circle, *static_cast<const Geo::Bezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*circle, *static_cast<const Geo::BSpline *>(object),
                dynamic_cast<const Geo::CubicBSpline *>(object), intersections, nullptr);
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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == circle)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _graph->container_group(_current_group).insert(i, arc);
            add_items.emplace_back(arc, _current_group, i);
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editer::trim(Geo::Arc *arc, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
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
            Geo::is_intersected(*arc, *static_cast<const Geo::Bezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*arc, *static_cast<const Geo::BSpline *>(object),
                dynamic_cast<const Geo::CubicBSpline *>(object), intersections, nullptr);
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
            arc0 = new Geo::Arc(arc->x, arc->y, arc->radius,
                Geo::angle(center, arc->control_points[0]), angle0, !arc->is_cw());
        }
        if (index1 < SIZE_MAX)
        {
            angle1 = Geo::angle(center, intersections[index1]);
            arc1 = new Geo::Arc(arc->x, arc->y, arc->radius, angle1,
                Geo::angle(center, arc->control_points[2]), !arc->is_cw());
        }
    }

    if (arc0 == nullptr && arc1 == nullptr)
    {
        return;
    }
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == arc)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            if (arc0 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, arc0);
                add_items.emplace_back(arc0, _current_group, i++);
            }
            if (arc1 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, arc1);
                add_items.emplace_back(arc1, _current_group, i);
            }
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editer::trim(Geo::Ellipse *ellipse, const double x, const double y)
{
    std::vector<Geo::Point> intersections;
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        std::vector<Geo::Point> temp;
        switch (object->type())
        {
        case Geo::Type::POLYGON:
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
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object),
                    *ellipse, point0, point1, point2, point3))
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
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object),
                    *ellipse, point0, point1, point2, point3))
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
            Geo::is_intersected(*ellipse, *static_cast<const Geo::Bezier *>(object), intersections, nullptr);
            break;
        case Geo::Type::BSPLINE:
            Geo::is_intersected(*ellipse, *static_cast<const Geo::BSpline *>(object),
                dynamic_cast<const Geo::CubicBSpline *>(object), intersections, nullptr);
            break;
        case Geo::Type::ARC:
            {
                Geo::Point point0, point1, point2, point3;
                switch (Geo::is_intersected(*ellipse, *static_cast<const Geo::Arc *>(object),
                    point0, point1, point2, point3))
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
            if (Geo::distance(intersections[i], point0) < Geo::EPSILON ||
                Geo::distance(intersections[i], point1) < Geo::EPSILON)
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
            ellipse0 = new Geo::Ellipse(center.x, center.y, a, b,
                Geo::angle(ellipse->a1(), center, start), angle0, false);
            ellipse0->rotate(center.x, center.y, ellipse->angle());
        }
        if (index1 < SIZE_MAX)
        {
            angle1 = Geo::angle(ellipse->a1(), center, intersections[index1]);
            ellipse1 = new Geo::Ellipse(center.x, center.y, a, b,
                angle1, Geo::angle(ellipse->a1(), center, end), false);
            ellipse1->rotate(center.x, center.y, ellipse->angle());
        }
    }

    if (ellipse0 == nullptr && ellipse1 == nullptr)
    {
        return;
    }
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    for (size_t i = 0, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == ellipse)
        {
            remove_items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            if (ellipse0 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, ellipse0);
                add_items.emplace_back(ellipse0, _current_group, i++);
            }
            if (ellipse1 != nullptr)
            {
                _graph->container_group(_current_group).insert(i, ellipse1);
                add_items.emplace_back(ellipse1, _current_group, i);
            }
            break;
        }
    }
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editer::extend(Geo::Polyline *polyline, const double x, const double y)
{
    Geo::Point head, tail;
    if (Geo::distance_square(polyline->front().x, polyline->front().y, x, y) <=
        Geo::distance_square(polyline->back().x, polyline->back().y, x, y)) // 延长头
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).bounding_rect();
        head = polyline->front();
        tail = head + (head - (*polyline)[1]).normalize()
            * std::hypot(rect.width(), rect.height());
    }
    else // 延长尾
    {
        const Geo::AABBRect rect = _graph->container_group(_current_group).bounding_rect();
        head = polyline->back();
        tail = head + (head - (*polyline)[polyline->size() - 2]).normalize()
            * std::hypot(rect.width(), rect.height());
    }

    std::vector<Geo::Point> intersections;
    for (size_t count = (head == polyline->front() ? polyline->size() : polyline->size() - 2),
        i = (head == polyline->front() ? 2 : 1); i < count; ++i) // 找到自身交点
    {
        if (Geo::Point point; Geo::is_intersected(head, tail,
            (*polyline)[i - 1], (*polyline)[i],  point))
        {
            intersections.emplace_back(point);
        }
    }
    for (const Geo::Geometry *object : _graph->container_group(_current_group))
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            if (const Geo::Polygon *polygon = static_cast<const Geo::Polygon *>(object);
                Geo::is_intersected(polygon->bounding_rect(), head, tail))
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
                polyline2 != polyline && Geo::is_intersected(polyline2->bounding_rect(), head, tail))
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
            if (const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(object);
                Geo::is_intersected(bezier->bounding_rect(), head, tail))
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
                Geo::is_intersected(bspline->bounding_rect(), head, tail))
            {
                if (std::vector<Geo::Point> points; Geo::is_intersected(head, tail,
                    *bspline, dynamic_cast<const Geo::CubicBSpline *>(bspline), points))
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
}


bool Editer::auto_aligning(Geo::Geometry *src, const Geo::Geometry *dst, std::list<QLineF> &reflines)
{
    if (src == nullptr || dst == nullptr
        || !(src->type() == Geo::Type::POLYGON || src->type() == Geo::Type::CIRCLE || src->type() == Geo::Type::ELLIPSE)
        || !(dst->type() == Geo::Type::POLYGON || dst->type() == Geo::Type::CIRCLE || dst->type() == Geo::Type::ELLIPSE))
    {
        return false;
    }

    const Geo::AABBRect rect(src->bounding_rect());
    Geo::Point center(rect.center());
    double left = rect.left(), top = rect.top(), right = rect.right(), bottom = rect.bottom();
    const double height = top - bottom, width = right - left;

    const size_t count = reflines.size();
    const Geo::AABBRect dst_rect(dst->bounding_rect());
    const Geo::Point dst_center(dst_rect.center());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_height = dst_top - dst_bottom, dst_width = dst_right - dst_left;
    const double align_distance = 2.0 / _view_ratio;

    if (std::abs(dst_left - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(top, dst_top), dst_left, std::min(bottom, dst_bottom)));
        src->translate(dst_left - center.x, 0);
        left += (dst_left - center.x);
        right += (dst_left - center.x);
        center.x = dst_left;
    }
    if (std::abs(dst_center.x - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::max(top, dst_top), dst_center.x, std::min(bottom, dst_bottom)));
        src->translate(dst_center.x - center.x, 0);
        left += (dst_center.x - center.x);
        right += (dst_center.x - center.x);
        center.x = dst_center.x;
    }
    if (std::abs(dst_right - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(top, dst_top), dst_right, std::min(bottom, dst_bottom)));
        src->translate(dst_right - center.x, 0);
        left += (dst_right - center.x);
        right += (dst_right - center.x);
        center.x = dst_right;
    }
    
    if (std::abs(dst_top - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - center.y);
        top += (dst_top - center.y);
        bottom += (dst_top - center.y);
        center.y = dst_top;
    }
    if (std::abs(dst_center.y - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - center.y);
        top += (dst_center.y - center.y);
        bottom += (dst_center.y - center.y);
        center.y = dst_center.y;
    }
    if (std::abs(dst_bottom - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - center.y);
        top += (dst_bottom - center.y);
        bottom += (dst_bottom - center.y);
        center.y = dst_bottom;
    }
    
    if (std::abs(dst_top - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - top);
        center.y += (dst_top - top);
        bottom += (dst_top - top);
        top = dst_top;
    }
    if (std::abs(dst_center.y - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - top);
        center.y += (dst_center.y - top);
        bottom += (dst_center.y - top);
        top = dst_center.y;
    }
    if (std::abs(dst_bottom - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - top);
        center.y += (dst_bottom - top);
        bottom += (dst_bottom - top);
        top = dst_bottom;
    }
    
    if (std::abs(dst_top - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - bottom);
        center.y += (dst_top - bottom);
        top += (dst_top - bottom);
        bottom = dst_top;
    }
    if (std::abs(dst_center.y - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - bottom);
        center.y += (dst_center.y - bottom);
        top += (dst_center.y - bottom);
        bottom = dst_center.y;
    }
    if (std::abs(dst_bottom - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - bottom);
        center.y += (dst_bottom - bottom);
        top += (dst_bottom - bottom);
        bottom = dst_bottom;
    }
    
    if (std::abs(dst_left - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(bottom, dst_bottom), dst_left, std::max(top, dst_top)));
        src->translate(dst_left - left, 0);
        center.x += (dst_left - left);
        right += (dst_left - left);
        left = dst_left;
    }
    if (std::abs(dst_center.x - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(bottom, dst_bottom), dst_center.x, std::max(top, dst_top)));
        src->translate(dst_center.x - left, 0);
        center.x += (dst_center.x - left);
        right += (dst_center.x - left);
        left = dst_center.x;
    }
    if (std::abs(dst_right - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(bottom, dst_bottom), dst_right, std::max(top, dst_top)));
        src->translate(dst_right - left, 0);
        center.x += (dst_right - left);
        right += (dst_right - left);
        left = dst_right;
    }
    
    if (std::abs(dst_left - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(bottom, dst_bottom), dst_left, std::max(top, dst_top)));
        src->translate(dst_left - right, 0);
        center.x += (dst_left - right);
        left += (dst_left - right);
        right = dst_left;
    }
    if (std::abs(dst_center.x - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(bottom, dst_bottom), dst_center.x, std::max(top, dst_top)));
        src->translate(dst_center.x - right, 0);
        center.x += (dst_center.x - right);
        left += (dst_center.x - right);
        right = dst_center.x;
    }
    if (std::abs(dst_right - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(bottom, dst_bottom), dst_right, std::max(top, dst_top)));
        src->translate(dst_right - right, 0);
        center.x += (dst_right - right);
        left += (dst_right - right);
        right = dst_right;
    }

    return count != reflines.size();
}

bool Editer::auto_aligning(Geo::Point &coord, const Geo::Geometry *dst, std::list<QLineF> &reflines)
{
    if (dst == nullptr || !(dst->type() == Geo::Type::POLYGON || dst->type() == Geo::Type::CIRCLE || dst->type() == Geo::Type::ELLIPSE))
    {
        return false;
    }

    const size_t count = reflines.size();
    const Geo::AABBRect dst_rect(dst->bounding_rect());
    const Geo::Point dst_center(dst_rect.center());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_height = dst_top - dst_bottom, dst_width = dst_right - dst_left;
    const double align_distance = 2.0 / _view_ratio;

    if (std::abs(dst_center.x - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::max(coord.y, dst_top), dst_center.x, std::min(coord.y, dst_bottom)));
        coord.x = dst_center.x;
    }
    if (std::abs(dst_center.y - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_center.y, std::max(coord.x, dst_right), dst_center.y));
        coord.y = dst_center.y;
    }
    if (std::abs(dst_top - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_top, std::max(coord.x, dst_right), dst_top));
        coord.y = dst_top;
    }
    if (std::abs(dst_bottom - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_bottom, std::max(coord.x, dst_right), dst_bottom));
        coord.y = dst_bottom;
    }
    if (std::abs(dst_left - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(coord.y, dst_bottom), dst_left, std::max(coord.y, dst_top)));
        coord.x = dst_left;
    }
    if (std::abs(dst_right - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(coord.y, dst_bottom), dst_right, std::max(coord.y, dst_top)));
        coord.x = dst_right;
    }

    return count != reflines.size();
}

bool Editer::auto_aligning(Geo::Geometry *points, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point center(points->bounding_rect().center());
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(center, *static_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(center, *static_cast<Geo::Circle *>(geo));
                break;
            case Geo::Type::ELLIPSE:
                temp = Geo::distance(center, *static_cast<Geo::Ellipse *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                    || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(center, *static_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(center, *static_cast<Geo::Circle *>(geo));
                    break;
                case Geo::Type::ELLIPSE:
                    temp = Geo::distance(center, *static_cast<Geo::Ellipse *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }
    
    bool flag = false;
    if (points != _catched_points && auto_aligning(points, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(points, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}

bool Editer::auto_aligning(Geo::Geometry *points, const double x, const double y, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point anchor(x, y);
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(anchor, *static_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(anchor, *static_cast<Geo::Circle *>(geo));
                break;
            case Geo::Type::ELLIPSE:
                temp = Geo::distance(anchor, *static_cast<Geo::Ellipse *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                    || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(anchor, *static_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(anchor, *static_cast<Geo::Circle *>(geo));
                    break;
                case Geo::Type::ELLIPSE:
                    temp = Geo::distance(anchor, *static_cast<Geo::Ellipse *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }

    _catched_points = nullptr;
    bool flag = false;
    if (points != _catched_points && auto_aligning(points, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(points, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}

bool Editer::auto_aligning(Geo::Point &coord, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (_graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point anchor(coord);
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                || geo->is_selected)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(anchor, *static_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(anchor, *static_cast<Geo::Circle *>(geo));
                break;
            case Geo::Type::ELLIPSE:
                temp = Geo::distance(anchor, *static_cast<Geo::Ellipse *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE)
                    || geo->is_selected)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(anchor, *static_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(anchor, *static_cast<Geo::Circle *>(geo));
                    break;
                case Geo::Type::ELLIPSE:
                    temp = Geo::distance(anchor, *static_cast<Geo::Ellipse *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }

    _catched_points = nullptr;
    bool flag = false;
    if (auto_aligning(coord, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(coord, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}

void Editer::auto_combinate()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::Geometry *> all_containers, all_polylines;
    std::unordered_map<const Geo::Geometry *, double> areas, lengths;
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
        for (Geo::Geometry *item : all_polylines)
        {
            _graph->back().append(item);
        }
        return;
    }

    std::sort(all_containers.begin(), all_containers.end(), [&](const Geo::Geometry *a, const Geo::Geometry *b)
        { return areas[a] > areas[b]; });
    std::sort(all_polylines.begin(), all_polylines.end(), [&](const Geo::Geometry *a, const Geo::Geometry *b)
        { return lengths[a] > lengths[b]; });

    std::unordered_map<const Geo::Geometry *, Geo::AABBRect> container_rects, polyline_rects;
    for (const Geo::Geometry *object : all_containers)
    {
        container_rects.insert_or_assign(object, object->bounding_rect());
    }
    for (const Geo::Geometry *object : all_polylines)
    {
        polyline_rects.insert_or_assign(object, object->bounding_rect());
    }

    _graph->append_group();
    for (size_t i = 0, count = all_containers.size(); i < count; ++i)
    {
        Geo::AABBRect current_rect = container_rects[all_containers[i]];
        std::vector<Geo::AABBRect> current_rects({current_rect});
        std::vector<Geo::Geometry *> objects({all_containers[i]});
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
                            if (Geo::Point point0, point1, point2, point3; Geo::is_intersected(*circle,
                                *static_cast<Geo::Ellipse *>(all_containers[j]), point0, point1, point2, point3))
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
                            if (Geo::Point point0, point1, point2, point3; Geo::is_intersected(*ellipse,
                                *static_cast<Geo::Ellipse *>(all_containers[j]), point0, point1, point2, point3))
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
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]),
                                *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::Bezier *>(all_polylines[k])->shape(),
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
                        if (Geo::is_intersected(*static_cast<Geo::AABBRect *>(all_polylines[k]),
                                *static_cast<Geo::Polygon *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]),
                                *static_cast<Geo::Polygon *>(objects[j]), true))
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
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]),
                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::Bezier *>(all_polylines[k])->shape(),
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
                        if (Geo::is_intersected(*static_cast<Geo::AABBRect *>(all_polylines[k]),
                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]),
                                *static_cast<Geo::Circle *>(objects[j]), true))
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
                        if (Geo::is_intersected(*static_cast<Geo::Polyline *>(all_polylines[k]),
                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(static_cast<Geo::Bezier *>(all_polylines[k])->shape(),
                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::BSPLINE:
                        if (Geo::is_intersected(static_cast<Geo::Bezier *>(all_polylines[k])->shape(),
                                *static_cast<Geo::Ellipse *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::TEXT:
                        if (Geo::is_intersected(*static_cast<Geo::AABBRect *>(all_polylines[k]),
                                *static_cast<Geo::Circle *>(objects[j])))
                        {
                            objects.push_back(all_polylines[k]);
                            all_polylines.erase(all_polylines.begin() + k--);
                            --polyline_count;
                            j = object_count - 1;
                        }
                        break;
                    case Geo::Type::POINT:
                        if (Geo::is_inside(*static_cast<Geo::Point *>(all_polylines[k]),
                                *static_cast<Geo::Ellipse *>(objects[j]), true))
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

    for (Geo::Geometry *polyline : all_polylines)
    {
        _graph->back().append(polyline);
    }
}

void Editer::auto_layering()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::Geometry *> all_containers, all_polylines;
    std::unordered_map<const Geo::Geometry *, Geo::AABBRect> rects;
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
                rects.insert_or_assign(all_containers.back(), all_containers.back()->bounding_rect());
                break;
            }
        }
    }
    _graph->clear();

    if (all_containers.empty())
    {
        _graph->append_group();
        for (Geo::Geometry *item : all_polylines)
        {
            _graph->back().append(item);
        }
        return;
    }

    {
        std::unordered_map<const Geo::Geometry *, double> areas;
        for (const Geo::Geometry *object : all_containers)
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
            }
        }
        std::sort(all_containers.begin(), all_containers.end(), [&](const Geo::Geometry *a, const Geo::Geometry *b)
            { return areas[a] > areas[b]; });
    }

    _graph->append_group();
    while (!all_containers.empty())
    {
        _graph->back().append(all_containers.front());
        all_containers.erase(all_containers.begin());
        std::unordered_map<const Geo::Geometry *, Geo::AABBRect> current_rects;
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
                    for (Geo::Geometry *geo : _graph->back())
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
                    for (Geo::Geometry *geo : _graph->back())
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
                    for (Geo::Geometry *geo : _graph->back())
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

    for (Geo::Geometry *geo : all_polylines)
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

void Editer::auto_connect()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::unordered_map<Geo::Polyline *, size_t> object_order;
    std::unordered_map<int, std::vector<Geo::Polyline *>> distance_object_map0, distance_object_map1; // front distance, back distance
    {
        size_t index = 0;
        for (Geo::Geometry *object : _graph->container_group())
        {
            if (object->type() == Geo::Type::POLYLINE)
            {
                Geo::Polyline *polyline = static_cast<Geo::Polyline *>(object);
                double distance0 = std::hypot(polyline->front().x, polyline->front().y);
                double distance1 = std::hypot(polyline->back().x, polyline->back().y);
                object_order.insert_or_assign(polyline, index++);
                if (distance_object_map0.find(distance0) == distance_object_map0.end())
                {
                    distance_object_map0.insert_or_assign(distance0, std::vector<Geo::Polyline *>{polyline});
                }
                else
                {
                    distance_object_map0[distance0].push_back(polyline);
                }
                if (distance_object_map1.find(distance1) == distance_object_map1.end())
                {
                    distance_object_map1.insert_or_assign(distance1, std::vector<Geo::Polyline *>{polyline});
                }
                else
                {
                    distance_object_map1[distance1].push_back(polyline);
                }
            }
        }
    }

    for (size_t i = 0, count = _graph->container_group().size(); i < count; ++i)
    {
        Geo::Polyline *polyline0 = nullptr;
        Geo::Point front_i, back_i;
        if (_graph->container_group()[i]->type() == Geo::Type::POLYLINE)
        {
            polyline0 = static_cast<Geo::Polyline *>(_graph->container_group()[i]);
            front_i = polyline0->front();
            back_i = polyline0->back();
        }
        else
        {
            continue;
        }
        {
            const double distance0 = std::hypot(front_i.x, front_i.y), distance1 = std::hypot(back_i.x, back_i.y);
            distance_object_map0[distance0].erase(std::find(distance_object_map0[distance0].begin(),
                distance_object_map0[distance0].end(), polyline0));
            distance_object_map1[distance1].erase(std::find(distance_object_map1[distance1].begin(),
                distance_object_map1[distance1].end(), polyline0));
        }

        while (true)
        {
            size_t j = SIZE_MAX;
            Geo::Polyline *polyline1 = nullptr;
            Geo::Point front_j, back_j;

            {
                const double distance0 = std::hypot(front_i.x, front_i.y);
                const double distance1 = std::hypot(back_i.x, back_i.y);
                Geo::Polyline *object_j = nullptr;

                if (distance_object_map0.find(distance0) != distance_object_map0.end())
                {
                    for (Geo::Polyline *object : distance_object_map0[distance0])
                    {
                        polyline1 = object;
                        front_j = polyline1->front();
                        if (front_i == front_j && object_order[object] < j)
                        {
                            object_j = object;
                            j = object_order[object_j];
                        }
                    }
                }

                if (distance_object_map1.find(distance0) != distance_object_map1.end())
                {
                    for (Geo::Polyline *object : distance_object_map1[distance0])
                    {
                        polyline1 = object;
                        back_j = polyline1->back();
                        if (front_i == back_j && object_order[object] < j)
                        {
                            object_j = object;
                            j = object_order[object_j];
                        }
                    }
                }

                if (distance_object_map0.find(distance1) != distance_object_map0.end())
                {
                    for (Geo::Polyline *object : distance_object_map0[distance1])
                    {
                        polyline1 = static_cast<Geo::Polyline *>(object);
                        front_j = polyline1->front();
                        if (back_i == front_j && object_order[object] < j)
                        {
                            object_j = object;
                            j = object_order[object_j];
                        }
                    }
                }

                if (distance_object_map1.find(distance1) != distance_object_map1.end())
                {
                    for (Geo::Polyline *object : distance_object_map1[distance1])
                    {
                        polyline1 = object;
                        back_j = polyline1->back();
                        if (back_i == back_j && object_order[object] < j)
                        {
                            object_j = object;
                            j = object_order[object_j];
                        }
                    }
                }

                polyline1 = object_j;
            }

            if (polyline1 == nullptr)
            {
                break;
            }

            front_j = polyline1->front();
            back_j = polyline1->back();

            if (const double distance = std::hypot(front_j.x, front_j.y); distance_object_map0.find(distance) != distance_object_map0.end())
            {
                distance_object_map0[distance].erase(std::find(distance_object_map0[distance].begin(),
                    distance_object_map0[distance].end(), polyline1));
            }
            if (const double distance = std::hypot(back_j.x, back_j.y); distance_object_map1.find(distance) != distance_object_map1.end())
            {
                distance_object_map1[distance].erase(std::find(distance_object_map1[distance].begin(),
                    distance_object_map1[distance].end(), polyline1));
            }
            j = std::distance(_graph->container_group().begin(), std::find(_graph->container_group().begin(), _graph->container_group().end(), polyline1));

            if (front_i == front_j)
            {
                polyline0->insert(0, polyline1->rbegin(), polyline1->rend());
                front_i = polyline0->front();
                _graph->container_group().remove(j--);
                --count;
            }
            else if (front_i == back_j)
            {
                polyline0->insert(0, polyline1->begin(), polyline1->end());
                front_i = polyline0->front();
                _graph->container_group().remove(j--);
                --count;
            }
            else if (back_i == front_j)
            {
                polyline0->append(polyline1->begin(), polyline1->end());
                back_i = polyline0->back();
                _graph->container_group().remove(j--);
                --count;
            }
            else if (back_i == back_j)
            {
                polyline0->append(polyline1->rbegin(), polyline1->rend());
                back_i = polyline0->back();
                _graph->container_group().remove(j--);
                --count;
            }

            if (front_i == back_i)
            {
                _graph->container_group().insert(i, new Geo::Polygon(polyline0->begin(), polyline0->end()));
                _graph->container_group().remove(i + 1);
                break;
            }
        }
    }
}


void Editer::text_to_polylines(Text *text)
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
    const std::string result = TextEncoding::uft8_to_gbk(text->text().toStdString());
    for (int i = 0, init_x = text->bounding_rect().left(), x = text->bounding_rect().left(),
        y = text->bounding_rect().top() - GlobalSetting::setting().text_size,
        count = result.length(), font_size = GlobalSetting::setting().text_size; i < count; ++i)
    {
        if (result[i] < 0)
        {
            const int code = (static_cast<uint8_t>(result[i]) << 8) | static_cast<uint8_t>(result[++i]);
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
    const size_t index = std::distance(_graph->container_group(_current_group).begin(),
        std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), text));
    _graph->container_group(_current_group).pop(index);
    _graph->container_group(_current_group).insert(index, combination);
    _graph->modified = true;
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    add_items.emplace_back(combination, _current_group, index);
    remove_items.emplace_back(text, _current_group, index);
    _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
}

void Editer::bezier_to_bspline(Geo::Bezier *bezier)
{
    if (bezier == nullptr)
    {
        return;
    }
    if (Geo::BSpline *bspline = Geo::bezier_to_bspline(*bezier))
    {
        const size_t index = std::distance(_graph->container_group(_current_group).begin(),
            std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bezier));
        _graph->container_group(_current_group).pop(index);
        _graph->container_group(_current_group).insert(index, bspline);
        _graph->modified = true;
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        add_items.emplace_back(bspline, _current_group, index);
        remove_items.emplace_back(bezier, _current_group, index);
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
}

void Editer::bspline_to_bezier(Geo::BSpline *bspline)
{
    if (bspline == nullptr)
    {
        return;
    }
    if (Geo::Bezier *bezier = Geo::bspline_to_bezier(*bspline))
    {
        const size_t index = std::distance(_graph->container_group(_current_group).begin(),
            std::find(_graph->container_group(_current_group).begin(), _graph->container_group(_current_group).end(), bspline));
        _graph->container_group(_current_group).pop(index);
        _graph->container_group(_current_group).insert(index, bezier);
        _graph->modified = true;
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        add_items.emplace_back(bezier, _current_group, index);
        remove_items.emplace_back(bspline, _current_group, index);
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
    }
}


void Editer::select_subfunc(const Geo::AABBRect &rect, const size_t start, const size_t end, std::vector<Geo::Geometry *> *result)
{
    for (size_t i = start; i < end; ++i)
    {
        Geo::Geometry *container = _graph->container_group(_current_group)[i];
        if (container->is_selected)
        {
            result->push_back(container);
            continue;
        }
        switch (container->type())
        {
        case Geo::Type::TEXT:
            if (Geo::is_intersected(*static_cast<const Geo::AABBRect *>(container), rect))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::POLYGON:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polygon *>(container)))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::CIRCLE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Circle *>(container)))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::ELLIPSE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Ellipse *>(container)))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::COMBINATION:
            if (Geo::is_intersected(rect, static_cast<Combination *>(container)->border(), true))
            {
                bool end = false;
                for (Geo::Geometry *item : *static_cast<Combination *>(container))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (Geo::is_intersected(*static_cast<const Geo::AABBRect *>(item), rect))
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
                        if (Geo::is_intersected(rect, static_cast<Geo::Bezier *>(item)->shape()))
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
                    result->push_back(container);
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::is_intersected(rect, *static_cast<Geo::Polyline *>(container)))
            {
                container->is_selected = true;
                result->push_back(container);
            } 
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_intersected(rect, static_cast<Geo::Bezier *>(container)->shape()))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::BSPLINE:
            if (Geo::is_intersected(rect, static_cast<Geo::BSpline *>(container)->shape()))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::ARC:
            if (Geo::is_intersected(rect, *static_cast<Geo::Arc *>(container)))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        case Geo::Type::POINT:
            if (Geo::is_inside(*static_cast<Geo::Point *>(container), rect, true))
            {
                container->is_selected = true;
                result->push_back(container);
            }
            break;
        default:
            break;
        }
    }
}