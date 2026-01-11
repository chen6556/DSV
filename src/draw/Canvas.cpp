#include <future>
#include <QPainterPath>

#include "draw/Canvas.hpp"
#include "draw/GLSL.hpp"
#include "io/GlobalSetting.hpp"

// only for GL test
const char *glErrorToString(GLenum err)
{
    switch (err)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNKNOWN_ERROR";
    }
}
// only for GL test
void checkGLError()
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        qDebug() << "GL error:" << err << glErrorToString(err);
    }
}

Canvas::Canvas(QWidget *parent)
    : QOpenGLWidget(parent), _input_line(this)
{
    init();
}

Canvas::~Canvas()
{
    delete _menu;
    delete _up;
    delete _down;
    delete _text_to_polylines;
    delete _bspline_to_bezier;
    delete _bezier_to_bspline;
    delete _change_bspline_model;
}


void Canvas::init()
{
    CanvasOperations::CanvasOperation::operation().init();
    CanvasOperations::CanvasOperation::canvas = this;

    _input_line.hide();
    init_menu();
}

void Canvas::init_menu()
{
    _menu = new QMenu(this);
    _up = new QAction("Up");
    _down = new QAction("Down");
    _text_to_polylines = new QAction("To Polylines");
    _bezier_to_bspline = new QAction("To BSpline");
    _bspline_to_bezier = new QAction("To Bezier");
    _change_bspline_model = new QAction("Show Controls");

    _menu->addAction(_up);
    _menu->addAction(_down);
    _menu->addAction(_text_to_polylines);
    _menu->addAction(_bezier_to_bspline);
    _menu->addAction(_bspline_to_bezier);
    _menu->addAction(_change_bspline_model);
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
    CanvasOperations::CanvasOperation::editer = editer;
}




void Canvas::initializeGL()
{
    qDebug() << "context:" << context();
    qDebug() << "context valid:" << context()->isValid();
    qDebug() << "format:" << context()->format();

    bool ok = initializeOpenGLFunctions();
    // assume init success
    Q_ASSERT(ok);
    glClearColor(0.117647f, 0.156862f, 0.188235f, 1.0f);

    unsigned int vertex_shader;
    unsigned int fragment_shader;

    glPointSize(7.8f); // 点大小
    glLineWidth(1.4f); // 线宽
    // glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFF);
    // glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 抗锯齿
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

    int maxUniformBlockSize;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxUniformBlockSize);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &GLSL::base_vss, NULL);
    glCompileShader(vertex_shader);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &GLSL::base_fss, NULL);
    glCompileShader(fragment_shader);

    _shader_program = glCreateProgram();
    glAttachShader(_shader_program, vertex_shader);
    glAttachShader(_shader_program, fragment_shader);
    glLinkProgram(_shader_program);
    glDeleteShader(fragment_shader);

    glDeleteShader(vertex_shader);
    _uniforms.w = glGetUniformLocation(_shader_program, "w");
    _uniforms.h = glGetUniformLocation(_shader_program, "h");
    _uniforms.vec0 = glGetUniformLocation(_shader_program, "vec0");
    _uniforms.vec1 = glGetUniformLocation(_shader_program, "vec1");
    _uniforms.color = glGetUniformLocation(_shader_program, "color");
    const bool show_control_points = _editer->selected_count() == 1;

    glUseProgram(_shader_program);
    glUniform3d(_uniforms.vec0, 1.0, 0.0, 0.0); // vec0
    glUniform3d(_uniforms.vec1, 0.0, -1.0, 0.0); // vec1

    glGenVertexArrays(1, &_VAO);
    glBindVertexArray(_VAO);
    {
        unsigned int temp[4];
        glGenBuffers(4, temp);
        _base_vbo.origin_and_select_rect = temp[0];
        _base_vbo.catched_points = temp[1];
        _base_vbo.operation_shape = temp[2];
        _base_vbo.operation_tool_lines = temp[3];
    }
    {
        unsigned int temp[8];
        glGenBuffers(8, temp);
        _shape_vbo.polyline = temp[0];
        _shape_vbo.polygon = temp[1];
        _shape_vbo.circle = temp[2];
        _shape_vbo.curve = temp[3];
        _shape_vbo.text = temp[4];
        _shape_vbo.circle_printable_points = temp[5];
        _shape_vbo.curve_printable_points = temp[6];
        _shape_vbo.point = temp[7];
    }
    {
        unsigned int temp[4];
        glGenBuffers(4, temp);
        _shape_ibo.polyline = temp[0];
        _shape_ibo.polygon = temp[1];
        _shape_ibo.circle = temp[2];
        _shape_ibo.curve = temp[3];
    }
    glGenBuffers(1, &_text_brush_IBO);
    {
        unsigned int temp[5];
        glGenBuffers(5, temp);
        _selected_ibo.polyline = temp[0];
        _selected_ibo.polygon = temp[1];
        _selected_ibo.circle = temp[2];
        _selected_ibo.curve = temp[3];
        _selected_ibo.point = temp[4];
    }

    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.catched_points); // catcheline points
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), _catchline_points, GL_STREAM_DRAW);

    double data[24] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), data, GL_DYNAMIC_DRAW);
}

void Canvas::resizeGL(int w, int h)
{
    glUniform1i(_uniforms.w, w / 2); // w
    glUniform1i(_uniforms.h, h / 2); // h
    glViewport(0, 0, w, h);

    _canvas_ctm[7] += (h - _canvas_height);
    glUniform3d(_uniforms.vec1, _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    _view_ctm[7] += (h - _canvas_height) / _ratio;
    _canvas_width = w, _canvas_height = h;

    _visible_area = Geo::AABBRect(0, 0, w, h);
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);    
}

