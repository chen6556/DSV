#include <future>
#include <QPainter>
#include <QPainterPath>
#include "base/Algorithm.hpp"
#include "draw/Canvas.hpp"
#include "draw/GLSL.hpp"
#include "io/GlobalSetting.hpp"


Canvas *Canvas::canvas = nullptr;

Canvas::Canvas(QWidget *parent) : QOpenGLWidget(parent), _input_line(this), _menu(this)
{
    init();
    Canvas::canvas = this;
}

Canvas::~Canvas()
{
    makeCurrent();
    {
        unsigned int temp[4] = {_base_vbo.origin_and_select_rect, _base_vbo.catched_points, _base_vbo.operation_shape,
                                _base_vbo.operation_tool_lines};
        glDeleteBuffers(4, temp);
    }
    {
        unsigned int temp[7] = {_shape_vbo.polyline,
                                _shape_vbo.polygon,
                                _shape_vbo.circle,
                                _shape_vbo.curve,
                                _shape_vbo.circle_printable_points,
                                _shape_vbo.curve_printable_points,
                                _shape_vbo.point};
        glDeleteBuffers(7, temp);
    }
    {
        unsigned int temp[4] = {_shape_ibo.polyline, _shape_ibo.polygon, _shape_ibo.circle, _shape_ibo.curve};
        glDeleteBuffers(4, temp);
    }
    {
        unsigned int temp[5] = {_selected_ibo.polyline, _selected_ibo.polygon, _selected_ibo.circle, _selected_ibo.curve,
                                _selected_ibo.point};
        glDeleteBuffers(5, temp);
    }
    {
        unsigned int temp[2] = {_texture.vbo, _texture.ibo};
        glDeleteBuffers(2, temp);
    }
    glDeleteProgram(_shader_program);
    doneCurrent();
    _editor.delete_graph();
}


void Canvas::init()
{
    CanvasOperations::CanvasOperation::operation().init();
    _cpus = std::max(2u, std::thread::hardware_concurrency() / 2);
    _input_line.hide();
}

Editor &Canvas::editor()
{
    return _editor;
}


void Canvas::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.117647f, 0.156862f, 0.188235f, 1.0f);

    unsigned int vertex_shader = 0;
    unsigned int fragment_shader = 0;

    glPointSize(7.8f); // 点大小
    glLineWidth(1.4f); // 线宽
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 抗锯齿
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

    int maxUniformBlockSize = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxUniformBlockSize);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &GLSL::base_vss, nullptr);
    glCompileShader(vertex_shader);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &GLSL::base_fss, nullptr);
    glCompileShader(fragment_shader);

    _shader_program = glCreateProgram();
    glAttachShader(_shader_program, vertex_shader);
    glAttachShader(_shader_program, fragment_shader);
    glLinkProgram(_shader_program);
    glDeleteShader(fragment_shader);

    glDeleteShader(vertex_shader);
    _uniforms.window = glGetUniformLocation(_shader_program, "window");
    _uniforms.ctm = glGetUniformLocation(_shader_program, "ctm");
    _uniforms.color = glGetUniformLocation(_shader_program, "color");
    _uniforms.enable_tex = glGetUniformLocation(_shader_program, "enableTex");

    glUseProgram(_shader_program);
    glUniformMatrix3dv(_uniforms.ctm, 1, GL_FALSE, _canvas_ctm); // ctm

    {
        unsigned int temp[4];
        glCreateBuffers(4, temp);
        _base_vbo.origin_and_select_rect = temp[0];
        _base_vbo.catched_points = temp[1];
        _base_vbo.operation_shape = temp[2];
        _base_vbo.operation_tool_lines = temp[3];
    }
    {
        unsigned int temp[7];
        glCreateBuffers(7, temp);
        _shape_vbo.polyline = temp[0];
        _shape_vbo.polygon = temp[1];
        _shape_vbo.circle = temp[2];
        _shape_vbo.curve = temp[3];
        _shape_vbo.circle_printable_points = temp[4];
        _shape_vbo.curve_printable_points = temp[5];
        _shape_vbo.point = temp[6];
    }
    {
        unsigned int temp[4];
        glCreateBuffers(4, temp);
        _shape_ibo.polyline = temp[0];
        _shape_ibo.polygon = temp[1];
        _shape_ibo.circle = temp[2];
        _shape_ibo.curve = temp[3];
    }
    {
        unsigned int temp[5];
        glCreateBuffers(5, temp);
        _selected_ibo.polyline = temp[0];
        _selected_ibo.polygon = temp[1];
        _selected_ibo.circle = temp[2];
        _selected_ibo.curve = temp[3];
        _selected_ibo.point = temp[4];
    }

    {
        glGenTextures(1, &_texture.texture);
        glBindTexture(GL_TEXTURE_2D, _texture.texture);
        glUniform1i(_uniforms.enable_tex, 0);
        // 为当前绑定的纹理对象设置环绕、过滤方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        unsigned int temp[2];
        glCreateBuffers(2, temp);
        _texture.vbo = temp[0];
        _texture.ibo = temp[1];
        const double vertices[16] = {
            // positions  // texture coords
            1.0,  1.0,  1.0, 1.0, // top right
            1.0,  -1.0, 1.0, 0.0, // bottom right
            -1.0, -1.0, 0.0, 0.0, // bottom left
            -1.0, 1.0,  0.0, 1.0  // top left
        };
        const unsigned int indices[6] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };
        glBindBuffer(GL_ARRAY_BUFFER, _texture.vbo);
        glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(double), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _texture.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.catched_points); // catcheline points
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(double), _catchline_points, GL_STREAM_DRAW);

    double data[16] = {-10, 0, 10, 0, 0, -10, 0, 10};
    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(double), data, GL_STREAM_DRAW);
}

void Canvas::resizeGL(int w, int h)
{
    glUniform2f(_uniforms.window, static_cast<float>(w) / 2, static_cast<float>(h) / 2);
    glViewport(0, 0, w, h);

    _canvas_ctm[7] += (h - _canvas_height);
    glUniformMatrix3dv(_uniforms.ctm, 1, GL_FALSE, _canvas_ctm); // ctm
    _view_ctm[7] += (h - _canvas_height) / _ratio;
    _canvas_width = w, _canvas_height = h;

    _visible_area = Geo::AABBRect(0, 0, w, h);
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);

    _texture.image = QImage(w, h, QImage::Format::Format_RGBA8888);
    refresh_vbo(true);
}

