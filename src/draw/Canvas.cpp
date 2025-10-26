#include <future>
#include <QPainterPath>

#include "base/Algorithm.hpp"
#include "draw/Canvas.hpp"
#include "draw/GLSL.hpp"
#include "io/GlobalSetting.hpp"


Canvas::Canvas(QWidget *parent)
    : QOpenGLWidget(parent), _input_line(this)
{
    init();
}

Canvas::~Canvas()
{
    delete []_cache;
    delete _menu;
    delete _up;
    delete _down;
}




void Canvas::init()
{
    _cache = new double[_cache_len];
    _input_line.hide();
    _select_rect.clear();

    _menu = new QMenu(this);
    _up = new QAction("Up");
    _down = new QAction("Down");
    _menu->addAction(_up);
    _menu->addAction(_down);
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
}




void Canvas::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.117647f, 0.156862f, 0.188235f, 1.0f);

    unsigned int vertex_shader;
    unsigned int fragment_shader;

    glPointSize(9.6f); // 点大小
    glLineWidth(1.4f); // 线宽
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    // glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 抗锯齿
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

    int maxUniformBlockSize;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &maxUniformBlockSize);

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
    _uniforms[0] = glGetUniformLocation(_shader_program, "w");
    _uniforms[1] = glGetUniformLocation(_shader_program, "h");
    _uniforms[2] = glGetUniformLocation(_shader_program, "vec0");
    _uniforms[3] = glGetUniformLocation(_shader_program, "vec1");
    _uniforms[4] = glGetUniformLocation(_shader_program, "color");
    const bool show_control_points = _editer->selected_count() == 1;

    glUseProgram(_shader_program);
    glUniform3d(_uniforms[2], 1.0, 0.0, 0.0); // vec0
    glUniform3d(_uniforms[3], 0.0, -1.0, 0.0); // vec1

    glCreateVertexArrays(1, &_VAO);
    glCreateBuffers(4, _base_VBO);
    glCreateBuffers(7, _shape_VBO);
    glCreateBuffers(4, _shape_IBO);
    // glCreateBuffers(3, _brush_IBO);
    glCreateBuffers(1, &_text_brush_IBO);
    glCreateBuffers(4, _selected_IBO);

    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[3]); // catcheline points
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), _catchline_points, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[2]); // reflines
    glBufferData(GL_ARRAY_BUFFER, 30 * sizeof(double), _refline_points, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
    glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_STREAM_DRAW);

    double data[24] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[0]); // origin and select rect
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), data, GL_DYNAMIC_DRAW);
}

void Canvas::resizeGL(int w, int h)
{
    glUniform1i(_uniforms[0], w / 2); // w
    glUniform1i(_uniforms[1], h / 2); // h
    glViewport(0, 0, w, h);

    _canvas_ctm[7] += (h - _canvas_height);
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    _view_ctm[7] += (h - _canvas_height) / _ratio;
    _canvas_width = w, _canvas_height = h;

    _visible_area = Geo::AABBRect(0, 0, w, h);
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);    
}