void Canvas::paintGL()
{
    glUseProgram(_shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_shape_index_count.polyline > 0) // polyline
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline); // polyline
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.polyline, GL_UNSIGNED_INT, NULL);

        if (_selected_index_count.polyline > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.polyline, GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count.polygon > 0) // polygon
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon); // polygon
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.polygon, GL_UNSIGNED_INT, NULL);

        if (_selected_index_count.polygon > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.polygon, GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count.circle > 0) // circle
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle); // circle
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.circle, GL_UNSIGNED_INT, NULL);

        if (_selected_index_count.circle > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.circle, GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count.curve > 0) // curve
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve); // curve
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.curve, GL_UNSIGNED_INT, NULL);

        if (_selected_index_count.curve > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.curve, GL_UNSIGNED_INT, NULL);
        }
    }

    if (_point_count.point > 0) // point
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawArrays(GL_POINTS, 0, _point_count.point);

        if (_selected_index_count.point > 0) // selected
        {
            glBindBuffer(GL_ARRAY_BUFFER, _selected_ibo.point); // points
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_POINTS, _selected_index_count.point, GL_UNSIGNED_INT, NULL);
        }
    }

    if (GlobalSetting::setting().show_text && _text_brush_count > 0) // text
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // text
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _text_brush_IBO); // text
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color

        glEnable(GL_STENCIL_TEST); //开启模板测试
        glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT); //设置模板缓冲区更新方式(若通过则按位反转模板值)
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 1); //初始模板位为0，由于一定通过测试，所以全部会被置为1，而重复绘制区域由于画了两次模板位又归0
        glStencilMask(0x1); //开启模板缓冲区写入
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //第一次绘制只是为了构造模板缓冲区，没有必要显示到屏幕上，所以设置不显示第一遍的多边形
        glDrawElements(GL_TRIANGLES, _text_brush_count, GL_UNSIGNED_INT, NULL);

        glStencilFunc(GL_NOTEQUAL, 0, 1); //模板值不为0就通过
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0x1);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDrawElements(GL_TRIANGLES, _text_brush_count, GL_UNSIGNED_INT, NULL);
        glDisable(GL_STENCIL_TEST); //关闭模板测试
    }

    if (GlobalSetting::setting().show_points)
    {
        glUniform4f(_uniforms.color, 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        if (_point_count.polyline > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // polyline points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.polyline);
        }
        if (_point_count.polygon > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // polygon points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.polygon);
        }
        if (_point_count.circle > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.circle);
        }
        if (_point_count.curve > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.curve);
        }
    }

    if (CanvasOperations::CanvasOperation::shape_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.operation_shape); // operation shpae
        glBufferData(GL_ARRAY_BUFFER, CanvasOperations::CanvasOperation::shape_count * sizeof(double),
            CanvasOperations::CanvasOperation::shape, GL_STREAM_DRAW);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_STRIP, 0, CanvasOperations::CanvasOperation::shape_count / 3);
    }
    if (CanvasOperations::CanvasOperation::tool_lines_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.operation_tool_lines); // operation tool lines
        glBufferData(GL_ARRAY_BUFFER, CanvasOperations::CanvasOperation::tool_lines_count * sizeof(double),
            CanvasOperations::CanvasOperation::tool_lines, GL_STREAM_DRAW);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);
        
        glUniform4f(_uniforms.color, CanvasOperations::CanvasOperation::tool_line_color[0], CanvasOperations::CanvasOperation::tool_line_color[1],
            CanvasOperations::CanvasOperation::tool_line_color[2], CanvasOperations::CanvasOperation::tool_line_color[3]); // color
        glDrawArrays(GL_LINES, 0, CanvasOperations::CanvasOperation::tool_lines_count / 3);

        glUniform4f(_uniforms.color, 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glDrawArrays(GL_POINTS, 0, CanvasOperations::CanvasOperation::tool_lines_count / 3);
    }

    if (_bool_flags.show_catched_points) // catched point
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.catched_points); // catched point
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(double), _catchline_points);

        glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.649f); // color
        glLineWidth(2.8f);
        glDrawArrays(GL_LINES, 0, 8);
        glLineWidth(1.4f);
        glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.549f); // color
    }

    if (_bool_flags.show_origin || _select_rect[0] != _select_rect[6] || _select_rect[1] != _select_rect[7])
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        if (_bool_flags.show_origin) // origin
        {
            glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 画原点
            glDrawArrays(GL_LINES, 0, 4);
        }

        if (_select_rect[0] != _select_rect[6] || _select_rect[1] != _select_rect[7])
        {
            glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(double), 12 * sizeof(double), _select_rect);

            glUniform4f(_uniforms.color, 0.0f, 0.47f, 0.843f, 0.1f); // color
            glDrawArrays(GL_POLYGON, 4, 4);

            glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.549f); // color
            glDrawArrays(GL_LINE_LOOP, 4, 4);
        }
    }
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];

    if (Geo::Point coord; event->button() == Qt::MouseButton::LeftButton
        && catch_cursor(real_x1, real_y1, coord, _catch_distance, false))
    {
        real_x1 = coord.x, real_y1 = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
    }
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;
        CanvasOperations::CanvasOperation::press_pos[0] = real_x1, CanvasOperations::CanvasOperation::press_pos[1] = real_y1;
    }
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[
        CanvasOperations::CanvasOperation::tool[0]]; op != nullptr && op->mouse_press(event))
    {
        emit refresh_cmd_parameters_label();
        _info_labels[1]->setText(CanvasOperations::CanvasOperation::info);
        if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select)
        {
            emit tool_changed(CanvasOperations::Tool::Select);
        }
    }

    switch (event->button())
    {
    case Qt::LeftButton:
        break;
    case Qt::RightButton:
        cancel_painting();
        _editer->reset_selected_mark();
        refresh_selected_ibo();
        break;
    case Qt::MiddleButton:
        _bool_flags.view_movable = true; // view moveable
        break;
    default:
        break;
    }

    update();
    QOpenGLWidget::mousePressEvent(event);
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->position();
    double x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];

    if (Geo::Point coord; catch_cursor(x, y, coord, _catch_distance, false))
    {
        x = coord.x, y = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
    }
    CanvasOperations::CanvasOperation::real_pos[0] = x, CanvasOperations::CanvasOperation::real_pos[1] = y;
    CanvasOperations::CanvasOperation::release_pos[0] = x, CanvasOperations::CanvasOperation::release_pos[1] = y;
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[
        CanvasOperations::CanvasOperation::tool[0]]; op != nullptr && op->mouse_release(event))
    {
        emit refresh_cmd_parameters_label();
        _info_labels[1]->setText(CanvasOperations::CanvasOperation::info);
        if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select)
        {
            emit tool_changed(CanvasOperations::Tool::Select);
        }
    }

    switch (event->button())
    {
    case Qt::LeftButton:
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags.view_movable = false; // view moveable
        break;
    default:
        break;
    }

    update();
    QOpenGLWidget::mouseReleaseEvent(event);
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    const double center_x = size().width() / 2.0, center_y = size().height() / 2.0;
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double real_x0 = _mouse_pos_0.x() * _view_ctm[0] + _mouse_pos_0.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y0 = _mouse_pos_0.x() * _view_ctm[1] + _mouse_pos_0.y() * _view_ctm[4] + _view_ctm[7];
    double canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
    double canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
    const bool catched_point = _bool_flags.show_catched_points;
    if (Geo::Point coord; catch_cursor(real_x1, real_y1, coord,
        _catch_distance, event->buttons() & Qt::MouseButton::LeftButton))
    {
        real_x1 = coord.x, real_y1 = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
        canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
        canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
        update();
    }
    else
    {
        _bool_flags.show_catched_points = false;
        if (catched_point)
        {
            update();
        }
    }

    _info_labels[0]->setText(QString("X:%1 Y:%2").arg(real_x1, 0, 'f', 2).arg(real_y1, 0, 'f', 2));

    if (_bool_flags.view_movable) // 视图可移动
    {
        const double canvas_x0 = real_x0 * _canvas_ctm[0] + real_y0 * _canvas_ctm[3] + _canvas_ctm[6];
        const double canvas_y0 = real_x0 * _canvas_ctm[1] + real_y0 * _canvas_ctm[4] + _canvas_ctm[7];
        _canvas_ctm[6] += (canvas_x1 - canvas_x0), _canvas_ctm[7] += (canvas_y1 - canvas_y0);
        _view_ctm[6] -= (real_x1 - real_x0), _view_ctm[7] -= (real_y1 - real_y0);
        _visible_area.translate(real_x0 - real_x1, real_y0 - real_y1);
        makeCurrent();
        glUniform3d(_uniforms.vec0, _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
        glUniform3d(_uniforms.vec1, _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
        doneCurrent();
        update();
    }

    CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;
    CanvasOperations::CanvasOperation::real_pos[2] = real_x0, CanvasOperations::CanvasOperation::real_pos[3] = real_y0;
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[
        CanvasOperations::CanvasOperation::tool[0]]; op != nullptr && op->mouse_move(event))
    {
        _info_labels[1]->setText(CanvasOperations::CanvasOperation::info);
        if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select)
        {
            emit tool_changed(CanvasOperations::Tool::Select);
        }
        update();
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    const double real_x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double canvas_x = real_x * _canvas_ctm[0] + real_y * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y = real_x * _canvas_ctm[1] + real_y * _canvas_ctm[4] + _canvas_ctm[7];
    if (event->angleDelta().y() > 0 && _ratio < 1024)
    {
        _ratio *= 1.25;
        _canvas_ctm[0] *= 1.25;
        _canvas_ctm[4] *= 1.25;
        _canvas_ctm[6] = _canvas_ctm[6] * 1.25 - canvas_x * 0.25;
        _canvas_ctm[7] = _canvas_ctm[7] * 1.25 - canvas_y * 0.25;

        _view_ctm[0] *= 0.8;
        _view_ctm[4] *= 0.8;
        _view_ctm[6] = _view_ctm[6] * 0.8 + real_x * 0.2;
        _view_ctm[7] = _view_ctm[7] * 0.8 + real_y * 0.2;

        _visible_area.scale(real_x, real_y, 0.8);
        update();
    }
    else if (event->angleDelta().y() < 0 && _ratio > (1.0 / 1024.0))
    {
        _ratio *= 0.8;
        _canvas_ctm[0] *= 0.8;
        _canvas_ctm[4] *= 0.8;
        _canvas_ctm[6] = _canvas_ctm[6] * 0.8 + canvas_x * 0.2;
        _canvas_ctm[7] = _canvas_ctm[7] * 0.8 + canvas_y * 0.2;

        _view_ctm[0] *= 1.25;
        _view_ctm[4] *= 1.25;
        _view_ctm[6] = _view_ctm[6] * 1.25 - real_x * 0.25;
        _view_ctm[7] = _view_ctm[7] * 1.25 - real_y * 0.25;

        _visible_area.scale(real_x, real_y, 1.25);
        update();
    }
    {
        Geo::Point pos(real_x, real_y);
        refresh_catchline_points(_catched_objects, _catch_distance, pos);
    }
    makeCurrent();
    glUniform3d(_uniforms.vec0, _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
    glUniform3d(_uniforms.vec1, _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    double data[12] = {-10 / _ratio, 0, 0, 10 / _ratio, 0, 0, 0, -10 / _ratio, 0, 0, 10 / _ratio, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    doneCurrent();
    _editer->set_view_ratio(_ratio);
    CanvasOperations::CanvasOperation::view_ratio = _ratio;
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;

    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[
        CanvasOperations::CanvasOperation::tool[0]]; op != nullptr && op->mouse_double_click(event))
    {
        emit refresh_cmd_parameters_label();
        update();
        if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select)
        {
            emit tool_changed(CanvasOperations::Tool::Select);
        }
    }

    switch (event->button())
    {
    case Qt::LeftButton:
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        show_overview();
        update();
        break;
    default:
        break;
    }

    QOpenGLWidget::mouseDoubleClickEvent(event);
}

void Canvas::show_overview()
{
    _editer->set_view_ratio(1.0);
    _bool_flags.show_catched_points = false;

    Graph *graph = GlobalSetting::setting().graph;
    if (graph->empty())
    {
        _ratio = 1.0;

        _canvas_ctm[0] = 1;
        _canvas_ctm[4] = -1;
        _canvas_ctm[1] = _canvas_ctm[2] = _canvas_ctm[3] = _canvas_ctm[5] = 0;
        _canvas_ctm[8] = 1;
        _canvas_ctm[6] = 0;
        _canvas_ctm[7] = _canvas_height;

        _view_ctm[0] = 1;
        _view_ctm[4] = -1;
        _view_ctm[1] = _view_ctm[2] = _view_ctm[3] = _view_ctm[5] = 0;
        _view_ctm[8] = 1;
        _view_ctm[6] = 0;
        _view_ctm[7] = _canvas_height;

        // 可视区域为显示控件区域的反变换
        _visible_area = Geo::AABBRect(0, 0, _canvas_width, _canvas_height);
        update();
        return;
    }
    // 获取graph的边界
    Geo::AABBRect bounding_area = graph->bounding_rect();
    // 选择合适的缩放倍率
    double height_ratio = _canvas_height / bounding_area.height();
    double width_ratio = _canvas_width / bounding_area.width();
    _ratio = std::min(height_ratio, width_ratio);
    // 缩放减少2%，使其与边界留出一些空间
    _ratio = std::isinf(_ratio) ? 1 : _ratio * 0.98;

    _editer->set_view_ratio(_ratio);
    CanvasOperations::CanvasOperation::view_ratio = _ratio;

    // 置于控件中间
    double x_offset = (_canvas_width - bounding_area.width() * _ratio) / 2 - bounding_area.left() * _ratio;
    double y_offset = (bounding_area.height() * _ratio - _canvas_height) / 2 + bounding_area.bottom() * _ratio + _canvas_height;

    _canvas_ctm[0] = _ratio;
    _canvas_ctm[4] = -_ratio;
    _canvas_ctm[1] = _canvas_ctm[2] = _canvas_ctm[3] = _canvas_ctm[5] = 0;
    _canvas_ctm[8] = 1;
    _canvas_ctm[6] = x_offset;
    _canvas_ctm[7] = y_offset;

    _view_ctm[0] = 1 / _ratio;
    _view_ctm[4] = -1 / _ratio;
    _view_ctm[1] = _view_ctm[2] = _view_ctm[3] = _view_ctm[5] = 0;
    _view_ctm[8] = 1;
    _view_ctm[6] = -x_offset / _ratio;
    _view_ctm[7] = y_offset / _ratio;

    // 可视区域为显示控件区域的反变换
    double x0 = 0, y0 = 0, x1 = _canvas_width, y1 = _canvas_height;
    _visible_area = Geo::AABBRect(
        x0 * _view_ctm[0] + y0 * _view_ctm[3] + _view_ctm[6],
        x0 * _view_ctm[1] + y0 * _view_ctm[4] + _view_ctm[7],
        x1 * _view_ctm[0] + y1 * _view_ctm[3] + _view_ctm[6],
        x1 * _view_ctm[1] + y1 * _view_ctm[4] + _view_ctm[7]);

    makeCurrent();
    glUniform3d(_uniforms.vec0, _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
    glUniform3d(_uniforms.vec1, _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    {
        double data[12] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
        glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    }
    doneCurrent();
}




void Canvas::use_tool(const CanvasOperations::Tool tool)
{
    CanvasOperations::CanvasOperation::operation().clear();
    CanvasOperations::CanvasOperation::tool[1] = CanvasOperations::CanvasOperation::tool[0] = tool;
    _info_labels[1]->clear();
    emit tool_changed(CanvasOperations::CanvasOperation::tool[0]);
    update();
}

void Canvas::show_origin()
{
    _bool_flags.show_origin = true;
}

void Canvas::hide_origin()
{
    _bool_flags.show_origin = false;
}

const bool Canvas::is_typing() const
{
    return _input_line.isVisible();
}

void Canvas::set_catch_distance(const double value)
{
    _catch_distance = value;
}

void Canvas::set_cursor_catch(const CatchedPointType type, const bool value)
{
    switch (type)
    {
    case CatchedPointType::Vertex:
        _catch_types.vertex = value;
        break;
    case CatchedPointType::Center:
        _catch_types.center = value;
        break;
    case CatchedPointType::Foot:
        _catch_types.foot = value;
        break;
    case CatchedPointType::Tangency:
        _catch_types.tangency = value;
        break;
    case CatchedPointType::Intersection:
        _catch_types.intersection = value;
        break;
    default:
        break;
    }
}

const size_t Canvas::current_group() const
{
    return _editer->current_group();
}

void Canvas::set_current_group(const size_t index)
{
    assert(index < GlobalSetting::setting().graph->container_groups().size());
    _editer->set_current_group(index);
}

const size_t Canvas::groups_count() const
{
    return _editer->groups_count();
}


Geo::Point Canvas::center() const
{
    if (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty())
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
    if (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty())
    {
        return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
    }

    for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.x);
            y0 = std::min(y0, point.y);
            x1 = std::max(x1, point.x);
            y1 = std::max(y1, point.y);
        }
    }

    return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
}

Geo::AABBRect Canvas::bounding_rect() const
{
    if (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty())
    {
        return Geo::AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
    if (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty())
    {
        return Geo::AABBRect(x0, y0, x1, y1);
    }

    for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.x);
            y0 = std::min(y0, point.y);
            x1 = std::max(x1, point.x);
            y1 = std::max(y1, point.y);
        }
    }

    return Geo::AABBRect(x0, y0, x1, y1);
}

Geo::Point Canvas::mouse_position(const bool to_real_coord) const
{
    if (to_real_coord)
    {
        return Geo::Point(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
            _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    }
    else
    {
        return Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y());
    }
}

const bool Canvas::empty() const
{
    return GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty();
}

void Canvas::cancel_painting()
{
    _editer->point_cache().clear();
    CanvasOperations::CanvasOperation::operation().clear();
    _info_labels[1]->clear();
    emit tool_changed(CanvasOperations::CanvasOperation::tool[0]);
    update();
}

void Canvas::set_info_labels(QLabel **labels)
{
    _info_labels = labels;
}

void Canvas::add_geometry(Geo::Geometry *object)
{
    _editer->append(object);
    refresh_vbo(object->type(), true);
    update();
}

void Canvas::show_menu(Geo::Geometry *object)
{
    refresh_selected_ibo(object);
    _text_to_polylines->setVisible(dynamic_cast<Text *>(object) != nullptr);
    _bezier_to_bspline->setVisible(dynamic_cast<Geo::Bezier *>(object) != nullptr);
    _bspline_to_bezier->setVisible(dynamic_cast<Geo::BSpline *>(object) != nullptr);
    if (const Geo::BSpline *bspline = dynamic_cast<const Geo::BSpline *>(object))
    {
        _change_bspline_model->setVisible(true);
        _change_bspline_model->setText(bspline->controls_model ? "Show Path" : "Show Controls");
    }
    else
    {
        _change_bspline_model->setVisible(false);
    }
    if (const QAction *a = _menu->exec(QCursor::pos()); a == _up)
    {
        _editer->up(object);
        refresh_vbo(object->type(), true);
        refresh_selected_ibo();
    }
    else if (a == _down)
    {
        _editer->down(object);
        refresh_vbo(object->type(), true);
        refresh_selected_ibo();
    }
    else if (a == _text_to_polylines)
    {
        _editer->text_to_polylines(dynamic_cast<Text *>(object));
        refresh_vbo({ Geo::Type::TEXT, Geo::Type::POLYLINE }, true);
        refresh_selected_ibo();
    }
    else if (a == _bezier_to_bspline)
    {
        _editer->bezier_to_bspline(dynamic_cast<Geo::Bezier *>(object));
        CanvasOperations::CanvasOperation::tool_lines_count = 0;
        refresh_vbo({ Geo::Type::BEZIER, Geo::Type::BSPLINE }, true);
        refresh_selected_ibo();
    }
    else if (a == _change_bspline_model)
    {
        static_cast<Geo::BSpline *>(object)->controls_model =
            !static_cast<Geo::BSpline *>(object)->controls_model;
        if (object == CanvasOperations::CanvasOperation::clicked_object)
        {
            CanvasOperations::CanvasOperation::refresh_tool_lines(object);
        }
    }
    else if (a == _bspline_to_bezier)
    {
        _editer->bspline_to_bezier(dynamic_cast<Geo::BSpline *>(object));
        CanvasOperations::CanvasOperation::tool_lines_count = 0;
        refresh_vbo({ Geo::Type::BEZIER, Geo::Type::BSPLINE }, true);
        refresh_selected_ibo();
    }
}