void Canvas::paintGL()
{
    std::future<void> text_furture;
    if (GlobalSetting::setting().show_text)
    {
        text_furture = std::async(std::launch::async, &Canvas::paint_text, this);
    }

    glUseProgram(_shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_shape_index_count.polyline > 0) // polyline
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // points
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline); // polyline
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f);       // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.polyline, GL_UNSIGNED_INT, nullptr);

        if (_selected_index_count.polyline > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f);          // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.polyline, GL_UNSIGNED_INT, nullptr);
        }
    }

    if (_shape_index_count.polygon > 0) // polygon
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // points
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon); // polygon
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f);      // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.polygon, GL_UNSIGNED_INT, nullptr);

        if (_selected_index_count.polygon > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f);         // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.polygon, GL_UNSIGNED_INT, nullptr);
        }
    }

    if (_shape_index_count.circle > 0) // circle
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // points
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle); // circle
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f);     // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.circle, GL_UNSIGNED_INT, nullptr);

        if (_selected_index_count.circle > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f);        // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.circle, GL_UNSIGNED_INT, nullptr);
        }
    }

    if (_shape_index_count.curve > 0) // curve
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // points
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve); // curve
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f);    // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count.curve, GL_UNSIGNED_INT, nullptr);

        if (_selected_index_count.curve > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve); // selected
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f);       // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count.curve, GL_UNSIGNED_INT, nullptr);
        }
    }

    if (_point_count.point > 0) // point
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // points
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);
        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawArrays(GL_POINTS, 0, _point_count.point);

        if (_selected_index_count.point > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point); // points
            glUniform4f(_uniforms.color, 1.0f, 0.0f, 0.0f, 1.0f);       // color 绘制线 selected
            glDrawElements(GL_POINTS, _selected_index_count.point, GL_UNSIGNED_INT, nullptr);
        }
    }

    if (GlobalSetting::setting().show_points)
    {
        glUniform4f(_uniforms.color, 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        if (_point_count.polyline > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // polyline points
            glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.polyline);
        }
        if (_point_count.polygon > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // polygon points
            glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.polygon);
        }
        if (_point_count.circle > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle points
            glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.circle);
        }
        if (_point_count.curve > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve points
            glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count.curve);
        }
    }

    if (!CanvasOperations::CanvasOperation::shape.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.operation_shape); // operation shpae
        glBufferData(GL_ARRAY_BUFFER, CanvasOperations::CanvasOperation::shape.size() * sizeof(double),
                     CanvasOperations::CanvasOperation::shape.data(), GL_STREAM_DRAW);
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_STRIP, 0, CanvasOperations::CanvasOperation::shape.size() / 2);
    }
    if (!CanvasOperations::CanvasOperation::tool_lines.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.operation_tool_lines); // operation tool lines
        glBufferData(GL_ARRAY_BUFFER, CanvasOperations::CanvasOperation::tool_lines.size() * sizeof(double),
                     CanvasOperations::CanvasOperation::tool_lines.data(), GL_STREAM_DRAW);
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms.color, CanvasOperations::CanvasOperation::tool_line_color[0],
                    CanvasOperations::CanvasOperation::tool_line_color[1], CanvasOperations::CanvasOperation::tool_line_color[2],
                    CanvasOperations::CanvasOperation::tool_line_color[3]); // color
        glDrawArrays(GL_LINES, 0, CanvasOperations::CanvasOperation::tool_lines.size() / 2);

        glUniform4f(_uniforms.color, 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glDrawArrays(GL_POINTS, 0, CanvasOperations::CanvasOperation::tool_lines.size() / 2);
    }

    if (_bool_flags.show_catched_points) // catched point
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.catched_points); // catched point
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 16 * sizeof(double), _catchline_points);

        glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.649f); // color
        glLineWidth(2.8f);
        glDrawArrays(GL_LINES, 0, 8);
        glLineWidth(1.4f);
        glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.549f); // color
    }

    if (_bool_flags.show_origin || _select_rect[0] != _select_rect[6] || _select_rect[1] != _select_rect[7])
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 2 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);

        if (_bool_flags.show_origin) // origin
        {
            glUniform4f(_uniforms.color, 1.0f, 1.0f, 1.0f, 1.0f); // color 画原点
            glDrawArrays(GL_LINES, 0, 4);
        }

        if (_select_rect[0] != _select_rect[6] || _select_rect[1] != _select_rect[7])
        {
            glBufferSubData(GL_ARRAY_BUFFER, 8 * sizeof(double), 8 * sizeof(double), _select_rect);

            glUniform4f(_uniforms.color, 0.0f, 0.47f, 0.843f, 0.1f); // color
            glDrawArrays(GL_POLYGON, 4, 4);

            glUniform4f(_uniforms.color, 0.0f, 1.0f, 0.0f, 0.549f); // color
            glDrawArrays(GL_LINE_LOOP, 4, 4);
        }
    }


    if (GlobalSetting::setting().show_text)
    {
        glUniform1i(_uniforms.enable_tex, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _texture.texture);
        glBindBuffer(GL_ARRAY_BUFFER, _texture.vbo);
        glVertexAttribLPointer(0, 2, GL_DOUBLE, 4 * sizeof(double), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribLPointer(1, 2, GL_DOUBLE, 4 * sizeof(double), (void *)(2 * sizeof(double)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _texture.ibo);

        text_furture.wait();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _canvas_width, _canvas_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture.image.constBits());
        glGenerateMipmap(GL_TEXTURE_2D);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glUniform1i(_uniforms.enable_tex, 0);
    }
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];

    if (Geo::Point coord; event->button() == Qt::MouseButton::LeftButton && catch_cursor(real_x1, real_y1, coord, _catch_distance, false))
    {
        real_x1 = coord.x, real_y1 = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        _ignore_mouse_move = true;
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
    }
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;
        CanvasOperations::CanvasOperation::press_pos[0] = real_x1, CanvasOperations::CanvasOperation::press_pos[1] = real_y1;
    }
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]];
        op != nullptr && op->mouse_press(event))
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
    _mouse_pos_0 = _mouse_pos_1;
    _mouse_pos_1 = event->position();
    double x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];

    if (Geo::Point coord; catch_cursor(x, y, coord, _catch_distance, false))
    {
        x = coord.x, y = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        _ignore_mouse_move = true;
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
    }
    CanvasOperations::CanvasOperation::real_pos[0] = x, CanvasOperations::CanvasOperation::real_pos[1] = y;
    CanvasOperations::CanvasOperation::release_pos[0] = x, CanvasOperations::CanvasOperation::release_pos[1] = y;
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]];
        op != nullptr && op->mouse_release(event))
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
    if (_ignore_mouse_move)
    {
        _ignore_mouse_move = false;
        return QOpenGLWidget::mouseMoveEvent(event);
    }
    _mouse_pos_0 = _mouse_pos_1;
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double real_x0 = _mouse_pos_0.x() * _view_ctm[0] + _mouse_pos_0.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y0 = _mouse_pos_0.x() * _view_ctm[1] + _mouse_pos_0.y() * _view_ctm[4] + _view_ctm[7];
    double canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
    double canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
    const bool catched_point = _bool_flags.show_catched_points;
    if (Geo::Point coord; catch_cursor(real_x1, real_y1, coord, _catch_distance, event->buttons() & Qt::MouseButton::LeftButton))
    {
        real_x1 = coord.x, real_y1 = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        // QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
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
        glUniformMatrix3dv(_uniforms.ctm, 1, GL_FALSE, _canvas_ctm); // ctm
        doneCurrent();
        refresh_vbo(false);
        refresh_selected_ibo();
        update();
    }

    CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;
    CanvasOperations::CanvasOperation::real_pos[2] = real_x0, CanvasOperations::CanvasOperation::real_pos[3] = real_y0;
    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]];
        op != nullptr && op->mouse_move(event))
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
    }
    {
        Geo::Point pos(real_x, real_y);
        refresh_catchline_points(_catched_objects, _catch_distance, pos);
    }
    makeCurrent();
    glUniformMatrix3dv(_uniforms.ctm, 1, GL_FALSE, _canvas_ctm); // ctm
    double data[8] = {-10 / _ratio, 0, 10 / _ratio, 0, 0, -10 / _ratio, 0, 10 / _ratio};
    glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
    glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(double), data);
    doneCurrent();
    refresh_vbo(false);
    refresh_selected_ibo();
    _editor.set_view_ratio(_ratio);
    CanvasOperations::CanvasOperation::view_ratio = _ratio;
    update();
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    CanvasOperations::CanvasOperation::real_pos[0] = real_x1, CanvasOperations::CanvasOperation::real_pos[1] = real_y1;

    if (CanvasOperations::CanvasOperation *op = CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]];
        op != nullptr && op->mouse_double_click(event))
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
    _editor.set_view_ratio(1.0);
    _bool_flags.show_catched_points = false;

    Graph *graph = _editor.graph();
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

    _editor.set_view_ratio(_ratio);
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
    _visible_area =
        Geo::AABBRect(x0 * _view_ctm[0] + y0 * _view_ctm[3] + _view_ctm[6], x0 * _view_ctm[1] + y0 * _view_ctm[4] + _view_ctm[7],
                      x1 * _view_ctm[0] + y1 * _view_ctm[3] + _view_ctm[6], x1 * _view_ctm[1] + y1 * _view_ctm[4] + _view_ctm[7]);

    makeCurrent();
    glUniformMatrix3dv(_uniforms.ctm, 1, GL_FALSE, _canvas_ctm); // ctm
    {
        double data[8] = {-10 / _ratio, 0, 10 / _ratio, 0, 0, -10 / _ratio, 0, 10 / _ratio};
        glBindBuffer(GL_ARRAY_BUFFER, _base_vbo.origin_and_select_rect); // origin and select rect
        glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(double), data);
    }
    doneCurrent();
    refresh_vbo(false);
    refresh_selected_ibo();
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