void Canvas::paintGL()
{
    glUseProgram(_shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_shape_index_count[0] > 0) // polyline
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[0]); // polyline
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count[0], GL_UNSIGNED_INT, NULL);

        if (_selected_index_count[0] > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[0]); // selected
            glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count[0], GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count[1] > 0) // polygon
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[1]); // polygon
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count[1], GL_UNSIGNED_INT, NULL);

        if (_selected_index_count[1] > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[1]); // selected
            glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count[1], GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count[2] > 0) // circle
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[2]); // circle
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count[2], GL_UNSIGNED_INT, NULL);

        if (_selected_index_count[2] > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[2]); // selected
            glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count[2], GL_UNSIGNED_INT, NULL);
        }
    }

    if (_shape_index_count[3] > 0) // curve
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[3]); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[3]); // curve
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _shape_index_count[3], GL_UNSIGNED_INT, NULL);

        if (_selected_index_count[3] > 0) // selected
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[3]); // selected
            glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
            glDrawElements(GL_LINE_STRIP, _selected_index_count[3], GL_UNSIGNED_INT, NULL);
        }
    }

    if (GlobalSetting::setting().show_text && _brush_index_count[2] > 0) // text
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[4]); // text
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _text_brush_IBO); // text
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color

        glEnable(GL_STENCIL_TEST); //开启模板测试
        glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT); //设置模板缓冲区更新方式(若通过则按位反转模板值)
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 1); //初始模板位为0，由于一定通过测试，所以全部会被置为1，而重复绘制区域由于画了两次模板位又归0
        glStencilMask(0x1); //开启模板缓冲区写入
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //第一次绘制只是为了构造模板缓冲区，没有必要显示到屏幕上，所以设置不显示第一遍的多边形
        glDrawElements(GL_TRIANGLES, _brush_index_count[2], GL_UNSIGNED_INT, NULL);

        glStencilFunc(GL_NOTEQUAL, 0, 1); //模板值不为0就通过
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0x1);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDrawElements(GL_TRIANGLES, _brush_index_count[2], GL_UNSIGNED_INT, NULL);
        glDisable(GL_STENCIL_TEST); //关闭模板测试
    }

    if (_editer->point_cache().empty() && _cache_count > 0) // cache
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
        if (_AABBRect_cache.empty() && _circle_cache.empty() && _ellipse_cache.empty())
        {
            glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
            glDrawArrays(GL_POINTS, 0, _cache_count / 3);
        }
    }
    else if (_measure_angle_flag > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, 0, 9 * sizeof(double), _cache);
        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_STRIP, 0, 3);
        glLineWidth(1.4f);
    }

    if (!_reflines.empty()) // reflines
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[2]); // reflines
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, _reflines.size() * 2);
        glLineWidth(1.4f);
    }

    if (GlobalSetting::setting().show_points)
    {
        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        if (_point_count[0] > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]); // polyline points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count[0]);
        }
        if (_point_count[1] > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]); // polygon points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count[1]);
        }
        if (_point_count[2] > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count[2]);
        }
        if (_point_count[3] > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[6]); // curve points
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, _point_count[3]);
        }
    }

    if (!_editer->point_cache().empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        if (_tool_flags[0] == Tool::BSpline || _tool_flags[0] == Tool::Bezier)
        {
            glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
        }
        else
        {
            glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color
        }
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
        if (_tool_flags[0] == Tool::BSpline || _tool_flags[0] == Tool::Bezier
            || GlobalSetting::setting().show_points)
        {
            glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
            glDrawArrays(GL_POINTS, 0, _cache_count / 3);
        }
    }
    else if (!_AABBRect_cache.empty() && _AABBRect_cache[0] != _AABBRect_cache[2])
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    else if (!_circle_cache.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, _cache_count / 3);
        _cache_count = 0;
    }
    else if (!_ellipse_cache.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, _cache_count / 3);
        _cache_count = 0;
    }

    if (_bool_flags[8]) // catched point
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[3]); // catched point
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(double), _catchline_points);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 0.649f); // color
        glLineWidth(2.8f);
        glDrawArrays(GL_LINES, 0, 8);
        glLineWidth(1.4f);
        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 0.549f); // color
    }

    if (_bool_flags[7] || (!_select_rect.empty() && _select_rect[0] != _select_rect[2]))
    {
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[0]); // origin and select rect
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        if (_bool_flags[7]) // origin
        {
            glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 画原点
            glDrawArrays(GL_LINES, 0, 4);
        }

        if (!_select_rect.empty() && _select_rect[0] != _select_rect[2])
        {
            glUniform4f(_uniforms[4], 0.0f, 0.47f, 0.843f, 0.1f); // color
            glDrawArrays(GL_POLYGON, 4, 4);

            glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 0.549f); // color
            glDrawArrays(GL_LINE_LOOP, 4, 4);
        }
    }
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    _mouse_press_pos.x = real_x1, _mouse_press_pos.y = real_y1;
    switch (event->button())
    {
    case Qt::LeftButton:
        if (Geo::Point coord; catch_cursor(real_x1, real_y1, coord, _catch_distance, false))
        {
            real_x1 = coord.x, real_y1 = coord.y;
            _mouse_press_pos.x = real_x1, _mouse_press_pos.y = real_y1;
            coord = real_coord_to_view_coord(coord.x, coord.y);
            _mouse_pos_1.setX(coord.x);
            _mouse_pos_1.setY(coord.y);
            QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
        }
        if (is_paintable()) // paintable
        {
            switch (_tool_flags[0])
            {
            case Tool::Circle:
                if (!is_painting()) // not painting
                {
                    _circle_cache = Geo::Circle(real_x1, real_y1, 10);
                    refresh_circle_cache_vbo();
                }
                else
                {
                    _circle_cache.update_shape(Geo::Circle::default_down_sampling_value);
                    _editer->append(_circle_cache);
                    _circle_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NoTool;
                    _bool_flags[1] = false; // moveable
                    emit tool_changed(_tool_flags[0]);
                    refresh_vbo(Geo::Type::CIRCLE, true);
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::Ellipse:
                if (!is_painting()) // not painting
                {
                    _ellipse_cache = Geo::Ellipse(real_x1, real_y1, 10, 10);
                    refresh_ellipse_cache_vbo();
                    _last_point.x = _stored_coord.x = real_x1;
                    _last_point.y = _stored_coord.y = real_y1;
                    _bool_flags[2] = true; // painting
                }
                else
                {
                    if (_last_point == _stored_coord)
                    {
                        _ellipse_cache.set_lengtha(Geo::distance(real_x1, real_y1, _stored_coord.x, _stored_coord.y));
                        if (event->modifiers() != Qt::ControlModifier)
                        {
                            _ellipse_cache.rotate(_stored_coord.x, _stored_coord.y, Geo::angle(_mouse_press_pos, _stored_coord));
                        }
                        _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
                        refresh_ellipse_cache_vbo();
                        _last_point.x += 1;
                    }
                    else
                    {
                        _ellipse_cache.set_lengthb(Geo::distance(real_x1, real_y1, _stored_coord.x, _stored_coord.y));
                        _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
                        _editer->append(_ellipse_cache);
                        _ellipse_cache.clear();
                        _tool_flags[1] = _tool_flags[0];
                        _tool_flags[0] = Tool::NoTool;
                        _bool_flags[1] = false; // moveable
                        emit tool_changed(_tool_flags[0]);
                        refresh_vbo(Geo::Type::ELLIPSE, true);
                        _bool_flags[2] = false;
                    }
                }
                break;
            case Tool::Polyline:
            case Tool::Bezier:
            case Tool::BSpline:
                if (is_painting())
                {
                    _editer->point_cache().emplace_back(real_x1, real_y1);
                    _cache[_cache_count++] = real_x1;
                    _cache[_cache_count++] = real_y1;
                    _cache[_cache_count++] = 0;
                    check_cache();
                    refresh_cache_vbo(3);
                }
                else
                {
                    _editer->point_cache().emplace_back(real_x1, real_y1);
                    _editer->point_cache().emplace_back(real_x1, real_y1);
                    _bool_flags[2] = true; // painting
                    _cache_count = 6;
                    _cache[0] = _cache[3] = real_x1;
                    _cache[1] = _cache[4] = real_y1;
                    _cache[2] = _cache[5] = 0;
                    check_cache();
                    refresh_cache_vbo(0);
                }
                break;
            case Tool::Rect:
                if (!is_painting())
                {
                    _last_point.x = real_x1;
                    _last_point.y = real_y1;
                    _AABBRect_cache = Geo::AABBRect(real_x1, real_y1, real_x1 + 2, real_y1 + 2);
                    refresh_AABBRect_cache_vbo();
                }
                else
                {
                    _editer->append(_AABBRect_cache);
                    _AABBRect_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NoTool;
                    _bool_flags[1] = false; // paintable
                    emit tool_changed(_tool_flags[0]);
                    refresh_vbo(Geo::Type::POLYGON, true);
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::Text:
                _editer->append_text(real_x1, real_y1);
                _tool_flags[0] = Tool::NoTool;
                _bool_flags[1] = _bool_flags[2] = false;
                emit tool_changed(_tool_flags[0]);
                refresh_vbo(Geo::Type::TEXT, true);
                break;
            default:
                break;
            }
            update();
        }
        else
        {
            switch (_operation)
            {
            case Operation::RingArray:
                _operation = Operation::NoOperation;
                if (_editer->ring_array(_object_cache, real_x1, real_y1, GlobalSetting::setting().ui->array_item->value()))
                {
                    std::set<Geo::Type> types;
                    for (const Geo::Geometry *object : _object_cache)
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
                }
                emit tool_changed(Tool::NoTool);
                _object_cache.clear();
                update();
                return QOpenGLWidget::mousePressEvent(event);
            case Operation::NoOperation:
                break;
            case Operation::Rotate:
                switch (_measure_angle_flag)
                {
                case 0:
                    _stored_coord.x = _cache[6] = _cache[3] = _cache[0] = real_x1;
                    _stored_coord.y = _cache[7] = _cache[4] = _cache[1] = real_y1;
                    _cache[8] = _cache[5] = _cache[2] = 0.51;
                    _measure_angle_flag = 1;
                    break;
                case 1:
                    _cache[0] = real_x1;
                    _cache[1] = real_y1;
                    _measure_angle_flag = 2;
                    break;
                case 2:
                    if (std::vector<Geo::Geometry *> objects = _editer->selected(); !objects.empty())
                    {
                        const double angle = Geo::angle(Geo::Point(_cache[0], _cache[1]),
                            _stored_coord, Geo::Point(real_x1, real_y1));
                        for (Geo::Geometry *obj : objects)
                        {
                            obj->rotate(_stored_coord.x, _stored_coord.y, angle);
                        }
                        _measure_angle_flag = 0;
                        refresh_selected_vbo();
                        update();
                    }
                    break;
                default:
                    break;
                }
                return QOpenGLWidget::mousePressEvent(event);
                break;
            default:
                break;
            }

            const bool reset = !(GlobalSetting::setting().multiple_select || event->modifiers() == Qt::ControlModifier);
            if (_clicked_obj = _editer->select(Geo::Point(real_x1, real_y1), reset); _clicked_obj == nullptr)
            {
                _editer->reset_selected_mark();
                std::fill_n(_selected_index_count, 4, 0);
                _cache_count = 0;
                _select_rect = Geo::AABBRect(real_x1, real_y1, real_x1, real_y1);
                _last_point.x = real_x1;
                _last_point.y = real_y1;
                _bool_flags[5] = false; // is obj selected

                switch (_operation)
                {
                case Operation::Mirror:
                    _operation = Operation::NoOperation;
                    emit tool_changed(Tool::NoTool);
                    _object_cache.clear();
                    break;
                case Operation::PolygonDifference:
                    _operation = Operation::NoOperation;
                    emit tool_changed(Tool::NoTool);
                    _object_cache.clear();
                    break;
                default:
                    break;
                }

                switch (_tool_flags[0])
                {
                case Tool::Measure:
                    if (_measure_angle_flag == 0 || _measure_angle_flag == 2)
                    {
                        Geo::Point coord(real_x1, real_y1);
                        catch_point(real_x1, real_y1, coord, 12.0 / _ratio);
                        _cache[2] = _cache[5] = _cache[8] = 0.51;
                        _cache[0] = _cache[3] = _cache[6] = coord.x;
                        _cache[1] = _cache[4] = _cache[7] = coord.y;
                        _measure_angle_flag = 1;
                    }
                    else
                    {
                        Geo::Point coord(real_x1, real_y1);
                        catch_point(real_x1, real_y1, coord, 12.0 / _ratio);
                        _cache[3] = _cache[6] = coord.x;
                        _cache[4] = _cache[7] = coord.y;
                        _info_labels[1]->setText("Length:" +
                            QString::number(Geo::distance(_cache[0], _cache[1],
                                _cache[3], _cache[4])));
                        _measure_angle_flag = 2;
                    }
                    update();
                    return QOpenGLWidget::mousePressEvent(event);
                case Tool::Angle:
                    switch (_measure_angle_flag)
                    {
                    case 1:
                        _measure_angle_flag = 2;
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _cache[3] = _cache[6] = coord.x;
                            _cache[4] = _cache[7] = coord.y;
                        }
                        else
                        {
                            _cache[3] = _cache[6] = real_x1;
                            _cache[4] = _cache[7] = real_y1;
                        }
                        break;
                    case 2:
                        _measure_angle_flag = 3;
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _cache[6] = coord.x;
                            _cache[7] = coord.y;
                        }
                        else
                        {
                            _cache[6] = real_x1;
                            _cache[7] = real_y1;
                        }
                        _info_labels[1]->setText(QString("Angle: %1°").arg(
                            std::abs(Geo::rad_to_degree(Geo::angle(Geo::Point(_cache[0], _cache[1]),
                            Geo::Point(_cache[3], _cache[4]), Geo::Point(_cache[6], _cache[7]))))));
                        break;
                    default:
                        _measure_angle_flag = 1;
                        _cache[2] = _cache[5] = _cache[8] = 0.51;
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _cache[0] = _cache[3] = _cache[6] = coord.x;
                            _cache[1] = _cache[4] = _cache[7] = coord.y;
                        }
                        else
                        {
                            _cache[0] = _cache[3] = _cache[6] = real_x1;
                            _cache[1] = _cache[4] = _cache[7] = real_y1;
                        }
                        break;
                    }
                    update();
                    return QOpenGLWidget::mousePressEvent(event);
                default:
                    break;
                }

                if (_input_line.isVisible() && _last_clicked_obj != nullptr)
                {
                    QString text;
                    if (dynamic_cast<Text *>(_last_clicked_obj) != nullptr)
                    {
                        text = static_cast<Text *>(_last_clicked_obj)->text();
                        static_cast<Text *>(_last_clicked_obj)->set_text(_input_line.toPlainText(),
                            GlobalSetting::setting().text_size);
                    }

                    if (text != _input_line.toPlainText())
                    {
                        _editer->push_backup_command(new UndoStack::TextChangedCommand(_last_clicked_obj, text));
                    }

                    refresh_text_vbo();
                    _input_line.clear();
                    _input_line.hide();
                }
            }
            else
            {
                _pressed_obj = _clicked_obj;
                if (reset)
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->is_selected = true;
                }

                switch (_operation)
                {
                case Operation::Mirror:
                    if (_editer->mirror(_object_cache, _clicked_obj, event->modifiers() == Qt::ControlModifier))
                    {
                        std::set<Geo::Type> types;
                        for (const Geo::Geometry *object : _object_cache)
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
                    }
                    _object_cache.clear();
                    _operation = Operation::NoOperation;
                    emit tool_changed(Tool::NoTool);
                    update();
                    return QOpenGLWidget::mousePressEvent(event);
                case Operation::PolygonDifference:
                    if (_editer->polygon_difference(dynamic_cast<Geo::Polygon *>(_last_clicked_obj), 
                        dynamic_cast<const Geo::Polygon *>(_clicked_obj)))
                    {
                        refresh_vbo(Geo::Type::POLYGON, true);
                        refresh_selected_ibo();
                    }
                    _object_cache.clear();
                    emit tool_changed(Tool::NoTool);
                    _operation = Operation::NoOperation;
                    update();
                    return QOpenGLWidget::mousePressEvent(event);
                case Operation::Fillet:
                    {
                        double dis = DBL_MAX;
                        Geo::Point point;
                        switch (_clicked_obj->type())
                        {
                        case Geo::Type::POLYGON:
                            for (const Geo::Point &p : *static_cast<Geo::Polygon *>(_clicked_obj))
                            {
                                if (Geo::distance(p.x, p.y, real_x1, real_y1) < dis)
                                {
                                    dis = Geo::distance(p.x, p.y, real_x1, real_y1);
                                    point = p;
                                }
                            }
                            if (_editer->fillet(dynamic_cast<Geo::Polygon *>(_clicked_obj),
                                point, GlobalSetting::setting().ui->fillet_sbx->value()))
                            {
                                refresh_vbo(Geo::Type::POLYGON, true);
                                refresh_selected_ibo();
                            }
                            break;
                        case Geo::Type::POLYLINE:
                            for (const Geo::Point &p : *static_cast<Geo::Polyline *>(_clicked_obj))
                            {
                                if (Geo::distance(p.x, p.y, real_x1, real_y1) < dis)
                                {
                                    dis = Geo::distance(p.x, p.y, real_x1, real_y1);
                                    point = p;
                                }
                            }
                            if (_editer->fillet(dynamic_cast<Geo::Polyline *>(_clicked_obj),
                                point, GlobalSetting::setting().ui->fillet_sbx->value()))
                            {
                                refresh_vbo(Geo::Type::POLYLINE, true);
                                refresh_selected_ibo();
                            }
                            break;
                        default:
                            break;
                        }
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    }
                case Operation::Trim:
                    switch (_clicked_obj->type())
                    {
                    case Geo::Type::POLYLINE:
                        _editer->trim(static_cast<Geo::Polyline *>(_clicked_obj), real_x1, real_y1);
                        refresh_vbo(Geo::Type::POLYLINE, true);
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    case Geo::Type::POLYGON:
                        _editer->trim(static_cast<Geo::Polygon *>(_clicked_obj), real_x1, real_y1);
                        refresh_vbo({Geo::Type::POLYGON, Geo::Type::POLYLINE}, true);
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    case Geo::Type::BEZIER:
                        _editer->trim(static_cast<Geo::Bezier *>(_clicked_obj), real_x1, real_y1);
                        refresh_vbo(Geo::Type::BEZIER, true);
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    case Geo::Type::BSPLINE:
                        _editer->trim(static_cast<Geo::BSpline *>(_clicked_obj), real_x1, real_y1);
                        refresh_vbo(Geo::Type::BSPLINE, true);
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    default:
                        break;
                    }
                    break;
                case Operation::Extend:
                    if (Geo::Polyline *polyline = dynamic_cast<Geo::Polyline *>(_clicked_obj);
                        polyline != nullptr && polyline->type() == Geo::Type::POLYLINE)
                    {
                        _editer->extend(polyline, real_x1, real_y1);
                        refresh_vbo(Geo::Type::POLYLINE, true);
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    }
                    break;
                case Operation::Split:
                    if (_editer->split(_clicked_obj, Geo::Point(real_x1, real_y1)))
                    {
                        refresh_vbo(_clicked_obj->type(), true);
                        _clicked_obj = nullptr;
                        refresh_selected_ibo();
                        update();
                        return QOpenGLWidget::mousePressEvent(event);
                    }
                    break;
                default:
                    _operation = Operation::NoOperation;
                    break;
                }

                switch (_tool_flags[0])
                {
                case Tool::Measure:
                    if (_measure_angle_flag == 0 || _measure_angle_flag == 2)
                    {
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_angle_flag = 1;
                            _cache[2] = _cache[5] = _cache[8] = 0.51;
                            _cache[0] = _cache[3] = _cache[6] = coord.x;
                            _cache[1] = _cache[4] = _cache[7] = coord.y;
                            update();
                            return QOpenGLWidget::mousePressEvent(event);
                        }
                    }
                    else
                    {
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_angle_flag = 2;
                            _cache[3] = _cache[6] = coord.x;
                            _cache[4] = _cache[7] = coord.y;
                            _info_labels[1]->setText("Length:" +
                                QString::number(Geo::distance(_cache[0], _cache[1],
                                    _cache[3], _cache[4])));
                            update();
                            return QOpenGLWidget::mousePressEvent(event);
                        }
                    }
                    break;
                case Tool::Angle:
                    switch (_measure_angle_flag)
                    {
                    case 1:
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_angle_flag = 2;
                            _cache[3] = _cache[6] = coord.x;
                            _cache[4] = _cache[7] = coord.y;
                            update();
                            return QOpenGLWidget::mousePressEvent(event);
                        }
                        break;
                    case 2:
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_angle_flag = 3;
                            _cache[6] = coord.x;
                            _cache[7] = coord.y;
                            _info_labels[1]->setText(QString("Angle: %1°").arg(
                                std::abs(Geo::rad_to_degree(Geo::angle(Geo::Point(_cache[0], _cache[1]),
                                Geo::Point(_cache[3], _cache[4]), Geo::Point(_cache[6], _cache[7]))))));
                            update();
                            return QOpenGLWidget::mousePressEvent(event);
                        }
                        break;
                    default:
                        if (Geo::Point coord(real_x1, real_y1); catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_angle_flag = 1;
                            _cache[2] = _cache[5] = _cache[8] = 0.51;
                            _cache[0] = _cache[3] = _cache[6] = coord.x;
                            _cache[1] = _cache[4] = _cache[7] = coord.y;
                            update();
                            return QOpenGLWidget::mousePressEvent(event);
                        }
                        break;
                    }
                    break;
                default:
                    break;
                }

                refresh_selected_ibo();
                refresh_cache_vbo(0);

                _bool_flags[4] = true; // is obj moveable
                _bool_flags[5] = true; // is obj selected

                switch (_tool_flags[0])
                {
                case Tool::Measure:
                    _measure_angle_flag = 0;
                    switch (_clicked_obj->type())
                    {
                    case Geo::Type::TEXT:
                        _info_labels[1]->setText("X:" + QString::number(static_cast<const Text*>(_clicked_obj)->center().x) +
                            "Y:" + QString::number(static_cast<const Text*>(_clicked_obj)->center().y));
                        break;
                    case Geo::Type::POLYGON:
                        {
                            const Geo::Point center = _clicked_obj->bounding_rect().center();
                            _info_labels[1]->setText("X:" + QString::number(center.x) +
                                " Y:" + QString::number(center.y) +
                                " Length:" + QString::number(_clicked_obj->length()) +
                                " Area:" + QString::number(static_cast<const Geo::Polygon *>(_clicked_obj)->area()));
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        _info_labels[1]->setText("X:" + QString::number(static_cast<const Geo::Circle *>(_clicked_obj)->x) +
                            " Y:" + QString::number(static_cast<const Geo::Circle *>(_clicked_obj)->y) +
                            " Radius:" + QString::number(static_cast<const Geo::Circle *>(_clicked_obj)->radius));
                        break;
                    case Geo::Type::POLYLINE:
                        _info_labels[1]->setText("Length:" + QString::number(static_cast<const Geo::Polyline*>(_clicked_obj)->length()));
                        break;
                    case Geo::Type::ELLIPSE:
                        _info_labels[1]->setText("X:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->center().x)
                            + " Y:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->center().y)
                            + " Angle:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->angle())
                            + " A:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->lengtha())
                            + " B:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->lengthb())
                            + " Length:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->length())
                            + " Area:" + QString::number(static_cast<const Geo::Ellipse*>(_clicked_obj)->area()));
                        break;
                    case Geo::Type::BEZIER:
                        _info_labels[1]->setText("Order:" + QString::number(static_cast<const Geo::Bezier*>(_clicked_obj)->order()) +
                            " Length:" + QString::number(static_cast<const Geo::Bezier*>(_clicked_obj)->length()));
                        break;
                    case Geo::Type::BSPLINE:
                        _info_labels[1]->setText("Length:" + QString::number(static_cast<const Geo::BSpline*>(_clicked_obj)->length()));
                        break;
                    default:
                        break;
                    }
                    update();
                    return QOpenGLWidget::mousePressEvent(event);
                default:
                    break;
                }

                Geo::Point coord;
                if (catch_cursor(real_x1, real_y1, coord, _catch_distance, false))
                {
                    real_x1 = coord.x, real_y1 = coord.y;
                    _mouse_press_pos.x = real_x1, _mouse_press_pos.y = real_y1;
                    coord = real_coord_to_view_coord(coord.x, coord.y);
                    _mouse_pos_1.setX(coord.x);
                    _mouse_pos_1.setY(coord.y);
                    QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
                }
                if (_input_line.isVisible() && _last_clicked_obj != _clicked_obj)
                {
                    _input_line.hide();
                    _input_line.clear();
                }
            }
            update();
        }
        break;
    case Qt::RightButton:
        if (is_paintable())
        {
            cancel_painting();
            _editer->reset_selected_mark();
            refresh_selected_ibo();
        }
        else
        {
            _operation = Operation::NoOperation;
            _object_cache.clear();
            if (event->modifiers() == Qt::ControlModifier)
            {
                if (_clicked_obj = _editer->select(real_x1, real_y1, false); _clicked_obj != nullptr)
                {
                    _clicked_obj->is_selected = false;
                    refresh_selected_ibo();
                }
            }
            else
            {
                if (_clicked_obj = _editer->select(real_x1, real_y1, true); _clicked_obj != nullptr)
                {
                    refresh_selected_ibo(_clicked_obj);
                    const QAction *a = _menu->exec(QCursor::pos());
                    if (a == _up)
                    {
                        _editer->up(_clicked_obj);
                        refresh_vbo(_clicked_obj->type(), true);
                        refresh_selected_ibo();
                    }
                    else if (a == _down)
                    {
                        _editer->down(_clicked_obj);
                        refresh_vbo(_clicked_obj->type(), true);
                        refresh_selected_ibo();
                    }
                }
            }
            use_tool(Tool::NoTool);
        }
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = true; // view moveable
        break;
    default:
        break;
    }

    QOpenGLWidget::mousePressEvent(event);
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->position();
    _mouse_release_pos.x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    _mouse_release_pos.y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];

    switch (event->button())
    {
    case Qt::LeftButton:
        _bool_flags[4] = false; // is obj moveable
        if (is_paintable()) // paintable
        {
            if (_circle_cache.empty() && _AABBRect_cache.empty())
            {
                _info_labels[1]->clear();
            }
        }
        else
        {
            _select_rect.clear();
            _last_point.clear();
            if (_tool_flags[0] != Tool::Measure && _tool_flags[0] != Tool::Angle)
            {
                _info_labels[1]->clear();
            }
            _bool_flags[6] = false; // is moving obj
            _last_clicked_obj = _clicked_obj;
            _pressed_obj = nullptr;
            update();

            switch (_operation)
            {
            case Operation::Rotate:
                if (_measure_angle_flag == 0)
                {
                    _operation = Operation::NoOperation;
                    _editer->push_backup_command(new UndoStack::RotateCommand(_editer->selected(),
                        _stored_coord.x, _stored_coord.y, Geo::degree_to_rad(_refline_points[0]), true));
                    emit operation_changed(Operation::NoOperation);
                }
                break;
            default:
                break;
            }

            if (const std::vector<Geo::Geometry *> &objects = _editer->selected(); 
                !objects.empty() && GlobalSetting::setting().translated_points && _mouse_press_pos != _mouse_release_pos)
            {
                GlobalSetting::setting().translated_points = false;
                if (_editer->edited_shape().empty())
                {
                    _editer->push_backup_command(new UndoStack::TranslateCommand(objects,
                        _mouse_release_pos.x - _mouse_press_pos.x, _mouse_release_pos.y - _mouse_press_pos.y));
                }
                else
                {
                    _editer->push_backup_command(new UndoStack::ChangeShapeCommand(objects.front(), _editer->edited_shape()));
                    _editer->edited_shape().clear();
                }
            }
        }
        _reflines.clear();
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = false; // view moveable
        break;
    default:
        break;
    }

    if (Geo::Point coord; catch_cursor(_mouse_release_pos.x, _mouse_release_pos.y, coord, _catch_distance, false))
    {
        _mouse_release_pos.x = coord.x, _mouse_release_pos.y = coord.y;
        coord = real_coord_to_view_coord(coord.x, coord.y);
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
    }

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
    const double canvas_x0 = real_x0 * _canvas_ctm[0] + real_y0 * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y0 = real_x0 * _canvas_ctm[1] + real_y0 * _canvas_ctm[4] + _canvas_ctm[7];
    double canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
    double canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
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
    }
    else
    {
        _bool_flags[8] = false;
    }

    _info_labels[0]->setText(QString("X:%1 Y:%2").arg(real_x1, 0, 'f', 2).arg(real_y1, 0, 'f', 2));

    if (is_view_moveable()) // 视图可移动
    {
        _canvas_ctm[6] += (canvas_x1 - canvas_x0), _canvas_ctm[7] += (canvas_y1 - canvas_y0);
        _view_ctm[6] -= (real_x1 - real_x0), _view_ctm[7] -= (real_y1 - real_y0);
        _visible_area.translate(real_x0 - real_x1, real_y0 - real_y1);
        makeCurrent();
        glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
        glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
        doneCurrent();
        update();
    }
    if (is_paintable() && is_painting()) // painting
    {
        switch (_tool_flags[0])
        {
        case Tool::Circle:
            _circle_cache.radius = Geo::distance(real_x1, real_y1, _circle_cache.x, _circle_cache.y);
            _circle_cache.update_shape(Geo::Circle::default_down_sampling_value);
            refresh_circle_cache_vbo();
            _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius)).c_str());
            break;
        case Tool::Ellipse:
            if (_last_point == _stored_coord)
            {
                const double radius = Geo::distance(real_x1, real_y1, _stored_coord.x, _stored_coord.y);
                _ellipse_cache.set_lengtha(radius);
                _ellipse_cache.set_lengthb(radius);
            }
            else
            {
                _ellipse_cache.set_lengthb(Geo::distance(real_x1, real_y1, _stored_coord.x, _stored_coord.y));
            }
            _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
            refresh_ellipse_cache_vbo();
            break;
        case Tool::Polyline:
            if (event->modifiers() == Qt::ControlModifier)
            {
                const Geo::Point &coord =_editer->point_cache().at(_editer->point_cache().size() - 2);
                if (std::abs(real_x1 - coord.x) > std::abs(real_y1 - coord.y))
                {
                    _editer->point_cache().back().x = real_x1;
                    _editer->point_cache().back().y = coord.y;
                }
                else
                {
                    _editer->point_cache().back().x = coord.x;
                    _editer->point_cache().back().y = real_y1;
                }
            }
            else
            {
                _editer->point_cache().back().x = real_x1;
                _editer->point_cache().back().y = real_y1;
            }
            _cache[_cache_count - 3] = _editer->point_cache().back().x;
            _cache[_cache_count - 2] = _editer->point_cache().back().y;
            refresh_cache_vbo(3);
            _info_labels[1]->setText(std::string("Length:").append(std::to_string(
                Geo::distance(_editer->point_cache().back(), _editer->point_cache()[_editer->point_cache().size() - 2]))).c_str());
            break;
        case Tool::Rect:
            if (event->modifiers() == Qt::ControlModifier)
            {
                const double width = std::max(std::abs(real_x1 - _last_point.x), std::abs(real_y1 - _last_point.y));
                if (real_x1 > _last_point.x)
                {
                    if (real_y1 > _last_point.y)
                    {
                        _AABBRect_cache = Geo::AABBRect(_last_point.x, _last_point.y, _last_point.x + width, _last_point.y + width);
                    }
                    else
                    {
                        _AABBRect_cache = Geo::AABBRect(_last_point.x, _last_point.y - width, _last_point.x + width, _last_point.y);
                    }
                }
                else
                {
                    if (real_y1 > _last_point.y)
                    {
                        _AABBRect_cache = Geo::AABBRect(_last_point.x - width, _last_point.y, _last_point.x, _last_point.y + width);
                    }
                    else
                    {
                        _AABBRect_cache = Geo::AABBRect(_last_point.x - width, _last_point.y - width, _last_point.x, _last_point.y);
                    }
                }
                refresh_AABBRect_cache_vbo();
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(width))
                    .append(" Height:").append(std::to_string(width)).c_str());
            }
            else
            {
                _AABBRect_cache = Geo::AABBRect(_last_point.x, _last_point.y, real_x1, real_y1);
                refresh_AABBRect_cache_vbo();
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.x)))
                    .append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.y))).c_str());
            }
            break;
        case Tool::Bezier:
            _editer->point_cache().back().x = real_x1;
            _editer->point_cache().back().y = real_y1;
            _cache[_cache_count - 3] = real_x1;
            _cache[_cache_count - 2] = real_y1;
            refresh_cache_vbo(3);
            break;
        case Tool::BSpline:
            _editer->point_cache().back().x = real_x1;
            _editer->point_cache().back().y = real_y1;
            _cache[_cache_count - 3] = real_x1;
            _cache[_cache_count - 2] = real_y1;
            refresh_cache_vbo(3);
            break;
        default:
            _info_labels[1]->clear();
            break;
        }
        update();
    }
    else
    {
        if (_operation == Operation::Rotate)
        {
            if (_measure_angle_flag > 0)
            {
                if (_measure_angle_flag == 1)
                {
                    _cache[0] = real_x1;
                    _cache[1] = real_y1;
                }
                else
                {
                    _cache[6] = real_x1;
                    _cache[7] = real_y1;
                }
            }
        }
        else if (is_obj_moveable())
        {
            _bool_flags[6] = true; // is moving obj
            const std::vector<Geo::Geometry *> selected_objects = _editer->selected();
            if (selected_objects.size() == 1)
            {
                _editer->translate_points(selected_objects.back(), real_x0, real_y0, real_x1, real_y1, event->modifiers() == Qt::ControlModifier);
                switch (selected_objects.back()->type())
                {
                case Geo::Type::BEZIER:
                    _cache_count = 0;
                    for (const Geo::Point &point : *static_cast<Geo::Bezier *>(selected_objects.back()))
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                    }
                    refresh_cache_vbo(0);
                    break;
                case Geo::Type::BSPLINE:
                    _cache_count = 0;
                    for (const Geo::Point &point : static_cast<Geo::BSpline *>(selected_objects.back())->path_points)
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                    }
                    refresh_cache_vbo(0);
                    break;
                default:
                    break;
                }
                refresh_vbo(selected_objects.back()->type(), event->modifiers() == Qt::ControlModifier);
                if (event->modifiers() == Qt::ControlModifier)
                {
                    refresh_selected_ibo(selected_objects.back());
                }
            }
            else
            {
                std::set<Geo::Type> types;
                for (Geo::Geometry *obj : selected_objects)
                {
                    _editer->translate_points(obj, real_x0, real_y0, real_x1, real_y1, false);
                    types.insert(obj->type());
                }
                refresh_vbo(types, false);
                // refresh_brush_ibo();
            }
            if (GlobalSetting::setting().show_text)
            {
                refresh_text_vbo();
            }
            if (event->modifiers() != Qt::ControlModifier && GlobalSetting::setting().auto_aligning)
            {
                _reflines.clear();
                if (_editer->auto_aligning(_pressed_obj, real_x1, real_y1, _reflines, true))
                {
                    refresh_selected_vbo();
                    refresh_reflines_vbo();
                }
            }
            _info_labels[1]->clear();
        }
        else if (!_select_rect.empty())
        {
            _select_rect = Geo::AABBRect(_last_point.x, _last_point.y, real_x1, real_y1);
            refresh_select_rect_vbo();
            if (std::vector<Geo::Geometry *> objects = _editer->select(_select_rect); !objects.empty())
            {
                refresh_selected_ibo(objects);
                _cache_count = 0; // 框选时不显示BSpline路径线和Bezier控制线
            }
            _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.x)))
                .append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.y))).c_str());
        }
        else if (_measure_angle_flag > 0)
        {
            switch (_measure_angle_flag)
            {
            case 1:
                _cache[3] = _cache[6] = real_x1;
                _cache[4] = _cache[7] = real_y1;
                if (_tool_flags[0] == Tool::Measure)
                {
                    _info_labels[1]->setText("Length:" + QString::number(
                        Geo::distance(_cache[0], _cache[1], real_x1, real_y1)));
                }
                break;
            case 2:
                if (_tool_flags[0] == Tool::Angle)
                {
                    _cache[6] = real_x1;
                    _cache[7] = real_y1;
                }
                break;
            default:
                break;
            }
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
    glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    double data[12] = {-10 / _ratio, 0, 0, 10 / _ratio, 0, 0, 0, -10 / _ratio, 0, 0, 10 / _ratio, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[0]); // origin and select rect
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    doneCurrent();
    _editer->set_view_ratio(_ratio);

    _reflines.clear();
    if (_editer->auto_aligning(_pressed_obj, _reflines))
    {
        refresh_reflines_vbo();
    }
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->position();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (is_paintable() && is_painting()) // paintable and painting
        {
            _bool_flags[1] = false; // paintable
            _bool_flags[2] = false; // painting
            switch (_tool_flags[0])
            {
            case Tool::Circle:
                _circle_cache.clear();
                break;
            case Tool::Polyline:
                switch (_editer->append_points())
                {
                case 1:
                    refresh_vbo(Geo::Type::POLYLINE, true);
                    break;
                case 2:
                    refresh_vbo(Geo::Type::POLYGON, true);
                    break;
                default:
                    break;
                }
                _cache_count = 0;
                break;
            case Tool::Rect:
                _AABBRect_cache.clear();
                break;
            case Tool::Bezier:
                _editer->append_bezier(_curve_order);
                refresh_vbo(Geo::Type::BEZIER, true);
                _cache_count = 0;
                break;
            case Tool::BSpline:
                _editer->append_bspline(_curve_order);
                refresh_vbo(Geo::Type::BSPLINE, true);
                _cache_count = 0;
                break;
            default:
                break;
            }
            _tool_flags[1] = _tool_flags[0];
            _tool_flags[0] = Tool::NoTool;
            emit tool_changed(_tool_flags[0]);
            update();
        }
        else
        {
            if (is_obj_selected() && dynamic_cast<Text *>(_last_clicked_obj) != nullptr)
            {
                Geo::AABBRect rect(_last_clicked_obj->bounding_rect());
                rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
                _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
                _input_line.move(rect.center().x - _input_line.rect().center().x(),
                                 rect.center().y - _input_line.rect().center().y());
                _input_line.setFocus();
                _input_line.setText(dynamic_cast<Text *>(_last_clicked_obj)->text());
                _input_line.moveCursor(QTextCursor::End);
                _input_line.show();
            }
        }
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
    _last_point = center();
    _editer->set_view_ratio(1.0);
    _bool_flags[8] = false;

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
    _ratio = _ratio * 0.98;

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
    glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    {
        double data[12] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[0]); // origin and select rect
        glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    }
    doneCurrent();
}




