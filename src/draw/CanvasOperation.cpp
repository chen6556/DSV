#include "draw/CanvasOperation.hpp"
#include "base/Algorithm.hpp"
#include "draw/Canvas.hpp"
#include "io/GlobalSetting.hpp"



using namespace CanvasOperations;

double *CanvasOperation::shape = new double[513];
double *CanvasOperation::tool_lines = new double[63];
unsigned int CanvasOperation::shape_len = 513;
unsigned int CanvasOperation::shape_count = 0;
unsigned int CanvasOperation::tool_lines_len = 63;
unsigned int CanvasOperation::tool_lines_count = 0;
float CanvasOperation::tool_line_width = 1.4f;
float CanvasOperation::tool_line_color[4] = {1.0f, 0.549f, 0.0f, 1.0f};
double CanvasOperation::real_pos[4];
double CanvasOperation::press_pos[2];
double CanvasOperation::release_pos[2];
Tool CanvasOperation::tool[2] = {Tool::Select, Tool::Select};
double CanvasOperation::view_ratio = 1;
QString CanvasOperation::info;
Editer *CanvasOperation::editer = nullptr;
Canvas *CanvasOperation::canvas = nullptr;

Geo::Geometry *CanvasOperation::clicked_object = nullptr;


CanvasOperation &CanvasOperation::operation()
{
    static CanvasOperation op;
    return op;
}

void CanvasOperation::init()
{
    std::fill_n(shape, shape_len, 0);
    std::fill_n(tool_lines, tool_lines_len, 0);
    std::fill_n(operations, static_cast<int>(Tool::End), nullptr);

    operations[static_cast<int>(Tool::Select)] = new SelectOperation();
    operations[static_cast<int>(Tool::Move)] = new MoveOperation();
    operations[static_cast<int>(Tool::Measure)] = new MeasureOperation();
    operations[static_cast<int>(Tool::Angle)] = new AngleOperation();
    operations[static_cast<int>(Tool::Circle0)] = new Circle0Operation();
    operations[static_cast<int>(Tool::Circle1)] = new Circle1Operation();
    operations[static_cast<int>(Tool::Circle2)] = new Circle2Operation();
    operations[static_cast<int>(Tool::Polyline)] = new PolythingOperation();
    operations[static_cast<int>(Tool::Rect)] = new RectOperation();
    operations[static_cast<int>(Tool::BSpline)] = new BSplineOperation();
    operations[static_cast<int>(Tool::Bezier)] = new BezierOperation();
    operations[static_cast<int>(Tool::Text)] = new TextOperation();
    operations[static_cast<int>(Tool::Ellipse)] = new EllipseOperation();
    operations[static_cast<int>(Tool::Mirror)] = new MirrorOperation();
    operations[static_cast<int>(Tool::RingArray)] = new RingArrayOperation();
    operations[static_cast<int>(Tool::PolygonDifference)] = new PolygonDifferenceOperation();
    operations[static_cast<int>(Tool::Fillet)] = new FilletOperation();
}

void CanvasOperation::clear()
{
    for (int i = 0, count = static_cast<int>(Tool::End); i < count; ++i)
    {
        if (operations[i] == nullptr)
        {
            continue;
        }
        operations[i]->reset();
    }
    shape_count = tool_lines_count = 0;
    tool[0] = Tool::Select;
    info.clear();
    clicked_object = nullptr;
}

CanvasOperation::~CanvasOperation()
{
    for (int i = 0, count = static_cast<int>(Tool::End); i < count; ++i)
    {
        delete operations[i];
    }
}

CanvasOperation *CanvasOperation::operator[](const Tool tool)
{
    return operations[static_cast<const int>(tool)];
}

void CanvasOperation::check_shape_size()
{
    if (shape_count == shape_len)
    {
        shape_len *= 2;
        double *temp = new double[shape_len];
        std::fill_n(temp, shape_len, 0);
        std::memmove(temp, shape, sizeof(double) * shape_count);
        delete[] shape;
        shape = temp;
    }
}

void CanvasOperation::check_shape_size(const size_t count)
{
    if (shape_len >= count)
    {
        return;
    }
    while (shape_len < count)
    {
        shape_len *= 2;
    }
    double *temp = new double[shape_len];
    std::fill_n(temp, shape_len, 0);
    std::memmove(temp, shape, sizeof(double) * shape_count);
    delete[] shape;
    shape = temp;
}

void CanvasOperation::check_tool_lines_size()
{
    if (tool_lines_count == tool_lines_len)
    {
        tool_lines_len *= 2;
        double *temp = new double[tool_lines_len];
        std::fill_n(temp, tool_lines_len, 0);
        std::memmove(temp, tool_lines, sizeof(double) * tool_lines_count);
        delete[] tool_lines;
        tool_lines = temp;
    }
}