bool Canvas::is_typing() const
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


Geo::Point Canvas::center() const
{
    if (_editor.graph() == nullptr || _editor.graph()->empty())
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);
    if (_editor.graph() == nullptr || _editor.graph()->empty())
    {
        return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
    }

    for (const ContainerGroup &group : _editor.graph()->container_groups())
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
    if (_editor.graph() == nullptr || _editor.graph()->empty())
    {
        return Geo::AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-DBL_MAX), y1 = (-DBL_MAX);

    for (const ContainerGroup &group : _editor.graph()->container_groups())
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
    return _editor.graph() == nullptr || _editor.graph()->empty();
}

void Canvas::cancel_painting()
{
    _editor.point_cache().clear();
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
    _editor.append(object);
    refresh_vbo(true, object->type());
    refresh_selected_ibo();
    update();
}

void Canvas::show_menu(Geo::Geometry *object)
{
    refresh_selected_ibo(object);
    _menu.exec(object);
    return;
}

void Canvas::show_text_edit(Text *text)
{
    _edited_text = text;
    Geo::AABBRect rect(text->bounding_rect());
    rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
    _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
    _input_line.move(rect.center().x - _input_line.rect().center().x(), rect.center().y - _input_line.rect().center().y());
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
            _edited_text->set_text(_input_line.toPlainText());
            UndoStack::TextChangedCommand *cmd = new UndoStack::TextChangedCommand(_edited_text, text);
            cmd->updated.push_back(_edited_text);
            _editor.push_backup_command(cmd);
            refresh_vbo(true, Geo::Type::TEXT);
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
    _editor.copy_selected();
}

void Canvas::cut()
{
    _points_cache.clear();
    _points_cache.emplace_back(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
                               _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    _editor.cut_selected();
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : _editor.paste_table())
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
    refresh_vbo(true, types);
    refresh_selected_ibo();
}

void Canvas::paste()
{
    const double x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    if (!_points_cache.empty() && _editor.paste(x - _points_cache.back().x, y - _points_cache.back().y))
    {
        std::set<Geo::Type> types;
        for (const Geo::Geometry *object : _editor.paste_table())
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
        refresh_vbo(true, types);
        refresh_selected_ibo();
        update();
    }
}