void Canvas::show_text_edit(Text *text)
{
    _edited_text = text;
    Geo::AABBRect rect(text->bounding_rect());
    rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
    _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
    _input_line.move(rect.center().x - _input_line.rect().center().x(),
        rect.center().y - _input_line.rect().center().y());
    _input_line.setFocus();
    _input_line.setText(text->text());
    _input_line.moveCursor(QTextCursor::End);
    _input_line.show();
}

void Canvas::hide_text_edit()
{
    if (_input_line.isVisible() && _edited_text != nullptr)
    {
        if (const QString text = _edited_text->text(); text != _input_line.toPlainText())
        {
            _edited_text->set_text(_input_line.toPlainText(), GlobalSetting::setting().text_size);
            _editer->push_backup_command(new UndoStack::TextChangedCommand(_edited_text, text));
            refresh_vbo(Geo::Type::TEXT, false);
            update();
        }
        _edited_text = nullptr;
        _input_line.clear();
        _input_line.hide();
    }
}

void Canvas::copy()
{
    _points_cache.clear();
    _points_cache.emplace_back(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
        _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    _editer->copy_selected();
}

void Canvas::cut()
{
    _points_cache.clear();
    _points_cache.emplace_back(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
        _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    _editer->cut_selected();
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : _editer->paste_table())
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
    refresh_vbo(types, true);
}

void Canvas::paste()
{
    const double x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    if (!_points_cache.empty() && _editer->paste(x - _points_cache.back().x, y - _points_cache.back().y))
    {
        std::set<Geo::Type> types;
        for (const Geo::Geometry *object : _editer->paste_table())
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
        refresh_vbo(types, true);
        refresh_selected_ibo();
        update();
    }
}

void Canvas::paste(const double x, const double y)
{
    if (!_points_cache.empty() && _editer->paste(x - _points_cache.back().x, y - _points_cache.back().y))
    {
        std::set<Geo::Type> types;
        for (const Geo::Geometry *object : _editer->paste_table())
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
        refresh_vbo(types, true);
        refresh_selected_ibo();
        update();
    }
    _points_cache.clear();
}


Geo::Point Canvas::real_coord_to_view_coord(const Geo::Point &input) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (input.x - _view_ctm[6]) - _view_ctm[3] * (input.y - _view_ctm[7])) / k,
        (_view_ctm[0] * (input.y - _view_ctm[7]) - _view_ctm[1] * (input.x - _view_ctm[6])) / k};
}

Geo::Point Canvas::real_coord_to_view_coord(const double x, const double y) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (x - _view_ctm[6]) - _view_ctm[3] * (y - _view_ctm[7])) / k,
        (_view_ctm[0] * (y - _view_ctm[7]) - _view_ctm[1] * (x - _view_ctm[6])) / k};
}

Geo::Point Canvas::canvas_coord_to_real_coord(const Geo::Point &input) const
{
    const double t = (input.y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (input.x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(input.x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}

Geo::Point Canvas::canvas_coord_to_real_coord(const double x, const double y) const
{
    const double t = (y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}

bool Canvas::catch_cursor(const double x, const double y, Geo::Point &coord, const double distance, const bool skip_selected)
{
    _catched_objects.clear();
    refresh_catached_points(x, y, distance, _catched_objects, skip_selected);
    Geo::Point pos(x, y);
    if (refresh_catchline_points(_catched_objects, distance, pos))
    {
        coord = pos;
        _bool_flags.show_catched_points = true;
    }
    else
    {
        _bool_flags.show_catched_points = false;
    }
    return _bool_flags.show_catched_points;
}

bool Canvas::catch_point(const double x, const double y, Geo::Point &coord, const double distance)
{
    _catched_objects.clear();
    if (refresh_catached_points(x, y, distance, _catched_objects, false))
    {
        Geo::Point pos(x, y);
        if (refresh_catchline_points(_catched_objects, distance, pos))
        {
            coord.x = pos.x;
            coord.y = pos.y;
            _bool_flags.show_catched_points = true;
        }
        else
        {
            _bool_flags.show_catched_points = false;
        }
        return _bool_flags.show_catched_points;
    }
    else
    {
        _bool_flags.show_catched_points = false;
        return false;
    }
}


void Canvas::refresh_vbo(const bool refresh_ibo)
{
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>>
        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this),
        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this),
        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this),
        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
    std::future<std::tuple<double*, unsigned int>> point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this);
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>> text_vbo;
    if (GlobalSetting::setting().show_text)
    {
        text_vbo = std::async(std::launch::async,
            static_cast<std::tuple<double*, unsigned int, unsigned int*, unsigned int>(Canvas::*)(void)>(&Canvas::refresh_text_vbo), this);
    }
    else
    {
        _text_brush_count = 0;
    }
    std::future<std::tuple<double*, unsigned int>> circle_printable_points, curve_printable_points;
    if (GlobalSetting::setting().show_points)
    {
        circle_printable_points = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
        curve_printable_points = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
    }

    makeCurrent();
    if (GlobalSetting::setting().show_points)
    {
        circle_printable_points.wait();
        auto [circle_data, circle_data_count] = circle_printable_points.get();
        if (circle_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
        }
        delete []circle_data;

        curve_printable_points.wait();
        auto [curve_data, curve_data_count] = curve_printable_points.get();
        if (curve_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
        }
        delete []curve_data;
    }

    circle_vbo.wait();
    auto [circle_data, circle_data_count, circle_indexs, circle_index_count] = circle_vbo.get();
    if (circle_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * circle_index_count, circle_indexs, GL_DYNAMIC_DRAW);
    }
    delete []circle_data;
    delete []circle_indexs;

    curve_vbo.wait();
    auto [curve_data, curve_data_count, curve_indexs, curve_index_count] = curve_vbo.get();
    if (curve_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * curve_index_count, curve_indexs, GL_DYNAMIC_DRAW);
    }
    delete []curve_data;
    delete []curve_indexs;

    polyline_vbo.wait();
    auto [polyline_data, polyline_data_count, polyline_indexs, polyline_index_count] = polyline_vbo.get();
    if (polyline_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polyline_data_count, polyline_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyline_index_count, polyline_indexs, GL_DYNAMIC_DRAW);
    }
    delete []polyline_data;
    delete []polyline_indexs;

    polygon_vbo.wait();
    auto [polygon_data, polygon_data_count, polygon_indexs, polygon_index_count] = polygon_vbo.get();
    if (polygon_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polygon_data_count, polygon_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    }
    delete []polygon_data;
    delete []polygon_indexs;

    point_vbo.wait();
    auto [point_data, point_data_count] = point_vbo.get();
    if (point_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * point_data_count, point_data, GL_DYNAMIC_DRAW);
    }
    delete []point_data;

    if (GlobalSetting::setting().show_text)
    {
        text_vbo.wait();
        auto [text_data, text_data_count, text_indexs, text_index_count] = text_vbo.get();
        if (text_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // text
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * text_data_count, text_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _text_brush_IBO); // text
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * text_index_count, text_indexs, GL_DYNAMIC_DRAW);
        }
        delete []text_data;
        delete []text_indexs;
    }
    doneCurrent();
}

void Canvas::refresh_vbo(const Geo::Type type, const bool refresh_ibo)
{
    switch (type)
    {
    case Geo::Type::POLYLINE:
        {
            auto [data, data_count, indexs, index_count] = refresh_polyline_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
        }
        break;
    case Geo::Type::POLYGON:
        if (refresh_ibo)
        {
            auto [data, data_count, indexs, index_count] = refresh_polygon_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
        }
        else
        {
            auto [data, data_count, indexs, index_count] = refresh_polygon_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
        }
        break;
    case Geo::Type::CIRCLE:
    case Geo::Type::ELLIPSE:
    case Geo::Type::ARC:
        if (refresh_ibo)
        {
            std::future<std::tuple<double*, unsigned int>> point;
            if (GlobalSetting::setting().show_points)
            {
                point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
            }
            auto [data, data_count, indexs, index_count] = refresh_circle_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
            if (GlobalSetting::setting().show_points)
            {
                point.wait();
                auto [point_data, point_data_count] = point.get();
                if (point_data_count > 0)
                {
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
                    glBufferData(GL_ARRAY_BUFFER, sizeof(double) * point_data_count, point_data, GL_DYNAMIC_DRAW);
                    doneCurrent();
                }
                delete []point_data;
            }
        }
        else
        {
            std::future<std::tuple<double*, unsigned int>> point;
            if (GlobalSetting::setting().show_points)
            {
                point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
            }
            auto [data, data_count, indexs, index_count] = refresh_circle_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
            if (GlobalSetting::setting().show_points)
            {
                point.wait();
                auto [point_data, point_data_count] = point.get();
                if (point_data_count > 0)
                {
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
                    glBufferData(GL_ARRAY_BUFFER, sizeof(double) * point_data_count, point_data, GL_DYNAMIC_DRAW);
                    doneCurrent();
                }
                delete []point_data;
            }
        }
        break;
    case Geo::Type::BEZIER:
    case Geo::Type::BSPLINE:
        {
            std::future<std::tuple<double*, unsigned int>> point;
            if (GlobalSetting::setting().show_points)
            {
                point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
            }
            auto [data, data_count, indexs, index_count] = refresh_curve_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
            if (GlobalSetting::setting().show_points)
            {
                point.wait();
                auto [point_data, point_data_count] = point.get();
                if (point_data_count > 0)
                {
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
                    glBufferData(GL_ARRAY_BUFFER, sizeof(double) * point_data_count, point_data, GL_DYNAMIC_DRAW);
                    doneCurrent();
                }
                delete []point_data;
            }
        }
        break;
    case Geo::Type::TEXT:
        if (GlobalSetting::setting().show_text)
        {
            auto [data, data_count, indexs, index_count] = refresh_text_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // text
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _text_brush_IBO); // text
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
        }
        else
        {
            _text_brush_count = 0;
        }
        break;
    case Geo::Type::POINT:
        {
            auto [data, data_count] = refresh_point_vbo();
            if (data_count > 0)
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // point
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
        }
        break;
    default:
        refresh_vbo(refresh_ibo);
        break;
    }
}

void Canvas::refresh_vbo(const std::set<Geo::Type> &types, const bool refresh_ibo)
{
    if (types.find(Geo::Type::COMBINATION) != types.end())
    {
        return refresh_vbo(refresh_ibo);
    }

    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>>
        polyline_vbo, polygon_vbo, circle_vbo, curve_vbo;
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>> text_vbo;
    std::future<std::tuple<double*, unsigned int>> circle_printable_points, curve_printable_points, point_vbo;

    if (types.find(Geo::Type::POLYLINE) != types.end())
    {
        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this);
    }
    if (types.find(Geo::Type::POLYGON) != types.end())
    {
        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this);
    }
    if (GlobalSetting::setting().show_text && types.find(Geo::Type::TEXT) != types.end())
    {
        text_vbo = std::async(std::launch::async,
            static_cast<std::tuple<double*, unsigned int, unsigned int*, unsigned int>(Canvas::*)(void)>(&Canvas::refresh_text_vbo), this);
    }
    else
    {
        _text_brush_count = 0;
    }
    if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()
        || types.find(Geo::Type::ARC) != types.end())
    {
        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this);
    }
    if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
    {
        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
    }
    if (types.find(Geo::Type::POINT) != types.end())
    {
        point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this);
    }
    if (GlobalSetting::setting().show_points)
    {
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()
            || types.find(Geo::Type::ARC) != types.end())
        {
            circle_printable_points = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
        }
        if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
        {
            curve_printable_points = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
        }
    }

    makeCurrent();
    if (GlobalSetting::setting().show_points)
    {
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()
            || types.find(Geo::Type::ARC) != types.end())
        {
            circle_printable_points.wait();
            auto [circle_data, circle_data_count] = circle_printable_points.get();
            if (circle_data_count > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
            }
            delete []circle_data;
        }
        if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
        {
            curve_printable_points.wait();
            auto [curve_data, curve_data_count] = curve_printable_points.get();
            if (curve_data_count > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
            }
            delete []curve_data;
        }
    }

    if (circle_vbo.valid())
    {
        circle_vbo.wait();
        auto [circle_data, circle_data_count, circle_indexs, circle_index_count] = circle_vbo.get();
        if (circle_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * circle_index_count, circle_indexs, GL_DYNAMIC_DRAW);
        }
        delete []circle_data;
        delete []circle_indexs;
    }

    if (curve_vbo.valid())
    {
        curve_vbo.wait();
        auto [curve_data, curve_data_count, curve_indexs, curve_index_count] = curve_vbo.get();
        if (curve_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * curve_index_count, curve_indexs, GL_DYNAMIC_DRAW);
        }
        delete []curve_data;
        delete []curve_indexs;
    }

    if (polyline_vbo.valid())
    {
        polyline_vbo.wait();
        auto [polyline_data, polyline_data_count, polyline_indexs, polyline_index_count] = polyline_vbo.get();
        if (polyline_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polyline_data_count, polyline_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyline_index_count, polyline_indexs, GL_DYNAMIC_DRAW);
        }
        delete []polyline_data;
        delete []polyline_indexs;
    }

    if (polygon_vbo.valid())
    {
        polygon_vbo.wait();
        auto [polygon_data, polygon_data_count, polygon_indexs, polygon_index_count] = polygon_vbo.get();
        if (polygon_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polygon_data_count, polygon_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
        }
        delete []polygon_data;
        delete []polygon_indexs;
    }

    if (point_vbo.valid())
    {
        point_vbo.wait();
        auto [point_data, point_data_count] = point_vbo.get();
        if (point_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * point_data_count, point_data, GL_DYNAMIC_DRAW);
        }
        delete []point_data;
    }

    if (text_vbo.valid())
    {
        text_vbo.wait();
        auto [text_data, text_data_count, text_indexs, text_index_count] = text_vbo.get();
        if (text_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // text
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * text_data_count, text_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _text_brush_IBO); // text
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * text_index_count, text_indexs, GL_DYNAMIC_DRAW);
        }
        delete []text_data;
        delete []text_indexs;
    }

    doneCurrent();
}