void Canvas::use_tool(const Tool tool)
{
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = tool;
    _bool_flags[1] = (tool != Tool::NoTool && tool != Tool::Measure && tool != Tool::Angle); // paintable
    _bool_flags[2] = false; // painting

    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    _cache_count = 0;

    _measure_angle_flag = 0;
    _info_labels[1]->clear();

    emit tool_changed(_tool_flags[0]);
    update();
}

void Canvas::set_operation(const Operation operation)
{
    _operation = operation;
    switch (operation)
    {
    case Operation::Mirror:
    case Operation::RingArray:
        _object_cache = _editer->selected();
        break;
    default:
        _object_cache.clear();
        break;
    }
    emit operation_changed(operation);
}

void Canvas::show_origin()
{
    _bool_flags[7] = true;
}

void Canvas::hide_origin()
{
    _bool_flags[7] = false;
}

bool Canvas::origin_visible() const
{
    return _bool_flags[7];
}

const bool Canvas::is_view_moveable() const
{
    return _bool_flags[0];
}

const bool Canvas::is_paintable() const
{
    return _bool_flags[1];
}

const bool Canvas::is_painting() const
{
    return _bool_flags[2];
}

const bool Canvas::is_typing() const
{
    return _input_line.isVisible();
}

const bool Canvas::is_measureing() const
{
    return _bool_flags[3];
}