void Canvas::paste(const double x, const double y)
{
    if (!_points_cache.empty() && _editor.paste(x - _points_cache.back().x, y - _points_cache.back().y))
    {
        std::set<Geo::Type> types;
        for (const Geo::Geometry *object : _editor.paste_table())
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
        refresh_vbo(true, types);
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


void Canvas::refresh_vbo(const bool flush)
{
    _editor.refresh_visible_objects(_visible_area.aabbrect_params());
    std::future<VBOData> polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this, flush),
                         polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this, flush),
                         circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this, flush),
                         curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this, flush),
                         point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this, flush);
    std::future<VBOData> circle_printable_points, curve_printable_points;
    if (GlobalSetting::setting().show_points)
    {
        circle_printable_points = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
        curve_printable_points = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
    }

    makeCurrent();
    if (GlobalSetting::setting().show_points)
    {
        circle_printable_points.wait();
        if (VBOData data = circle_printable_points.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        }

        curve_printable_points.wait();
        if (VBOData data = curve_printable_points.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    circle_vbo.wait();
    if (VBOData data = circle_vbo.get(); !data.vbo_data.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
    }

    curve_vbo.wait();
    if (VBOData data = curve_vbo.get(); !data.vbo_data.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
    }

    polyline_vbo.wait();
    if (VBOData data = polyline_vbo.get(); !data.vbo_data.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
    }

    polygon_vbo.wait();
    if (VBOData data = polygon_vbo.get(); !data.vbo_data.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
    }

    point_vbo.wait();
    if (VBOData data = point_vbo.get(); !data.vbo_data.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }

    doneCurrent();
}

void Canvas::refresh_vbo(const bool flush, const Geo::Type type)
{
    _editor.refresh_visible_objects(_visible_area.aabbrect_params());
    switch (type)
    {
    case Geo::Type::POLYLINE:
        if (VBOData data = refresh_polyline_vbo(flush); !data.vbo_data.empty())
        {
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
            doneCurrent();
        }
        break;
    case Geo::Type::POLYGON:
        if (VBOData data = refresh_polygon_vbo(flush); !data.vbo_data.empty())
        {
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
            doneCurrent();
        }
        break;
    case Geo::Type::CIRCLE:
    case Geo::Type::ELLIPSE:
    case Geo::Type::ARC:
        {
            std::future<VBOData> point;
            if (GlobalSetting::setting().show_points)
            {
                point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
            }
            if (VBOData data = refresh_circle_vbo(flush); !data.vbo_data.empty())
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            if (GlobalSetting::setting().show_points)
            {
                point.wait();
                if (VBOData data = point.get(); !data.vbo_data.empty())
                {
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
                    glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
                    doneCurrent();
                }
            }
        }
        break;
    case Geo::Type::BEZIER:
    case Geo::Type::BSPLINE:
        {
            std::future<VBOData> point;
            if (GlobalSetting::setting().show_points)
            {
                point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
            }
            if (VBOData data = refresh_curve_vbo(flush); !data.vbo_data.empty())
            {
                makeCurrent();
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            if (GlobalSetting::setting().show_points)
            {
                point.wait();
                if (VBOData data = point.get(); !data.vbo_data.empty())
                {
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
                    glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
                    doneCurrent();
                }
            }
        }
        break;
    case Geo::Type::TEXT:
        break;
    case Geo::Type::POINT:
        if (VBOData data = refresh_point_vbo(flush); !data.vbo_data.empty())
        {
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            doneCurrent();
        }
        break;
    default:
        refresh_vbo(flush);
        break;
    }
}

void Canvas::refresh_vbo(const bool flush, const std::set<Geo::Type> &types)
{
    if (types.find(Geo::Type::COMBINATION) != types.end())
    {
        return refresh_vbo(flush);
    }

    _editor.refresh_visible_objects(_visible_area.aabbrect_params());
    std::future<VBOData> polyline_vbo, polygon_vbo, circle_vbo, curve_vbo, circle_printable_points, curve_printable_points, point_vbo;

    if (types.find(Geo::Type::POLYLINE) != types.end())
    {
        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this, flush);
    }
    if (types.find(Geo::Type::POLYGON) != types.end())
    {
        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this, flush);
    }
    if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end() ||
        types.find(Geo::Type::ARC) != types.end())
    {
        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this, flush);
    }
    if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
    {
        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this, flush);
    }
    if (types.find(Geo::Type::POINT) != types.end())
    {
        point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this, flush);
    }
    if (GlobalSetting::setting().show_points)
    {
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end() ||
            types.find(Geo::Type::ARC) != types.end())
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
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end() ||
            types.find(Geo::Type::ARC) != types.end())
        {
            circle_printable_points.wait();
            if (VBOData data = circle_printable_points.get(); !data.vbo_data.empty())
            {
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            }
        }
        if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
        {
            curve_printable_points.wait();
            if (VBOData data = curve_printable_points.get(); !data.vbo_data.empty())
            {
                glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            }
        }
    }

    if (circle_vbo.valid())
    {
        circle_vbo.wait();
        if (VBOData data = circle_vbo.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.circle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    if (curve_vbo.valid())
    {
        curve_vbo.wait();
        if (VBOData data = curve_vbo.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.curve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    if (polyline_vbo.valid())
    {
        polyline_vbo.wait();
        if (VBOData data = polyline_vbo.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polyline);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    if (polygon_vbo.valid())
    {
        polygon_vbo.wait();
        if (VBOData data = polygon_vbo.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_ibo.polygon);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * data.ibo_data.size(), data.ibo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    if (point_vbo.valid())
    {
        point_vbo.wait();
        if (VBOData data = point_vbo.get(); !data.vbo_data.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
        }
    }

    doneCurrent();
}

Canvas::VBOData Canvas::refresh_polyline_vbo(const bool flush)
{
    VBOData result;
    _visible_objects[0].polyline = _visible_objects[1].polyline;
    _visible_objects[1].polyline.clear();
    for (Geo::Geometry *geo : _editor.visible_objects())
    {
        if (geo->type() == Geo::Type::POLYLINE)
        {
            _visible_objects[1].polyline.push_back(static_cast<Geo::Polyline *>(geo));
        }
        else if (geo->type() == Geo::Type::COMBINATION)
        {
            for (Geo::Geometry *child : *static_cast<Combination *>(geo))
            {
                if (child->type() == Geo::Type::POLYLINE)
                {
                    _visible_objects[1].polyline.push_back(static_cast<Geo::Polyline *>(child));
                }
            }
        }
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select && !flush &&
        _visible_objects[0].polyline == _visible_objects[1].polyline)
    {
        return result;
    }

    for (Geo::Polyline *polyline : _visible_objects[1].polyline)
    {
        polyline->point_index = result.vbo_data.size() / 2;
        for (const Geo::Point &point : *polyline)
        {
            result.ibo_data.push_back(result.vbo_data.size() / 2);
            result.vbo_data.push_back(point.x);
            result.vbo_data.push_back(point.y);
        }
        result.ibo_data.push_back(UINT_MAX);
        polyline->point_count = polyline->size();
    }
    _point_count.polyline = result.vbo_data.size() / 2;
    _shape_index_count.polyline = result.ibo_data.size();
    return result;
}

Canvas::VBOData Canvas::refresh_polygon_vbo(const bool flush)
{
    VBOData result;
    _visible_objects[0].polygon = _visible_objects[1].polygon;
    _visible_objects[1].polygon.clear();
    for (Geo::Geometry *geo : _editor.visible_objects())
    {
        if (geo->type() == Geo::Type::POLYGON)
        {
            _visible_objects[1].polygon.push_back(static_cast<Geo::Polygon *>(geo));
        }
        else if (geo->type() == Geo::Type::COMBINATION)
        {
            for (Geo::Geometry *child : *static_cast<Combination *>(geo))
            {
                if (child->type() == Geo::Type::POLYGON)
                {
                    _visible_objects[1].polygon.push_back(static_cast<Geo::Polygon *>(child));
                }
            }
        }
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select && !flush &&
        _visible_objects[0].polygon == _visible_objects[1].polygon)
    {
        return result;
    }

    for (Geo::Polygon *polygon : _visible_objects[1].polygon)
    {
        polygon->point_index = result.vbo_data.size() / 2;
        for (const Geo::Point &point : *polygon)
        {
            result.ibo_data.push_back(result.vbo_data.size() / 2);
            result.vbo_data.push_back(point.x);
            result.vbo_data.push_back(point.y);
        }
        result.ibo_data.push_back(UINT_MAX);
        polygon->point_count = polygon->size();
    }
    _point_count.polygon = result.vbo_data.size() / 2;
    _shape_index_count.polygon = result.ibo_data.size();
    return result;
}

Canvas::VBOData Canvas::refresh_circle_vbo(const bool flush)
{
    VBOData result;
    _visible_objects[0].circle = _visible_objects[1].circle;
    _visible_objects[1].circle.clear();
    for (Geo::Geometry *geo : _editor.visible_objects())
    {
        if (geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ARC || geo->type() == Geo::Type::ELLIPSE)
        {
            _visible_objects[1].circle.push_back(geo);
        }
        else if (geo->type() == Geo::Type::COMBINATION)
        {
            for (Geo::Geometry *child : *static_cast<Combination *>(geo))
            {
                if (child->type() == Geo::Type::CIRCLE || child->type() == Geo::Type::ARC || child->type() == Geo::Type::ELLIPSE)
                {
                    _visible_objects[1].circle.push_back(child);
                }
            }
        }
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select && !flush &&
        _visible_objects[0].circle == _visible_objects[1].circle)
    {
        return result;
    }

    for (Geo::Geometry *item : _visible_objects[1].circle)
    {
        switch (item->type())
        {
        case Geo::Type::CIRCLE:
            {
                Geo::Circle *circle = static_cast<Geo::Circle *>(item);
                circle->point_index = result.vbo_data.size() / 2;
                for (const Geo::Point &point : circle->shape())
                {
                    result.ibo_data.push_back(result.vbo_data.size() / 2);
                    result.vbo_data.push_back(point.x);
                    result.vbo_data.push_back(point.y);
                }
                result.ibo_data.push_back(UINT_MAX);
                circle->point_count = circle->shape().size();
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                Geo::Ellipse *ellipse = static_cast<Geo::Ellipse *>(item);
                ellipse->point_index = result.vbo_data.size() / 2;
                for (const Geo::Point &point : ellipse->shape())
                {
                    result.ibo_data.push_back(result.vbo_data.size() / 2);
                    result.vbo_data.push_back(point.x);
                    result.vbo_data.push_back(point.y);
                }
                result.ibo_data.push_back(UINT_MAX);
                ellipse->point_count = ellipse->shape().size();
            }
            break;
        case Geo::Type::ARC:
            {
                Geo::Arc *arc = static_cast<Geo::Arc *>(item);
                arc->point_index = result.vbo_data.size() / 2;
                for (const Geo::Point &point : arc->shape())
                {
                    result.ibo_data.push_back(result.vbo_data.size() / 2);
                    result.vbo_data.push_back(point.x);
                    result.vbo_data.push_back(point.y);
                }
                result.ibo_data.push_back(UINT_MAX);
                arc->point_count = arc->shape().size();
            }
            break;
        default:
            break;
        }
    }

    _shape_index_count.circle = result.ibo_data.size();
    return result;
}

Canvas::VBOData Canvas::refresh_curve_vbo(const bool flush)
{
    VBOData result;
    _visible_objects[0].curve = _visible_objects[1].curve;
    _visible_objects[1].curve.clear();
    for (Geo::Geometry *geo : _editor.visible_objects())
    {
        if (geo->type() == Geo::Type::BEZIER || geo->type() == Geo::Type::BSPLINE)
        {
            _visible_objects[1].curve.push_back(geo);
        }
        else if (geo->type() == Geo::Type::COMBINATION)
        {
            for (Geo::Geometry *child : *static_cast<Combination *>(geo))
            {
                if (child->type() == Geo::Type::BEZIER || child->type() == Geo::Type::BSPLINE)
                {
                    _visible_objects[1].curve.push_back(child);
                }
            }
        }
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select && !flush &&
        _visible_objects[0].curve == _visible_objects[1].curve)
    {
        return result;
    }

    for (Geo::Geometry *item : _visible_objects[1].curve)
    {
        switch (item->type())
        {
        case Geo::Type::BEZIER:
            {
                Geo::CubicBezier *bezier = static_cast<Geo::CubicBezier *>(item);
                bezier->point_index = result.vbo_data.size() / 2;
                for (const Geo::Point &point : bezier->shape())
                {
                    result.ibo_data.push_back(result.vbo_data.size() / 2);
                    result.vbo_data.push_back(point.x);
                    result.vbo_data.push_back(point.y);
                }
                result.ibo_data.push_back(UINT_MAX);
                bezier->point_count = bezier->shape().size();
            }
            break;
        case Geo::Type::BSPLINE:
            {
                Geo::BSpline *bspline = static_cast<Geo::BSpline *>(item);
                bspline->point_index = result.vbo_data.size() / 2;
                for (const Geo::Point &point : bspline->shape())
                {
                    result.ibo_data.push_back(result.vbo_data.size() / 2);
                    result.vbo_data.push_back(point.x);
                    result.vbo_data.push_back(point.y);
                }
                result.ibo_data.push_back(UINT_MAX);
                bspline->point_count = bspline->shape().size();
            }
            break;
        default:
            break;
        }
    }

    _shape_index_count.curve = result.ibo_data.size();
    return result;
}

Canvas::VBOData Canvas::refresh_point_vbo(const bool flush)
{
    VBOData result;
    Geo::AABBRectParams visible_area_params;
    visible_area_params.left = _visible_area.left() - 2;
    visible_area_params.right = _visible_area.right() + 2;
    visible_area_params.bottom = _visible_area.bottom() - 2;
    visible_area_params.top = _visible_area.top() + 2;
    _visible_objects[0].point = _visible_objects[1].point;
    _visible_objects[1].point.clear();

    for (const ContainerGroup &group : _editor.graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            if (geo->type() == Geo::Type::POINT)
            {
                if (Geo::Point *point = static_cast<Geo::Point *>(geo); Geo::is_inside(*point, visible_area_params))
                {
                    _visible_objects[1].point.push_back(point);
                }
            }
            else if (geo->type() == Geo::Type::COMBINATION)
            {
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
                {
                    if (item->type() == Geo::Type::POINT)
                    {
                        if (Geo::Point *point = static_cast<Geo::Point *>(item); Geo::is_inside(*point, visible_area_params))
                        {
                            _visible_objects[1].point.push_back(point);
                        }
                    }
                }
            }
        }
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select && !flush &&
        _visible_objects[0].point == _visible_objects[1].point)
    {
        return result;
    }

    unsigned int index = 0;
    for (Geo::Point *point : _visible_objects[1].point)
    {
        point->point_index = index++;
        point->point_count = 1;
        result.vbo_data.push_back(point->x);
        result.vbo_data.push_back(point->y);
    }

    _point_count.point = result.vbo_data.size() / 2;
    return result;
}

Canvas::VBOData Canvas::refresh_circle_printable_points()
{
    VBOData result;
    std::vector<Geo::Geometry *> output;
    for (Geo::Geometry *geo : _editor.visible_objects())
    {
        if (geo->type() == Geo::Type::CIRCLE || geo->type() == Geo::Type::ELLIPSE || geo->type() == Geo::Type::ARC)
        {
            output.push_back(geo);
        }
        else if (geo->type() == Geo::Type::COMBINATION)
        {
            for (Geo::Geometry *child : *static_cast<Combination *>(geo))
            {
                if (child->type() == Geo::Type::CIRCLE || child->type() == Geo::Type::ELLIPSE || child->type() == Geo::Type::ARC)
                {
                    output.push_back(child);
                }
            }
        }
    }

    for (const Geo::Geometry *geo : output)
    {
        switch (geo->type())
        {
        case Geo::Type::CIRCLE:
            {
                const Geo::Circle *circle = static_cast<const Geo::Circle *>(geo);
                result.vbo_data.push_back(circle->x);
                result.vbo_data.push_back(circle->y);
                result.vbo_data.push_back(circle->x - circle->radius);
                result.vbo_data.push_back(circle->y);
                result.vbo_data.push_back(circle->x);
                result.vbo_data.push_back(circle->y + circle->radius);
                result.vbo_data.push_back(circle->x + circle->radius);
                result.vbo_data.push_back(circle->y);
                result.vbo_data.push_back(circle->x);
                result.vbo_data.push_back(circle->y - circle->radius);
            }
            break;
        case Geo::Type::ELLIPSE:
            if (const Geo::Ellipse *ellipse = static_cast<const Geo::Ellipse *>(geo); ellipse->is_arc())
            {
                const Geo::Point point0(ellipse->arc_point0());
                result.vbo_data.push_back(point0.x);
                result.vbo_data.push_back(point0.y);
                const Geo::Point point1(ellipse->arc_point1());
                result.vbo_data.push_back(point1.x);
                result.vbo_data.push_back(point1.y);
            }
            else
            {
                result.vbo_data.push_back((ellipse->a0().x + ellipse->a1().x + ellipse->b0().x + ellipse->b1().x) / 4);
                result.vbo_data.push_back((ellipse->a0().y + ellipse->a1().y + ellipse->b0().y + ellipse->b1().y) / 4);
                result.vbo_data.push_back(ellipse->a0().x);
                result.vbo_data.push_back(ellipse->a0().y);
                result.vbo_data.push_back(ellipse->a1().x);
                result.vbo_data.push_back(ellipse->a1().y);
                result.vbo_data.push_back(ellipse->b0().x);
                result.vbo_data.push_back(ellipse->b0().y);
                result.vbo_data.push_back(ellipse->b1().x);
                result.vbo_data.push_back(ellipse->b1().y);
            }
            break;
        case Geo::Type::ARC:
            for (const Geo::Point &point : static_cast<const Geo::Arc *>(geo)->control_points)
            {
                result.vbo_data.push_back(point.x);
                result.vbo_data.push_back(point.y);
            }
            break;
        default:
            break;
        }
    }

    _point_count.circle = result.vbo_data.size() / 2;
    return result;
}

Canvas::VBOData Canvas::refresh_curve_printable_points()
{
    VBOData result;
    Geo::AABBRectParams visible_area_params;
    visible_area_params.left = _visible_area.left() - 2;
    visible_area_params.right = _visible_area.right() + 2;
    visible_area_params.top = _visible_area.top() + 2;
    visible_area_params.bottom = _visible_area.bottom() - 2;

    for (ContainerGroup &group : _editor.graph()->container_groups())
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
                        for (const Geo::Point &point : bspline->path_points)
                        {
                            if (Geo::is_inside(point, visible_area_params))
                            {
                                result.vbo_data.push_back(point.x);
                                result.vbo_data.push_back(point.y);
                            }
                        }
                    }
                }
                break;
            case Geo::Type::BSPLINE:
                for (const Geo::Point &point : static_cast<const Geo::BSpline *>(geo)->path_points)
                {
                    if (Geo::is_inside(point, visible_area_params))
                    {
                        result.vbo_data.push_back(point.x);
                        result.vbo_data.push_back(point.y);
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    _point_count.curve = result.vbo_data.size() / 2;
    return result;
}


void Canvas::refresh_select_rect(const double x0, const double y0, const double x1, const double y1)
{
    _select_rect[0] = x0, _select_rect[1] = y0;
    _select_rect[2] = x1, _select_rect[3] = y0;
    _select_rect[4] = x1, _select_rect[5] = y1;
    _select_rect[6] = x0, _select_rect[7] = y1;
}

void Canvas::refresh_selected_ibo()
{
    std::vector<unsigned int> polyline_indexs, polygon_indexs, circle_indexs, curve_indexs, point_indexs;
    for (const Geo::Geometry *geo : _editor.visible_objects())
    {
        if (!geo->is_selected)
        {
            continue;
        }
        switch (geo->type())
        {
        case Geo::Type::POLYLINE:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polyline_indexs.push_back(index++);
            }
            polyline_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::POLYGON:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polygon_indexs.push_back(index++);
            }
            polygon_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                circle_indexs.push_back(index++);
            }
            circle_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                curve_indexs.push_back(index++);
            }
            curve_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
            {
                switch (item->type())
                {
                case Geo::Type::POLYLINE:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polyline_indexs.push_back(index++);
                    }
                    polyline_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::POLYGON:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polygon_indexs.push_back(index++);
                    }
                    polygon_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        circle_indexs.push_back(index++);
                    }
                    circle_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        curve_indexs.push_back(index++);
                    }
                    curve_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::POINT:
                    point_indexs.push_back(item->point_index);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::POINT:
            point_indexs.push_back(geo->point_index);
            break;
        default:
            continue;
        }
    }

    _selected_index_count.polyline = polyline_indexs.size();
    _selected_index_count.polygon = polygon_indexs.size();
    _selected_index_count.circle = circle_indexs.size();
    _selected_index_count.curve = curve_indexs.size();
    _selected_index_count.point = point_indexs.size();

    makeCurrent();
    if (!polyline_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_indexs.size() * sizeof(unsigned int), polyline_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!polygon_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_indexs.size() * sizeof(unsigned int), polygon_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!circle_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_indexs.size() * sizeof(unsigned int), circle_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!curve_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_indexs.size() * sizeof(unsigned int), curve_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!point_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_indexs.size() * sizeof(unsigned int), point_indexs.data(), GL_DYNAMIC_DRAW);
    }
    doneCurrent();
}