void CanvasOperation::check_tool_lines_size(const size_t count)
{
    if (tool_lines_len >= count)
    {
        return;
    }
    while (tool_lines_len < count)
    {
        tool_lines_len *= 2;
    }
    double *temp = new double[tool_lines_len];
    std::fill_n(temp, tool_lines_len, 0);
    std::memmove(temp, tool_lines, sizeof(double) * tool_lines_count);
    delete[] tool_lines;
    tool_lines = temp;
}

bool CanvasOperation::mouse_press(QMouseEvent *event)
{
    return false;
}

bool CanvasOperation::mouse_release(QMouseEvent *event)
{
    return false;
}

bool CanvasOperation::mouse_move(QMouseEvent *event)
{
    return false;
}

bool CanvasOperation::mouse_double_click(QMouseEvent *event)
{
    return false;
}

void CanvasOperation::reset() {}


bool SelectOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        _select = true;
        _pos[0] = real_pos[0], _pos[1] = real_pos[1];
        tool_lines_count = 0;
        if (clicked_object = editer->select(_pos[0], _pos[1],
            event->modifiers() != Qt::KeyboardModifier::ControlModifier); clicked_object == nullptr)
        {
            if (event->modifiers() != Qt::KeyboardModifier::ControlModifier)
            {
                editer->reset_selected_mark();
            }
        }
        else
        {
            if (std::vector<Geo::Geometry *> selected_objects = editer->selected(); selected_objects.size() == 1)
            {
                switch (clicked_object->type())
                {
                case Geo::Type::BEZIER:
                    {
                        const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(clicked_object);
                        check_tool_lines_size(bezier->size() * 6);
                        for (size_t i = 1, count = bezier->size(); i < count; ++i)
                        {
                            tool_lines[tool_lines_count++] = (*bezier)[i - 1].x;
                            tool_lines[tool_lines_count++] = (*bezier)[i - 1].y;
                            ++tool_lines_count;
                            tool_lines[tool_lines_count++] = (*bezier)[i].x;
                            tool_lines[tool_lines_count++] = (*bezier)[i].y;
                            ++tool_lines_count;
                        }
                    }
                    break;
                case Geo::Type::BSPLINE:
                    {
                        const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(clicked_object);
                        check_tool_lines_size(bspline->path_points.size() * 6);
                        for (size_t i = 1, count = bspline->path_points.size(); i < count; ++i)
                        {
                            tool_lines[tool_lines_count++] = bspline->path_points[i - 1].x;
                            tool_lines[tool_lines_count++] = bspline->path_points[i - 1].y;
                            ++tool_lines_count;
                            tool_lines[tool_lines_count++] = bspline->path_points[i].x;
                            tool_lines[tool_lines_count++] = bspline->path_points[i].y;
                            ++tool_lines_count;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            if (event->modifiers() != Qt::ControlModifier && GlobalSetting::setting().auto_aligning)
            {
                std::list<QLineF> reflines;
                if (editer->auto_aligning(clicked_object, real_pos[0], real_pos[1], reflines, true))
                {
                    canvas->refresh_selected_vbo();
                    canvas->refresh_reflines_vbo();
                }
                check_tool_lines_size(reflines.size() * 6);
                for (const QLineF &line : reflines)
                {
                    tool_lines[tool_lines_count++] = line.x1();
                    tool_lines[tool_lines_count++] = line.y1();
                    ++tool_lines_count;
                    tool_lines[tool_lines_count++] = line.x2();
                    tool_lines[tool_lines_count++] = line.y2();
                    ++tool_lines_count;
                }
            }
            tool[0] = Tool::Move;
            _select = false;
        }
        canvas->refresh_selected_ibo();
        return true;
    }
    else
    {
        return false;
    }
}

bool SelectOperation::mouse_release(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        canvas->refresh_select_rect(0, 0, 0, 0);
        _select = false;
    }
    return false;
}

bool SelectOperation::mouse_move(QMouseEvent *event)
{
    if (_select)
    {
        canvas->refresh_select_rect(_pos[0], _pos[1], real_pos[0], real_pos[1]);
        if (std::vector<Geo::Geometry *> selected_objects = editer->select(
            Geo::AABBRect(_pos[0], _pos[1], real_pos[0], real_pos[1]),
            event->modifiers() != Qt::KeyboardModifier::ControlModifier); !selected_objects.empty())
        {
            canvas->refresh_selected_ibo(selected_objects);
        }
        tool_lines_count = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void SelectOperation::reset()
{
    _select = false;
}


bool MoveOperation::mouse_release(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        tool[0] = Tool::Select;
        if (std::vector<Geo::Geometry *> selected_objects = editer->selected(); !selected_objects.empty()
            && (press_pos[0] != release_pos[0] || press_pos[1] != release_pos[1]))
        {
            if (editer->edited_shape().empty())
            {
                editer->push_backup_command(new UndoStack::TranslateCommand(selected_objects,
                    release_pos[0] - press_pos[0], release_pos[1] - press_pos[1]));
            }
            else
            {
                editer->push_backup_command(new UndoStack::ChangeShapeCommand(selected_objects.front(), editer->edited_shape()));
                editer->edited_shape().clear();
            }
        }
    }
    return false;
}

bool MoveOperation::mouse_move(QMouseEvent *event)
{
    tool_lines_count = 0;
    if (std::vector<Geo::Geometry *> selected_objects = editer->selected(); selected_objects.size() <= 1 && clicked_object != nullptr)
    {
        editer->translate_points(clicked_object, real_pos[2], real_pos[3], real_pos[0], real_pos[1], event->modifiers() == Qt::ControlModifier);
        switch (clicked_object->type())
        {
        case Geo::Type::BEZIER:
            {
                const Geo::Bezier *bezier = static_cast<const Geo::Bezier *>(clicked_object);
                check_tool_lines_size(bezier->size() * 6);
                for (size_t i = 1, count = bezier->size(); i < count; ++i)
                {
                    tool_lines[tool_lines_count++] = (*bezier)[i - 1].x;
                    tool_lines[tool_lines_count++] = (*bezier)[i - 1].y;
                    ++tool_lines_count;
                    tool_lines[tool_lines_count++] = (*bezier)[i].x;
                    tool_lines[tool_lines_count++] = (*bezier)[i].y;
                    ++tool_lines_count;
                }
                canvas->refresh_cache_vbo(0);
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline *bspline = static_cast<const Geo::BSpline *>(clicked_object);
                check_tool_lines_size(bspline->path_points.size() * 6);
                for (size_t i = 1, count = bspline->path_points.size(); i < count; ++i)
                {
                    tool_lines[tool_lines_count++] = bspline->path_points[i - 1].x;
                    tool_lines[tool_lines_count++] = bspline->path_points[i - 1].y;
                    ++tool_lines_count;
                    tool_lines[tool_lines_count++] = bspline->path_points[i].x;
                    tool_lines[tool_lines_count++] = bspline->path_points[i].y;
                    ++tool_lines_count;
                }
                canvas->refresh_cache_vbo(0);
            }
            break;
        default:
            break;
        }
        canvas->refresh_vbo(clicked_object->type(), event->modifiers() == Qt::ControlModifier);
        if (event->modifiers() == Qt::ControlModifier)
        {
            canvas->refresh_selected_ibo(clicked_object);
        }
        if (event->modifiers() != Qt::ControlModifier && GlobalSetting::setting().auto_aligning)
        {
            std::list<QLineF> reflines;
            if (editer->auto_aligning(clicked_object, real_pos[0], real_pos[1], reflines, true))
            {
                canvas->refresh_selected_vbo();
                canvas->refresh_reflines_vbo();
            }
            check_tool_lines_size(reflines.size() * 6);
            for (const QLineF &line : reflines)
            {
                tool_lines[tool_lines_count++] = line.x1();
                tool_lines[tool_lines_count++] = line.y1();
                ++tool_lines_count;
                tool_lines[tool_lines_count++] = line.x2();
                tool_lines[tool_lines_count++] = line.y2();
                ++tool_lines_count;
            }
        }
    }
    else
    {
        std::set<Geo::Type> types;
        for (Geo::Geometry *object : selected_objects)
        {
            editer->translate_points(object, real_pos[2], real_pos[3], real_pos[0], real_pos[1], false);
            types.insert(object->type());
        }
        canvas->refresh_vbo(types, false);
    }
    if (GlobalSetting::setting().show_text)
    {
        canvas->refresh_text_vbo();
    }
    return true;
}


bool MeasureOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_is_head)
        {
            _pos[0].x = tool_lines[3] = tool_lines[0] = real_pos[0];
            _pos[0].y = tool_lines[4] = tool_lines[1] = real_pos[1];
            info.clear();
            tool_lines_count = 6;
        }
        else
        {
            _pos[1].x = real_pos[0];
            _pos[1].y = real_pos[1];
            tool_lines[3] = real_pos[0];
            tool_lines[4] = real_pos[1];
            info = QString("Length:%1").arg(Geo::distance(_pos[0], _pos[1]));
        }
        _is_head = !_is_head;
        return true;
    }
    else
    {
        return false;
    }
}