const bool Canvas::is_obj_moveable() const
{
    return _bool_flags[4];
}

const bool Canvas::is_obj_selected() const
{
    return _bool_flags[5];
}

const bool Canvas::is_moving_obj() const
{
    return _bool_flags[6];
}

void Canvas::set_catch_distance(const double value)
{
    _catch_distance = value;
}

void Canvas::set_cursor_catch(const CatchedPointType type, const bool value)
{
    _catch_types[static_cast<int>(type)] = value;
}

const bool Canvas::is_catching(const CatchedPointType type) const
{
    return _catch_types[static_cast<int>(type)];
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

void Canvas::set_curve_order(const size_t order)
{
    _curve_order = order;
}

const size_t Canvas::curve_order() const
{
    return _curve_order;
}



double Canvas::ratio() const
{
    return _ratio;
}

Geo::Point Canvas::center() const
{
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty()))
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
    {
        x0 = std::min(x0, point.x);
        y0 = std::min(y0, point.y);
        x1 = std::max(x1, point.x);
        y1 = std::max(y1, point.y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.x - _circle_cache.radius);
        y0 = std::min(y0, _circle_cache.y - _circle_cache.radius);
        x1 = std::max(x1, _circle_cache.x + _circle_cache.radius);
        y1 = std::max(y1, _circle_cache.y + _circle_cache.radius);
    }

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
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty()))
    {
        return Geo::AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
    {
        x0 = std::min(x0, point.x);
        y0 = std::min(y0, point.y);
        x1 = std::max(x1, point.x);
        y1 = std::max(y1, point.y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.x - _circle_cache.radius);
        y0 = std::min(y0, _circle_cache.y - _circle_cache.radius);
        x1 = std::max(x1, _circle_cache.x + _circle_cache.radius);
        y1 = std::max(y1, _circle_cache.y + _circle_cache.radius);
    }

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
    return _circle_cache.empty() && _AABBRect_cache.empty() &&
           (GlobalSetting::setting().graph == nullptr || GlobalSetting::setting().graph->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NoTool;

    _editer->point_cache().clear();
    _circle_cache.clear();
    _ellipse_cache.clear();
    _AABBRect_cache.clear();
    _cache_count = 0;

    _measure_angle_flag = 0;
    _info_labels[1]->clear();

    _operation = Operation::NoOperation;
    _object_cache.clear();

    emit tool_changed(_tool_flags[0]);
    update();
}

void Canvas::use_last_tool()
{
    if (is_painting())
    {
        return;
    }
    _tool_flags[0] = _tool_flags[1];
    _measure_angle_flag = 0;
    _info_labels[1]->clear();
    _bool_flags[1] = (_tool_flags[0] != Tool::Measure && _tool_flags[0] != Tool::Angle); // paintable
    if (_tool_flags[0] == Tool::NoTool)
    {
        cancel_painting();
    }
    else
    {
        emit tool_changed(_tool_flags[0]);
    }
}

void Canvas::set_info_labels(QLabel **labels)
{
    _info_labels = labels;
}

void Canvas::copy()
{
    _stored_coord.x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    _stored_coord.y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    _editer->copy_selected();
}

void Canvas::cut()
{
    _stored_coord.x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    _stored_coord.y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
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
    if (_editer->paste(x - _stored_coord.x, y - _stored_coord.y))
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
    if (_editer->paste(x - _stored_coord.x, y - _stored_coord.y))
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

void Canvas::rotate(const double rad, const bool unitary, const bool to_all_layers)
{
    if (_operation == Operation::Rotate)
    {
        if (std::vector<Geo::Geometry *> objects = _editer->selected(); !objects.empty())
        {
            if (_last_point == _stored_coord)
            {
                _editer->rotate(objects, rad, unitary, to_all_layers);
            }
            else
            {
                _editer->rotate(objects, _stored_coord.x, _stored_coord.y, rad);
            }
            std::set<Geo::Type> types;
            for (const Geo::Geometry *object : objects)
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
            if (types.empty())
            {
                refresh_vbo(false);
            }
            else
            {
                refresh_vbo(types, false);
            }
            if (objects.size() == 1)
            {
                refresh_selected_ibo(objects.front());
                refresh_cache_vbo(0);
            }
            update();
        }
        _measure_angle_flag = 0;
        _operation = Operation::NoOperation;
        emit operation_changed(Operation::NoOperation);
    }
    else
    {
        _stored_coord.x = _stored_coord.y = _last_point.x = _last_point.y = 0;
        _operation = Operation::Rotate;
        _object_cache.clear();
        emit operation_changed(Operation::Rotate);
    }
}

void Canvas::polyline_cmd(const double x, const double y)
{
    if (is_painting())
    {
        _editer->point_cache().back().x = x;
        _editer->point_cache().back().y = y;
        _cache[_cache_count - 3] = x;
        _cache[_cache_count - 2] = y;
        _editer->point_cache().emplace_back(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
            _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
        _cache[_cache_count++] = _editer->point_cache().back().x;
        _cache[_cache_count++] = _editer->point_cache().back().y;
        _cache[_cache_count++] = 0;
        check_cache();
        refresh_cache_vbo(6);
    }
    else
    {
        _editer->point_cache().emplace_back(x, y);
        _editer->point_cache().emplace_back(_mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
            _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
        _bool_flags[2] = true; // painting
        _cache_count = 6;
        _cache[0] = x;
        _cache[1] = y;
        _cache[3] = _editer->point_cache().back().x;
        _cache[4] = _editer->point_cache().back().y;
        _cache[2] = _cache[5] = 0;
        refresh_cache_vbo(0);
    }
    update();
}

void Canvas::polyline_cmd()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    switch (_tool_flags[0])
    {
    case Tool::Polyline:
        switch (_editer->append_points())
        {
        case 1:
            refresh_vbo(Geo::Type::POLYLINE, true);
            break;
        case 2:
            refresh_vbo(Geo::Type::POLYGON, true);
            break;
        default:
            break;
        }
        break;
    case Tool::BSpline:
        _editer->append_bspline(_curve_order);
        refresh_vbo(Geo::Type::BSPLINE, true);
        break;
    case Tool::Bezier:
        _editer->append_bezier(_curve_order);
        refresh_vbo(Geo::Type::BEZIER, true);
        break;
    default:
        break;
    }
    _cache_count = 0;
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NoTool;
    emit tool_changed(_tool_flags[0]);
    update();
}

void Canvas::rect_cmd(const double x, const double y)
{
    if (is_painting())
    {
        _editer->append(Geo::AABBRect(_last_point.x, _last_point.y,
            _last_point.x + x, _last_point.y + y));
        _AABBRect_cache.clear();
        _tool_flags[1] = _tool_flags[0];
        _tool_flags[0] = Tool::NoTool;
        _bool_flags[1] = false; // paintable
        emit tool_changed(_tool_flags[0]);
        refresh_vbo(Geo::Type::POLYGON, true);
    }
    else
    {
        _last_point.x = x;
        _last_point.y = y;
        _AABBRect_cache = Geo::AABBRect(x, y,
            _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
            _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
        refresh_AABBRect_cache_vbo();
    }
    _bool_flags[2] = !_bool_flags[2]; // painting
}

void Canvas::rect_cmd()
{
    if (is_painting())
    {
        _editer->append(_AABBRect_cache);
        _AABBRect_cache.clear();
        _tool_flags[1] = _tool_flags[0];
        _tool_flags[0] = Tool::NoTool;
        _bool_flags[1] = false; // paintable
        emit tool_changed(_tool_flags[0]);
        refresh_vbo(Geo::Type::POLYGON, true);
    }
    else
    {
        _last_point.x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
        _last_point.y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
        _AABBRect_cache = Geo::AABBRect(_last_point.x, _last_point.y,
            _last_point.x + 2, _last_point.y + 2);
        refresh_AABBRect_cache_vbo();
    }
    _bool_flags[2] = !_bool_flags[2]; // painting
}

void Canvas::circle_cmd(const double x, const double y)
{
    const double r = Geo::distance(x, y,
        _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
        _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    if (r > 0)
    {
        _circle_cache = Geo::Circle(x, y, r);
    }
    else
    {
        _circle_cache = Geo::Circle(x, y, 10);
    }
    refresh_circle_cache_vbo();
    _bool_flags[2] = !_bool_flags[2]; // painting
    update();
}

void Canvas::circle_cmd(const double x, const double y, const double r)
{
    if (r == 0)
    {
        _editer->append(Geo::Circle(x, y, 10));
    }
    else if (r < 0)
    {
        _editer->append(Geo::Circle(x, y, -r));
    }
    else
    {
        _editer->append(Geo::Circle(x, y, r));
    }
    _circle_cache.clear();
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NoTool;
    _bool_flags[1] = false; // moveable
    emit tool_changed(_tool_flags[0]);
    refresh_vbo(Geo::Type::CIRCLE, true);
    _bool_flags[2] = !_bool_flags[2]; // painting
    update();
}

void Canvas::ellipse_cmd(const double x, const double y)
{
    const double r = Geo::distance(x, y,
        _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
        _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
    if (r > 0)
    {
        _ellipse_cache.set_center(x, y);
        _ellipse_cache.set_lengtha(r);
        _ellipse_cache.set_lengthb(r);
    }
    else
    {
        _ellipse_cache.set_center(x, y);
        _ellipse_cache.set_lengtha(10);
        _ellipse_cache.set_lengthb(10);
    }
    _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
    refresh_ellipse_cache_vbo();
    _stored_coord.x = _last_point.x = x;
    _stored_coord.y = _last_point.y = y;
    _bool_flags[2] = true; // painting
    update();
}

void Canvas::ellipse_cmd(const double x, const double y, const double rad, const double a)
{
    const double b = _ellipse_cache.lengthb();
    _ellipse_cache.clear();
    _ellipse_cache.set_center(x, y);
    _ellipse_cache.set_lengtha(a);
    _ellipse_cache.set_lengthb(b);
    _ellipse_cache.rotate(_ellipse_cache.center().x, _ellipse_cache.center().y, rad);
    _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
    refresh_ellipse_cache_vbo();
    _last_point.x = x + 1;
    _stored_coord.x = x;
    _stored_coord.y = y;
    update();
}

void Canvas::ellipse_cmd(const double x, const double y, const double rad, const double a, const double b)
{
    _ellipse_cache.clear();
    _ellipse_cache.set_center(x, y);
    _ellipse_cache.set_lengtha(a);
    _ellipse_cache.set_lengthb(b);
    _ellipse_cache.rotate(_ellipse_cache.center().x, _ellipse_cache.center().y, rad);
    _ellipse_cache.update_shape(Geo::Ellipse::default_down_sampling_value);
    _editer->append(_ellipse_cache);
    _ellipse_cache.clear();
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NoTool;
    _bool_flags[1] = false; // moveable
    emit tool_changed(_tool_flags[0]);
    refresh_vbo(Geo::Type::ELLIPSE, true);
    _bool_flags[2] = false; // painting
    update();
}

void Canvas::text_cmd(const double x, const double y)
{
    _editer->append_text(x, y);
    _tool_flags[0] = Tool::NoTool;
    _bool_flags[1] = _bool_flags[2] = false;
    emit tool_changed(_tool_flags[0]);
    refresh_vbo(Geo::Type::TEXT, true);
    update();
}



bool Canvas::is_visible(const Geo::Point &point) const
{
    return point.x > _visible_area.left() && point.x < _visible_area.right()
        && point.y > _visible_area.bottom() && point.y < _visible_area.top();
}

bool Canvas::is_visible(const Geo::Polyline &polyline) const
{
    if (!Geo::is_intersected(_visible_area, polyline.bounding_rect()))
    {
        return false;
    }
    if (is_visible(polyline.front()))
    {
        return true;
    }
    const Geo::Point center(_visible_area.center());
    const double len = std::pow(std::min(_visible_area.width(), _visible_area.height()), 2);
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (is_visible(polyline[i]) || Geo::distance_square(center, polyline[i - 1], polyline[i]) < len)
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Polygon &polygon) const
{
    if (!Geo::is_intersected(_visible_area, polygon.bounding_rect()))
    {
        return false;
    }
    const Geo::Point center(_visible_area.center());
    const double len = std::pow(std::min(_visible_area.width(), _visible_area.height()), 2);
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (is_visible(polygon[i - 1]) || Geo::distance_square(center, polygon[i - 1], polygon[i]) < len)
        {
            return true;
        }
    }
    for (const Geo::Point &point : _visible_area)
    {
        if (Geo::is_inside(point, polygon))
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Circle &circle) const
{
    return circle.x > _visible_area.left() - circle.radius &&
        circle.x < _visible_area.right() + circle.radius &&
        circle.y > _visible_area.bottom() - circle.radius &&
        circle.y < _visible_area.top() + circle.radius;
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
        _bool_flags[8] = true;
    }
    else
    {
        _bool_flags[8] = false;
    }
    return _bool_flags[8];
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
            _bool_flags[8] = true;
        }
        else
        {
            _bool_flags[8] = false;
        }
        return _bool_flags[8];
    }
    else
    {
        _bool_flags[8] = false;
        return false;
    }
}


void Canvas::check_cache()
{
    if (_cache_count == _cache_len)
    {
        _cache_len *= 2;
        double *temp = new double[_cache_len];
        std::move(_cache, _cache + _cache_count, temp);
        delete[] _cache;
        _cache = temp;
    }
}


void Canvas::refresh_vbo(const bool refresh_ibo)
{
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>>
        polyline_vbo = std::async(std::launch::async, &Canvas::refresh_polyline_vbo, this),
        polygon_vbo = std::async(std::launch::async, &Canvas::refresh_polygon_vbo, this),
        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this),
        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>> text_vbo;
    if (GlobalSetting::setting().show_text)
    {
        text_vbo = std::async(std::launch::async,
            static_cast<std::tuple<double*, unsigned int, unsigned int*, unsigned int>(Canvas::*)(void)>(&Canvas::refresh_text_vbo), this);
    }
    else
    {
        _brush_index_count[2] = 0;
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
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
        }
        delete []circle_data;

        curve_printable_points.wait();
        auto [curve_data, curve_data_count] = curve_printable_points.get();
        if (curve_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[6]); // curve printable points
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
        }
        delete []curve_data;
    }

    circle_vbo.wait();
    auto [circle_data, circle_data_count, circle_indexs, circle_index_count] = circle_vbo.get();
    if (circle_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * circle_index_count, circle_indexs, GL_DYNAMIC_DRAW);
    }
    delete []circle_data;
    delete []circle_indexs;

    curve_vbo.wait();
    auto [curve_data, curve_data_count, curve_indexs, curve_index_count] = curve_vbo.get();
    if (curve_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[3]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * curve_index_count, curve_indexs, GL_DYNAMIC_DRAW);
    }
    delete []curve_data;
    delete []curve_indexs;

    polyline_vbo.wait();
    auto [polyline_data, polyline_data_count, polyline_indexs, polyline_index_count] = polyline_vbo.get();
    if (polyline_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polyline_data_count, polyline_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyline_index_count, polyline_indexs, GL_DYNAMIC_DRAW);
    }
    delete []polyline_data;
    delete []polyline_indexs;

    polygon_vbo.wait();
    auto [polygon_data, polygon_data_count, polygon_indexs, polygon_index_count] = polygon_vbo.get();
    if (polygon_data_count > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polygon_data_count, polygon_data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    }
    delete []polygon_data;
    delete []polygon_indexs;

    if (GlobalSetting::setting().show_text)
    {
        text_vbo.wait();
        auto [text_data, text_data_count, text_indexs, text_index_count] = text_vbo.get();
        if (text_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[4]); // text
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[0]);
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[1]);
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[1]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
            }
            delete []data;
            delete []indexs;
        }
        break;
    case Geo::Type::CIRCLE:
    case Geo::Type::ELLIPSE:
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[2]);
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
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle printable points
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[2]);
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
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle printable points
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[3]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[3]);
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
                    glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[6]); // curve printable points
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[4]); // text
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
            _brush_index_count[2] = 0;
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
    std::future<std::tuple<double*, unsigned int>> circle_printable_points, curve_printable_points;

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
        _brush_index_count[2] = 0;
    }
    if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end())
    {
        circle_vbo = std::async(std::launch::async, &Canvas::refresh_circle_vbo, this);
    }
    if (types.find(Geo::Type::BEZIER) != types.end() || types.find(Geo::Type::BSPLINE) != types.end())
    {
        curve_vbo = std::async(std::launch::async, &Canvas::refresh_curve_vbo, this);
    }
    if (GlobalSetting::setting().show_points)
    {
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end())
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
        if (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end())
        {
            circle_printable_points.wait();
            auto [circle_data, circle_data_count] = circle_printable_points.get();
            if (circle_data_count > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle printable points
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
                glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[6]); // curve printable points
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
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_data_count, circle_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[2]);
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
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[3]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_data_count, curve_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[3]);
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
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polyline_data_count, polyline_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[0]);
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
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(double) * polygon_data_count, polygon_data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape_IBO[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
        }
        delete []polygon_data;
        delete []polygon_indexs;
    }

    if (text_vbo.valid())
    {
        text_vbo.wait();
        auto [text_data, text_data_count, text_indexs, text_index_count] = text_vbo.get();
        if (text_data_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[4]); // text
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

    _point_count[0] = data_count / 3;
    _shape_index_count[0] = index_count;
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

    _point_count[1] = data_count / 3;
    _shape_index_count[1] = index_count;
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
            default:
                break;
            }
        }
    }

    _shape_index_count[2] = index_count;
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

    _shape_index_count[3] = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}