void Canvas::refresh_selected_ibo(const Geo::Geometry *object)
{
    {
        Geo::AABBRectParams visible_area_params;
        visible_area_params.left = _visible_area.left() - 2;
        visible_area_params.right = _visible_area.right() + 2;
        visible_area_params.top = _visible_area.top() + 2;
        visible_area_params.bottom = _visible_area.bottom() - 2;
        if (!Geo::is_intersected(visible_area_params, object->aabbrect_params()))
        {
            return;
        }
    }
    if (object->type() == Geo::Type::COMBINATION)
    {
        std::vector<unsigned int> polyline_indexs, polygon_indexs, circle_indexs, curve_indexs, point_indexs;
        for (const Geo::Geometry *item : *static_cast<const Combination *>(object))
        {
            switch (item->type())
            {
            case Geo::Type::POLYLINE:
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    polyline_indexs.push_back(index++);
                }
                polyline_indexs.push_back(UINT_MAX);
                break;
            case Geo::Type::POLYGON:
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    polygon_indexs.push_back(index++);
                }
                polygon_indexs.push_back(UINT_MAX);
                break;
            case Geo::Type::CIRCLE:
            case Geo::Type::ELLIPSE:
            case Geo::Type::ARC:
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    circle_indexs.push_back(index++);
                }
                circle_indexs.push_back(UINT_MAX);
                break;
            case Geo::Type::BEZIER:
            case Geo::Type::BSPLINE:
                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                {
                    curve_indexs.push_back(index++);
                }
                curve_indexs.push_back(UINT_MAX);
                break;
            case Geo::Type::POINT:
                point_indexs.push_back(item->point_index);
                break;
            default:
                break;
            }
        }

        _selected_index_count.polyline = polyline_indexs.size();
        _selected_index_count.polygon = polygon_indexs.size();
        _selected_index_count.circle = circle_indexs.size();
        _selected_index_count.curve = curve_indexs.size();
        _selected_index_count.point = point_indexs.size();
        makeCurrent();
        if (!polyline_indexs.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_indexs.size() * sizeof(unsigned int), polyline_indexs.data(), GL_DYNAMIC_DRAW);
        }
        if (!polygon_indexs.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_indexs.size() * sizeof(unsigned int), polygon_indexs.data(), GL_DYNAMIC_DRAW);
        }
        if (!circle_indexs.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_indexs.size() * sizeof(unsigned int), circle_indexs.data(), GL_DYNAMIC_DRAW);
        }
        if (!curve_indexs.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_indexs.size() * sizeof(unsigned int), curve_indexs.data(), GL_DYNAMIC_DRAW);
        }
        if (!point_indexs.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_indexs.size() * sizeof(unsigned int), point_indexs.data(), GL_DYNAMIC_DRAW);
        }
        doneCurrent();
    }
    else
    {
        std::vector<unsigned int> indexs;
        for (size_t i = 0, index = object->point_index, count = object->point_count; i < count; ++i)
        {
            indexs.push_back(index++);
        }
        indexs.push_back(UINT_MAX);
        clear_selected_ibo();
        unsigned int IBO_index = 0;
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            IBO_index = _selected_ibo.polyline;
            _selected_index_count.polyline = indexs.size();
            break;
        case Geo::Type::POLYGON:
            IBO_index = _selected_ibo.polygon;
            _selected_index_count.polygon = indexs.size();
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            IBO_index = _selected_ibo.circle;
            _selected_index_count.circle = indexs.size();
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            IBO_index = _selected_ibo.curve;
            _selected_index_count.curve = indexs.size();
            break;
        case Geo::Type::POINT:
            IBO_index = _selected_ibo.point;
            _selected_index_count.point = indexs.size();
            break;
        default:
            break;
        }
        if (!indexs.empty())
        {
            makeCurrent();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_index);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexs.size() * sizeof(unsigned int), indexs.data(), GL_DYNAMIC_DRAW);
            doneCurrent();
        }
    }
}