bool MeasureOperation::mouse_move(QMouseEvent *event)
{
    if (_is_head)
    {
        return false;
    }
    else
    {
        _pos[1].x = real_pos[0];
        _pos[1].y = real_pos[1];
        tool_lines[3] = real_pos[0];
        tool_lines[4] = real_pos[1];
        info = QString("Length:%1").arg(Geo::distance(_pos[0], _pos[1]));
        return true;
    }
}

void MeasureOperation::reset()
{
    _is_head = true;
}


bool AngleOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        switch (_index++)
        {
        case 0:
            _pos[1].x = tool_lines[3] = tool_lines[9] = real_pos[0];
            _pos[1].y = tool_lines[4] = tool_lines[10] = real_pos[1];
            tool_lines_count = 6;
            info.clear();
            break;
        case 1:
            _pos[0].x = tool_lines[0] = tool_lines[6] = real_pos[0];
            _pos[0].y = tool_lines[1] = tool_lines[7] = real_pos[1];
            tool_lines_count = 12;
            break;
        case 2:
            _pos[2].x = tool_lines[6] = real_pos[0];
            _pos[2].y = tool_lines[7] = real_pos[1];
            _index = 0;
            info = QString("Angle:%1°").arg(Geo::rad_to_degree(Geo::angle(_pos[0], _pos[1], _pos[2])));
            break;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool AngleOperation::mouse_move(QMouseEvent *event)
{
    switch (_index)
    {
    case 1:
        tool_lines[0] = real_pos[0];
        tool_lines[1] = real_pos[1];
        return true;
    case 2:
        _pos[2].x = tool_lines[6] = real_pos[0];
        _pos[2].y = tool_lines[7] = real_pos[1];
        info = QString("Angle:%1°").arg(Geo::rad_to_degree(Geo::angle(_pos[0], _pos[1], _pos[2])));
        return true;
    default:
        return false;
    }
}

void AngleOperation::reset()
{
    _index = 0;
}


bool Circle0Operation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_set_center)
        {
            _parameters[0] = real_pos[0];
            _parameters[1] = real_pos[1];
            _parameters[2] = 10;
            const Geo::Polygon points = Geo::circle_to_polygon(_parameters[0],
                _parameters[1], _parameters[2], Geo::Circle::default_down_sampling_value);
            shape_count = 0;
            check_shape_size(points.size() * 3);
            for (const Geo::Point &point : points)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }

            tool_lines[0] = tool_lines[3] = real_pos[0];
            tool_lines[1] = tool_lines[4] = real_pos[1];
            tool_lines_count = 6;
        }
        else
        {
            _parameters[2] = Geo::distance(_parameters[0], _parameters[1], real_pos[0], real_pos[1]);
            canvas->add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2]));
            info.clear();
            tool[0] = Tool::Select;
            shape_count = tool_lines_count = 0;
        }
        _set_center = !_set_center;
        return true;
    }
    else
    {
        return false;
    }
}