std::tuple<double*, unsigned int, unsigned int*, unsigned int> Canvas::refresh_polyline_vbo()
{
    unsigned int data_len = 1026, data_count = 0;
    unsigned int index_len = 512, index_count = 0;
    double *data = new double[data_len];
    unsigned int *indexs = new unsigned int[index_len];

    Geo::Polyline *polyline = nullptr;
    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    if (item->type() == Geo::Type::POLYLINE)
                    {
                        polyline = static_cast<Geo::Polyline *>(item);
                        polyline->point_index = data_count / 3;
                        while (data_count + polyline->size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        if (index_count + polyline->size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : *polyline)
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        polyline->point_count = polyline->size();
                    }
                }
                break;
            case Geo::Type::POLYLINE:
                polyline = static_cast<Geo::Polyline *>(geo);
                polyline->point_index = data_count / 3;
                while (data_count + polyline->size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + polyline->size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : *polyline)
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                polyline->point_count = polyline->size();
                break;
            default:
                break;
            }
        }
    }

    _point_count.polyline = data_count / 3;
    _shape_index_count.polyline = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}

std::tuple<double*, unsigned int, unsigned int*, unsigned int> Canvas::refresh_polygon_vbo()
{
    unsigned int data_len = 1026, data_count = 0;
    unsigned int index_len = 512, index_count = 0;
    double *data = new double[data_len];
    unsigned int *indexs = new unsigned int[index_len];

    Geo::Polygon *polygon = nullptr;
    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    if (item->type() == Geo::Type::POLYGON)
                    {
                        polygon = static_cast<Geo::Polygon *>(item);
                        polygon->point_index = data_count / 3;
                        while (data_count + polygon->size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + polygon->size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : *polygon)
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        polygon->point_count = polygon->size();
                    }
                }
                break;
            case Geo::Type::POLYGON:
                polygon = static_cast<Geo::Polygon *>(geo);
                polygon->point_index = data_count / 3;
                while (data_count + polygon->size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + polygon->size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : *polygon)
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                polygon->point_count = polygon->size();
                break;
            default:
                break;
            }
        }
    }

    _point_count.polygon = data_count / 3;
    _shape_index_count.polygon = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}

std::tuple<double*, unsigned int, unsigned int*, unsigned int> Canvas::refresh_circle_vbo()
{
    unsigned int data_len = 1026, data_count = 0;
    unsigned int index_len = 512, index_count = 0;
    double *data = new double[data_len];
    unsigned int *indexs = new unsigned int[index_len];

    Geo::Circle *circle = nullptr;
    Geo::Ellipse *ellipse = nullptr;
    Geo::Arc *arc = nullptr;
    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::CIRCLE:
                        circle = static_cast<Geo::Circle *>(item);
                        circle->point_index = data_count / 3;
                        while (data_count + circle->shape().size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + circle->shape().size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : circle->shape())
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        circle->point_count = circle->shape().size();
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<Geo::Ellipse *>(item);
                        ellipse->point_index = data_count / 3;
                        while (data_count + ellipse->shape().size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + ellipse->shape().size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : ellipse->shape())
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        ellipse->point_count = ellipse->shape().size();
                        break;
                    case Geo::Type::ARC:
                        arc = static_cast<Geo::Arc *>(item);
                        arc->point_index = data_count / 3;
                        while (data_count + arc->shape().size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + arc->shape().size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : arc->shape())
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        arc->point_count = arc->shape().size();
                        break;
                    default:
                        break;
                    }
                }
                break;
            case Geo::Type::CIRCLE:
                circle = static_cast<Geo::Circle *>(geo);
                circle->point_index = data_count / 3;
                while (data_count + circle->shape().size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + circle->shape().size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : circle->shape())
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                circle->point_count = circle->shape().size();
                break;
            case Geo::Type::ELLIPSE:
                ellipse = static_cast<Geo::Ellipse *>(geo);
                ellipse->point_index = data_count / 3;
                while (data_count + ellipse->shape().size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + ellipse->shape().size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : ellipse->shape())
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                ellipse->point_count = ellipse->shape().size();
                break;
            case Geo::Type::ARC:
                arc = static_cast<Geo::Arc *>(geo);
                arc->point_index = data_count / 3;
                while (data_count + arc->shape().size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + arc->shape().size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : arc->shape())
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                arc->point_count = arc->shape().size();
                break;
            default:
                break;
            }
        }
    }

    _point_count.circle = data_count / 3;
    _shape_index_count.circle = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}

std::tuple<double*, unsigned int, unsigned int*, unsigned int> Canvas::refresh_curve_vbo()
{
    unsigned int data_len = 1026, data_count = 0;
    unsigned int index_len = 512, index_count = 0;
    double *data = new double[data_len];
    unsigned int *indexs = new unsigned int[index_len];

    Geo::Bezier *bezier = nullptr;
    Geo::BSpline *bspline = nullptr;
    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::BEZIER:
                        bezier = static_cast<Geo::Bezier *>(item);
                        bezier->point_index = data_count / 3;
                        while (data_count + bezier->shape().size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + bezier->shape().size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : bezier->shape())
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                        indexs[index_count++] = UINT_MAX;
                        bezier->point_count = bezier->shape().size();
                        break;
                    case Geo::Type::BSPLINE:
                        bspline = static_cast<Geo::BSpline *>(item);
                        bspline->point_index = data_count / 3;
                        while (data_count + bspline->shape().size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        while (index_count + bspline->shape().size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const Geo::Point &point : bspline->shape())
                        {
                            indexs[index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;           
                        }
                        indexs[index_count++] = UINT_MAX;
                        bspline->point_count = bspline->shape().size();
                        break;
                    default:
                        break;
                    }
                }
                break;
            case Geo::Type::BEZIER:
                bezier = static_cast<Geo::Bezier *>(geo);
                bezier->point_index = data_count / 3;
                while (data_count + bezier->shape().size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + bezier->shape().size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : bezier->shape())
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;                  
                }
                indexs[index_count++] = UINT_MAX;
                bezier->point_count = bezier->shape().size();
                break;
            case Geo::Type::BSPLINE:
                bspline = static_cast<Geo::BSpline *>(geo);
                bspline->point_index = data_count / 3;
                while (data_count + bspline->shape().size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                while (index_count + bspline->shape().size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const Geo::Point &point : bspline->shape())
                {
                    indexs[index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                indexs[index_count++] = UINT_MAX;
                bspline->point_count = bspline->shape().size();
                break;
            default:
                break;
            }
        }
    }

    _point_count.curve = data_count / 3;
    _shape_index_count.curve = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}

std::tuple<double*, unsigned int> Canvas::refresh_point_vbo()
{
    unsigned int data_len = 513, data_count = 0, index = 0;
    double *data = new double[data_len];

    for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            if (geo->type() == Geo::Type::POINT)
            {
                Geo::Point *point = static_cast<Geo::Point *>(geo);
                point->point_index = index++;
                point->point_count = 1;
                data[data_count++] = point->x;
                data[data_count++] = point->y;
                data[data_count++] = 0;
                if (data_count == data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_len, temp);
                    delete []data;
                    data = temp;
                }
            }
        }
    }

    _point_count.point = data_count / 3;
    return std::make_tuple(data, data_count);
}

std::tuple<double *, unsigned int> Canvas::refresh_circle_printable_points()
{
    unsigned int data_len = 1200, data_count = 0;
    double *data = new double[data_len];
    const Geo::Circle *circle = nullptr;
    const Geo::Ellipse *ellipse = nullptr;

    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            if (data_count + 15 > data_len)
            {
                data_len *= 2;
                double *temp = new double[data_len];
                std::move(data, data + data_count, temp);
                delete []data;
                data = temp;
            }
            switch (geo->type())
            {
            case Geo::Type::CIRCLE:
                circle = static_cast<const Geo::Circle *>(geo);
                data[data_count++] = circle->x;
                data[data_count++] = circle->y;
                data[data_count++] = 0.5;
                data[data_count++] = circle->x - circle->radius;
                data[data_count++] = circle->y;
                data[data_count++] = 0.5;
                data[data_count++] = circle->x;
                data[data_count++] = circle->y + circle->radius;
                data[data_count++] = 0.5;
                data[data_count++] = circle->x + circle->radius;
                data[data_count++] = circle->y;
                data[data_count++] = 0.5;
                data[data_count++] = circle->x;
                data[data_count++] = circle->y - circle->radius;
                data[data_count++] = 0.5;
                break;
            case Geo::Type::ELLIPSE:
                ellipse = static_cast<const Geo::Ellipse *>(geo);
                if (ellipse->is_arc())
                {
                    const Geo::Point point0(ellipse->arc_point0());
                    data[data_count++] = point0.x;
                    data[data_count++] = point0.y;
                    data[data_count++] = 0.5;
                    const Geo::Point point1(ellipse->arc_point1());
                    data[data_count++] = point1.x;
                    data[data_count++] = point1.y;
                    data[data_count++] = 0.5;
                }
                else
                {
                    data[data_count++] = (ellipse->a0().x + ellipse->a1().x + ellipse->b0().x + ellipse->b1().x) / 4;
                    data[data_count++] = (ellipse->a0().y + ellipse->a1().y + ellipse->b0().y + ellipse->b1().y) / 4;
                    data[data_count++] = 0.5;
                    data[data_count++] = ellipse->a0().x;
                    data[data_count++] = ellipse->a0().y;
                    data[data_count++] = 0.5;
                    data[data_count++] = ellipse->a1().x;
                    data[data_count++] = ellipse->a1().y;
                    data[data_count++] = 0.5;
                    data[data_count++] = ellipse->b0().x;
                    data[data_count++] = ellipse->b0().y;
                    data[data_count++] = 0.5;
                    data[data_count++] = ellipse->b1().x;
                    data[data_count++] = ellipse->b1().y;
                    data[data_count++] = 0.5;
                }
                break;
            case Geo::Type::ARC:
                for (const Geo::Point &point : static_cast<const Geo::Arc *>(geo)->control_points)
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    if (data_count + 15 > data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::move(data, data + data_count, temp);
                        delete []data;
                        data = temp;
                    }
                    switch (item->type())
                    {
                    case Geo::Type::CIRCLE:
                        circle = static_cast<const Geo::Circle *>(item);
                        data[data_count++] = circle->x;
                        data[data_count++] = circle->y;
                        data[data_count++] = 0.5;
                        data[data_count++] = circle->x - circle->radius;
                        data[data_count++] = circle->y;
                        data[data_count++] = 0.5;
                        data[data_count++] = circle->x;
                        data[data_count++] = circle->y + circle->radius;
                        data[data_count++] = 0.5;
                        data[data_count++] = circle->x + circle->radius;
                        data[data_count++] = circle->y;
                        data[data_count++] = 0.5;
                        data[data_count++] = circle->x;
                        data[data_count++] = circle->y - circle->radius;
                        data[data_count++] = 0.5;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<const Geo::Ellipse *>(item);
                        data[data_count++] = (ellipse->a0().x + ellipse->a1().x + ellipse->b0().x + ellipse->b1().x) / 4;
                        data[data_count++] = (ellipse->a0().y + ellipse->a1().y + ellipse->b0().y + ellipse->b1().y) / 4;
                        data[data_count++] = 0.5;
                        data[data_count++] = ellipse->a0().x;
                        data[data_count++] = ellipse->a0().y;
                        data[data_count++] = 0.5;
                        data[data_count++] = ellipse->a1().x;
                        data[data_count++] = ellipse->a1().y;
                        data[data_count++] = 0.5;
                        data[data_count++] = ellipse->b0().x;
                        data[data_count++] = ellipse->b0().y;
                        data[data_count++] = 0.5;
                        data[data_count++] = ellipse->b1().x;
                        data[data_count++] = ellipse->b1().y;
                        data[data_count++] = 0.5;
                        break;
                    case Geo::Type::ARC:
                        for (const Geo::Point &point : static_cast<const Geo::Arc *>(item)->control_points)
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
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

    _point_count.circle = data_count / 3;
    return std::make_tuple(data, data_count);
}

std::tuple<double *, unsigned int> Canvas::refresh_curve_printable_points()
{
    unsigned int data_len = 1026, data_count = 0;
    double *data = new double[data_len];

    for (ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    if (const Geo::BSpline *bspline = dynamic_cast<const Geo::BSpline *>(item))
                    {
                        while (data_count + bspline->path_points.size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        for (const Geo::Point &point : bspline->path_points)
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                        }
                    }
                }
                break;
            case Geo::Type::BSPLINE:
                while (data_count + static_cast<const Geo::BSpline *>(geo)->path_points.size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                for (const Geo::Point &point : static_cast<const Geo::BSpline *>(geo)->path_points)
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                }
                break;
            default:
                break;
            }
        }
    }

    _point_count.curve = data_count / 3;
    return std::make_tuple(data, data_count);
}