void Canvas::refresh_selected_ibo(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        return;
    }

    Geo::AABBRectParams visible_area_params;
    visible_area_params.left = _visible_area.left() - 2;
    visible_area_params.right = _visible_area.right() + 2;
    visible_area_params.top = _visible_area.top() + 2;
    visible_area_params.bottom = _visible_area.bottom() - 2;
    std::vector<unsigned int> polyline_indexs, polygon_indexs, circle_indexs, curve_indexs, point_indexs;
    for (const Geo::Geometry *geo : objects)
    {
        if (!Geo::is_intersected(visible_area_params, geo->aabbrect_params()))
        {
            continue;
        }
        switch (geo->type())
        {
        case Geo::Type::POLYLINE:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polyline_indexs.push_back(index++);
            }
            polyline_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::POLYGON:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                polygon_indexs.push_back(index++);
            }
            polygon_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
        case Geo::Type::ARC:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                circle_indexs.push_back(index++);
            }
            circle_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::BEZIER:
        case Geo::Type::BSPLINE:
            for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
            {
                curve_indexs.push_back(index++);
            }
            curve_indexs.push_back(UINT_MAX);
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
            {
                switch (item->type())
                {
                case Geo::Type::POLYLINE:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polyline_indexs.push_back(index++);
                    }
                    polyline_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::POLYGON:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        polygon_indexs.push_back(index++);
                    }
                    polygon_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        circle_indexs.push_back(index++);
                    }
                    circle_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                    {
                        curve_indexs.push_back(index++);
                    }
                    curve_indexs.push_back(UINT_MAX);
                    break;
                case Geo::Type::POINT:
                    point_indexs.push_back(item->point_index);
                    break;
                default:
                    break;
                }
            }
            break;
        case Geo::Type::POINT:
            point_indexs.push_back(geo->point_index);
            break;
        default:
            continue;
        }
    }

    _selected_index_count.polyline = polyline_indexs.size();
    _selected_index_count.polygon = polygon_indexs.size();
    _selected_index_count.circle = circle_indexs.size();
    _selected_index_count.curve = curve_indexs.size();
    _selected_index_count.point = point_indexs.size();

    makeCurrent();
    if (!polyline_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polyline);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_indexs.size() * sizeof(unsigned int), polyline_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!polygon_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.polygon);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_indexs.size() * sizeof(unsigned int), polygon_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!circle_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.circle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_indexs.size() * sizeof(unsigned int), circle_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!curve_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.curve);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_indexs.size() * sizeof(unsigned int), curve_indexs.data(), GL_DYNAMIC_DRAW);
    }
    if (!point_indexs.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_ibo.point);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, point_indexs.size() * sizeof(unsigned int), point_indexs.data(), GL_DYNAMIC_DRAW);
    }
    doneCurrent();
}