bool Circle0Operation::mouse_move(QMouseEvent *event)
{
    if (_set_center)
    {
        return false;
    }
    else
    {
        tool_lines[3] = real_pos[0], tool_lines[4] = real_pos[1];
        _parameters[2] = Geo::distance(_parameters[0], _parameters[1], real_pos[0], real_pos[1]); 
        const Geo::Polygon points = Geo::circle_to_polygon(_parameters[0],
            _parameters[1], _parameters[2], Geo::Circle::default_down_sampling_value);
        shape_count = 0;
        check_shape_size(points.size() * 3);
        for (const Geo::Point &point : points)
        {
            shape[shape_count++] = point.x;
            shape[shape_count++] = point.y;
            ++shape_count;
        }
        info = QString("Radius:%1").arg(_parameters[2]);
        return true;
    }
}

void Circle0Operation::reset()
{
    _set_center = true;
}


bool Circle1Operation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_set_first_point)
        {
            _parameters[0] = _parameters[2] = real_pos[0];
            _parameters[1] = _parameters[3] = real_pos[1];
            tool_lines_count = 6;
            tool_lines[0] = tool_lines[3] = real_pos[0];
            tool_lines[1] = tool_lines[4] = real_pos[1];
        }
        else
        {
            _parameters[2] = real_pos[0], _parameters[3] = real_pos[1];
            canvas->add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2], _parameters[3]));
            shape_count = tool_lines_count = 0;
            tool[0] = Tool::Select;
            info.clear();
        }
        _set_first_point = !_set_first_point;
        return true;
    }
    else
    {
        return false;
    }
}

bool Circle1Operation::mouse_move(QMouseEvent *event)
{
    if (_set_first_point)
    {
        return false;
    }
    else
    {
        _parameters[2] = tool_lines[3] = real_pos[0];
        _parameters[3] = tool_lines[4] = real_pos[1];
        const double r = std::hypot(_parameters[0] - _parameters[2], _parameters[1] - _parameters[3]) / 2;
        const Geo::Polygon points = Geo::circle_to_polygon((_parameters[0] + _parameters[2]) / 2,
            (_parameters[1] + _parameters[3]) / 2, r, Geo::Circle::default_down_sampling_value);
        shape_count = 0;
        check_shape_size(points.size() * 3);
        for (const Geo::Point &point : points)
        {
            shape[shape_count++] = point.x;
            shape[shape_count++] = point.y;
            ++shape_count;
        }
        info = QString("Length:%1 Angle:%2°").arg(r).arg(Geo::rad_to_degree(Geo::angle(
            Geo::Point(_parameters[0], _parameters[1]), Geo::Point(real_pos[0], real_pos[1]))));
        return true;
    }
}

void Circle1Operation::reset()
{
    _set_first_point = true;
}


