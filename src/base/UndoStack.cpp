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
        _commands.pop_front();
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
    delete _commands.back();
    _commands.pop_back();
}


// ObjectCommand
ObjectCommand::ObjectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &objects, const bool add)
{
    if (add)
    {
        _add_items.assign(objects.begin(), objects.end());
    }
    else
    {
        _remove_items.assign(objects.begin(), objects.end());
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
    : _add_items(add_items), _remove_items(remove_items) {}

ObjectCommand::~ObjectCommand()
{
    for (std::tuple<Geo::Geometry *, size_t, size_t> &item : _remove_items)
    {
        delete std::get<0>(item);
    }
}

void ObjectCommand::undo(Graph *graph)
{
    std::sort(_add_items.begin(), _add_items.end(), [](const std::tuple<Geo::Geometry *, size_t, size_t> &a,
        const std::tuple<Geo::Geometry *, size_t, size_t> &b) { return std::get<1>(a) > std::get<1>(b) || std::get<2>(a) > std::get<2>(b); });
    for (const std::tuple<Geo::Geometry *, size_t, size_t> &item : _add_items)
    {
        if (std::get<2>(item) >= graph->container_group(std::get<1>(item)).size())
        {
            graph->container_group(std::get<1>(item)).remove_back();
        }
        else
        {
            graph->container_group(std::get<1>(item)).remove(std::get<2>(item));
        }
    }
    std::sort(_remove_items.begin(), _remove_items.end(), [](const std::tuple<Geo::Geometry *, size_t, size_t> &a,
        const std::tuple<Geo::Geometry *, size_t, size_t> &b) { return std::get<1>(a) < std::get<1>(b) || std::get<2>(a) < std::get<2>(b); });
    for (const std::tuple<Geo::Geometry *, size_t, size_t> &item : _remove_items)
    {
        if (std::get<2>(item) >= graph->container_group(std::get<1>(item)).size())
        {
            graph->container_group(std::get<1>(item)).append(std::get<0>(item));
        }
        else
        {
            graph->container_group(std::get<1>(item)).insert(std::get<2>(item), std::get<0>(item));
        }
    }
    _add_items.clear();
    _remove_items.clear();
}


// TranslateCommand
TranslateCommand::TranslateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y)
    : _items(objects), _dx(x), _dy(y) {}

TranslateCommand::TranslateCommand(std::list<Geo::Geometry *>::const_iterator begin, std::list<Geo::Geometry *>::const_iterator end,
    const double x, const double y) : _items(begin, end), _dx(x), _dy(y) {}

TranslateCommand::TranslateCommand(Geo::Geometry *object, const double x, const double y)
    : _items({object}), _dx(x), _dy(y) {}

void TranslateCommand::undo(Graph *graph)
{
    for (Geo::Geometry *object : _items)
    {
        object->translate(-_dx, -_dy);
    }
    _items.clear();
}


// TransformCommand
TransformCommand::TransformCommand(const std::vector<Geo::Geometry *> &objects, const double mat[6])
    : _items(objects)
{
    const double k = mat[0] * mat[4] - mat[1] * mat[3];
    _invmat[0] = mat[4] / k, _invmat[1] = -mat[1] / k;
    _invmat[2] = (mat[1] * mat[5] - mat[2] * mat[4]) / k;
    _invmat[3] = -mat[3] / k, _invmat[4] = mat[0] / k;
    _invmat[5] = (mat[2] * mat[3] - mat[0] * mat[5]) / k;
}

TransformCommand::TransformCommand(std::list<Geo::Geometry *>::const_iterator begin, std::list<Geo::Geometry *>::const_iterator end, const double mat[6])
    : _items(begin, end)
{
    const double k = mat[0] * mat[4] - mat[1] * mat[3];
    _invmat[0] = mat[4] / k, _invmat[1] = -mat[1] / k;
    _invmat[2] = (mat[1] * mat[5] - mat[2] * mat[4]) / k;
    _invmat[3] = -mat[3] / k, _invmat[4] = mat[0] / k;
    _invmat[5] = (mat[2] * mat[3] - mat[0] * mat[5]) / k;
}

TransformCommand::TransformCommand(Geo::Geometry *object, const double mat[6])
    : _items({object})
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
}