void Canvas::refresh_selected_vbo()
{
    std::future<VBOData> polyline_vbo, polygon_vbo, circle_vbo, curve_vbo, circle_point, curve_point, point_vbo;
    bool refresh[5] = {false, false, false, false, false};
    for (const ContainerGroup &group : _editor.graph()->container_groups())
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
                        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this, true);
                        refresh[0] = true;
                    }
                    break;
                case Geo::Type::POLYGON:
                    if (!refresh[1])
                    {
                        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this, true);
                        refresh[1] = true;
                    }
                    break;
                case Geo::Type::CIRCLE:
                case Geo::Type::ELLIPSE:
                case Geo::Type::ARC:
                    if (!refresh[2])
                    {
                        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this, true);
                        circle_point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
                        refresh[2] = true;
                    }
                    break;
                case Geo::Type::BEZIER:
                case Geo::Type::BSPLINE:
                    if (!refresh[3])
                    {
                        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this, true);
                        curve_point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
                        refresh[3] = true;
                    }
                    break;
                case Geo::Type::COMBINATION:
                    for (const Geo::Geometry *item : *static_cast<const Combination *>(object))
                    {
                        switch (item->type())
                        {
                        case Geo::Type::POLYLINE:
                            if (!refresh[0])
                            {
                                polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this, true);
                                refresh[0] = true;
                            }
                            break;
                        case Geo::Type::POLYGON:
                            if (!refresh[1])
                            {
                                polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this, true);
                                refresh[1] = true;
                            }
                            break;
                        case Geo::Type::CIRCLE:
                        case Geo::Type::ELLIPSE:
                        case Geo::Type::ARC:
                            if (!refresh[2])
                            {
                                circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this, true);
                                circle_point = std::async(std::launch::async, &Canvas::refresh_circle_printable_points, this);
                                refresh[2] = true;
                            }
                            break;
                        case Geo::Type::BEZIER:
                        case Geo::Type::BSPLINE:
                            if (!refresh[3])
                            {
                                curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this, true);
                                curve_point = std::async(std::launch::async, &Canvas::refresh_curve_printable_points, this);
                                refresh[3] = true;
                            }
                            break;
                        case Geo::Type::POINT:
                            if (!refresh[4])
                            {
                                point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this, true);
                                refresh[4] = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case Geo::Type::POINT:
                    if (!refresh[4])
                    {
                        point_vbo = std::async(std::launch::async, &Canvas::refresh_point_vbo, this, true);
                        refresh[4] = true;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    makeCurrent();
    if (refresh[4])
    {
        point_vbo.wait();
        VBOData data = point_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.point); // point
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }
    if (refresh[2])
    {
        circle_point.wait();
        VBOData data = circle_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle_printable_points); // circle printable points
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);

        circle_vbo.wait();
        data = circle_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.circle); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }
    if (refresh[3])
    {
        curve_point.wait();
        VBOData data = curve_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve_printable_points); // curve printable points
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);

        curve_vbo.wait();
        data = curve_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.curve); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }
    if (refresh[0])
    {
        polyline_vbo.wait();
        VBOData data = polyline_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polyline); // polyline
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }
    if (refresh[1])
    {
        polygon_vbo.wait();
        VBOData data = polygon_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_vbo.polygon); // polygon
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data.vbo_data.size(), data.vbo_data.data(), GL_DYNAMIC_DRAW);
    }
    doneCurrent();
}

void Canvas::clear_selected_ibo()
{
    _selected_index_count.polyline = _selected_index_count.polygon = _selected_index_count.circle = _selected_index_count.curve =
        _selected_index_count.point = 0;
}

void Canvas::paint_text()
{
    _texture.image.fill(Qt::GlobalColor::transparent);
    const QColor white(255, 255, 255), red(255, 0, 0);
    QPainter painter(&_texture.image);
    // painter.setRenderHint(QPainter::RenderHint::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode::CompositionMode_Source);

    for (const ContainerGroup &group : _editor.graph()->container_groups())
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
                {
                    Text *text = static_cast<Text *>(geo);
                    if (text->text().isEmpty() ||
                        !Geo::is_intersected(_visible_area, text->shape(0), text->shape(1), text->shape(2), text->shape(3)))
                    {
                        continue;
                    }
                    const double x = text->shape(3).x * _canvas_ctm[0] + text->shape(3).y * _canvas_ctm[3] + _canvas_ctm[6];
                    const double y = text->shape(3).x * _canvas_ctm[1] + text->shape(3).y * _canvas_ctm[4] + _canvas_ctm[7];
                    painter.save();
                    painter.setPen(text->is_selected ? red : white);
                    painter.translate(x, y);
                    painter.rotate(-Geo::rad_to_degree(text->angle()));
                    painter.scale(_ratio, _ratio);
                    text->paint(painter);
                    painter.restore();
                }
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    if (Text *text = dynamic_cast<Text *>(item); text != nullptr)
                    {
                        if (text->text().isEmpty() ||
                            !Geo::is_intersected(_visible_area, text->shape(0), text->shape(1), text->shape(2), text->shape(3)))
                        {
                            continue;
                        }
                        const double x = text->shape(3).x * _canvas_ctm[0] + text->shape(3).y * _canvas_ctm[3] + _canvas_ctm[6];
                        const double y = text->shape(3).x * _canvas_ctm[1] + text->shape(3).y * _canvas_ctm[4] + _canvas_ctm[7];
                        painter.save();
                        painter.setPen(text->is_selected ? red : white);
                        painter.translate(x, y);
                        painter.rotate(-Geo::rad_to_degree(text->angle()));
                        painter.scale(_ratio, _ratio);
                        text->paint(painter);
                        painter.restore();
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    _texture.image.mirror(false, true);
}


bool Canvas::refresh_catached_points(const double x, const double y, const double distance,
                                     std::vector<const Geo::Geometry *> &catched_objects, const bool skip_selected,
                                     const bool current_group_only) const
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
        std::vector<Geo::Geometry *> objects;
        std::vector<Geo::Geometry *> current_group_objects(_editor.graph()->container_group(_editor.current_group()).begin(),
                                                           _editor.graph()->container_group(_editor.current_group()).end());
        std::sort(current_group_objects.begin(), current_group_objects.end());
        std::set_intersection(_editor.visible_objects().begin(), _editor.visible_objects().end(), current_group_objects.begin(),
                              current_group_objects.end(), std::back_inserter(objects));
        for (const Geo::Geometry *geo : objects)
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
                    std::abs(Geo::distance(pos, *static_cast<const Geo::Circle *>(geo)) - static_cast<const Geo::Circle *>(geo)->radius) *
                            _ratio <
                        distance)
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
                    if (Geo::distance(pos, static_cast<const Geo::CubicBezier *>(geo)->shape()) < distance)
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
        for (const Geo::Geometry *geo : _editor.visible_objects())
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
                    std::abs(Geo::distance(pos, *static_cast<const Geo::Circle *>(geo)) -
                                static_cast<const Geo::Circle *>(geo)->radius) *
                            _ratio <
                        distance)
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

    return catched_objects.size() > count;
}