bool Circle2Operation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        switch (_index++)
        {
        case 0:
            _parameters[0] = real_pos[0];
            _parameters[1] = real_pos[1];
            tool_lines[0] = tool_lines[3] = tool_lines[6] = real_pos[0];
            tool_lines[1] = tool_lines[4] = tool_lines[7] = real_pos[1];
            tool_lines_count = 6;
            break;
        case 1:
            _parameters[2] = real_pos[0];
            _parameters[3] = real_pos[1];
            tool_lines[3] = tool_lines[6] = tool_lines[9] = real_pos[0];
            tool_lines[4] = tool_lines[7] = tool_lines[10] = real_pos[1];
            tool_lines_count = 12;
            break;
        case 2:
            _parameters[4] = real_pos[0], _parameters[5] = real_pos[1];
            canvas->add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2],
                _parameters[3], _parameters[4], _parameters[5]));
            _index = shape_count = tool_lines_count = 0;
            tool[0] = Tool::Select;
            info.clear();
            break;
        default:
            break;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Circle2Operation::mouse_move(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        switch (_index)
        {
        case 1:
            _parameters[2] = real_pos[0];
            _parameters[3] = real_pos[1];
            tool_lines[3] = tool_lines[6] = real_pos[0];
            tool_lines[4] = tool_lines[7] = real_pos[1];
            info = QString("Length:%1").arg(Geo::distance(_parameters[0],
                _parameters[1], _parameters[2], _parameters[3]));
            break;
        case 2:
            {
                _parameters[4] = tool_lines[9] = real_pos[0];
                _parameters[5] = tool_lines[10] = real_pos[1];
                const Geo::Polygon points = Geo::circle_to_polygon(Geo::Circle(_parameters[0], _parameters[1], _parameters[2],
                    _parameters[3], _parameters[4], _parameters[5]), Geo::Circle::default_down_sampling_value);
                shape_count = 0;
                check_shape_size(points.size() * 3);
                for (const Geo::Point &point : points)
                {
                    shape[shape_count++] = point.x;
                    shape[shape_count++] = point.y;
                    ++shape_count;
                }
                info = QString("Length:%1 Angle:%2°").arg(Geo::distance(_parameters[2],
                    _parameters[3], _parameters[4], _parameters[5])).arg(Geo::rad_to_degree(
                        Geo::angle(Geo::Point(_parameters[0], _parameters[1]),
                        Geo::Point(_parameters[2], _parameters[3]), Geo::Point(_parameters[4], _parameters[5]))));
            }
            break;
        default:
            break;
        }
        return true;
    }
    else
    {
        return false;
    }
}

void Circle2Operation::reset()
{
    _index = 0;
}



bool PolythingOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.empty())
        {
            _points.emplace_back(real_pos[0], real_pos[1]);
            shape[0] = real_pos[0];
            shape[1] = real_pos[1];
            shape_count = 3;
        }
        _points.emplace_back(real_pos[0], real_pos[1]);
        shape[shape_count++] = real_pos[0];
        shape[shape_count++] = real_pos[1];
        ++shape_count;
        check_shape_size();
        return true;
    }
    else
    {
        return false;
    }
}

bool PolythingOperation::mouse_move(QMouseEvent *event)
{
    if (_points.empty())
    {
        return false;
    }
    else
    {
        if (event->modifiers() == Qt::KeyboardModifier::ControlModifier)
        {
            const Geo::Point &coord = _points[_points.size() - 2];
            if (std::abs(real_pos[0] - coord.x) > std::abs(real_pos[1] - coord.y))
            {
                _points.back().x = real_pos[0];
                _points.back().y = coord.y;
            }
            else
            {
                _points.back().x = coord.x;
                _points.back().y = real_pos[1];
            }
        }
        else
        {
            _points.back().x = real_pos[0];
            _points.back().y = real_pos[1];
        }
        shape[shape_count - 3] = _points.back().x;
        shape[shape_count - 2] = _points.back().y;
        info = QString("Length:%1 Angle:%2°").arg(Geo::distance(_points[_points.size() - 2], _points.back())
            ).arg(Geo::rad_to_degree(Geo::angle(_points[_points.size() - 2], _points.back())));
        return true;
    }
}

bool PolythingOperation::mouse_double_click(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.size() < 4)
        {
            _points.clear();
            shape_count = 0;
            return false;
        }
        _points.pop_back();
        if (_points.size() > 3 && Geo::distance(_points.front(), _points.back()) <= 8 / view_ratio)
        {
            _points.back() = _points.front();
            canvas->add_geometry(new Geo::Polygon(_points.begin(), _points.end()));
        }
        else
        {
            _points.pop_back();
            canvas->add_geometry(new Geo::Polyline(_points.begin(), _points.end()));
        }
        _points.clear();
        tool[0] = Tool::Select;
        info.clear();
        shape_count = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void PolythingOperation::reset()
{
    _points.clear();
}