void Canvas::refresh_select_rect(const double x0, const double y0, const double x1, const double y1)
{
    _select_rect[0] = x0, _select_rect[1] = y0;
    _select_rect[3] = x1, _select_rect[4] = y0;
    _select_rect[6] = x1, _select_rect[7] = y1;
    _select_rect[9] = x0, _select_rect[10] = y1;
}

void Canvas::refresh_selected_ibo()
{
    unsigned int polyline_index_len = 512, polyline_index_count = 0;
    unsigned int polygon_index_len = 512, polygon_index_count = 0;
    unsigned int circle_index_len = 512, circle_index_count = 0;
    unsigned int curve_index_len = 512, curve_index_count = 0;
    unsigned int point_index_len = 256, point_index_count = 0;
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
    unsigned int *circle_indexs = new unsigned int[circle_index_len];
    unsigned int *curve_indexs = new unsigned int[curve_index_len];
    unsigned int *point_indexs = new unsigned int[point_index_len];
    for (const Geo::Geometry *geo : _editer->selected())
    {
        switch (geo->type())
        {
        case Geo::Type::POLYLINE:
            while (polyline_index_count + geo->point_count >= polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                delete []polyline_indexs;
                polyline_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polyline_indexs[polyline_index_count++] = index++;
            }
            polyline_indexs[polyline_index_count++] = UINT_MAX;
            if (polyline_index_count == polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                delete []polyline_indexs;
                polyline_indexs = temp;
            }
            break;
        case Geo::Type::POLYGON:
            while (polygon_index_count + geo->point_count >= polygon_index_len)
            {
                polygon_index_len *= 2;
                unsigned int *temp = new unsigned int[polygon_index_len];
                std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                delete []polygon_indexs;
                polygon_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polygon_indexs[polygon_index_count++] = index++;
            }
            polygon_indexs[polygon_index_count++] = UINT_MAX;
            if (polygon_index_count == polygon_index_len)
            {
                polygon_index_len *= 2;
                unsigned int *temp = new unsigned int[polygon_index_len];
                std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                delete []polygon_indexs;
                polygon_indexs = temp;
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            while (circle_index_count + geo->point_count >= circle_index_len)
            {
                circle_index_len *= 2;
                unsigned int *temp = new unsigned int[circle_index_len];
                std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                delete []circle_indexs;
                circle_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                circle_indexs[circle_index_count++] = index++;
            }
            circle_indexs[circle_index_count++] = UINT_MAX;
            if (circle_index_count == circle_index_len)
            {
                circle_index_len *= 2;
                unsigned int *temp = new unsigned int[circle_index_len];
                std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                delete []circle_indexs;
                circle_indexs = temp;
            }
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            while (curve_index_count + geo->point_count >= curve_index_len)
            {
                curve_index_len *= 2;
                unsigned int *temp = new unsigned int[curve_index_len];
                std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                delete []curve_indexs;
                curve_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                curve_indexs[curve_index_count++] = index++;
            }
            curve_indexs[curve_index_count++] = UINT_MAX;
            if (curve_index_count == curve_index_len)
            {
                curve_index_len *= 2;
                unsigned int *temp = new unsigned int[curve_index_len];
                std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                delete []curve_indexs;
                curve_indexs = temp;
            }
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
            {
                switch (item->type())
                {
                case Geo::Type::POLYLINE:
                    while (polyline_index_count + item->point_count >= polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polyline_indexs[polyline_index_count++] = index++;
                    }
                    polyline_indexs[polyline_index_count++] = UINT_MAX;
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                    break;
                case Geo::Type::POLYGON:
                    while (polygon_index_count + item->point_count >= polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polygon_indexs[polygon_index_count++] = index++;
                    }
                    polygon_indexs[polygon_index_count++] = UINT_MAX;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    while (circle_index_count + item->point_count >= circle_index_len)
                    {
                        circle_index_len *= 2;
                        unsigned int *temp = new unsigned int[circle_index_len];
                        std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                        delete []circle_indexs;
                        circle_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        circle_indexs[circle_index_count++] = index++;
                    }
                    circle_indexs[circle_index_count++] = UINT_MAX;
                    if (circle_index_count == circle_index_len)
                    {
                        circle_index_len *= 2;
                        unsigned int *temp = new unsigned int[circle_index_len];
                        std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                        delete []circle_indexs;
                        circle_indexs = temp;
                    }
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    while (curve_index_count + item->point_count >= curve_index_len)
                    {
                        curve_index_len *= 2;
                        unsigned int *temp = new unsigned int[curve_index_len];
                        std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                        delete []curve_indexs;
                        curve_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        curve_indexs[curve_index_count++] = index++; 
                    }
                    curve_indexs[curve_index_count++] = UINT_MAX;
                    if (curve_index_count == curve_index_len)
                    {
                        curve_index_len *= 2;
                        unsigned int *temp = new unsigned int[curve_index_len];
                        std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                        delete []curve_indexs;
                        curve_indexs = temp;
                    }
                    break;
                case Geo::Type::POINT:
                    point_indexs[point_index_count++] = item->point_index;
                    if (point_index_len == point_index_count)
                    {
                        point_index_len *= 2;
                        unsigned int *temp = new unsigned int[point_index_len];
                        std::move(point_indexs, point_indexs + point_index_count, temp);
                        delete []point_indexs;
                        point_indexs = temp;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::POINT:
            point_indexs[point_index_count++] = geo->point_index;
            if (point_index_len == point_index_count)
            {
                point_index_len *= 2;
                unsigned int *temp = new unsigned int[point_index_len];
                std::move(point_indexs, point_indexs + point_index_count, temp);
                delete []point_indexs;
                point_indexs = temp;
            }
            break;
        default:
            continue;
        }
    }

    _selected_index_count.polyline = polyline_index_count;
    _selected_index_count.polygon = polygon_index_count;
    _selected_index_count.circle = circle_index_count;
    _selected_index_count.curve = curve_index_count;
    _selected_index_count.point = point_index_count;

    makeCurrent();
    if (polyline_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
    }
    if (polygon_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
    }
    if (circle_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
    }
    if (curve_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
    }
    if (point_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_index_count * sizeof(unsigned int), point_indexs, GL_DYNAMIC_DRAW);
    }
    doneCurrent();

    delete []polyline_indexs;
    delete []polygon_indexs;
    delete []circle_indexs;
    delete []curve_indexs;
    delete []point_indexs;
}

void Canvas::refresh_selected_ibo(const Geo::Geometry *object)
{
    if (object->type() == Geo::Type::COMBINATION)
    {
        unsigned int polyline_index_len = 512, polyline_index_count = 0;
        unsigned int polygon_index_len = 512, polygon_index_count = 0;
        unsigned int circle_index_len = 512, circle_index_count = 0;
        unsigned int curve_index_len = 512, curve_index_count = 0;
        unsigned int point_index_len = 256, point_index_count = 0;
        unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
        unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
        unsigned int *circle_indexs = new unsigned int[circle_index_len];
        unsigned int *curve_indexs = new unsigned int[curve_index_len];
        unsigned int *point_indexs = new unsigned int[point_index_len];
        for (const Geo::Geometry *item : *static_cast<const Combination *>(object))
        {
            switch (item->type())
            {
            case Geo::Type::POLYLINE:
                while (polyline_index_count + item->point_count >= polyline_index_len)
                {
                    polyline_index_len *= 2;
                    unsigned int *temp = new unsigned int[polyline_index_len];
                    std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                    delete []polyline_indexs;
                    polyline_indexs = temp;
                }
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    polyline_indexs[polyline_index_count++] = index++;
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                if (polyline_index_count == polyline_index_len)
                {
                    polyline_index_len *= 2;
                    unsigned int *temp = new unsigned int[polyline_index_len];
                    std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                    delete []polyline_indexs;
                    polyline_indexs = temp;
                }
                break;
            case Geo::Type::POLYGON:
                while (polygon_index_count + item->point_count >= polygon_index_len)
                {
                    polygon_index_len *= 2;
                    unsigned int *temp = new unsigned int[polygon_index_len];
                    std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                    delete []polygon_indexs;
                    polygon_indexs = temp;
                }
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    polygon_indexs[polygon_index_count++] = index++;
                }
                polygon_indexs[polygon_index_count++] = UINT_MAX;
                if (polygon_index_count == polygon_index_len)
                {
                    polygon_index_len *= 2;
                    unsigned int *temp = new unsigned int[polygon_index_len];
                    std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                    delete []polygon_indexs;
                    polygon_indexs = temp;
                }
                break;
            case Geo::Type::CIRCLE:
            case Geo::Type::ELLIPSE:
            case Geo::Type::ARC:
                while (circle_index_count + item->point_count >= circle_index_len)
                {
                    circle_index_len *= 2;
                    unsigned int *temp = new unsigned int[circle_index_len];
                    std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                    delete []circle_indexs;
                    circle_indexs = temp;
                }
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    circle_indexs[circle_index_count++] = index++;
                }
                circle_indexs[circle_index_count++] = UINT_MAX;
                if (circle_index_count == circle_index_len)
                {
                    circle_index_len *= 2;
                    unsigned int *temp = new unsigned int[circle_index_len];
                    std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                    delete []circle_indexs;
                    circle_indexs = temp;
                }
                break;
            case Geo::Type::BEZIER:
            case Geo::Type::BSPLINE:
                while (curve_index_count + item->point_count >= curve_index_len)
                {
                    curve_index_len *= 2;
                    unsigned int *temp = new unsigned int[curve_index_len];
                    std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                    delete []curve_indexs;
                    curve_indexs = temp;
                }
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    curve_indexs[curve_index_count++] = index++;   
                }
                curve_indexs[curve_index_count++] = UINT_MAX;
                if (curve_index_count == curve_index_len)
                {
                    curve_index_len *= 2;
                    unsigned int *temp = new unsigned int[curve_index_len];
                    std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                    delete []curve_indexs;
                    curve_indexs = temp;
                }
                break;
            case Geo::Type::POINT:
                point_indexs[point_index_count++] = item->point_index;
                if (point_index_len == point_index_count)
                {
                    point_index_len *= 2;
                    unsigned int *temp = new unsigned int[point_index_len];
                    std::move(point_indexs, point_indexs + point_index_count, temp);
                    delete []point_indexs;
                    point_indexs = temp;
                }
                break;
            default:
                break;
            }
        }

        _selected_index_count.polyline = polyline_index_count;
        _selected_index_count.polygon = polygon_index_count;
        _selected_index_count.circle = circle_index_count;
        _selected_index_count.curve = curve_index_count;
        _selected_index_count.point = point_index_count;
        makeCurrent();
        if (polyline_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
        }
        if (polygon_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
        }
        if (circle_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
        }
        if (curve_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
        }
        if (point_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_index_count * sizeof(unsigned int), point_indexs, GL_DYNAMIC_DRAW);
        }
        doneCurrent();

        delete []polyline_indexs;
        delete []polygon_indexs;
        delete []circle_indexs;
        delete []curve_indexs;
        delete []point_indexs;
    }
    else
    {
        unsigned int index_count = 0;
        unsigned int *indexs = new unsigned int[object->point_count + 1];
        for (size_t i = 0, index = object->point_index, count = object->point_count; i < count; ++i)
        {
            indexs[index_count++] = index++;
        }
        indexs[index_count++] = UINT_MAX;
        clear_selected_ibo();
        unsigned int IBO_index = _selected_ibo.point;
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            IBO_index = _selected_ibo.polyline;
            _selected_index_count.polyline = index_count;
            break;
        case Geo::Type::POLYGON:
            IBO_index = _selected_ibo.polygon;
            _selected_index_count.polygon = index_count;
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            IBO_index = _selected_ibo.circle;
            _selected_index_count.circle = index_count;
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            IBO_index = _selected_ibo.curve;
            _selected_index_count.curve = index_count;
            break;
        case Geo::Type::POINT:
            _selected_index_count.point = index_count;
            break;
        default:
            break;
        }
        if (index_count > 0)
        {
            makeCurrent();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_index);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
            doneCurrent();
        }
        delete []indexs;
    }
}