bool Canvas::refresh_catchline_points(const std::vector<const Geo::Geometry *> &objects, const double distance, Geo::Point &pos)
{
    const CanvasOperations::Tool tool = CanvasOperations::CanvasOperation::tool[0];
    const bool catch_flags[Canvas::catch_count] = {
        _catch_types.vertex, _catch_types.center,
        _catch_types.foot && (tool > CanvasOperations::Tool::Move && tool < CanvasOperations::Tool::Mirror),
        _catch_types.tangency && (tool > CanvasOperations::Tool::Move && tool < CanvasOperations::Tool::Mirror), _catch_types.intersection};
    if (!std::any_of(catch_flags, catch_flags + Canvas::catch_count, [](const bool v) { return v; }))
    {
        return false;
    }

    Geo::Point result[Canvas::catch_count]; // Vertex, Center, Foot, Tangency, Intersection
    double dis[Canvas::catch_count] = {DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX};
    const Geo::Point press_pos(CanvasOperations::CanvasOperation::press_pos[0], CanvasOperations::CanvasOperation::press_pos[1]);
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
                            if (Geo::Point point;
                                Geo::is_intersected(polyline[i - 1], polyline[i], polyline[j - 1], polyline[j], point, false))
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
                        result[0].x = c->x;
                        result[0].y = c->y;
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
                const Geo::CubicBezier &bezier = *static_cast<const Geo::CubicBezier *>(object);
                if (catch_flags[0])
                {
                    if (double dis0 = Geo::distance(pos, bezier.front()), dis1 = Geo::distance(pos, bezier.back()); dis0 <= dis1)
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

    if (_editor.point_cache().size() > 2)
    {
        if (const double d = Geo::distance(pos, _editor.point_cache().front()); catch_flags[0] && d < dis[0])
        {
            dis[0] = d;
            result[0] = _editor.point_cache().front();
        }
        for (size_t i = 1, count = _editor.point_cache().size() - 1; i < count; ++i)
        {
            if (const double d = Geo::distance(pos, _editor.point_cache()[i]); catch_flags[0] && d < dis[0])
            {
                result[0] = _editor.point_cache()[i];
                dis[0] = d;
            }
            const Geo::Point center((_editor.point_cache()[i - 1] + _editor.point_cache()[i]) / 2);
            if (const double d = Geo::distance(pos, center); catch_flags[1] && d < dis[1])
            {
                dis[1] = d;
                result[1] = center;
            }
            if (Geo::Point foot; catch_flags[2] && Geo::foot_point(_editor.point_cache()[i - 1], _editor.point_cache()[i], press_pos, foot))
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
            for (size_t i = 1, count = _editor.point_cache().size() - 1; i < count; ++i)
            {
                if (Geo::distance(pos, _editor.point_cache()[i - 1], _editor.point_cache()[i], false) > distance)
                {
                    continue;
                }
                for (size_t j = i + 2; j < count; ++j)
                {
                    if (Geo::Point point; Geo::is_intersected(_editor.point_cache()[i - 1], _editor.point_cache()[i],
                                                              _editor.point_cache()[j - 1], _editor.point_cache()[j], point, false))
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

    if (std::all_of(dis, dis + Canvas::catch_count, [this, distance](const double d) { return d > distance / _ratio; }))
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
            _catchline_points[2] = pos.x + w, _catchline_points[3] = pos.y + w;

            _catchline_points[4] = pos.x + w, _catchline_points[5] = pos.y + w;
            _catchline_points[6] = pos.x + w, _catchline_points[7] = pos.y - w;

            _catchline_points[8] = pos.x + w, _catchline_points[9] = pos.y - w;
            _catchline_points[10] = pos.x - w, _catchline_points[11] = pos.y - w;

            _catchline_points[12] = pos.x - w, _catchline_points[13] = pos.y - w;
            _catchline_points[14] = pos.x - w, _catchline_points[15] = pos.y + w;
        }
        break;
    case CatchedPointType::Center:
        {
            const double w = 4.8 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y - w;
            _catchline_points[2] = pos.x, _catchline_points[3] = pos.y + w * 2.5;

            _catchline_points[4] = pos.x, _catchline_points[5] = pos.y + w * 2.5;
            _catchline_points[6] = pos.x + w, _catchline_points[7] = pos.y - w;

            _catchline_points[8] = pos.x + w, _catchline_points[9] = pos.y - w;
            _catchline_points[10] = pos.x - w, _catchline_points[11] = pos.y - w;

            _catchline_points[12] = pos.x - w, _catchline_points[13] = pos.y - w;
            _catchline_points[14] = pos.x, _catchline_points[15] = pos.y + w * 2.5;
        }
        break;
    case CatchedPointType::Foot:
        {
            const double w = 6 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y;
            _catchline_points[2] = pos.x, _catchline_points[3] = pos.y;

            _catchline_points[4] = pos.x, _catchline_points[5] = pos.y;
            _catchline_points[6] = pos.x, _catchline_points[7] = pos.y - w;

            _catchline_points[8] = pos.x + w * 1.2, _catchline_points[9] = pos.y - w;
            _catchline_points[10] = pos.x - w, _catchline_points[11] = pos.y - w;

            _catchline_points[12] = pos.x - w, _catchline_points[13] = pos.y - w;
            _catchline_points[14] = pos.x - w, _catchline_points[15] = pos.y + w * 1.2;
        }
        break;
    case CatchedPointType::Tangency:
        {
            const double w = 8 / _ratio, h = 3 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y + h;
            _catchline_points[2] = pos.x - h, _catchline_points[3] = pos.y + w;

            _catchline_points[4] = pos.x + h, _catchline_points[5] = pos.y + w;
            _catchline_points[6] = pos.x + w, _catchline_points[7] = pos.y + h;

            _catchline_points[8] = pos.x + w, _catchline_points[9] = pos.y - h;
            _catchline_points[10] = pos.x + h, _catchline_points[11] = pos.y - w;

            _catchline_points[12] = pos.x - h, _catchline_points[13] = pos.y - w;
            _catchline_points[14] = pos.x - w, _catchline_points[15] = pos.y - h;
        }
        break;
    case Canvas::CatchedPointType::Intersection:
        {
            const double w = 7 / _ratio;
            _catchline_points[0] = pos.x - w, _catchline_points[1] = pos.y + w;
            _catchline_points[2] = pos.x + w, _catchline_points[3] = pos.y - w;

            _catchline_points[4] = pos.x - w, _catchline_points[5] = pos.y + w;
            _catchline_points[6] = pos.x + w, _catchline_points[7] = pos.y - w;

            _catchline_points[8] = pos.x + w, _catchline_points[9] = pos.y + w;
            _catchline_points[10] = pos.x - w, _catchline_points[11] = pos.y - w;

            _catchline_points[12] = pos.x + w, _catchline_points[13] = pos.y + w;
            _catchline_points[14] = pos.x - w, _catchline_points[15] = pos.y - w;
        }
        break;
    default:
        break;
    }

    return true;
}
