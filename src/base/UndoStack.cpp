#include "base/UndoStack.hpp"
#include "io/GlobalSetting.hpp"


using namespace UndoStack;


void CommandStack::set_count(const size_t count)
{
    _count = count;
}

void CommandStack::set_graph(Graph *graph)
{
    _graph = graph;
}

void CommandStack::push_command(Command *command)
{
    if (_commands.size() > _count)
    {
        delete _commands.front();
        _commands.erase(_commands.begin());
    }
    _commands.push_back(command);
}

void CommandStack::clear()
{
    while (!_commands.empty())
    {
        delete _commands.back();
        _commands.pop_back();
    }
    _graph = nullptr;
}

void CommandStack::undo()
{
    if (_commands.empty())
    {
        return;
    }

    _commands.back()->undo(_graph);
    appended = _commands.back()->appended;
    removed = _commands.back()->removed;
    updated = _commands.back()->updated;
    delete _commands.back();
    _commands.pop_back();
}


// ObjectCommand
ObjectCommand::ObjectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &objects, const bool add)
{
    if (add)
    {
        _add_items.assign(objects.begin(), objects.end());
        std::vector<Geo::Geometry *> items;
        for (const auto &item : _add_items)
        {
            items.push_back(std::get<0>(item));
        }
    }
    else
    {
        _remove_items.assign(objects.begin(), objects.end());
        std::vector<Geo::Geometry *> items;
        for (const auto &item : _remove_items)
        {
            items.push_back(std::get<0>(item));
        }
    }
}

ObjectCommand::ObjectCommand(Geo::Geometry *object, const size_t group, const size_t index, const bool add)
{
    if (add)
    {
        _add_items.emplace_back(object, group, index);
    }
    else
    {
        _remove_items.emplace_back(object, group, index);
    }
}

ObjectCommand::ObjectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &add_items,
                             const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &remove_items)
    : _add_items(add_items), _remove_items(remove_items)
{
}

ObjectCommand::~ObjectCommand()
{
    for (std::tuple<Geo::Geometry *, size_t, size_t> &item : _remove_items)
    {
        delete std::get<0>(item);
    }
}

void ObjectCommand::undo(Graph *graph)
{
    std::sort(_add_items.begin(), _add_items.end(),
              [](const std::tuple<Geo::Geometry *, size_t, size_t> &a, const std::tuple<Geo::Geometry *, size_t, size_t> &b)
              { return std::get<1>(a) > std::get<1>(b) || std::get<2>(a) > std::get<2>(b); });
    for (const std::tuple<Geo::Geometry *, size_t, size_t> &item : _add_items)
    {
        removed.push_back(std::get<0>(item));
        if (std::get<2>(item) >= graph->container_group(std::get<1>(item)).size())
        {
            graph->container_group(std::get<1>(item)).remove_back();
        }
        else
        {
            graph->container_group(std::get<1>(item)).remove(std::get<2>(item));
        }
    }
    std::sort(_remove_items.begin(), _remove_items.end(),
              [](const std::tuple<Geo::Geometry *, size_t, size_t> &a, const std::tuple<Geo::Geometry *, size_t, size_t> &b)
              { return std::get<1>(a) < std::get<1>(b) || std::get<2>(a) < std::get<2>(b); });
    for (const std::tuple<Geo::Geometry *, size_t, size_t> &item : _remove_items)
    {
        appended.push_back(std::get<0>(item));
        if (std::get<2>(item) >= graph->container_group(std::get<1>(item)).size())
        {
            graph->container_group(std::get<1>(item)).append(std::get<0>(item));
        }
        else
        {
            graph->container_group(std::get<1>(item)).insert(std::get<2>(item), std::get<0>(item));
        }
    }
    _remove_items.clear();
}


// TranslateCommand
TranslateCommand::TranslateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y)
    : _items(objects), _dx(x), _dy(y)
{
}

TranslateCommand::TranslateCommand(Geo::Geometry *object, const double x, const double y) : _items({object}), _dx(x), _dy(y)
{
}

void TranslateCommand::undo(Graph *graph)
{
    for (Geo::Geometry *object : _items)
    {
        object->translate(-_dx, -_dy);
    }
    updated = _items;
}


// TransformCommand
TransformCommand::TransformCommand(const std::vector<Geo::Geometry *> &objects, const double mat[6]) : _items(objects)
{
    const double k = mat[0] * mat[4] - mat[1] * mat[3];
    _invmat[0] = mat[4] / k, _invmat[1] = -mat[1] / k;
    _invmat[2] = (mat[1] * mat[5] - mat[2] * mat[4]) / k;
    _invmat[3] = -mat[3] / k, _invmat[4] = mat[0] / k;
    _invmat[5] = (mat[2] * mat[3] - mat[0] * mat[5]) / k;
}