bool RectOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_set_first_point)
        {
            _parameters[0] = _parameters[2] = real_pos[0];
            _parameters[1] = _parameters[3] = real_pos[1];
            shape_count = 15;
            shape[0] = shape[3] = shape[6] = shape[9] = shape[12] = real_pos[0];
            shape[1] = shape[4] = shape[7] = shape[10] = shape[13] = real_pos[1];
        }
        else
        {
            _parameters[2] = real_pos[0], _parameters[3] = real_pos[1];
            canvas->add_geometry(new Geo::Polygon(Geo::AABBRect(_parameters[0],
                _parameters[1], _parameters[2], _parameters[3])));
            shape_count = 0;
            tool[0] = Tool::Select;
            info.clear();
        }
        _set_first_point = !_set_first_point;
        return true;
    }
    else
    {
        return false;
    }
}

bool RectOperation::mouse_move(QMouseEvent *event)
{
    if (_set_first_point)
    {
        return false;
    }
    else
    {
        if (event->modifiers() == Qt::ControlModifier)
        {
            const double width = std::max(std::abs(real_pos[0] - _parameters[0]), std::abs(real_pos[1] - _parameters[1]));
            if (real_pos[0] > _parameters[0])
            {
                if (real_pos[1] > _parameters[1])
                {
                    _parameters[2] = _parameters[0] + width;
                    _parameters[3] = _parameters[1] + width;
                }
                else
                {
                    _parameters[2] = _parameters[0] + width;
                    _parameters[3] = _parameters[1];
                    _parameters[1] -= width;
                }
            }
            else
            {
                if (real_pos[1] > _parameters[1])
                {
                    _parameters[2] = _parameters[0];
                    _parameters[3] = _parameters[1] + width;
                    _parameters[0] -= width;
                }
                else
                {
                    _parameters[2] = _parameters[0];
                    _parameters[3] = _parameters[1];
                    _parameters[0] -= width;
                    _parameters[1] -= width;
                }
            }
        }
        else
        {
            _parameters[2] = real_pos[0];
            _parameters[3] = real_pos[1];
        }
        shape[0] = _parameters[0], shape[1] = _parameters[1];
        shape[3] = _parameters[2], shape[4] = _parameters[1];
        shape[6] = _parameters[2], shape[7] = _parameters[3];
        shape[9] = _parameters[0], shape[10] = _parameters[3];
        shape[12] = _parameters[0], shape[13] = _parameters[1];
        info = QString("Width:%1 Height:%2").arg(std::abs(_parameters[0]
            - _parameters[2])).arg(std::abs(_parameters[1] - _parameters[3]));
        return true;
    }
}

void RectOperation::reset()
{
    _set_first_point = true;
}


bool BSplineOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.empty())
        {
            _order = GlobalSetting::setting().ui->curve_spb->value();
            _points.emplace_back(real_pos[0], real_pos[1]);
            tool_lines_count = 0;
        }
        tool_lines[tool_lines_count++] = _points.back().x;
        tool_lines[tool_lines_count++] = _points.back().y;
        ++tool_lines_count;
        check_tool_lines_size();
        _points.emplace_back(real_pos[0], real_pos[1]);
        tool_lines[tool_lines_count++] = real_pos[0];
        tool_lines[tool_lines_count++] = real_pos[1];
        ++tool_lines_count;
        check_tool_lines_size();

        if (_points.size() > 2)
        {
            shape_count = 0;
            const Geo::Polyline path = _order == 3 ? Geo::CubicBSpline(_points.begin(), _points.end(),
                true).shape() : Geo::QuadBSpline(_points.begin(), _points.end(), true).shape();
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool BSplineOperation::mouse_move(QMouseEvent *event)
{
    if (_points.empty())
    {
        return false;
    }
    else
    {
        tool_lines[tool_lines_count - 3] = real_pos[0];
        tool_lines[tool_lines_count - 2] = real_pos[1];
        _points.back().x = real_pos[0], _points.back().y = real_pos[1];
        if (_points.size() > 2)
        {
            shape_count = 0;
            const Geo::Polyline path = _order == 3 ? Geo::CubicBSpline(_points.begin(), _points.end(),
                true).shape() : Geo::QuadBSpline(_points.begin(), _points.end(), true).shape();
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
        }
        info = QString("Length:%1 Angle:%2°").arg(Geo::distance(_points[_points.size() - 2], _points.back())
            ).arg(Geo::rad_to_degree(Geo::angle(_points[_points.size() - 2], _points.back())));
        return true;
    }
}

bool BSplineOperation::mouse_double_click(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.size() > 4)
        {
            _points.pop_back();
            _points.pop_back();
            if (_order == 3)
            {
                canvas->add_geometry(new Geo::CubicBSpline(_points.begin(), _points.end(), true));
            }
            else
            {
                canvas->add_geometry(new Geo::QuadBSpline(_points.begin(), _points.end(), true));
            }
        }
        tool[0] = Tool::Select;
        shape_count = tool_lines_count = 0;
        _points.clear();
        info.clear();
        return true;
    }
    else
    {
        return false;
    }
}

void BSplineOperation::reset()
{
    _points.clear();
}