std::tuple<unsigned int*, unsigned int> Canvas::refresh_polygon_brush_ibo()
{
    unsigned int index_len = 512, index_count = 0, data_count = 0;
    unsigned int *indexs = new unsigned int[index_len];
    const Geo::Polygon *polygon = nullptr;
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
            case Geo::Type::POLYGON:
                polygon = static_cast<const Geo::Polygon *>(geo);
                while (index_count + polygon->triangle_indices.size() > index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const unsigned int i : polygon->triangle_indices)
                {
                    indexs[index_count++] = data_count + i;
                }
                data_count += polygon->size();
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    if (polygon = dynamic_cast<const Geo::Polygon *>(item))
                    {
                        while (index_count + polygon->triangle_indices.size() > index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const unsigned int i : polygon->triangle_indices)
                        {
                            indexs[index_count++] = data_count + i;
                        }
                        data_count += polygon->size();
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    _brush_index_count[0] = index_count;
    return std::make_tuple(indexs, index_count);
}

std::tuple<unsigned int*, unsigned int> Canvas::refresh_circle_brush_ibo()
{
    unsigned int index_len = 512, index_count = 0, data_count = 0;
    unsigned int *indexs = new unsigned int[index_len];
    const Geo::Circle *circle = nullptr;
    const Geo::Ellipse *ellipse = nullptr;

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
            case Geo::Type::CIRCLE:
                circle = static_cast<const Geo::Circle *>(geo);
                while (index_count + circle->triangle_indices.size() > index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const unsigned int i : circle->triangle_indices)
                {
                    indexs[index_count++] = data_count + i;
                }
                data_count += circle->shape().size();
                break;
            case Geo::Type::ELLIPSE:
                ellipse = static_cast<const Geo::Ellipse *>(geo);
                while (index_count + ellipse->triangle_indices.size() > index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::move(indexs, indexs + index_count, temp);
                    delete []indexs;
                    indexs = temp;
                }
                for (const unsigned int i : ellipse->triangle_indices)
                {
                    indexs[index_count++] = data_count + i;
                }
                data_count += ellipse->shape().size();
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::CIRCLE:
                        circle = static_cast<const Geo::Circle *>(item);
                        while (index_count + circle->triangle_indices.size() > index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const unsigned int i : circle->triangle_indices)
                        {
                            indexs[index_count++] = data_count + i;
                        }
                        data_count += circle->shape().size();
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<const Geo::Ellipse *>(item);
                        while (index_count + ellipse->triangle_indices.size() > index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::move(indexs, indexs + index_count, temp);
                            delete []indexs;
                            indexs = temp;
                        }
                        for (const unsigned int i : ellipse->triangle_indices)
                        {
                            indexs[index_count++] = data_count + i;
                        }
                        data_count += ellipse->shape().size();
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

    _brush_index_count[1] = index_count;
    return std::make_tuple(indexs, index_count);
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

        for (Geo::Geometry *geo : group)
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
                circle = static_cast<Geo::Circle *>(geo);
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
                ellipse = static_cast<Geo::Ellipse *>(geo);
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
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *static_cast<Combination *>(geo))
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
                        circle = static_cast<Geo::Circle *>(item);
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
                        ellipse = static_cast<Geo::Ellipse *>(item);
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

    _point_count[2] = data_count / 3;
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

    _point_count[3] = data_count / 3;
    return std::make_tuple(data, data_count);
}