TransformCommand::TransformCommand(Geo::Geometry *object, const double mat[6]) : _items({object})
{
    const double k = mat[0] * mat[4] - mat[1] * mat[3];
    _invmat[0] = mat[4] / k, _invmat[1] = -mat[1] / k;
    _invmat[2] = (mat[1] * mat[5] - mat[2] * mat[4]) / k;
    _invmat[3] = -mat[3] / k, _invmat[4] = mat[0] / k;
    _invmat[5] = (mat[2] * mat[3] - mat[0] * mat[5]) / k;
}

void TransformCommand::undo(Graph *graph)
{
    for (Geo::Geometry *object : _items)
    {
        object->transform(_invmat);
    }
    updated = _items;
}


// ChangeShapeCommand
ChangeShapeCommand::ChangeShapeCommand(Geo::Geometry *object, const std::vector<std::tuple<double, double>> &shape)
    : _object(object), _shape(shape)
{
}

ChangeShapeCommand::ChangeShapeCommand(Geo::BSpline *bspline, const std::vector<std::tuple<double, double>> &shape,
                                       const std::vector<std::tuple<double, double>> &path_points, const std::vector<double> &knots)
    : _object(bspline), _shape(shape), _path_points(path_points), _knots(knots)
{
}

void ChangeShapeCommand::undo(Graph *graph)
{
    switch (_object->type())
    {
    case Geo::Type::POLYLINE:
        {
            Geo::Polyline *polyline = static_cast<Geo::Polyline *>(_object);
            polyline->clear();
            for (const std::tuple<double, double> &point : _shape)
            {
                polyline->append(Geo::Point(std::get<0>(point), std::get<1>(point)));
            }
        }
        break;
    case Geo::Type::BEZIER:
        {
            Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(_object);
            bezier->clear();
            for (const std::tuple<double, double> &point : _shape)
            {
                bezier->append(Geo::Point(std::get<0>(point), std::get<1>(point)));
            }
            bezier->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
        }
        break;
    case Geo::Type::BSPLINE:
        {
            Geo::BSpline *bspline = static_cast<Geo::BSpline *>(_object);
            bspline->path_points.clear();
            bspline->control_points.clear();
            for (const std::tuple<double, double> &point : _shape)
            {
                bspline->control_points.emplace_back(std::get<0>(point), std::get<1>(point));
            }
            for (const std::tuple<double, double> &point : _path_points)
            {
                bspline->path_points.emplace_back(std::get<0>(point), std::get<1>(point));
            }
            bspline->set_knots(_knots.begin(), _knots.end());
            bspline->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
        }
        break;
    case Geo::Type::POLYGON:
        {
            Geo::Polygon *polygon = static_cast<Geo::Polygon *>(_object);
            polygon->clear();
            for (const std::tuple<double, double> &point : _shape)
            {
                polygon->append(Geo::Point(std::get<0>(point), std::get<1>(point)));
            }
        }
        break;
    case Geo::Type::CIRCLE:
        {
            Geo::Circle *circle = static_cast<Geo::Circle *>(_object);
            circle->x = std::get<0>(_shape.front());
            circle->y = std::get<1>(_shape.front());
            circle->radius = std::get<0>(_shape.back());
            circle->update_shape(Geo::Circle::default_down_sampling_value);
        }
        break;
    case Geo::Type::ELLIPSE:
        {
            Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(_object);
            const double parameter[10] = {std::get<0>(_shape[0]), std::get<1>(_shape[0]), std::get<0>(_shape[1]), std::get<1>(_shape[1]),
                                          std::get<0>(_shape[2]), std::get<1>(_shape[2]), std::get<0>(_shape[3]), std::get<1>(_shape[3]),
                                          std::get<0>(_shape[4]), std::get<1>(_shape[4])};
            ellipse->reset_parameter(parameter);
            ellipse->update_shape(Geo::Ellipse::default_down_sampling_value);
        }
        break;
    case Geo::Type::ARC:
        {
            Geo::Arc *arc = static_cast<Geo::Arc *>(_object);
            const double x0 = std::get<0>(_shape[0]), y0 = std::get<1>(_shape[0]);
            const double x1 = std::get<0>(_shape[1]), y1 = std::get<1>(_shape[1]);
            const double x2 = std::get<0>(_shape[2]), y2 = std::get<1>(_shape[2]);
            arc->control_points[0].x = x0, arc->control_points[0].y = y0;
            arc->control_points[1].x = x1, arc->control_points[1].y = y1;
            arc->control_points[2].x = x2, arc->control_points[2].y = y2;
            const double a = x0 - x1, b = y0 - y1, c = x0 - x2, d = y0 - y2;
            const double e = (x0 * x0 - x1 * x1 + y0 * y0 - y1 * y1) / 2;
            const double f = (x0 * x0 - x2 * x2 + y0 * y0 - y2 * y2) / 2;
            const double t = b * c - a * d;
            // assert(t != 0);
            arc->x = (b * f - d * e) / t, arc->y = (c * e - a * f) / t;
            arc->radius =
                (std::hypot(arc->x - x0, arc->y - y0) + std::hypot(arc->x - x1, arc->y - y1) + std::hypot(arc->x - x2, arc->y - y2)) / 3;
            arc->update_shape(Geo::Circle::default_down_sampling_value);
        }
        break;
    default:
        break;
    }
    updated.push_back(_object);
}


