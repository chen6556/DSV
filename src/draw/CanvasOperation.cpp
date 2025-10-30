#include "draw/CanvasOperation.hpp"
#include "base/Algorithm.hpp"
#include "io/GlobalSetting.hpp"



using namespace CanvasOperations;

double *CanvasOperation::shape = new double[513];
double *CanvasOperation::tool_lines = new double[63];
unsigned int CanvasOperation::shape_len = 513;
unsigned int CanvasOperation::shape_count = 0;
unsigned int CanvasOperation::tool_lines_len = 63;
unsigned int CanvasOperation::tool_lines_count = 0;
float CanvasOperation::tool_line_width = 1.4f;
double CanvasOperation::real_pos[2];
double CanvasOperation::view_ratio = 1;
bool CanvasOperation::finish = false;
QString CanvasOperation::info;
std::function<void(Geo::Geometry *)> CanvasOperation::add_geometry;

void CanvasOperation::init(std::function<void(Geo::Geometry *)> func)
{
    add_geometry = func;
    std::fill_n(shape, shape_len, 0);
    std::fill_n(tool_lines, tool_lines_len, 0);

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
}

void CanvasOperation::clear()
{
    for (int i = 1; i < 3; ++i)
    {
        operations[i]->reset();
    }
    shape_count = tool_lines_count = 0;
    finish = false;
    info.clear();
}

CanvasOperation::~CanvasOperation()
{
    for (int i = 1, count = static_cast<int>(Tool::End); i < count; ++i)
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
            while (shape_len < points.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
            shape_count = 0;
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
            add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2]));
            info.clear();
            finish = true;
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
        while (shape_len < points.size() * 3)
        {
            shape_len *= 2;
        }
        delete[] shape;
        shape = new double[shape_len];
        std::fill_n(shape, shape_len, 0);
        shape_count = 0;
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
            add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2], _parameters[3]));
            shape_count = tool_lines_count = 0;
            finish = true;
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
        while (shape_len < points.size() * 3)
        {
            shape_len *= 2;
        }
        delete[] shape;
        shape = new double[shape_len];
        std::fill_n(shape, shape_len, 0);
        shape_count = 0;
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
            add_geometry(new Geo::Circle(_parameters[0], _parameters[1], _parameters[2],
                _parameters[3], _parameters[4], _parameters[5]));
            _index = shape_count = tool_lines_count = 0;
            finish = true;
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
                while (shape_len < points.size() * 3)
                {
                    shape_len *= 2;
                }
                delete[] shape;
                shape = new double[shape_len];
                std::fill_n(shape, shape_len, 0);
                shape_count = 0;
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
            add_geometry(new Geo::Polygon(_points.begin(), _points.end()));
        }
        else
        {
            _points.pop_back();
            add_geometry(new Geo::Polyline(_points.begin(), _points.end()));
        }
        _points.clear();
        finish = true;
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
            add_geometry(new Geo::Polygon(Geo::AABBRect(_parameters[0],
                _parameters[1], _parameters[2], _parameters[3])));
            shape_count = 0;
            finish = true;
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
            tool_lines[0] = real_pos[0];
            tool_lines[1] = real_pos[1];
            tool_lines_count = 3;
        }
        _points.emplace_back(real_pos[0], real_pos[1]);
        tool_lines[tool_lines_count++] = real_pos[0];
        tool_lines[tool_lines_count++] = real_pos[1];
        ++tool_lines_count;

        if (_points.size() > 2)
        {
            shape_count = 0;
            const Geo::Polyline path = _order == 3 ? Geo::CubicBSpline(_points.begin(), _points.end(),
                true).shape() : Geo::QuadBSpline(_points.begin(), _points.end(), true).shape();
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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
        if (_points.size() > 3)
        {
            _points.pop_back();
            if (_order == 3)
            {
                add_geometry(new Geo::CubicBSpline(_points.begin(), _points.end(), true));
            }
            else
            {
                add_geometry(new Geo::QuadBSpline(_points.begin(), _points.end(), true));
            }
        }
        finish = true;
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
            tool_lines[0] = real_pos[0];
            tool_lines[1] = real_pos[1];
            tool_lines_count = 3;
        }
        _points.emplace_back(real_pos[0], real_pos[1]);
        tool_lines[tool_lines_count++] = real_pos[0];
        tool_lines[tool_lines_count++] = real_pos[1];
        ++tool_lines_count;

        if (_points.size() > 2)
        {
            shape_count = 0;
            const Geo::Polyline path = Geo::Bezier(_points.begin(), _points.end(), _order, true).shape();
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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
        if (_points.size() > 2)
        {
            shape_count = 0;
            const Geo::Polyline path = Geo::Bezier(_points.begin(), _points.end(), _order, true).shape();
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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
        if (_points.size() > 3)
        {
            _points.pop_back();
            add_geometry(new Geo::Bezier(_points.begin(), _points.end(), _order, true));
        }
        finish = true;
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
        finish = true;
        add_geometry(new Text(real_pos[0], real_pos[1], GlobalSetting::setting().text_size));
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
            _parameters[3] = Geo::angle(Geo::Point(_parameters[0], _parameters[1]), Geo::Point(real_pos[0], real_pos[1]));
            _index = shape_count = tool_lines_count = 0;
            finish = true;
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
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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
            _parameters[3] = Geo::angle(Geo::Point(_parameters[0], _parameters[1]), Geo::Point(real_pos[0], real_pos[1]));
            tool_lines[6] = real_pos[0], tool_lines[7] = real_pos[1];
            tool_lines[9] = _parameters[0] * 2 - real_pos[0], tool_lines[10] = _parameters[1] * 2 - real_pos[1];
            Geo::Ellipse ellipse(_parameters[0], _parameters[1], _parameters[2], _parameters[3]);
            ellipse.rotate(_parameters[0], _parameters[1], _parameters[4]);
            const Geo::Polyline &path = ellipse.shape();
            shape_count = 0;
            while (shape_len < path.size() * 3)
            {
                shape_len *= 2;
            }
            delete[] shape;
            shape = new double[shape_len];
            std::fill_n(shape, shape_len, 0);
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












