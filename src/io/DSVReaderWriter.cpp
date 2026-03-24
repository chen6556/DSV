#include <set>
#include <iomanip>
#include <QDebug>
#include "DSVReaderWriter.hpp"
#include "io/GlobalSetting.hpp"


DSVReaderWriter::DSVReaderWriter(Graph *graph) : _graph(graph)
{
}

void DSVReaderWriter::read(std::ifstream &stream)
{
    _data.clear();
    _object_to_handle.clear();
    _handle_to_object.clear();
    _parent_to_children.clear();
    _child_to_parent.clear();
    while (read_pair(stream))
        ;
    for (const std::vector<Pair> &data : _data)
    {
        if (!read_data(data))
        {
            qDebug() << "Error object handle: " << _info.hanlde;
            break;
        }
    }

    for (ContainerGroup &group : *_graph)
    {
        // update combination border
        _current_layer = group.name.toStdString();
        std::vector<Geo::Geometry *> temp(group.rbegin(), group.rend());
        while (!temp.empty())
        {
            Geo::Geometry *object = temp.back();
            temp.pop_back();
            if (object->type() == Geo::Type::COMBINATION)
            {
                temp.insert(temp.end(), static_cast<Combination *>(object)->rbegin(), static_cast<Combination *>(object)->rend());
                static_cast<Combination *>(object)->update_border();
            }
        }
    }
}

void DSVReaderWriter::write(std::ofstream &stream)
{
    stream << std::setprecision(16);
    check_group_name(_graph);
    record_handle(_graph);
    for (ContainerGroup &group : *_graph)
    {
        _current_layer = group.name.toStdString();
        std::vector<Geo::Geometry *> temp(group.rbegin(), group.rend());
        while (!temp.empty())
        {
            Geo::Geometry *object = temp.back();
            temp.pop_back();
            switch (object->type())
            {
            case Geo::Type::POINT:
                write(stream, static_cast<Geo::Point *>(object));
                break;
            case Geo::Type::POLYLINE:
                write(stream, static_cast<Geo::Polyline *>(object));
                break;
            case Geo::Type::POLYGON:
                write(stream, static_cast<Geo::Polygon *>(object));
                break;
            case Geo::Type::CIRCLE:
                write(stream, static_cast<Geo::Circle *>(object));
                break;
            case Geo::Type::ARC:
                write(stream, static_cast<Geo::Arc *>(object));
                break;
            case Geo::Type::ELLIPSE:
                write(stream, static_cast<Geo::Ellipse *>(object));
                break;
            case Geo::Type::BSPLINE:
                write(stream, static_cast<Geo::BSpline *>(object));
                break;
            case Geo::Type::BEZIER:
                write(stream, static_cast<Geo::CubicBezier *>(object));
                break;
            case Geo::Type::TEXT:
                write(stream, static_cast<Text *>(object));
                break;
            case Geo::Type::COMBINATION:
                write(stream, static_cast<Combination *>(object));
                temp.insert(temp.end(), static_cast<Combination *>(object)->rbegin(), static_cast<Combination *>(object)->rend());
                break;
            default:
                break;
            }
        }
    }
}

void DSVReaderWriter::check_group_name(Graph *graph)
{
    std::set<QString> names;
    for (ContainerGroup &group : *graph)
    {
        if (!group.name.isEmpty() && names.find(group.name) == names.cend())
        {
            names.insert(group.name);
        }
        else
        {
            group.name.clear();
        }
    }
    int index = 0;
    for (ContainerGroup &group : *graph)
    {
        if (group.name.isEmpty())
        {
            while (names.find(QString::number(index)) != names.cend())
            {
                ++index;
            }
            group.name = QString::number(index++);
        }
    }
}