// RotateCommand
RotateCommand::RotateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double rad)
    : _items(objects), _x(x), _y(y), _rad(rad)
{
}

RotateCommand::RotateCommand(Geo::Geometry *object, const double x, const double y, const double rad)
    : _items({object}), _x(x), _y(y), _rad(rad)
{
}

void RotateCommand::undo(Graph *graph)
{
    for (Geo::Geometry *object : _items)
    {
        object->rotate(_x, _y, -_rad);
    }
    updated = _items;
}


// ScaleCommand
ScaleCommand::ScaleCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double k, const bool unitary)
    : _items(objects), _x(x), _y(y), _k(k), _unitary(unitary)
{
}

ScaleCommand::ScaleCommand(Geo::Geometry *object, const double x, const double y, const double k)
    : _items({object}), _x(x), _y(y), _k(k), _unitary(true)
{
}

void ScaleCommand::undo(Graph *graph)
{
    if (_unitary)
    {
        for (Geo::Geometry *object : _items)
        {
            object->scale(_x, _y, 1.0 / _k);
        }
    }
    else
    {
        Geo::AABBRect rect;
        for (Geo::Geometry *object : _items)
        {
            rect = object->bounding_rect();
            object->scale((rect.left() + rect.right()) / 2, (rect.top() + rect.bottom()) / 2, 1.0 / _k);
        }
    }
    updated = _items;
}


// CombinateCommand
CombinateCommand::CombinateCommand(const std::vector<std::tuple<Combination *, size_t>> &combinations, const size_t index)
    : _group_index(index)
{
    for (const std::tuple<Combination *, size_t> &combination : combinations)
    {
        _items.emplace_back(std::get<0>(combination), std::get<1>(combination),
                            std::vector<Geo::Geometry *>(std::get<0>(combination)->begin(), std::get<0>(combination)->end()));
    }
}

CombinateCommand::CombinateCommand(Combination *combination,
                                   const std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> &items,
                                   const size_t index)
    : _combination(combination), _items(items), _group_index(index)
{
}

CombinateCommand::~CombinateCommand()
{
    for (std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>> &item : _items)
    {
        delete std::get<0>(item);
    }
}

void CombinateCommand::undo(Graph *graph)
{
    if (_combination == nullptr)
    {
        for (std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>> &item : _items)
        {
            for (Geo::Geometry *object : std::get<2>(item))
            {
                std::get<0>(item)->append(
                    graph->container_group(_group_index)
                        .pop(std::find(graph->container_group(_group_index).begin(), graph->container_group(_group_index).end(), object)));
                removed.push_back(object);
            }
            if (std::get<1>(item) >= graph->container_group(_group_index).size())
            {
                graph->container_group(_group_index).append(std::get<0>(item));
            }
            else
            {
                graph->container_group(_group_index).insert(std::get<1>(item), std::get<0>(item));
            }
            appended.push_back(std::get<0>(item));
        }
    }
    else
    {
        graph->container_group(_group_index)
            .pop(std::find(graph->container_group(_group_index).begin(), graph->container_group(_group_index).end(), _combination));
        std::reverse(_combination->begin(), _combination->end());
        for (std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>> &item : _items)
        {
            for (Geo::Geometry *object : std::get<2>(item))
            {
                std::get<0>(item)->append(object);
                _combination->pop(std::find(_combination->begin(), _combination->end(), object));
            }
            appended.push_back(std::get<0>(item));
            if (std::get<1>(item) >= graph->container_group(_group_index).size())
            {
                graph->container_group(_group_index).append(std::get<0>(item));
            }
            else
            {
                graph->container_group(_group_index).insert(std::get<1>(item), std::get<0>(item));
            }
        }
        appended.assign(_combination->begin(), _combination->end());
        removed.push_back(_combination);
        graph->container_group(_group_index).append(*static_cast<ContainerGroup *>(_combination));
        delete _combination;
        _combination = nullptr;
    }
    _items.clear();
}


// FlipCommand
FlipCommand::FlipCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const bool direction,
                         const bool unitary)
    : _items(objects), _x(x), _y(y), _direction(direction), _unitary(unitary)
{
}

FlipCommand::FlipCommand(Geo::Geometry *object, const double x, const double y, const bool direction)
    : _items({object}), _x(x), _y(y), _direction(direction), _unitary(true)
{
}