void Canvas::refresh_cache_vbo(const unsigned int count)
{
    if (_cache_count == 0)
    {
        return;
    }
    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
    if (_cache_count == _cache_len)
    {
        _cache_len *= 2;
        double *temp = new double[_cache_len];
        std::move(_cache, _cache + _cache_count, temp);
        delete []_cache;
        _cache = temp;
        glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_STREAM_DRAW);
    }
    else
    {
        if (count == 0)
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - count) * sizeof(double), count * sizeof(double), &_cache[_cache_count - count]);
        }
    }
    doneCurrent();
}

void Canvas::refresh_AABBRect_cache_vbo()
{
    for (int i = 0; i < 4; ++i)
    {
        _cache[i * 3] = _AABBRect_cache[i].x;
        _cache[i * 3 + 1] = _AABBRect_cache[i].y;
        _cache[i * 3 + 2] = 0;
    }
    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), _cache);
    doneCurrent();
}

void Canvas::refresh_reflines_vbo()
{
    int i = 0;
    for (const QLineF &line : _reflines)
    {
        _refline_points[i++] = line.p1().x();
        _refline_points[i++] = line.p1().y();
        _refline_points[i++] = 0.51;
        _refline_points[i++] = line.p2().x();
        _refline_points[i++] = line.p2().y();
        _refline_points[i++] = 0.51;
    }

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[2]); // reflines
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(double), _refline_points);
    doneCurrent();
}