void Canvas::refresh_selected_ibo(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        return;
    }

    unsigned int polyline_index_len = 512, polyline_index_count = 0;
    unsigned int polygon_index_len = 512, polygon_index_count = 0;
    unsigned int circle_index_len = 512, circle_index_count = 0;
    unsigned int curve_index_len = 512, curve_index_count = 0;
    unsigned int point_index_len = 256, point_index_count = 0;
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
    unsigned int *circle_indexs = new unsigned int[circle_index_len];
    unsigned int *curve_indexs = new unsigned int[curve_index_len];
    unsigned int *point_indexs = new unsigned int[point_index_len];
    for (const Geo::Geometry *geo : objects)
    {
        switch (geo->type())
        {
        case Geo::Type::POLYLINE:
            while (polyline_index_count + geo->point_count >= polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                delete []polyline_indexs;
                polyline_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polyline_indexs[polyline_index_count++] = index++;
            }
            polyline_indexs[polyline_index_count++] = UINT_MAX;
            if (polyline_index_count == polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                delete []polyline_indexs;
                polyline_indexs = temp;
            }
            break;
        case Geo::Type::POLYGON:
            while (polygon_index_count + geo->point_count >= polygon_index_len)
            {
                polygon_index_len *= 2;
                unsigned int *temp = new unsigned int[polygon_index_len];
                std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                delete []polygon_indexs;
                polygon_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polygon_indexs[polygon_index_count++] = index++;
            }
            polygon_indexs[polygon_index_count++] = UINT_MAX;
            if (polygon_index_count == polygon_index_len)
            {
                polygon_index_len *= 2;
                unsigned int *temp = new unsigned int[polygon_index_len];
                std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                delete []polygon_indexs;
                polygon_indexs = temp;
            }
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            while (circle_index_count + geo->point_count >= circle_index_len)
            {
                circle_index_len *= 2;
                unsigned int *temp = new unsigned int[circle_index_len];
                std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                delete []circle_indexs;
                circle_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                circle_indexs[circle_index_count++] = index++;
            }
            circle_indexs[circle_index_count++] = UINT_MAX;
            if (circle_index_count == circle_index_len)
            {
                circle_index_len *= 2;
                unsigned int *temp = new unsigned int[circle_index_len];
                std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                delete []circle_indexs;
                circle_indexs = temp;
            }
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            while (curve_index_count + geo->point_count >= curve_index_len)
            {
                curve_index_len *= 2;
                unsigned int *temp = new unsigned int[curve_index_len];
                std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                delete []curve_indexs;
                curve_indexs = temp;
            }
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                curve_indexs[curve_index_count++] = index++;
            }
            curve_indexs[curve_index_count++] = UINT_MAX;
            if (curve_index_count == curve_index_len)
            {
                curve_index_len *= 2;
                unsigned int *temp = new unsigned int[curve_index_len];
                std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                delete []curve_indexs;
                curve_indexs = temp;
            }
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
            {
                switch (item->type())
                {
                case Geo::Type::POLYLINE:
                    while (polyline_index_count + item->point_count >= polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polyline_indexs[polyline_index_count++] = index++;
                    }
                    polyline_indexs[polyline_index_count++] = UINT_MAX;
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::move(polyline_indexs, polyline_indexs + polyline_index_count, temp);
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                    break;
                case Geo::Type::POLYGON:
                    while (polygon_index_count + item->point_count >= polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polygon_indexs[polygon_index_count++] = index++;
                    }
                    polygon_indexs[polygon_index_count++] = UINT_MAX;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::move(polygon_indexs, polygon_indexs + polygon_index_count, temp);
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    while (circle_index_count + item->point_count >= circle_index_len)
                    {
                        circle_index_len *= 2;
                        unsigned int *temp = new unsigned int[circle_index_len];
                        std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                        delete []circle_indexs;
                        circle_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        circle_indexs[circle_index_count++] = index++;
                    }
                    circle_indexs[circle_index_count++] = UINT_MAX;
                    if (circle_index_count == circle_index_len)
                    {
                        circle_index_len *= 2;
                        unsigned int *temp = new unsigned int[circle_index_len];
                        std::move(circle_indexs, circle_indexs + circle_index_count, temp);
                        delete []circle_indexs;
                        circle_indexs = temp;
                    }
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    while (curve_index_count + item->point_count >= curve_index_len)
                    {
                        curve_index_len *= 2;
                        unsigned int *temp = new unsigned int[curve_index_len];
                        std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                        delete []curve_indexs;
                        curve_indexs = temp;
                    }
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        curve_indexs[curve_index_count++] = index++; 
                    }
                    curve_indexs[curve_index_count++] = UINT_MAX;
                    if (curve_index_count == curve_index_len)
                    {
                        curve_index_len *= 2;
                        unsigned int *temp = new unsigned int[curve_index_len];
                        std::move(curve_indexs, curve_indexs + curve_index_count, temp);
                        delete []curve_indexs;
                        curve_indexs = temp;
                    }
                    break;
                case Geo::Type::POINT:
                    point_indexs[point_index_count++] = item->point_index;
                    if (point_index_len == point_index_count)
                    {
                        point_index_len *= 2;
                        unsigned int *temp = new unsigned int[point_index_len];
                        std::move(point_indexs, point_indexs + point_index_count, temp);
                        delete []point_indexs;
                        point_indexs = temp;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::POINT:
            point_indexs[point_index_count++] = geo->point_index;
            if (point_index_len == point_index_count)
            {
                point_index_len *= 2;
                unsigned int *temp = new unsigned int[point_index_len];
                std::move(point_indexs, point_indexs + point_index_count, temp);
                delete []point_indexs;
                point_indexs = temp;
            }
            break;
        default:
            continue;
        }
    }

    _selected_index_count.polyline = polyline_index_count;
    _selected_index_count.polygon = polygon_index_count;
    _selected_index_count.circle = circle_index_count;
    _selected_index_count.curve = curve_index_count;
    _selected_index_count.point = point_index_count;

    makeCurrent();
    if (polyline_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
    }
    if (polygon_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
    }
    if (circle_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
    }
    if (curve_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
    }
    if (point_index_count > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_index_count * sizeof(unsigned int), point_indexs, GL_DYNAMIC_DRAW);
    }
    doneCurrent();
    
    delete []polyline_indexs;
    delete []polygon_indexs;
    delete []circle_indexs;
    delete []curve_indexs;
    delete []point_indexs;
}

void Canvas::refresh_selected_vbo()
{
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>> polyline_vbo,
        polygon_vbo, circle_vbo, curve_vbo, text_vbo;
    std::future<std::tuple<double*, unsigned int>> circle_point, curve_point, point_vbo;
    bool refresh[6] = {false, false, false, false, false, false};
    for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        for (const Geo::Geometry *object : group)
        {
            if (object->is_selected)
            {
                switch (object->type())
                {
                case Geo::Type::POLYLINE:
                    if (!refresh[0])
                    {
                        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this);
                        refresh[0] = true;
                    }
                    break;
                case Geo::Type::POLYGON:
                    if (!refresh[1])
                    {
                        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this);
                        refresh[1] = true;
                    }
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    if (!refresh[2])
                    {
                        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this);
                        circle_point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
                        refresh[2] = true;
                    }
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    if (!refresh[3])
                    {
                        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
                        curve_point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
                        refresh[3] = true;
                    }
                    break;
                case Geo::Type::TEXT:
                    if (!refresh[4])
                    {
                        text_vbo = std::async(std::launch::async, &Canvas::refresh_text_vbo, this);
                        refresh[4] = true;
                    }
                    break;
                case Geo::Type::COMBINATION:
                    for (const Geo::Geometry *item: *static_cast<const Combination *>(object))
                    {
                        switch (item->type())
                        {
                        case Geo::Type::POLYLINE:
                            if (!refresh[0])
                            {
                                polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this);
                                refresh[0] = true;
                            }
                            break;
                        case Geo::Type::POLYGON:
                            if (!refresh[1])
                            {
                                polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this);
                                refresh[1] = true;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                        case Geo::Type::ELLIPSE:
                        case Geo::Type::ARC:
                            if (!refresh[2])
                            {
                                circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this);
                                circle_point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
                                refresh[2] = true;
                            }
                            break;
                        case Geo::Type::BEZIER:
                        case Geo::Type::BSPLINE:
                            if (!refresh[3])
                            {
                                curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
                                curve_point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
                                refresh[3] = true;
                            }
                            break;
                        case Geo::Type::TEXT:
                            if (!refresh[4])
                            {
                                text_vbo = std::async(std::launch::async, &Canvas::refresh_text_vbo, this);
                                refresh[4] = true;
                            }
                            break;
                        case Geo::Type::POINT:
                            if (!refresh[5])
                            {
                                point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this);
                                refresh[5] = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case Geo::Type::POINT:
                    if (!refresh[5])
                    {
                        point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this);
                        refresh[5] = true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    makeCurrent();
    if (refresh[5])
    {
        point_vbo.wait();
        auto [data, data_count] = point_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
    }
    if (refresh[2])
    {
        circle_point.wait();
        auto [circle_printable_points, circle_printable_count] = circle_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_printable_count, circle_printable_points, GL_DYNAMIC_DRAW);
        delete []circle_printable_points;
        circle_vbo.wait();
        auto [data, data_count, indexs, index_count] = circle_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[3])
    {
        curve_point.wait();
        auto [curve_printable_points, curve_printable_count] = curve_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_printable_count, curve_printable_points, GL_DYNAMIC_DRAW);
        delete []curve_printable_points;
        curve_vbo.wait();
        auto [data, data_count, indexs, index_count] = curve_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[4])
    {
        text_vbo.wait();
        auto [data, data_count, indexs, index_count] = text_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.text); // text
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[0])
    {
        polyline_vbo.wait();
        auto [data, data_count, indexs, index_count] = polyline_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // polyline
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[1])
    {
        polygon_vbo.wait();
        auto [data, data_count, indexs, index_count] = polygon_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // polygon
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    doneCurrent();
}

void Canvas::clear_selected_ibo()
{
    _selected_index_count.polyline = _selected_index_count.polygon = 
        _selected_index_count.circle = _selected_index_count.curve =
        _selected_index_count.point = 0;
}