bool BezierOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.empty())
        {
            _order = GlobalSetting::setting().ui->curve_spb->value();
            _points.emplace_back(real_pos[0], real_pos[1]);
            tool_lines_count = 0;
        }
        tool_lines[tool_lines_count++] = _points.back().x;
        tool_lines[tool_lines_count++] = _points.back().y;
        ++tool_lines_count;
        check_tool_lines_size();
        _points.emplace_back(real_pos[0], real_pos[1]);
        tool_lines[tool_lines_count++] = real_pos[0];
        tool_lines[tool_lines_count++] = real_pos[1];
        ++tool_lines_count;
        check_tool_lines_size();

        if (_points.size() > 3)
        {
            shape_count = 0;
            const Geo::Polyline path = Geo::Bezier(_points.begin(), _points.end(), _order, true).shape();
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool BezierOperation::mouse_move(QMouseEvent *event)
{
    if (_points.empty())
    {
        return false;
    }
    else
    {
        tool_lines[tool_lines_count - 3] = real_pos[0];
        tool_lines[tool_lines_count - 2] = real_pos[1];
        _points.back().x = real_pos[0], _points.back().y = real_pos[1];
        if (_points.size() > 3)
        {
            shape_count = 0;
            const Geo::Polyline path = Geo::Bezier(_points.begin(), _points.end(), _order, true).shape();
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
        }
        info = QString("Length:%1 Angle:%2°").arg(Geo::distance(_points[_points.size() - 2], _points.back())
            ).arg(Geo::rad_to_degree(Geo::angle(_points[_points.size() - 2], _points.back())));
        return true;
    }
}

bool BezierOperation::mouse_double_click(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (_points.size() > 4)
        {
            _points.pop_back();
            _points.pop_back();
            canvas->add_geometry(new Geo::Bezier(_points.begin(), _points.end(), _order, true));
        }
        tool[0] = Tool::Select;
        shape_count = tool_lines_count = 0;
        _points.clear();
        info.clear();
        return true;
    }
    else
    {
        return false;
    }
}

void BezierOperation::reset()
{
    _points.clear();
}


bool TextOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        tool[0] = Tool::Select;
        canvas->add_geometry(new Text(real_pos[0], real_pos[1], GlobalSetting::setting().text_size));
        return true;
    }
    else
    {
        return false;
    }
}