void Canvas::refresh_circle_cache_vbo()
{
    const Geo::Polygon &points = _circle_cache.shape();
    _cache_count = _cache_len;
    while (_cache_len < points.size() * 3)
    {
        _cache_len *= 2;
    }
    if (_cache_count < _cache_len)
    {
        _cache_len *= 2;
        delete []_cache;
        _cache = new double[_cache_len];
        _cache_count = 0;
        for (const Geo::Point &point : points)
        {
            _cache[_cache_count++] = point.x;
            _cache[_cache_count++] = point.y;
            _cache[_cache_count++] = 0;
        }

        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_STREAM_DRAW);
        doneCurrent();
    }
    else
    {
        _cache_count = 0;
        for (const Geo::Point &point : points)
        {
            _cache[_cache_count++] = point.x;
            _cache[_cache_count++] = point.y;
            _cache[_cache_count++] = 0;
        }

        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        doneCurrent();
    }
}

void Canvas::refresh_ellipse_cache_vbo()
{
    const Geo::Polygon &points = _ellipse_cache.shape();
    _cache_count = _cache_len;
    while (_cache_len < points.size() * 3)
    {
        _cache_len *= 2;
    }
    if (_cache_count < _cache_len)
    {
        _cache_len *= 2;
        delete []_cache;
        _cache = new double[_cache_len];
        _cache_count = 0;
        for (const Geo::Point &point : points)
        {
            _cache[_cache_count++] = point.x;
            _cache[_cache_count++] = point.y;
            _cache[_cache_count++] = 0;
        }

        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_STREAM_DRAW);
        doneCurrent();
    }
    else
    {
        _cache_count = 0;
        for (const Geo::Point &point : points)
        {
            _cache[_cache_count++] = point.x;
            _cache[_cache_count++] = point.y;
            _cache[_cache_count++] = 0;
        }

        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[1]); // cache
        glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        doneCurrent();
    }
}

void Canvas::refresh_select_rect_vbo()
{
    double data[12];
    for (int i = 0; i < 4; ++i)
    {
        data[i * 3] = _select_rect[i].x;
        data[i * 3 + 1] = _select_rect[i].y;
        data[i * 3 + 2] = 0;
    }

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _base_VBO[0]); // origin and select rect
    glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(double), 12 * sizeof(double), data);
    doneCurrent();
}

void Canvas::clear_cache()
{
    _cache_count = 0;
    std::fill_n(_selected_index_count, 4, 0);
}

void Canvas::refresh_selected_ibo()
{
    _cache_count = 0;
    unsigned int polyline_index_len = 512, polyline_index_count = 0;
    unsigned int polygon_index_len = 512, polygon_index_count = 0;
    unsigned int circle_index_len = 512, circle_index_count = 0;
    unsigned int curve_index_len = 512, curve_index_count = 0;
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
    unsigned int *circle_indexs = new unsigned int[circle_index_len];
    unsigned int *curve_indexs = new unsigned int[curve_index_len];
    size_t count = 0;
    for (const Geo::Geometry *geo : _editer->selected())
    {
        ++count;
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
            if (count == 1)
            {
                _cache_count = 0;
                switch (geo->type())
                {
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : *static_cast<const Geo::Bezier *>(geo))
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                        check_cache();
                    }
                    break;
                case Geo::Type::BSPLINE:
                    for (const Geo::Point &point : static_cast<const Geo::BSpline *>(geo)->path_points)
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                        check_cache();
                    }
                default:
                    break;
                }
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
                default:
                    break;
                }
            }
            break;
        default:
            continue;
        }
    }

    if (count > 1)
    {
        _cache_count = 0;
    }

    _selected_index_count[0] = polyline_index_count;
    _selected_index_count[1] = polygon_index_count;
    _selected_index_count[2] = circle_index_count;
    _selected_index_count[3] = curve_index_count;

    if (count > 0)
    {
        makeCurrent();
        if (polyline_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[0]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
        }
        if (polygon_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
        }
        if (circle_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[2]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
        }
        if (curve_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[3]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
        }
        doneCurrent();
    }
    
    delete []polyline_indexs;
    delete []polygon_indexs;
    delete []circle_indexs;
    delete []curve_indexs;
}