std::tuple<double*, unsigned int, unsigned int*, unsigned int> Canvas::refresh_text_vbo()
{
    QPainterPath path;
    const QFont font("SimSun", GlobalSetting::setting().text_size);
    const QFontMetrics font_metrics(font);
    QRectF text_rect;

    Text *text = nullptr;
    Geo::Point coord;
    Geo::Polygon points;
    unsigned int offset;
    int string_index;
    int width;
    QStringList strings;

    unsigned int data_len = 4104, data_count = 0;
    double *data = new double[data_len];
    unsigned int index_len = 1368, index_count = 0;
    unsigned int *indexs = new unsigned int[index_len];
    for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = static_cast<Text *>(geo);
                if (text->text().isEmpty())
                {
                    continue;
                }
                coord = text->center();
                strings = text->text().split('\n');
                string_index = 1;
                width = 0;
                for (const QString &string : strings)
                {
                    width = std::max(font_metrics.boundingRect(string).width(), width);
                }
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - width / 2, coord.y - text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                text->text_index = data_count;
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    if (text = dynamic_cast<Text *>(item); text != nullptr)
                    {
                        if (text->text().isEmpty())
                        {
                            continue;
                        }
                        coord = text->center();
                        strings = text->text().split('\n');
                        string_index = 1;
                        width = 0;
                        for (const QString &string : strings)
                        {
                            width = std::max(font_metrics.boundingRect(string).width(), width);
                        }
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - width / 2, coord.y - text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        text->text_index = data_count;
                    }
                    for (const QPolygonF &polygon : path.toSubpathPolygons())
                    {
                        offset = data_count / 3;
                        while (data_count + polygon.size() * 3 > data_len)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            std::move(data, data + data_count, temp);
                            delete []data;
                            data = temp;
                        }
                        for (const QPointF &point : polygon)
                        {
                            points.append(Geo::Point(point.x(), coord.y * 2 - point.y()));
                            data[data_count++] = point.x();
                            data[data_count++] = coord.y * 2 - point.y();
                            data[data_count++] = 0.5;
                        }

                        const std::vector<unsigned int> index(Geo::ear_cut_to_indexs(points));
                        while (index_count + index.size() >= index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const unsigned int i : index)
                        {
                            indexs[index_count++] = offset + i;
                        }

                        points.clear();
                        indexs[index_count++] = UINT_MAX;
                    }
                    if (text != nullptr)
                    {
                        text->text_count = data_count - text->text_index;
                    }
                    path.clear();
                }
                break;
            default:
                break;
            }
            for (const QPolygonF &polygon : path.toSubpathPolygons())
            {
                offset = data_count / 3;
                while (data_count + polygon.size() * 3 > data_len)
                {
                    data_len *= 2;
                    double *temp = new double[data_len];
                    std::move(data, data + data_count, temp);
                    delete []data;
                    data = temp;
                }
                for (const QPointF &point : polygon)
                {
                    points.append(Geo::Point(point.x(),  coord.y * 2 - point.y()));
                    data[data_count++] = point.x();
                    data[data_count++] = coord.y * 2 - point.y();
                    data[data_count++] = 0.5;
                }

                const std::vector<unsigned int> index(Geo::ear_cut_to_indexs(points));
                while (index_count + index.size() >= index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const unsigned int i : index)
                {
                    indexs[index_count++] = offset + i;
                }

                points.clear();
                indexs[index_count++] = UINT_MAX;
            }
            if (dynamic_cast<Text *>(geo) != nullptr)
            {
                text->text_count = data_count - text->text_index;
            }
            path.clear();
        }
    }

    _text_brush_count = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}


