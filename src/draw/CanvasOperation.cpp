#include "draw/CanvasOperation.hpp"
#include "base/Algorithm.hpp"



using namespace CanvasOperations;

const double *CanvasOperation::matrix = nullptr;
double *CanvasOperation::shape = new double[513];
double *CanvasOperation::tool_lines = new double[63];
unsigned int CanvasOperation::shape_len = 513;
unsigned int CanvasOperation::shape_count = 0;
unsigned int CanvasOperation::tool_lines_len = 63;
unsigned int CanvasOperation::tool_lines_count = 0;
float CanvasOperation::tool_line_width = 1.4f;
double CanvasOperation::real_pos[2];
QString CanvasOperation::info;

void CanvasOperation::init(const double *mat)
{
    matrix = mat;
    std::fill_n(shape, shape_len, 0);
    std::fill_n(tool_lines, tool_lines_len, 0);

    operations[1] = new MeasureOperation();
    operations[2] = new AngleOperation();
}

void CanvasOperation::clear()
{
    for (int i = 1; i < 3; ++i)
    {
        operations[i]->reset();
    }
    shape_count = tool_lines_count = 0;
}

CanvasOperation::~CanvasOperation()
{
    for (int i = 1; i < 3; ++i)
    {
        delete operations[i];
    }
}

CanvasOperation *CanvasOperation::operator[](const Tool tool)
{
    if (const int index = static_cast<const int>(tool); index >= 0 && index <= 2)
    {
        return operations[index];
    }
    else
    {
        return nullptr;
    }
}

void CanvasOperation::calc_real_pos(const double x, const double y)
{
    real_pos[0] = x * matrix[0] + y * matrix[3] + matrix[6];
    real_pos[1] = x * matrix[1] + y * matrix[4] + matrix[7];
}

bool CanvasOperation::mouse_press(QMouseEvent *event)
{
    calc_real_pos(event->position().x(), event->position().y());
    return false;
}

bool CanvasOperation::mouse_release(QMouseEvent *event)
{
    calc_real_pos(event->position().x(), event->position().y());
    return false;
}

bool CanvasOperation::mouse_move(QMouseEvent *event)
{
    calc_real_pos(event->position().x(), event->position().y());
    return false;
}

bool CanvasOperation::mouse_double_click(QMouseEvent *event)
{
    calc_real_pos(event->position().x(), event->position().y());
    return false;
}

Geo::Geometry *CanvasOperation::geometry()
{
    return nullptr;
}

void CanvasOperation::reset() {}

#define o_press CanvasOperation::mouse_press(event);
#define o_release CanvasOperation::mouse_release(event);
#define o_move CanvasOperation::mouse_move(event);
#define o_double_click CanvasOperation::mouse_double_click(event);


bool MeasureOperation::mouse_press(QMouseEvent *event)
{
    o_press
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
    o_move
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
    o_press
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
    o_move
    switch (_index)
    {
    case 0:
        return false;
    case 1:
        tool_lines[0] = real_pos[0];
        tool_lines[1] = real_pos[1];
        return true;
    case 2:
        _pos[2].x = tool_lines[6] = real_pos[0];
        _pos[2].y = tool_lines[7] = real_pos[1];
        info = QString("Angle:%1°").arg(Geo::rad_to_degree(Geo::angle(_pos[0], _pos[1], _pos[2])));
        return true;
    }
}

void AngleOperation::reset()
{
    _index = 0;
}


bool PolythingOperation::mouse_press(QMouseEvent *event)
{
    o_press
    if (_points.empty())
    {
        _points.emplace_back(real_pos[0], real_pos[1]);
        shape[0] = shape[3] = real_pos[0];
        shape[1] = shape[4] = real_pos[1];
        shape_count = 3;
    }
    _points.emplace_back(real_pos[0], real_pos[1]);
    
    shape_count += 3;
}

bool PolythingOperation::mouse_move(QMouseEvent *event)
{
    o_move
    if (!_points.empty())
    {
        _points.back().x = real_pos[0];
        _points.back().y = real_pos[1];
    }
}

bool PolythingOperation::mouse_double_click(QMouseEvent *event)
{
    o_double_click

}

void PolythingOperation::reset()
{
    _points.clear();
}