void FlipCommand::undo(Graph *graph)
{
    if (_unitary)
    {
        if (_direction)
        {
            for (Geo::Geometry *object : _items)
            {
                object->translate(-_x, 0);
                object->transform(-1, 0, 0, 0, 1, 0);
                object->translate(_x, 0);
            }
        }
        else
        {
            for (Geo::Geometry *object : _items)
            {
                object->translate(0, -_y);
                object->transform(1, 0, 0, 0, -1, 0);
                object->translate(0, _y);
            }
        }
    }
    else
    {
        Geo::Point coord;
        if (_direction)
        {
            for (Geo::Geometry *object : _items)
            {
                coord = object->bounding_rect().center();
                object->translate(-coord.x, 0);
                object->transform(-1, 0, 0, 0, 1, 0);
                object->translate(coord.x, 0);
            }
        }
        else
        {
            for (Geo::Geometry *object : _items)
            {
                coord = object->bounding_rect().center();
                object->translate(0, -coord.y);
                object->transform(1, 0, 0, 0, -1, 0);
                object->translate(0, coord.y);
            }
        }
    }
    updated = _items;
}


// ConnectCommand
ConnectCommand::ConnectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t>> &polylines, const Geo::Polyline *polyline,
                               const size_t index)
    : _items(polylines), _polyline(polyline), _group_index(index)
{
}

ConnectCommand::~ConnectCommand()
{
    for (std::tuple<Geo::Geometry *, size_t> &item : _items)
    {
        delete std::get<Geo::Geometry *>(item);
    }
}

void ConnectCommand::undo(Graph *graph)
{
    removed.push_back(const_cast<Geo::Polyline *>(_polyline));
    graph->container_group(_group_index)
        .remove(std::find(graph->container_group(_group_index).begin(), graph->container_group(_group_index).end(), _polyline));
    for (std::tuple<Geo::Geometry *, size_t> &item : _items)
    {
        graph->container_group(_group_index).insert(std::get<size_t>(item), std::get<Geo::Geometry *>(item));
        appended.push_back(std::get<0>(item));
    }
    _items.clear();
}


// GroupCommand
GroupCommand::GroupCommand(const size_t index, const bool add) : _index(index), _add(add)
{
}

GroupCommand::GroupCommand(const size_t index, const bool add, ContainerGroup &group) : _index(index), _add(add)
{
    group.transfer(_group);
}

void GroupCommand::undo(Graph *graph)
{
    if (_add)
    {
        removed.assign(graph->container_group(_index).begin(), graph->container_group(_index).end());
        graph->remove_group(_index);
    }
    else
    {
        if (_index >= graph->size())
        {
            graph->append_group();
        }
        else
        {
            graph->insert_group(_index);
        }
        _group.transfer(graph->container_group(_index));
        appended.assign(graph->container_group(_index).begin(), graph->container_group(_index).end());
    }
}


// ReorderGroupCommand
ReorderGroupCommand::ReorderGroupCommand(const size_t from, const size_t to) : _from(from), _to(to)
{
}

void ReorderGroupCommand::undo(Graph *graph)
{
    std::swap(_from, _to);
    if (_from < _to)
    {
        ++_to;
    }
    else if (_from > _to)
    {
        ++_from;
    }
    else
    {
        return;
    }

    if (_to >= graph->size())
    {
        graph->append_group();
    }
    else
    {
        graph->insert_group(_to);
    }
    graph->container_group(_from).transfer(graph->container_group(_to));
    graph->remove_group(_from);
}


// RenameGroupCommand
RenameGroupCommand::RenameGroupCommand(const size_t index, QString old_name) : _index(index), _old_name(std::move(old_name))
{
}

void RenameGroupCommand::undo(Graph *graph)
{
    graph->container_group(_index).name = _old_name;
}


// TextChangedCommand
TextChangedCommand::TextChangedCommand(Text *item, QString text) : _item(item), _text(std::move(text))
{
}

void TextChangedCommand::undo(Graph *graph)
{
    _item->set_text(_text);
    updated.push_back(_item);
}


// RevreseCommand
ReverseCommand::ReverseCommand(const std::vector<Geo::Geometry *> &objects)
{
    _objects.assign(objects.begin(), objects.end());
}

void ReverseCommand::undo(Graph *graph)
{
    for (Geo::Geometry *object : _objects)
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
                std::reverse(bezier->begin(), bezier->end());
                bezier->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
            }
            break;
        case Geo::Type::BSPLINE:
            static_cast<Geo::BSpline *>(object)->reverse();
            break;
        case Geo::Type::POLYGON:
        case Geo::Type::POLYLINE:
            std::reverse(static_cast<Geo::Polyline *>(object)->begin(), static_cast<Geo::Polyline *>(object)->end());
            break;
        default:
            break;
        }
    }
}