bool Canvas::refresh_catached_points(const double x, const double y, const double distance, std::vector<const Geo::Geometry *> &catched_objects, const bool skip_selected, const bool current_group_only) const
{
    if (!(_catch_types.vertex || _catch_types.center || _catch_types.foot || _catch_types.tangency || _catch_types.intersection))
    {
        return false;
    }

    const Geo::AABBRect rect(x - distance / _ratio, y + distance / _ratio, x + distance / _ratio, y - distance / _ratio);
    const Geo::Point pos(x, y);
    const size_t count = catched_objects.size();

    if (current_group_only)
    {
        for (const Geo::Geometry *geo : GlobalSetting::setting().graph->container_group(_editer->current_group()))
        {
            if (skip_selected && geo->is_selected)
            {
                continue;
            }
            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                if (Geo::is_intersected(rect, geo->bounding_rect()))
                {
                    if (Geo::distance(pos, *static_cast<const Geo::Polygon *>(geo)) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                }
                break;
            case Geo::Type::CIRCLE:
                if (Geo::distance(pos, *static_cast<const Geo::Circle *>(geo)) * _ratio < distance ||
                    std::abs(Geo::distance(pos, *static_cast<const Geo::Circle *>(geo))
                        - static_cast<const Geo::Circle *>(geo)->radius) * _ratio < distance)
                {
                    catched_objects.push_back(geo);
                }
                break;
            case Geo::Type::ELLIPSE:
                if (Geo::is_intersected(rect, geo->bounding_rect()))
                {
                    const Geo::Ellipse *e = static_cast<const Geo::Ellipse *>(geo);
                    if (Geo::distance(pos, e->center()) * _ratio < distance || Geo::distance(pos, *e) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                }
                break;
            case Geo::Type::POLYLINE:
                if (Geo::is_intersected(rect, geo->bounding_rect()))
                {
                    if (Geo::distance(pos, *static_cast<const Geo::Polyline *>(geo)) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                }
                break;
            case Geo::Type::BSPLINE:
                if (Geo::is_intersected(rect, geo->bounding_rect()))
                {
                    if (Geo::distance(pos, static_cast<const Geo::BSpline *>(geo)->shape()) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                }
                break;
            case Geo::Type::BEZIER:
                if (Geo::is_intersected(rect, geo->bounding_rect()))
                {
                    if (Geo::distance(pos, static_cast<const Geo::Bezier *>(geo)->shape()) < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                }
                break;
            case Geo::Type::ARC:
                if (Geo::is_intersected(rect, *static_cast<const Geo::Arc *>(geo)))
                {
                    catched_objects.push_back(geo);
                }
                break;
            case Geo::Type::POINT:
                if (Geo::distance(pos, *static_cast<const Geo::Point *>(geo)) * _ratio < distance)
                {
                    catched_objects.push_back(geo);
                }
                break;
            default:
                break;
            }
        }
    }
    else
    {
        for (const ContainerGroup &group : GlobalSetting::setting().graph->container_groups())
        {
            if (!group.visible())
            {
                continue;
            }

            for (const Geo::Geometry *geo : group)
            {
                if (skip_selected && geo->is_selected)
                {
                    continue;
                }
                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    if (Geo::is_intersected(rect, geo->bounding_rect()))
                    {
                        if (Geo::distance(pos, *static_cast<const Geo::Polygon *>(geo)) * _ratio < distance)
                        {
                            catched_objects.push_back(geo);
                        }
                    }
                    break;
                case Geo::Type::CIRCLE:
                    if (Geo::distance(pos, *static_cast<const Geo::Circle *>(geo)) * _ratio < distance ||
                        std::abs(Geo::distance(pos, *static_cast<const Geo::Circle *>(geo))
                            - static_cast<const Geo::Circle *>(geo)->radius) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                    break;
                case Geo::Type::ELLIPSE:
                    if (Geo::is_intersected(rect, geo->bounding_rect()))
                    {
                        const Geo::Ellipse *e = static_cast<const Geo::Ellipse *>(geo);
                        if (Geo::distance(pos, e->center()) * _ratio < distance || Geo::distance(pos, *e) * _ratio < distance)
                        {
                            catched_objects.push_back(geo);
                        }
                    }
                    break;
                case Geo::Type::POLYLINE:
                    if (Geo::is_intersected(rect, geo->bounding_rect()))
                    {
                        if (Geo::distance(pos, *static_cast<const Geo::Polyline *>(geo)) * _ratio < distance)
                        {
                            catched_objects.push_back(geo);
                        }
                    }
                    break;
                case Geo::Type::ARC:
                    if (Geo::is_intersected(rect, *static_cast<const Geo::Arc *>(geo)))
                    {
                        catched_objects.push_back(geo);
                    }
                    break;
                case Geo::Type::POINT:
                    if (Geo::distance(pos, *static_cast<const Geo::Point *>(geo)) * _ratio < distance)
                    {
                        catched_objects.push_back(geo);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    } 

    return catched_objects.size() > count;
}

bool Canvas::refresh_catchline_points(const std::vector<const Geo::Geometry *> &objects, const double distance, Geo::Point &pos)
{
    const CanvasOperations::Tool tool = CanvasOperations::CanvasOperation::tool[0];
    const bool catch_flags[Canvas::catch_count] = { _catch_types.vertex, _catch_types.center,
        _catch_types.foot && (tool > CanvasOperations::Tool::Move && tool < CanvasOperations::Tool::Mirror),
        _catch_types.tangency && (tool > CanvasOperations::Tool::Move && tool < CanvasOperations::Tool::Mirror),
        _catch_types.intersection };
    if (!std::any_of(catch_flags, catch_flags + Canvas::catch_count, [](const bool v){ return v; }))
    {
        return false;
    }

    Geo::Point result[Canvas::catch_count]; // Vertex, Center, Foot, Tangency, Intersection
    double dis[Canvas::catch_count] = {DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX};
    const Geo::Point press_pos(CanvasOperations::CanvasOperation::press_pos[0],
        CanvasOperations::CanvasOperation::press_pos[1]);
    for (const Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline &polyline = *static_cast<const Geo::Polyline *>(object);
                if (catch_flags[0])
                {
                    dis[0] = Geo::distance(pos, polyline.front());
                    result[0] = polyline.front();
                }
                for (size_t i = 1, count = polyline.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, polyline[i]); catch_flags[0] && d < dis[0])
                    {
                        result[0] = polyline[i];
                        dis[0] = d;
                    }
                    const Geo::Point center((polyline[i - 1] + polyline[i]) / 2);
                    if (const double d = Geo::distance(pos, center); catch_flags[1] && d < dis[1])
                    {
                        dis[1] = d;
                        result[1] = center;
                    }
                    if (Geo::Point foot; catch_flags[2] && Geo::foot_point(polyline[i - 1], polyline[i], press_pos, foot))
                    {
                        if (const double d = Geo::distance(pos, foot); d < dis[2])
                        {
                            dis[2] = d;
                            result[2] = foot;
                        }
                    }
                }
                if (catch_flags[4])
                {
                    for (size_t i = 1, count = polyline.size(); i < count; ++i)
                    {
                        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) > distance)
                        {
                            continue;
                        }
                        for (size_t j = i + 2; j < count; ++j)
                        {
                            if (Geo::Point point; Geo::is_intersected(polyline[i - 1], polyline[i],
                                polyline[j - 1], polyline[j], point, false))
                            {
                                if (const double d = Geo::distance(pos, point); d < dis[4])
                                {
                                    dis[4] = d;
                                    result[4] = point;
                                }
                            }
                        }
                    }
                }
            }
            break;
        case Geo::Type::POLYGON:
            {
                const Geo::Polygon &polygon = *static_cast<const Geo::Polygon *>(object);
                if (catch_flags[0])
                {
                    dis[0] = Geo::distance(pos, polygon.front());
                    result[0] = polygon.front();
                }
                for (size_t i = 1, count = polygon.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, polygon[i]); catch_flags[0] && d < dis[0])
                    {
                        dis[0] = d;
                        result[0] = polygon[i];
                    }
                    const Geo::Point center((polygon[i - 1] + polygon[i]) / 2);
                    if (const double d = Geo::distance(pos, center); catch_flags[1] && d < dis[1])
                    {
                        dis[1] = d;
                        result[1] = center;
                    }
                    if (Geo::Point foot; catch_flags[2] && Geo::foot_point(polygon[i - 1], polygon[i], press_pos, foot))
                    {
                        if (const double d = Geo::distance(pos, foot); d < dis[2])
                        {
                            dis[2] = d;
                            result[2] = foot;
                        }
                    }
                }
            }
            break;
        case Geo::Type::CIRCLE:
            {
                const Geo::Circle *c = static_cast<const Geo::Circle *>(object);
                if (catch_flags[0])
                {
                    if (const double d = Geo::distance(pos.x, pos.y, c->x, c->y); d < dis[0])
                    {
                        dis[0] = d;
                        result[0] = *c;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, c->x - c->radius, c->y); d < dis[0])
                    {
                        dis[0] = d;
                        result[0].x = c->x - c->radius;
                        result[0].y = c->y;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, c->x + c->radius, c->y); d < dis[0])
                    {
                        dis[0] = d;
                        result[0].x = c->x + c->radius;
                        result[0].y = c->y;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, c->x, c->y + c->radius); d < dis[0])
                    {
                        dis[0] = d;
                        result[0].x = c->x;
                        result[0].y = c->y + c->radius;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, c->x, c->y - c->radius); d < dis[0])
                    {
                        dis[0] = d;
                        result[0].x = c->x;
                        result[0].y = c->y - c->radius;
                    }
                }
                if (Geo::Point output0(DBL_MAX, DBL_MAX), output1(DBL_MAX, DBL_MAX);
                    catch_flags[2] && Geo::foot_point(*c, press_pos, output0, output1))
                {
                    if (const double d = Geo::distance(pos.x, pos.y, output0.x, output0.y); d < dis[2])
                    {
                        dis[2] = d;
                        result[2].x = output0.x;
                        result[2].y = output0.y;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, output1.x, output1.y); d < dis[2])
                    {
                        dis[2] = d;
                        result[2].x = output1.x;
                        result[2].y = output1.y;
                    }
                }
                if (Geo::Point output0(DBL_MAX, DBL_MAX), output1(DBL_MAX, DBL_MAX);
                    catch_flags[3] && Geo::tangency_point(press_pos, *c, output0, output1))
                {
                    if (const double d = Geo::distance(pos.x, pos.y, output0.x, output0.y); d < dis[3])
                    {
                        dis[3] = d;
                        result[3].x = output0.x;
                        result[3].y = output0.y;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, output1.x, output1.y); d < dis[3])
                    {
                        dis[3] = d;
                        result[3].x = output1.x;
                        result[3].y = output1.y;
                    }
                }
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                const Geo::Ellipse *e = static_cast<const Geo::Ellipse *>(object);
                if (catch_flags[0])
                {
                    if (e->is_arc())
                    {
                        if (const double d = Geo::distance(pos, e->shape().front()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->shape().front();
                        }
                        if (const double d = Geo::distance(pos, e->shape().back()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->shape().back();
                        }
                    }
                    else
                    {
                        if (const double d = Geo::distance(pos, e->center()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->center();
                        }
                        if (const double d = Geo::distance(pos, e->a0()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->a0();
                        }
                        if (const double d = Geo::distance(pos, e->a1()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->a1();
                        }
                        if (const double d = Geo::distance(pos, e->b0()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->b0();
                        }
                        if (const double d = Geo::distance(pos, e->b1()); d < dis[0])
                        {
                            dis[0] = d;
                            result[0] = e->b1();
                        }
                    }
                }
                if (std::vector<Geo::Point> output; catch_flags[2] && Geo::foot_point(*e, press_pos, output))
                {
                    for (const Geo::Point &p : output)
                    {
                        if (const double d = Geo::distance(pos.x, pos.y, p.x, p.y); d < dis[2])
                        {
                            dis[2] = d;
                            result[2].x = p.x;
                            result[2].y = p.y;
                        }
                    }
                }
                if (Geo::Point output0(DBL_MAX, DBL_MAX), output1(DBL_MAX, DBL_MAX);
                    catch_flags[3] && Geo::tangency_point(press_pos, *e, output0, output1))
                {
                    if (const double d = Geo::distance(pos.x, pos.y, output0.x, output0.y); d < dis[3])
                    {
                        dis[3] = d;
                        result[3].x = output0.x;
                        result[3].y = output0.y;
                    }
                    if (const double d = Geo::distance(pos.x, pos.y, output1.x, output1.y); d < dis[3])
                    {
                        dis[3] = d;
                        result[3].x = output1.x;
                        result[3].y = output1.y;
                    }
                }
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline &bspline = *static_cast<const Geo::BSpline *>(object);
                if (catch_flags[0])
                {
                    dis[0] = Geo::distance(pos, bspline.path_points.front());
                    result[0] = bspline.path_points.front();
                }
                if (catch_flags[2])
                {
                    if (std::vector<Geo::Point> points; Geo::foot_point(press_pos, bspline, points, nullptr))
                    {
                        for (const Geo::Point &point : points)
                        {
                            if (double dis2 = Geo::distance(pos, point); dis2 < dis[2])
                            {
                                dis[2] = dis2;
                                result[2] = point;
                            }
                        }
                    }
                }
                if (catch_flags[3])
                {
                    if (std::vector<Geo::Point> points; Geo::tangency_point(press_pos, bspline, points, nullptr))
                    {
                        for (const Geo::Point &point : points)
                        {
                            if (double dis3 = Geo::distance(pos, point); dis3 < dis[3])
                            {
                                dis[3] = dis3;
                                result[3] = point;
                            }
                        }
                    }
                }
                for (size_t i = 1, count = bspline.path_points.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, bspline.path_points[i]); catch_flags[0] && d < dis[0])
                    {
                        result[0] = bspline.path_points[i];
                        dis[0] = d;
                    }
                }
            }
            break;
        case Geo::Type::BEZIER:
            {
                const Geo::Bezier &bezier = *static_cast<const Geo::Bezier *>(object);
                if (catch_flags[0])
                {
                    if (double dis0 = Geo::distance(pos, bezier.front()),
                        dis1 = Geo::distance(pos, bezier.back()); dis0 <= dis1)
                    {
                        dis[0] = dis0;
                        result[0] = bezier.front();
                    }
                    else
                    {
                        dis[0] = dis1;
                        result[0] = bezier.back();
                    }
                }
                if (catch_flags[2])
                {
                    if (std::vector<Geo::Point> points; Geo::foot_point(press_pos, bezier, points, nullptr))
                    {
                        for (const Geo::Point &point : points)
                        {
                            if (double dis2 = Geo::distance(pos, point); dis2 < dis[2])
                            {
                                dis[2] = dis2;
                                result[2] = point;
                            }
                        }
                    }
                }
                if (catch_flags[3])
                {
                    if (std::vector<Geo::Point> points; Geo::tangency_point(press_pos, bezier, points, nullptr))
                    {
                        for (const Geo::Point &point : points)
                        {
                            if (double dis1 = Geo::distance(pos, point); dis1 < dis[3])
                            {
                                dis[3] = dis1;
                                result[3] = point;
                            }
                        }
                    }
                }
            }
            break;
        case Geo::Type::ARC:
            {
                const Geo::Arc *arc = static_cast<const Geo::Arc *>(object);
                if (catch_flags[0])
                {
                    dis[0] = Geo::distance(pos, arc->control_points[0]);
                    result[0] = arc->control_points[0];
                }
                for (int i = 1; i < 3; ++i)
                {
                    if (double d = Geo::distance(pos, arc->control_points[i]); d < dis[0])
                    {
                        dis[0] = d;
                        result[0] = arc->control_points[i];
                    }
                }
            }
            break;
        case Geo::Type::POINT:
            {
                const Geo::Point *point = static_cast<const Geo::Point *>(object);
                if (const double d = Geo::distance(pos, *point); catch_flags[0] && d < dis[0])
                {
                    dis[0] = d;
                    result[0] = *point;
                }
            }
            break;
        default:
            break;
        }
    }

    if (catch_flags[4])
    {
        for (size_t i = 0, count = objects.size(); i < count; ++i)
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                std::vector<Geo::Point> points;
                if (Geo::find_intersections(objects[i], objects[j], pos, distance, points))
                {
                    for (const Geo::Point &point : points)
                    {
                        if (const double d = Geo::distance(pos, point); d < dis[4])
                        {
                            dis[4] = d;
                            result[4] = point;
                        }
                    }
                }
            }
        }
    }

    if (_editer->point_cache().size() > 2)
    {
        if (const double d = Geo::distance(pos, _editer->point_cache().front()); catch_flags[0] && d < dis[0])
        {
            dis[0] = d;
            result[0] = _editer->point_cache().front();
        }
        for (size_t i = 1, count = _editer->point_cache().size() - 1; i < count; ++i)
        {
            if (const double d = Geo::distance(pos, _editer->point_cache()[i]); catch_flags[0] && d < dis[0])
            {
                result[0] = _editer->point_cache()[i];
                dis[0] = d;
            }
            const Geo::Point center((_editer->point_cache()[i - 1] + _editer->point_cache()[i]) / 2);
            if (const double d = Geo::distance(pos, center); catch_flags[1] && d < dis[1])
            {
                dis[1] = d;
                result[1] = center;
            }
            if (Geo::Point foot; catch_flags[2] && Geo::foot_point(_editer->point_cache()[i - 1],
                _editer->point_cache()[i], press_pos, foot))
            {
                if (const double d = Geo::distance(pos, foot); d < dis[2])
                {
                    dis[2] = d;
                    result[2] = foot;
                }
            }
        }

        if (catch_flags[4])
        {
            for (size_t i = 1, count = _editer->point_cache().size() - 1; i < count; ++i)
            {
                if (Geo::distance(pos, _editer->point_cache()[i - 1], _editer->point_cache()[i], false) > distance)
                {
                    continue;
                }
                for (size_t j = i + 2; j < count; ++j)
                {
                    if (Geo::Point point; Geo::is_intersected(_editer->point_cache()[i - 1], _editer->point_cache()[i],
                        _editer->point_cache()[j - 1], _editer->point_cache()[j], point, false))
                    {
                        if (const double d = Geo::distance(pos, point); d < dis[4])
                        {
                            dis[4] = d;
                            result[4] = point;
                        }
                    }
                }
            }
        }
    }

    if (std::all_of(dis, dis + Canvas::catch_count, [=](const double d){ return d > distance / _ratio;}))
    {
        return false;
    }

    pos = result[std::distance(dis, std::min_element(dis, dis + Canvas::catch_count))];
    switch (static_cast<CatchedPointType>(std::distance(dis, std::min_element(dis, dis + Canvas::catch_count))))
    {
    case CatchedPointType::Vertex:
        {
            const double w = 6 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y + w;
            _catchline_points[3] = pos.x + w, _catchline_points[4] = pos.y + w;
            _catchline_points[2] = _catchline_points[5] = 0.51;

            _catchline_points[6] = pos.x + w, _catchline_points[7] = pos.y + w;
            _catchline_points[9] = pos.x + w, _catchline_points[10] = pos.y - w;
            _catchline_points[8] = _catchline_points[11] = 0.51;

            _catchline_points[12] = pos.x + w, _catchline_points[13] = pos.y - w;
            _catchline_points[15] = pos.x - w, _catchline_points[16] = pos.y - w;
            _catchline_points[14] = _catchline_points[17] = 0.51;

            _catchline_points[18] = pos.x - w, _catchline_points[19] = pos.y - w;
            _catchline_points[21] = pos.x - w, _catchline_points[22] = pos.y + w;
            _catchline_points[20] = _catchline_points[23] = 0.51;
        }
        break;
    case CatchedPointType::Center:
        {
            const double w = 4.8 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y - w;
            _catchline_points[3] = pos.x, _catchline_points[4] = pos.y + w * 2.5;
            _catchline_points[2] = _catchline_points[5] = 0.51;

            _catchline_points[6] = pos.x, _catchline_points[7] = pos.y + w * 2.5;
            _catchline_points[9] = pos.x + w, _catchline_points[10] = pos.y - w;
            _catchline_points[8] = _catchline_points[11] = 0.51;

            _catchline_points[12] = pos.x + w, _catchline_points[13] = pos.y - w;
            _catchline_points[15] = pos.x - w, _catchline_points[16] = pos.y - w;
            _catchline_points[14] = _catchline_points[17] = 0.51;

            _catchline_points[18] = pos.x - w, _catchline_points[19] = pos.y - w;
            _catchline_points[21] = pos.x, _catchline_points[22] = pos.y + w * 2.5;
            _catchline_points[20] = _catchline_points[23] = 0.51;
        }
        break;
    case CatchedPointType::Foot:
        {
            const double w = 6 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y;
            _catchline_points[3] = pos.x, _catchline_points[4] = pos.y;
            _catchline_points[2] = _catchline_points[5] = 0.51;

            _catchline_points[6] = pos.x, _catchline_points[7] = pos.y;
            _catchline_points[9] = pos.x, _catchline_points[10] = pos.y - w;
            _catchline_points[8] = _catchline_points[11] = 0.51;

            _catchline_points[12] = pos.x + w * 1.2, _catchline_points[13] = pos.y - w;
            _catchline_points[15] = pos.x - w, _catchline_points[16] = pos.y - w;
            _catchline_points[14] = _catchline_points[17] = 0.51;

            _catchline_points[18] = pos.x - w, _catchline_points[19] = pos.y - w;
            _catchline_points[21] = pos.x - w, _catchline_points[22] = pos.y + w * 1.2;
            _catchline_points[20] = _catchline_points[23] = 0.51;
        }
        break;
    case CatchedPointType::Tangency:
        {
            const double w = 8 / _ratio, h = 3 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y + h;
            _catchline_points[3] = pos.x - h, _catchline_points[4] = pos.y + w;
            _catchline_points[2] = _catchline_points[5] = 0.51;

            _catchline_points[6] = pos.x + h, _catchline_points[7] = pos.y + w;
            _catchline_points[9] = pos.x + w, _catchline_points[10] = pos.y + h;
            _catchline_points[8] = _catchline_points[11] = 0.51;

            _catchline_points[12] = pos.x + w, _catchline_points[13] = pos.y - h;
            _catchline_points[15] = pos.x + h, _catchline_points[16] = pos.y - w;
            _catchline_points[14] = _catchline_points[17] = 0.51;

            _catchline_points[18] = pos.x - h, _catchline_points[19] = pos.y - w;
            _catchline_points[21] = pos.x - w, _catchline_points[22] = pos.y - h;
            _catchline_points[20] = _catchline_points[23] = 0.51;
        }
        break;
    case Canvas::CatchedPointType::Intersection:
        {
            const double w = 7 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y + w;
            _catchline_points[3] = pos.x + w, _catchline_points[4] = pos.y - w;
            _catchline_points[2] = _catchline_points[5] = 0.51;

            _catchline_points[6] = pos.x - w, _catchline_points[7] = pos.y + w;
            _catchline_points[9] = pos.x + w, _catchline_points[10] = pos.y - w;
            _catchline_points[8] = _catchline_points[11] = 0.51;

            _catchline_points[12] = pos.x + w, _catchline_points[13] = pos.y + w;
            _catchline_points[15] = pos.x - w, _catchline_points[16] = pos.y - w;
            _catchline_points[14] = _catchline_points[17] = 0.51;

            _catchline_points[18] = pos.x + w, _catchline_points[19] = pos.y + w;
            _catchline_points[21] = pos.x - w, _catchline_points[22] = pos.y - w;
            _catchline_points[20] = _catchline_points[23] = 0.51;
        }
        break;
    default:
        break;
    }

    return true;
}