void Canvas::refresh_selected_ibo(const Geo::Geometry *object)
{
    if (object->type() == Geo::Type::COMBINATION)
    {
        unsigned int polyline_index_len = 512, polyline_index_count = 0;
        unsigned int polygon_index_len = 512, polygon_index_count = 0;
        unsigned int circle_index_len = 512, circle_index_count = 0;
        unsigned int curve_index_len = 512, curve_index_count = 0;
        unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
        unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
        unsigned int *circle_indexs = new unsigned int[circle_index_len];
        unsigned int *curve_indexs = new unsigned int[curve_index_len];
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
            default:
                break;
            }
        }

        _selected_index_count[0] = polyline_index_count;
        _selected_index_count[1] = polygon_index_count;
        _selected_index_count[2] = circle_index_count;
        _selected_index_count[3] = curve_index_count;
        makeCurrent();
        if (polyline_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[0]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
        }
        if (polygon_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
        }
        if (circle_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[2]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
        }
        if (curve_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[3]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
        }
        doneCurrent();

        delete []polyline_indexs;
        delete []polygon_indexs;
        delete []circle_indexs;
        delete []curve_indexs;
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
        std::fill_n(_selected_index_count, 4, 0);
        _cache_count = 0;
        unsigned int IBO_index = _selected_IBO[3];
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            IBO_index = _selected_IBO[0];
            _selected_index_count[0] = index_count;
            break;
        case Geo::Type::POLYGON:
            IBO_index = _selected_IBO[1];
            _selected_index_count[1] = index_count;
            break;
        case Geo::Type::CIRCLE:
        case Geo::Type::ELLIPSE:
            IBO_index = _selected_IBO[2];
            _selected_index_count[2] = index_count;
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : *static_cast<const Geo::Bezier *>(object))
            {
                _cache[_cache_count++] = point.x;
                _cache[_cache_count++] = point.y;
                _cache[_cache_count++] = 0.5;
                check_cache();
            }
            _selected_index_count[3] = index_count;
            break;
        case Geo::Type::BSPLINE:
            for (const Geo::Point &point : static_cast<const Geo::BSpline *>(object)->path_points)
            {
                _cache[_cache_count++] = point.x;
                _cache[_cache_count++] = point.y;
                _cache[_cache_count++] = 0.5;
                check_cache();
            }
            _selected_index_count[3] = index_count;
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
    _cache_count = 0;
    unsigned int polyline_index_len = 512, polyline_index_count = 0;
    unsigned int polygon_index_len = 512, polygon_index_count = 0;
    unsigned int circle_index_len = 512, circle_index_count = 0;
    unsigned int curve_index_len = 512, curve_index_count = 0;
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
    unsigned int *circle_indexs = new unsigned int[circle_index_len];
    unsigned int *curve_indexs = new unsigned int[curve_index_len];
    size_t count = 0;
    for (const Geo::Geometry *geo : objects)
    {
        ++count;
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
            if (count == 1)
            {
                _cache_count = 0;
                switch (geo->type())
                {
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : *static_cast<const Geo::Bezier *>(geo))
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                        check_cache();
                    }
                    break;
                case Geo::Type::BSPLINE:
                    for (const Geo::Point &point : static_cast<const Geo::BSpline *>(geo)->path_points)
                    {
                        _cache[_cache_count++] = point.x;
                        _cache[_cache_count++] = point.y;
                        _cache[_cache_count++] = 0.5;
                        check_cache();
                    }
                default:
                    break;
                }
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
                default:
                    break;
                }
            }
            break;
        default:
            continue;
        }
    }

    if (count > 1)
    {
        _cache_count = 0;
    }

    _selected_index_count[0] = polyline_index_count;
    _selected_index_count[1] = polygon_index_count;
    _selected_index_count[2] = circle_index_count;
    _selected_index_count[3] = curve_index_count;

    if (count > 0)
    {
        makeCurrent();
        if (polyline_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[0]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyline_index_count * sizeof(unsigned int), polyline_indexs, GL_DYNAMIC_DRAW);
        }
        if (polygon_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_index_count * sizeof(unsigned int), polygon_indexs, GL_DYNAMIC_DRAW);
        }
        if (circle_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[2]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, circle_index_count * sizeof(unsigned int), circle_indexs, GL_DYNAMIC_DRAW);
        }
        if (curve_index_count > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _selected_IBO[3]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, curve_index_count * sizeof(unsigned int), curve_indexs, GL_DYNAMIC_DRAW);
        }
        doneCurrent();
    }
    
    delete []polyline_indexs;
    delete []polygon_indexs;
    delete []circle_indexs;
    delete []curve_indexs;
}

void Canvas::refresh_selected_vbo()
{
    std::future<std::tuple<double*, unsigned int, unsigned int*, unsigned int>> polyline_vbo,
        polygon_vbo, circle_vbo, curve_vbo, text_vbo;
    std::future<std::tuple<double*, unsigned int>> circle_point, curve_point;
    bool refresh[5] = {false, false, false, false, false};
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
    }

    makeCurrent();
    if (refresh[2])
    {
        circle_point.wait();
        auto [circle_printable_points, circle_printable_count] = circle_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[5]); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * circle_printable_count, circle_printable_points, GL_DYNAMIC_DRAW);
        delete []circle_printable_points;
        circle_vbo.wait();
        auto [data, data_count, indexs, index_count] = circle_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[2]); // circle
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[3])
    {
        curve_point.wait();
        auto [curve_printable_points, curve_printable_count] = curve_point.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[6]); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * curve_printable_count, curve_printable_points, GL_DYNAMIC_DRAW);
        delete []curve_printable_points;
        curve_vbo.wait();
        auto [data, data_count, indexs, index_count] = curve_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[3]); // curve
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[4])
    {
        text_vbo.wait();
        auto [data, data_count, indexs, index_count] = text_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[4]); // text
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[0])
    {
        polyline_vbo.wait();
        auto [data, data_count, indexs, index_count] = polyline_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[0]); // polyline
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    if (refresh[1])
    {
        polygon_vbo.wait();
        auto [data, data_count, indexs, index_count] = polygon_vbo.get();
        glBindBuffer(GL_ARRAY_BUFFER, _shape_VBO[1]); // polygon
        glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);
        delete []data;
        delete []indexs;
    }
    doneCurrent();
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
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
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
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
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

    _brush_index_count[2] = index_count;
    return std::make_tuple(data, data_count, indexs, index_count);
}


bool Canvas::refresh_catached_points(const double x, const double y, const double distance, std::vector<const Geo::Geometry *> &catched_objects, const bool skip_selected, const bool current_group_only) const
{
    if (!std::any_of(_catch_types, _catch_types + Canvas::catch_count, [](const bool v){ return v; }))
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
    if (!std::any_of(_catch_types, _catch_types + Canvas::catch_count, [](const bool v){ return v; }))
    {
        return false;
    }

    Geo::Point result[Canvas::catch_count]; // Vertex, Center, Foot, Tangency, Intersection
    double dis[Canvas::catch_count] = {DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX};

    for (const Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            {
                const Geo::Polyline &polyline = *static_cast<const Geo::Polyline *>(object);
                if (_catch_types[0])
                {
                    dis[0] = Geo::distance(pos, polyline.front());
                    result[0] = polyline.front();
                }
                for (size_t i = 1, count = polyline.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, polyline[i]); _catch_types[0] && d < dis[0])
                    {
                        result[0] = polyline[i];
                        dis[0] = d;
                    }
                    const Geo::Point center((polyline[i - 1] + polyline[i]) / 2);
                    if (const double d = Geo::distance(pos, center); _catch_types[1] && d < dis[1])
                    {
                        dis[1] = d;
                        result[1] = center;
                    }
                    Geo::Point foot;
                    if (is_painting() && _catch_types[2] && Geo::foot_point(polyline[i - 1], polyline[i], _mouse_press_pos, foot))
                    {
                        if (const double d = Geo::distance(pos, foot); d < dis[2])
                        {
                            dis[2] = d;
                            result[2] = foot;
                        }
                    }
                }
                if (_catch_types[4])
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
                if (_catch_types[0])
                {
                    dis[0] = Geo::distance(pos, polygon.front());
                    result[0] = polygon.front();
                }
                for (size_t i = 1, count = polygon.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, polygon[i]); _catch_types[0] && d < dis[0])
                    {
                        dis[0] = d;
                        result[0] = polygon[i];
                    }
                    const Geo::Point center((polygon[i - 1] + polygon[i]) / 2);
                    if (const double d = Geo::distance(pos, center); _catch_types[1] && d < dis[1])
                    {
                        dis[1] = d;
                        result[1] = center;
                    }
                    Geo::Point foot;
                    if (is_painting() && _catch_types[2] && Geo::foot_point(polygon[i - 1], polygon[i], _mouse_press_pos, foot))
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
                if (_catch_types[0])
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
                if (is_painting())
                {
                    Geo::Point output0, output1;
                    if (_catch_types[3] && Geo::tangency_point(_mouse_press_pos, *c, output0, output1))
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
            }
            break;
        case Geo::Type::ELLIPSE:
            {
                const Geo::Ellipse *e = static_cast<const Geo::Ellipse *>(object);
                if (_catch_types[0])
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
                if (is_painting())
                {
                    Geo::Point output0, output1;
                    if (_catch_types[3] && Geo::tangency_point(_mouse_press_pos, *e, output0, output1))
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
            }
            break;
        case Geo::Type::BSPLINE:
            {
                const Geo::BSpline &bspline = *static_cast<const Geo::BSpline *>(object);
                if (_catch_types[0])
                {
                    dis[0] = Geo::distance(pos, bspline.path_points.front());
                    result[0] = bspline.path_points.front();
                }
                for (size_t i = 1, count = bspline.path_points.size(); i < count; ++i)
                {
                    if (const double d = Geo::distance(pos, bspline.path_points[i]); _catch_types[0] && d < dis[0])
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
                if (_catch_types[0])
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
            }
            break;
        default:
            break;
        }
    }

    if (_catch_types[4])
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
        if (const double d = Geo::distance(pos, _editer->point_cache().front()); _catch_types[0] && d < dis[0])
        {
            dis[0] = d;
            result[0] = _editer->point_cache().front();
        }
        for (size_t i = 1, count = _editer->point_cache().size() - 1; i < count; ++i)
        {
            if (const double d = Geo::distance(pos, _editer->point_cache()[i]); _catch_types[0] && d < dis[0])
            {
                result[0] = _editer->point_cache()[i];
                dis[0] = d;
            }
            const Geo::Point center((_editer->point_cache()[i - 1] + _editer->point_cache()[i]) / 2);
            if (const double d = Geo::distance(pos, center); _catch_types[1] && d < dis[1])
            {
                dis[1] = d;
                result[1] = center;
            }
            Geo::Point foot;
            if (_catch_types[2] && Geo::foot_point(_editer->point_cache()[i - 1], _editer->point_cache()[i], _mouse_press_pos, foot))
            {
                if (const double d = Geo::distance(pos, foot); d < dis[2])
                {
                    dis[2] = d;
                    result[2] = foot;
                }
            }
        }

        if (_catch_types[4])
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