// ChangeShapeCommand
ChangeShapeCommand::ChangeShapeCommand(Geo::Geometry *object, const std::vector<std::tuple<double, double>> &shape)
    : _object(object), _shape(shape) {}

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
            Geo::Bezier *bezier = static_cast<Geo::Bezier *>(_object);
            bezier->clear();
            for (const std::tuple<double, double> &point : _shape)
            {
                bezier->append(Geo::Point(std::get<0>(point), std::get<1>(point)));
            }
            bezier->update_shape();
        }
        break;
    case Geo::Type::POLYGON:
    case Geo::Type::CONTAINER:
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
    case Geo::Type::CIRCLECONTAINER:
        {
            Geo::Circle *circle = static_cast<Geo::Circle *>(_object);
            circle->x = std::get<0>(_shape.front());
            circle->y = std::get<1>(_shape.front());
            circle->radius = std::get<0>(_shape.back());
        }
        break;
    default:
        break;
    }
}


// RotateCommand
RotateCommand::RotateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double rad, const bool unitary)
    : _items(objects), _x(x), _y(y), _rad(rad), _unitary(unitary) {}

RotateCommand::RotateCommand(std::list<Geo::Geometry *>::const_iterator begin, std::list<Geo::Geometry *>::const_iterator end,
    const double x, const double y, const double rad, const bool unitary) : _items(begin, end), _x(x), _y(y), _rad(rad), _unitary(unitary) {}

RotateCommand::RotateCommand(Geo::Geometry *object, const double x, const double y, const double rad)
    : _items({object}), _x(x), _y(y), _rad(rad), _unitary(true) {}

void RotateCommand::undo(Graph *graph)
{
    if (_unitary)
    {
        for (Geo::Geometry *object : _items)
        {
            object->rotate(_x, _y, -_rad);
        }
    }
    else
    {
        Geo::Point coord;
        for (Geo::Geometry *object : _items)
        {
            coord = object->bounding_rect().center();
            object->rotate(coord.x, coord.y, -_rad);
        }
    }
}


// ScaleCommand
ScaleCommand::ScaleCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double k, const bool unitary)
    : _items(objects), _x(x), _y(y), _k(k), _unitary(unitary) {}

ScaleCommand::ScaleCommand(std::list<Geo::Geometry *>::const_iterator begin, std::list<Geo::Geometry *>::const_iterator end,
    const double x, const double y, const double k, const bool unitary) : _items(begin, end), _x(x), _y(y), _k(k), _unitary(unitary) {}

ScaleCommand::ScaleCommand(Geo::Geometry *object, const double x, const double y, const double k)
    : _items({object}), _x(x), _y(y), _k(k), _unitary(true) {}

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
}


// CombinateCommand
CombinateCommand::CombinateCommand(const std::vector<std::tuple<Combination *, size_t>> &combinations, const size_t index)
    : _group_index(index)
{
    for (const std::tuple<Combination *, size_t> &combiantion : combinations)
    {
        _items.emplace_back(std::get<0>(combiantion), std::get<1>(combiantion),
            std::vector<Geo::Geometry *>(std::get<0>(combiantion)->begin(), std::get<0>(combiantion)->end()));
    }
}