void DSVReaderWriter::record_handle(Graph *graph)
{
    _handle_to_object.clear();
    _object_to_handle.clear();
    for (ContainerGroup &group : *graph)
    {
        std::vector<Geo::Geometry *> temp(group.rbegin(), group.rend());
        while (!temp.empty())
        {
            Geo::Geometry *object = temp.back();
            temp.pop_back();
            _handle_to_object.insert_or_assign(_global_handle++, object);
            _object_to_handle.insert_or_assign(object, _global_handle);
            if (Combination *combination = dynamic_cast<Combination *>(object))
            {
                temp.insert(temp.end(), combination->rbegin(), combination->rend());
            }
        }
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Point *point)
{
    stream << "0,Point" << std::endl;
    stream << "1," << _object_to_handle.at(point) << std::endl;
    stream << "3," << _current_layer << std::endl;
    stream << "10," << point->x << std::endl;
    stream << "11," << point->y << std::endl;
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Polyline *polyline)
{
    stream << "0,Polyline" << std::endl;
    stream << "1," << _object_to_handle.at(polyline) << std::endl;
    stream << "3," << _current_layer << std::endl;
    for (const Geo::Point &point : *polyline)
    {
        stream << "10," << point.x << std::endl;
        stream << "11," << point.y << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Polygon *polygon)
{
    stream << "0,Polygon" << std::endl;
    stream << "1," << _object_to_handle.at(polygon) << std::endl;
    stream << "3," << _current_layer << std::endl;
    for (const Geo::Point &point : *polygon)
    {
        stream << "10," << point.x << std::endl;
        stream << "11," << point.y << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Circle *circle)
{
    stream << "0,Circle" << std::endl;
    stream << "1," << _object_to_handle.at(circle) << std::endl;
    stream << "3," << _current_layer << std::endl;
    stream << "10," << circle->x << std::endl;
    stream << "11," << circle->y << std::endl;
    stream << "20," << circle->radius << std::endl;
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Arc *arc)
{
    stream << "0,Arc" << std::endl;
    stream << "1," << _object_to_handle.at(arc) << std::endl;
    stream << "3," << _current_layer << std::endl;
    for (int i = 0; i < 3; ++i)
    {
        stream << "10," << arc->control_points[i].x << std::endl;
        stream << "11," << arc->control_points[i].y << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::Ellipse *ellipse)
{
    stream << "0,Ellipse" << std::endl;
    stream << "1," << _object_to_handle.at(ellipse) << std::endl;
    stream << "3," << _current_layer << std::endl;
    stream << "10," << ellipse->center().x << std::endl;
    stream << "11," << ellipse->center().y << std::endl;
    stream << "20," << ellipse->lengtha() << std::endl;
    stream << "21," << ellipse->lengthb() << std::endl;
    stream << "30," << ellipse->angle() << std::endl;
    if (ellipse->is_arc())
    {
        stream << "31," << ellipse->arc_param0() << std::endl;
        stream << "32," << ellipse->arc_param1() << std::endl;
    }
    else
    {
        stream << "31,0" << std::endl;
        stream << "32,0" << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::BSpline *bspline)
{
    stream << "0,BSpline" << std::endl;
    stream << "1," << _object_to_handle.at(bspline) << std::endl;
    stream << "3," << _current_layer << std::endl;
    stream << (dynamic_cast<const Geo::CubicBSpline *>(bspline) != nullptr ? "40,3" : "40,2") << std::endl;
    for (const double k : bspline->knots())
    {
        stream << "41," << k << std::endl;
    }
    for (const Geo::Point &point : bspline->path_points)
    {
        stream << "10," << point.x << std::endl;
        stream << "11," << point.y << std::endl;
    }
    for (const Geo::Point &point : bspline->control_points)
    {
        stream << "12," << point.x << std::endl;
        stream << "13," << point.y << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Geo::CubicBezier *bezier)
{
    stream << "0,CubicBezier" << std::endl;
    stream << "1," << _object_to_handle.at(bezier) << std::endl;
    stream << "3," << _current_layer << std::endl;
    for (const Geo::Point &point : *bezier)
    {
        stream << "12," << point.x << std::endl;
        stream << "13," << point.y << std::endl;
    }
}

void DSVReaderWriter::write(std::ofstream &stream, Text *text)
{
    stream << "0,Text" << std::endl;
    stream << "1," << _object_to_handle.at(text) << std::endl;
    stream << "3," << _current_layer << std::endl;
    stream << "10," << text->shape(3).x << std::endl;
    stream << "11," << text->shape(3).y << std::endl;
    stream << "30, " << text->angle() << std::endl;
    stream << "40, " << text->font().pointSize() << std::endl;
    QString txt = text->text();
    txt.replace(QChar('\n'), QChar(0x1F));
    stream << "42," << txt.toStdString() << std::endl;
}

void DSVReaderWriter::write(std::ofstream &stream, Combination *combination)
{
    stream << "0,Combination" << std::endl;
    stream << "1," << _object_to_handle.at(combination) << std::endl;
    stream << "3," << _current_layer << std::endl;
    for (Geo::Geometry *object : *combination)
    {
        stream << "80," << _object_to_handle.at(object) << std::endl;
    }
}

bool DSVReaderWriter::read_code(std::ifstream &stream)
{
    std::string value;
    while (stream.good())
    {
        if (char c = stream.get(); '0' <= c && c <= '9')
        {
            value.push_back(c);
        }
        else if (c == ',')
        {
            break;
        }
        else
        {
            return false;
        }
    }
    if (value.empty())
    {
        return false;
    }
    else
    {
        _pair.code = std::stoi(value);
        return true;
    }
}

bool DSVReaderWriter::read_value(std::ifstream &stream)
{
    std::string value;
    while (stream.good())
    {
        if (char c = stream.get(); c == '\n')
        {
            break;
        }
        else if (c == '\r')
        {
            if (stream.peek() == '\n')
            {
                stream.get();
            }
            break;
        }
        else
        {
            value.push_back(c);
        }
    }
    if (value.empty())
    {
        return false;
    }
    if (_pair.code == code_hanlde || _pair.code == code_intvalue || _pair.code == code_pointer)
    {
        _pair.value = std::stoi(value);
    }
    else if (_pair.code == code_type || _pair.code == code_name || _pair.code == code_layer || _pair.code == code_strvalue)
    {
        _pair.str = value;
    }
    else
    {
        _pair.real = std::stod(value);
    }
    return true;
}

bool DSVReaderWriter::read_pair(std::ifstream &stream)
{
    _pair.code = _pair.value = 0;
    _pair.real = 0;
    _pair.str.clear();
    if (read_code(stream) && read_value(stream))
    {
        if (_pair.code == code_type)
        {
            _data.emplace_back();
        }
        _data.back().emplace_back(_pair);
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_data(const std::vector<Pair> &data)
{
    if (!check_data(data))
    {
        return false;
    }

    if (_info.type == "Point")
    {
        return read_point(data);
    }
    else if (_info.type == "Line")
    {
        return read_line(data);
    }
    else if (_info.type == "Polyline")
    {
        return read_polyline(data);
    }
    else if (_info.type == "Polygon")
    {
        return read_polygon(data);
    }
    else if (_info.type == "Circle")
    {
        return read_circle(data);
    }
    else if (_info.type == "Arc")
    {
        return read_arc(data);
    }
    else if (_info.type == "Ellipse")
    {
        return read_ellipse(data);
    }
    else if (_info.type == "BSpline")
    {
        return read_bspline(data);
    }
    else if (_info.type == "CubicBezier")
    {
        return read_cubicbezier(data);
    }
    else if (_info.type == "Text")
    {
        return read_text(data);
    }
    else if (_info.type == "Combination")
    {
        return read_combination(data);
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::check_data(const std::vector<Pair> &data)
{
    bool has_type = false, has_handle = false, has_layer = false;
    for (const Pair &pair : data)
    {
        switch (pair.code)
        {
        case code_type:
            has_type = true;
            _info.type = pair.str;
            break;
        case code_hanlde:
            has_handle = true;
            _info.hanlde = pair.value;
            break;
        case code_layer:
            has_layer = true;
            _info.layer = pair.str;
            check_group(pair.str);
            break;
        case code_name:
            _info.name = pair.str;
            break;
        default:
            break;
        }
    }
    return has_type && has_handle && has_layer;
}

void DSVReaderWriter::check_group(const std::string &name)
{
    if (!_graph->has_group(QString::fromStdString(name)))
    {
        _graph->append_group(QString::fromStdString(name));
        _group_name_to_index.insert_or_assign(name, _graph->container_groups().size() - 1);
    }
    else if (_group_name_to_index.find(name) == _group_name_to_index.cend())
    {
        for (size_t i = 0, count = _graph->container_groups().size(); i < count; ++i)
        {
            if (_graph->container_group(i).name == name)
            {
                _group_name_to_index.insert_or_assign(name, i);
                break;
            }
        }
    }
}

bool DSVReaderWriter::read_point(const std::vector<Pair> &data)
{
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pathx)
        {
            has_x = true;
            x = pair.real;
        }
        else if (pair.code == code_pathy)
        {
            has_y = true;
            y = pair.real;
        }
    }
    if (has_x && has_y)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                Geo::Point *point = new Geo::Point(x, y);
                combination->append(point);
                _object_to_handle.insert_or_assign(point, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, point);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Geo::Point *point = new Geo::Point(x, y);
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(point);
            _object_to_handle.insert_or_assign(point, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, point);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_line(const std::vector<Pair> &data)
{
    Geo::Polyline *polyline = new Geo::Polyline();
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pathx)
        {
            x = pair.real;
            has_x = true;
        }
        else if (pair.code == code_pathy)
        {
            y = pair.real;
            has_y = true;
        }
        if (has_x && has_y)
        {
            has_x = has_y = false;
            polyline->append(x, y);
        }
    }
    if (polyline->size() == 2)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                combination->append(polyline);
                _object_to_handle.insert_or_assign(polyline, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, polyline);
                return true;
            }
            else
            {
                delete polyline;
                return false;
            }
        }
        else
        {
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(polyline);
            _object_to_handle.insert_or_assign(polyline, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, polyline);
        }
        return true;
    }
    else
    {
        delete polyline;
        return false;
    }
}

bool DSVReaderWriter::read_polyline(const std::vector<Pair> &data)
{
    Geo::Polyline *polyline = new Geo::Polyline();
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pathx)
        {
            x = pair.real;
            has_x = true;
        }
        else if (pair.code == code_pathy)
        {
            y = pair.real;
            has_y = true;
        }
        if (has_x && has_y)
        {
            has_x = has_y = false;
            polyline->append(x, y);
        }
    }
    if (polyline->size() >= 2)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                combination->append(polyline);
                _object_to_handle.insert_or_assign(polyline, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, polyline);
                return true;
            }
            else
            {
                delete polyline;
                return false;
            }
        }
        else
        {
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(polyline);
            _object_to_handle.insert_or_assign(polyline, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, polyline);
        }
        return true;
    }
    else
    {
        delete polyline;
        return false;
    }
}