bool EllipseOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        switch (_index++)
        {
        case 0:
            _parameters[0] = real_pos[0], _parameters[1] = real_pos[1];
            tool_lines_count = 6;
            break;
        case 1:
            _parameters[2] = Geo::distance(_parameters[0], _parameters[1], real_pos[0], real_pos[1]);
            _parameters[4] = Geo::angle(Geo::Point(_parameters[0], _parameters[1]), Geo::Point(real_pos[0], real_pos[1]));
            tool_lines[0] = real_pos[0], tool_lines[1] = real_pos[1];
            tool_lines[3] = _parameters[0] * 2 - real_pos[0], tool_lines[4] = _parameters[1] * 2 - real_pos[1];
            tool_lines[6] = tool_lines[9] = tool_lines[0], tool_lines[7] = tool_lines[10] = tool_lines[1];
            tool_lines_count = 12;
            break;
        case 2:
            {
                _parameters[3] = Geo::distance(Geo::Point(real_pos[0], real_pos[1]),
                    Geo::Point(tool_lines[3], tool_lines[4]), Geo::Point(tool_lines[0], tool_lines[1]), true);
                Geo::Ellipse *ellipse = new Geo::Ellipse(_parameters[0], _parameters[1], _parameters[2], _parameters[3]);
                ellipse->rotate(_parameters[0], _parameters[1], _parameters[4]);
                canvas->add_geometry(ellipse);
                _index = shape_count = tool_lines_count = 0;
                tool[0] = Tool::Select;
                info.clear();
            }
            break;
        default:
            break;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool EllipseOperation::mouse_move(QMouseEvent *event)
{
    switch (_index)
    {
    case 1:
        {
            _parameters[2] = Geo::distance(_parameters[0], _parameters[1], real_pos[0], real_pos[1]);
            _parameters[4] = Geo::angle(Geo::Point(_parameters[0], _parameters[1]), Geo::Point(real_pos[0], real_pos[1]));
            tool_lines[0] = real_pos[0], tool_lines[1] = real_pos[1];
            tool_lines[3] = _parameters[0] * 2 - real_pos[0], tool_lines[4] = _parameters[1] * 2 - real_pos[1];
            const Geo::Polyline path = Geo::Circle(_parameters[0], _parameters[1], _parameters[2]).shape();
            shape_count = 0;
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
            info = QString("x:%1 y:%2 a:%3 b:%3").arg(_parameters[0]).arg(_parameters[1]).arg(_parameters[2]);
        }
        break;
    case 2:
        {
            _parameters[3] = Geo::distance(Geo::Point(real_pos[0], real_pos[1]),
                Geo::Point(tool_lines[3], tool_lines[4]), Geo::Point(tool_lines[0], tool_lines[1]), true);
            Geo::Ellipse ellipse(_parameters[0], _parameters[1], _parameters[2], _parameters[3]);
            ellipse.rotate(_parameters[0], _parameters[1], _parameters[4]);
            tool_lines[6] = ellipse.b0().x, tool_lines[7] = ellipse.b0().y;
            tool_lines[9] = ellipse.b1().x, tool_lines[10] = ellipse.b1().y;
            const Geo::Polyline &path = ellipse.shape();
            shape_count = 0;
            check_shape_size(path.size() * 3);
            for (const Geo::Point &point : path)
            {
                shape[shape_count++] = point.x;
                shape[shape_count++] = point.y;
                ++shape_count;
            }
            info = QString("x:%1 y:%2 a:%3 b:%4").arg(_parameters[0]).arg(_parameters[1]).arg(_parameters[2]).arg(_parameters[3]);
        }
        break;
    default:
        break;
    }
    return true;
}

void EllipseOperation::reset()
{
    _index = 0;
}


bool MirrorOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        std::vector<Geo::Geometry *> selected_objects = editer->selected();
        if (clicked_object = editer->select(real_pos[0], real_pos[1], false); !selected_objects.empty() && clicked_object != nullptr
            && editer->mirror(selected_objects, clicked_object, event->modifiers() == Qt::ControlModifier))
        {
            clicked_object->is_selected = false;
            std::set<Geo::Type> types;
            for (const Geo::Geometry *object : selected_objects)
            {
                if (const Combination *combination = dynamic_cast<const Combination *>(object))
                {
                    for (const Geo::Geometry *item : *combination)
                    {
                        types.insert(item->type());
                    }
                }
                else
                {
                    types.insert(object->type());
                }
            }
            canvas->refresh_vbo(types, true);
            canvas->refresh_selected_ibo();
            tool[0] = Tool::Select;
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


bool RingArrayOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (std::vector<Geo::Geometry *> selected_objects = editer->selected();
            editer->ring_array(selected_objects, real_pos[0], real_pos[1], GlobalSetting::setting().ui->array_item->value()))
        {
            std::set<Geo::Type> types;
            for (const Geo::Geometry *object : selected_objects)
            {
                if (const Combination *combination = dynamic_cast<const Combination *>(object))
                {
                    for (const Geo::Geometry *item : *combination)
                    {
                        types.insert(item->type());
                    }
                }
                else
                {
                    types.insert(object->type());
                }
            }
            canvas->refresh_vbo(types, true);
            canvas->refresh_selected_ibo();
            tool[0] = Tool::Select;
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


bool PolygonDifferenceOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (clicked_object = editer->select(real_pos[0], real_pos[1], true);
            clicked_object->type() == Geo::Type::POLYGON)
        {
            if (polygon == nullptr)
            {
                polygon = static_cast<Geo::Polygon *>(clicked_object);
            }
            else if (polygon != clicked_object && clicked_object->type() == Geo::Type::POLYGON)
            {
                if (editer->polygon_difference(polygon, static_cast<const Geo::Polygon *>(clicked_object)))
                {
                    canvas->refresh_vbo(Geo::Type::POLYGON, true);
                    canvas->refresh_selected_ibo();
                    tool[0] = Tool::Select;
                    return true;
                }
            }
        }
        return false;
    }
    else
    {
        return false;
    }
}


bool FilletOperation::mouse_press(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (clicked_object = editer->select(real_pos[0], real_pos[1], true))
        {
            bool result = false;
            double dis = DBL_MAX;
            Geo::Point point;
            switch (clicked_object->type())
            {
            case Geo::Type::POLYGON:
                for (const Geo::Point &p : *static_cast<Geo::Polygon *>(clicked_object))
                {
                    if (Geo::distance(p.x, p.y, real_pos[0], real_pos[1]) < dis)
                    {
                        dis = Geo::distance(p.x, p.y, real_pos[0], real_pos[1]);
                        point = p;
                    }
                }
                if (editer->fillet(dynamic_cast<Geo::Polygon *>(clicked_object),
                    point, GlobalSetting::setting().ui->fillet_sbx->value()))
                {
                    canvas->refresh_vbo(Geo::Type::POLYGON, true);
                    canvas->refresh_selected_ibo();
                    result = true;
                }
                break;
            case Geo::Type::POLYLINE:
                for (const Geo::Point &p : *static_cast<Geo::Polyline *>(clicked_object))
                {
                    if (Geo::distance(p.x, p.y, real_pos[0], real_pos[1]) < dis)
                    {
                        dis = Geo::distance(p.x, p.y, real_pos[0], real_pos[1]);
                        point = p;
                    }
                }
                if (editer->fillet(dynamic_cast<Geo::Polyline *>(clicked_object),
                    point, GlobalSetting::setting().ui->fillet_sbx->value()))
                {
                    canvas->refresh_vbo(Geo::Type::POLYLINE, true);
                    canvas->refresh_selected_ibo();
                    result = true;
                }
                break;
            default:
                break;
            }
            return result;
        }
        return false;
    }
    else
    {
        return false;
    }
}