CombinateCommand::CombinateCommand(Combination *combination, const std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> &items, const size_t index)
    : _combination(combination), _items(items), _group_index(index) {}

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
                std::get<0>(item)->append(graph->container_group(_group_index).pop(std::find(graph->container_group(_group_index).begin(),
                    graph->container_group(_group_index).end(), object)));
            }
            if (std::get<1>(item) >= graph->container_group(_group_index).size())
            {
                graph->container_group(_group_index).append(std::get<0>(item));
            }
            else
            {
                graph->container_group(_group_index).insert(std::get<1>(item), std::get<0>(item));
            }
        }
    }
    else
    {
        graph->container_group(_group_index).pop(std::find(graph->container_group(_group_index).begin(),
            graph->container_group(_group_index).end(), _combination));
        std::reverse(_combination->begin(), _combination->end());
        for (std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>> &item : _items)
        {
            for (Geo::Geometry *object : std::get<2>(item))
            {
                std::get<0>(item)->append(object);
                _combination->pop(std::find(_combination->begin(), _combination->end(), object));
            }
            if (std::get<1>(item) >= graph->container_group(_group_index).size())
            {
                graph->container_group(_group_index).append(std::get<0>(item));
            }
            else
            {
                graph->container_group(_group_index).insert(std::get<1>(item), std::get<0>(item));
            }
        }
        graph->container_group(_group_index).append(*static_cast<ContainerGroup *>(_combination));
        delete _combination;
        _combination = nullptr;
    }
    _items.clear();
}


// FlipCommand
FlipCommand::FlipCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const bool direction, const bool unitary)
    : _items(objects), _x(x), _y(y), _direction(direction), _unitary(unitary) {}

FlipCommand::FlipCommand(std::list<Geo::Geometry *>::const_iterator begin, std::list<Geo::Geometry *>::const_iterator end,
    const double x, const double y, const bool direction, const bool unitary)
    : _items(begin, end), _x(x), _y(y), _direction(direction), _unitary(unitary) {}

FlipCommand::FlipCommand(Geo::Geometry *object, const double x, const double y, const bool direction)
    : _items({object}), _x(x), _y(y), _direction(direction), _unitary(true) {}

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
}


// ConnectCommand
ConnectCommand::ConnectCommand(const std::vector<std::tuple<Geo::Polyline *, size_t>> &polylines, const Geo::Polyline *polyline, const size_t index)
    : _items(polylines), _polyline(polyline), _group_index(index) {}

ConnectCommand::~ConnectCommand()
{
    for (std::tuple<Geo::Polyline *, size_t> &item : _items)
    {
        delete std::get<Geo::Polyline *>(item);
    }
}

void ConnectCommand::undo(Graph *graph)
{
    graph->container_group(_group_index).remove(std::find(graph->container_group(_group_index).begin(), 
       graph->container_group(_group_index).end(), _polyline));
    for (std::tuple<Geo::Polyline *, size_t> &item : _items)
    {
        graph->container_group(_group_index).insert(std::get<size_t>(item),
            std::get<Geo::Polyline *>(item));
    }
    _items.clear();
}


// GroupCommand
GroupCommand::GroupCommand(const size_t index, const bool add)
    : _index(index), _add(add) {}

GroupCommand::GroupCommand(const size_t index, const bool add, ContainerGroup &group)
    : _index(index), _add(add)
{
    group.transfer(_group);
}

void GroupCommand::undo(Graph *graph)
{
    if (_add)
    {
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
    }
}


// ReorderGroupCommand
ReorderGroupCommand::ReorderGroupCommand(const size_t from, const size_t to)
    : _from(from), _to(to) {}

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
RenameGroupCommand::RenameGroupCommand(const size_t index, const QString old_name)
    : _index(index), _old_name(old_name) {}

void RenameGroupCommand::undo(Graph *graph)
{
    graph->container_group(_index).name = _old_name;
}


// TextChangedCommand
TextChangedCommand::TextChangedCommand(Geo::Geometry *item, const QString &text)
    : _item(item), _text(text) {}

void TextChangedCommand::undo(Graph *graph)
{
    switch (_item->type())
    {
    case Geo::Type::CONTAINER:
        static_cast<Container *>(_item)->set_text(_text);
        break;
    case Geo::Type::CIRCLECONTAINER:
        static_cast<CircleContainer *>(_item)->set_text(_text);
        break;
    case Geo::Type::TEXT:
        static_cast<Text *>(_item)->set_text(_text,
            GlobalSetting::get_instance()->setting["text_size"].toInt());
        break;
    default:
        break;
    }
}