bool DSVReaderWriter::read_polygon(const std::vector<Pair> &data)
{
    Geo::Polygon *polygon = new Geo::Polygon();
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pathx)
        {
            x = pair.real;
            has_x = true;
        }
        else if (pair.code == code_pathy)
        {
            y = pair.real;
            has_y = true;
        }
        if (has_x && has_y)
        {
            has_x = has_y = false;
            polygon->append(x, y);
        }
    }
    if (polygon->size() >= 3)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                combination->append(polygon);
                _object_to_handle.insert_or_assign(polygon, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, polygon);
                return true;
            }
            else
            {
                delete polygon;
                return false;
            }
        }
        else
        {
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(polygon);
            _object_to_handle.insert_or_assign(polygon, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, polygon);
        }
        return true;
    }
    else
    {
        delete polygon;
        return false;
    }
}

bool DSVReaderWriter::read_circle(const std::vector<Pair> &data)
{
    bool has_x = false, has_y = false, has_r = false;
    double x = 0, y = 0, r = 0;
    for (const Pair &pair : data)
    {
        switch (pair.code)
        {
        case code_pathx:
            x = pair.real;
            has_x = true;
            break;
        case code_pathy:
            y = pair.real;
            has_y = true;
            break;
        case code_xlength:
            r = pair.real;
            has_r = true;
            break;
        default:
            break;
        }
    }
    if (has_x && has_y && has_r)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                Geo::Circle *circle = new Geo::Circle(x, y, r);
                combination->append(circle);
                _object_to_handle.insert_or_assign(circle, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, circle);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Geo::Circle *circle = new Geo::Circle(x, y, r);
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(circle);
            _object_to_handle.insert_or_assign(circle, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, circle);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_arc(const std::vector<Pair> &data)
{
    std::vector<Geo::Point> points;
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pathx)
        {
            x = pair.real;
            has_x = true;
        }
        else if (pair.code == code_pathy)
        {
            y = pair.real;
            has_y = true;
        }
        if (has_x && has_y)
        {
            points.emplace_back(x, y);
            has_x = has_y = false;
        }
    }
    if (points.size() == 3)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                Geo::Arc *arc = new Geo::Arc(points[0], points[1], points[2]);
                combination->append(arc);
                _object_to_handle.insert_or_assign(arc, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, arc);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Geo::Arc *arc = new Geo::Arc(points[0], points[1], points[2]);
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(arc);
            _object_to_handle.insert_or_assign(arc, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, arc);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_ellipse(const std::vector<Pair> &data)
{
    bool has_x = false, has_y = false, has_rx = false, has_ry = false;
    double x = 0, y = 0, rx = 0, ry = 0, angle = 0, startangle = 0, endangle = 0;
    for (const Pair &pair : data)
    {
        switch (pair.code)
        {
        case code_pathx:
            has_x = true;
            x = pair.real;
            break;
        case code_pathy:
            has_y = true;
            y = pair.real;
            break;
        case code_xlength:
            has_rx = true;
            rx = pair.real;
            break;
        case code_ylength:
            has_ry = true;
            ry = pair.real;
            break;
        case code_rotateangle:
            angle = pair.real;
            break;
        case code_startangle:
            startangle = pair.real;
            break;
        case code_endangle:
            endangle = pair.real;
            break;
        default:
            break;
        }
    }
    if (has_x && has_y && has_rx && has_ry)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                Geo::Ellipse *ellipse =
                    startangle == endangle ? new Geo::Ellipse(x, y, rx, ry) : new Geo::Ellipse(x, y, rx, ry, startangle, endangle, true);
                if (angle != 0)
                {
                    ellipse->rotate(x, y, angle);
                }
                combination->append(ellipse);
                _object_to_handle.insert_or_assign(ellipse, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, ellipse);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Geo::Ellipse *ellipse =
                startangle == endangle ? new Geo::Ellipse(x, y, rx, ry) : new Geo::Ellipse(x, y, rx, ry, startangle, endangle, true);
            if (angle != 0)
            {
                ellipse->rotate(x, y, angle);
            }
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(ellipse);
            _object_to_handle.insert_or_assign(ellipse, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, ellipse);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_bspline(const std::vector<Pair> &data)
{
    bool has_px = false, has_py = false, has_cx = false, has_cy = false, is_cubic = true;
    double px = 0, py = 0, cx = 0, cy = 0;
    std::vector<double> knots;
    std::vector<Geo::Point> path, controls;
    for (const Pair &pair : data)
    {
        switch (pair.code)
        {
        case code_pathx:
            has_px = true;
            px = pair.real;
            break;
        case code_pathy:
            has_py = true;
            py = pair.real;
            break;
        case code_floatvalue:
            knots.push_back(pair.real);
            break;
        case code_controlx:
            has_cx = true;
            cx = pair.real;
            break;
        case code_controly:
            has_cy = true;
            cy = pair.real;
            break;
        case code_intvalue:
            is_cubic = pair.value == 3;
            break;
        default:
            break;
        }
        if (has_px && has_py)
        {
            has_px = has_py = false;
            path.emplace_back(px, py);
        }
        else if (has_cx && has_cy)
        {
            has_cx = has_cy = false;
            controls.emplace_back(cx, cy);
        }
    }

    Geo::BSpline *bspline = nullptr;
    if (!controls.empty())
    {
        if (is_cubic)
        {
            bspline = new Geo::CubicBSpline(controls.begin(), controls.end(), knots, false);
        }
        else
        {
            bspline = new Geo::QuadBSpline(controls.begin(), controls.end(), knots, false);
        }
    }
    else if (!path.empty())
    {
        if (is_cubic)
        {
            bspline = new Geo::CubicBSpline(path.begin(), path.end(), knots, true);
        }
        else
        {
            bspline = new Geo::QuadBSpline(path.begin(), path.end(), knots, true);
        }
    }
    else
    {
        return false;
    }

    if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
    {
        if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
        {
            combination->append(bspline);
            _object_to_handle.insert_or_assign(bspline, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, bspline);
            return true;
        }
        else
        {
            delete bspline;
            return false;
        }
    }
    else
    {
        _graph->container_group(_group_name_to_index.at(_info.layer)).append(bspline);
        _object_to_handle.insert_or_assign(bspline, _info.hanlde);
        _handle_to_object.insert_or_assign(_info.hanlde, bspline);
    }
    return true;
}

bool DSVReaderWriter::read_cubicbezier(const std::vector<Pair> &data)
{
    bool has_x = false, has_y = false;
    double x = 0, y = 0;
    std::vector<Geo::Point> points;
    for (const Pair &pair : data)
    {
        if (pair.code == code_controlx)
        {
            has_x = true;
            x = pair.real;
        }
        else if (pair.code == code_controly)
        {
            has_y = true;
            y = pair.real;
        }
        if (has_x && has_y)
        {
            has_x = has_y = false;
            points.emplace_back(x, y);
        }
    }

    if (points.size() % 3 == 1)
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                Geo::CubicBezier *bezier = new Geo::CubicBezier(points.begin(), points.end(), false);
                combination->append(bezier);
                _object_to_handle.insert_or_assign(bezier, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, bezier);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Geo::CubicBezier *bezier = new Geo::CubicBezier(points.begin(), points.end(), false);
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(bezier);
            _object_to_handle.insert_or_assign(bezier, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, bezier);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_text(const std::vector<Pair> &data)
{
    bool has_x = false, has_y = false;
    double x = 0, y = 0, size = 14, angle = 0;
    QString txt;
    for (const Pair &pair : data)
    {
        switch (pair.code)
        {
        case code_pathx:
            has_x = true;
            x = pair.real;
            break;
        case code_pathy:
            has_y = true;
            y = pair.real;
            break;
        case code_rotateangle:
            angle = pair.real;
            break;
        case code_intvalue:
            size = pair.value;
            break;
        case code_strvalue:
            txt = QString::fromStdString(pair.str);
            txt.replace(QChar(0x1F), QChar('\n'));
            break;
        default:
            break;
        }
    }

    if (has_x && has_y && !txt.isEmpty())
    {
        if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
        {
            if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
            {
                QFont font("SimSun");
                font.setPointSize(size);
                Text *text = new Text(x, y, font, txt);
                if (angle != 0)
                {
                    text->rotate(x, y, angle);
                }
                combination->append(text);
                _object_to_handle.insert_or_assign(text, _info.hanlde);
                _handle_to_object.insert_or_assign(_info.hanlde, text);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            QFont font("SimSun");
            font.setPointSize(size);
            Text *text = new Text(x, y, font, txt);
            if (angle != 0)
            {
                text->rotate(x, y, angle);
            }
            _graph->container_group(_group_name_to_index.at(_info.layer)).append(text);
            _object_to_handle.insert_or_assign(text, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, text);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DSVReaderWriter::read_combination(const std::vector<Pair> &data)
{
    std::vector<int> children;
    for (const Pair &pair : data)
    {
        if (pair.code == code_pointer)
        {
            children.push_back(pair.value);
            _child_to_parent.insert_or_assign(pair.value, _info.hanlde);
        }
    }
    _parent_to_children.insert_or_assign(_info.hanlde, children);
    if (_child_to_parent.find(_info.hanlde) != _child_to_parent.cend())
    {
        if (Combination *combination = dynamic_cast<Combination *>(_handle_to_object.at(_child_to_parent.at(_info.hanlde))))
        {
            Combination *combi = new Combination();
            combination->append(combi);
            _object_to_handle.insert_or_assign(combi, _info.hanlde);
            _handle_to_object.insert_or_assign(_info.hanlde, combi);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        Combination *combi = new Combination();
        _graph->container_group(_group_name_to_index.at(_info.layer)).append(combi);
        _object_to_handle.insert_or_assign(combi, _info.hanlde);
        _handle_to_object.insert_or_assign(_info.hanlde, combi);
    }
    return true;
}