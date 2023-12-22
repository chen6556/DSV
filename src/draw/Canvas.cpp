#include "draw/Canvas.hpp"
#include "draw/GLSL.hpp"
#include <QPainterPath>
#include "io/GlobalSetting.hpp"


Canvas::Canvas(QLabel **labels, QWidget *parent)
    : QOpenGLWidget(parent), _info_labels(labels), _input_line(this)
{
    setCursor(Qt::CursorShape::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    init();
}

Canvas::~Canvas()
{
    delete _cache;
}




void Canvas::init()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    setAutoFillBackground(false);

    _cache = new double[_cache_len];
    _input_line.hide();
    _select_rect.clear();
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
}




void Canvas::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.999f, 0.999f, 0.999f, 1.0f);

    unsigned int vertex_shader;
    unsigned int fragment_shader;

    glPointSize(6.0f);
    glLineWidth(3.0f);
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    glUseProgram(_shader_program);
    glUniform3d(_uniforms[2], 1.0, 0.0, 0.0); // vec0
    glUniform3d(_uniforms[3], 0.0, 1.0, 0.0); // vec1

    glGenVertexArrays(1, &_VAO);
    glGenBuffers(4, _VBO);

    glBindVertexArray(_VAO);
    glVertexAttribLFormat(0, 3, GL_DOUBLE, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
    glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_DYNAMIC_DRAW);

    double data[24] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]); // origin and select rect
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    glGenBuffers(4, _IBO);

}

void Canvas::resizeGL(int w, int h)
{
    glUniform1i(_uniforms[0], w / 2); // w
    glUniform1i(_uniforms[1], h / 2); // h
    glViewport(0, 0, w, h);
}

void Canvas::paintGL()
{
    if (!_select_rect.empty())
    {
        _editer->select(_select_rect);
        size_t index_len = 512, index_count = 0;
        unsigned int *indexs = new unsigned int[index_len];
        for (const Geo::Geometry *obj : _editer->selected())
        {
            for (size_t i = 0, index = obj->memo()["point_index"].to_ull(), count = obj->memo()["point_count"].to_ull(); i < count; ++i)
            {
                indexs[index_count++] = index++;
                if (index_count == index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                    delete indexs;
                    indexs = temp;
                }
            }
            indexs[index_count++] = UINT_MAX;
            if (index_count == index_len)
            {
                index_len *= 2;
                unsigned int *temp = new unsigned int[index_len];
                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                delete indexs;
                indexs = temp;
            }
        }
        _indexs_count[2] = index_count;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
        delete indexs;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_indexs_count[1] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]); // polygon
        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // color 绘制填充色
        glDrawElements(GL_TRIANGLES, _indexs_count[1], GL_UNSIGNED_INT, NULL);
    }

    if (_indexs_count[0] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[0]); // polyline
        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _indexs_count[0], GL_UNSIGNED_INT, NULL);
    }

    if (_indexs_count[2] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
        glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
        glDrawElements(GL_LINE_STRIP, _indexs_count[2], GL_UNSIGNED_INT, NULL);

        if (_cache_count > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
            glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
            glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 1.0f); // color
            glDrawArrays(GL_POINTS, 0, _cache_count / 3);
        }
    }

    if (_indexs_count[3] > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[3]); // text
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[3]); // text
        glUniform4f(_uniforms[4], 0.0f, 0.0f, 0.0f, 1.0f); // color

        glEnable(GL_STENCIL_TEST); //开启模板测试
        glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT); //设置模板缓冲区更新方式(若通过则按位反转模板值)
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 1); //初始模板位为0，由于一定通过测试，所以全部会被置为1，而重复绘制区域由于画了两次模板位又归0
        glStencilMask(0x1); //开启模板缓冲区写入
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //第一次绘制只是为了构造模板缓冲区，没有必要显示到屏幕上，所以设置不显示第一遍的多边形
        glDrawElements(GL_TRIANGLES, _indexs_count[3], GL_UNSIGNED_INT, NULL);

        glStencilFunc(GL_NOTEQUAL, 0, 1); //模板值不为0就通过
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0x1);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDrawElements(GL_TRIANGLES, _indexs_count[3], GL_UNSIGNED_INT, NULL);
        glDisable(GL_STENCIL_TEST); //关闭模板测试
    }

    if (GlobalSetting::get_instance()->setting()["show_points"].toBool())
    {
        glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 1.0f); // color
        glDrawArrays(GL_POINTS, 0, _points_count);
    }

    if (!_bool_flags[7] && _select_rect.empty() && _editer->point_cache().empty()
        && _AABBRect_cache.empty() && _circle_cache.empty())
    {
        return;
    }

    if (!_editer->point_cache().empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        if (_tool_flags[0] != Tool::CURVE)
        {
            glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // color
        }
        else
        {
            glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
        }
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
        if (_tool_flags[0] == Tool::CURVE || GlobalSetting::get_instance()->setting()["show_points"].toBool())
        {
            glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 1.0f); // color
            glDrawArrays(GL_POINTS, 0, _cache_count / 3);
        }
    }
    else if (!_AABBRect_cache.empty())
    {
        for (int i = 0; i < 4; ++i)
        {
            _cache[i * 3] = _AABBRect_cache[i].coord().x;
            _cache[i * 3 + 1] = _AABBRect_cache[i].coord().y;
            _cache[i * 3 + 2] = 0;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), _cache);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // color 绘制填充色
        glDrawArrays(GL_POLYGON, 0, 4);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    else if (!_circle_cache.empty())
    {
        const Geo::Polygon points(Geo::circle_to_polygon(_circle_cache));
        _cache_count = _cache_len;
        while (_cache_len < points.size() * 3)
        {
            _cache_len *= 2;
        }
        if (_cache_count < _cache_len)
        {
            _cache_len *= 2;
            delete _cache;
            _cache = new double[_cache_len];
            _cache_count = 0;
            for (const Geo::Point &point : points)
            {
                _cache[_cache_count++] = point.coord().x;
                _cache[_cache_count++] = point.coord().y;
                _cache[_cache_count++] = 0;
            }

            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_DYNAMIC_DRAW);
        }
        else
        {
            _cache_count = 0;
            for (const Geo::Point &point : points)
            {
                _cache[_cache_count++] = point.coord().x;
                _cache[_cache_count++] = point.coord().y;
                _cache[_cache_count++] = 0;
            }
            
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        }

        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // color 绘制填充色
        glDrawArrays(GL_TRIANGLE_FAN, 0, _cache_count / 3);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, _cache_count / 3);
        _cache_count = 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]); // origin and select rect
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    if (_bool_flags[7])
    {
        glUniform4f(_uniforms[4], 0.0f, 0.0f, 0.0f, 1.0f); // color 画原点
        glDrawArrays(GL_LINES, 0, 4);
    }

    if (!_select_rect.empty())
    {
        double data[12];
        for (int i = 0; i < 4; ++i)
        {
            data[i * 3] = _select_rect[i].coord().x;
            data[i * 3 + 1] = _select_rect[i].coord().y;
            data[i * 3 + 2] = 0;
        }
        
        glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(double), 12 * sizeof(double), data);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.0f, 0.47f, 0.843f, 0.1f); // color
        glDrawArrays(GL_POLYGON, 4, 4);

        glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 0.549f); // color
        glDrawArrays(GL_LINE_LOOP, 4, 4);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    for (QPointF &point : _catched_points)
    {
        Geo::Coord coord(canvas_coord_to_real_coord(point.x(), point.y()));
        point.setX(coord.x);
        point.setY(coord.y);
    }
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    const double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    switch (event->button())
    {
    case Qt::LeftButton:
        if (is_paintable()) // paintable
        {
            switch (_tool_flags[0])
            {
            case Tool::CIRCLE:
                if (!is_painting()) // not painting
                {
                    _circle_cache = Geo::Circle(real_x1, real_y1, 10);
                }
                else
                {
                    _editer->append(_circle_cache);
                    _circle_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NONE;
                    _bool_flags[1] = false; // moveable
                    emit tool_changed(_tool_flags[0]);
                    refresh_vbo();
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::POLYLINE:
            case Tool::CURVE:
                if (is_painting())
                {
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _cache[_cache_count++] = real_x1;
                    _cache[_cache_count++] = real_y1;
                    _cache[_cache_count++] = 0;
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
                    if (_cache_count == _cache_len)
                    {
                        _cache_len *= 2;
                        double *temp = new double[_cache_len];
                        std::memmove(temp, _cache, _cache_count * sizeof(double));
                        delete _cache;
                        _cache = temp;
                        glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_DYNAMIC_DRAW);
                    }
                    else
                    {
                        glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 3 * sizeof(double), &_cache[_cache_count - 3]);
                    }
                    doneCurrent();
                }
                else
                {
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _bool_flags[2] = true; // painting
                    _cache_count = 6;
                    _cache[0] = _cache[3] = real_x1;
                    _cache[1] = _cache[4] = real_y1;
                    _cache[2] = _cache[5] = 0;
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
                    glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(double), _cache);
                    doneCurrent();
                }
                break;
            case Tool::RECT:
                if (!is_painting())
                {
                    _last_point.coord().x = real_x1;
                    _last_point.coord().y = real_y1;
                    _AABBRect_cache = Geo::AABBRect(real_x1, real_y1, real_x1 + 2, real_y1 + 2);
                }
                else
                {
                    _editer->append(_AABBRect_cache);
                    _AABBRect_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NONE;
                    _bool_flags[1] = false; // paintable
                    emit tool_changed(_tool_flags[0]);
                    refresh_vbo();
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::TEXT:
                _editer->append_text(real_x1, real_y1);
                _tool_flags[0] = Tool::NONE;
                _bool_flags[1] = _bool_flags[2] = false;
                emit tool_changed(_tool_flags[0]);
                break;
            default:
                break;
            }
            update();
        }
        else
        {
            const bool reset = !(GlobalSetting::get_instance()->setting()["multiple_select"].toBool()
                || event->modifiers() == Qt::ControlModifier);
            _clicked_obj = _editer->select(real_x1, real_y1, reset);
            std::list<Geo::Geometry *> selected_objs = _editer->selected();
            if (_clicked_obj == nullptr)
            {
                _editer->reset_selected_mark();
                _indexs_count[2] = 0;
                _cache_count = 0;
                _select_rect = Geo::AABBRect(real_x1, real_y1, real_x1, real_y1);
                _last_point.coord().x = real_x1;
                _last_point.coord().y = real_y1;
                _bool_flags[5] = false; // is obj selected
                if (_input_line.isVisible() && _last_clicked_obj != nullptr)
                {
                    switch (_last_clicked_obj->type())
                    {
                    case Geo::Type::TEXT:
                        dynamic_cast<Text *>(_last_clicked_obj)->set_text(_input_line.toPlainText(), 
                            GlobalSetting::get_instance()->setting()["text_size"].toInt());
                        break;
                    case Geo::Type::CONTAINER:
                        dynamic_cast<Container *>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        dynamic_cast<CircleContainer *>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                        break;
                    default:
                        break;
                    }
                }
            }
            else
            {
                if (std::find(selected_objs.begin(), selected_objs.end(), _clicked_obj) == selected_objs.end())
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->is_selected() = true;
                }

                size_t index_len = 512, index_count = 0;
                unsigned int *indexs = new unsigned int[index_len];
                for (const Geo::Geometry *obj : selected_objs)
                {
                    if (obj->is_selected())
                    {
                        if (obj->type() == Geo::Type::COMBINATION)
                        {
                            for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                            {
                                for (size_t i = 0, index = item->memo()["point_index"].to_ull(), count = item->memo()["point_count"].to_ull(); i < count; ++i)
                                {
                                    indexs[index_count++] = index++;
                                    if (index_count == index_len)
                                    {
                                        index_len *= 2;
                                        unsigned int *temp = new unsigned int[index_len];
                                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                        delete indexs;
                                        indexs = temp;
                                    }
                                }
                                indexs[index_count++] = UINT_MAX;
                                if (index_count == index_len)
                                {
                                    index_len *= 2;
                                    unsigned int *temp = new unsigned int[index_len];
                                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                    delete indexs;
                                    indexs = temp;
                                }
                            }
                        }
                        else
                        {
                            for (size_t i = 0, index = obj->memo()["point_index"].to_ull(), count = obj->memo()["point_count"].to_ull(); i < count; ++i)
                            {
                                indexs[index_count++] = index++;
                                if (index_count == index_len)
                                {
                                    index_len *= 2;
                                    unsigned int *temp = new unsigned int[index_len];
                                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                    delete indexs;
                                    indexs = temp;
                                }
                            }
                            indexs[index_count++] = UINT_MAX;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete indexs;
                                indexs = temp;
                            }

                            if (obj->type() == Geo::Type::BEZIER)
                            {
                                _cache_count = 0;
                                for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(obj))
                                {
                                    _cache[_cache_count++] = point.coord().x;
                                    _cache[_cache_count++] = point.coord().y;
                                    _cache[_cache_count++] = obj->memo()["point_depth"].to_double();
                                }
                            }
                        }
                    }
                }
                _indexs_count[2] = index_count;
                makeCurrent();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
                delete indexs;

                _bool_flags[4] = true;
                _bool_flags[5] = true;
                bool catched_point = false;
                if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool())
                {
                    const double catch_distance = GlobalSetting::get_instance()->setting()["catch_distance"].toDouble();
                    for (const QPointF &point : _catched_points)
                    {
                        if (Geo::distance(point.x(), point.y(), real_x1, real_y1) < catch_distance)
                        {
                            catched_point = true;
                            Geo::Coord coord(real_coord_to_view_coord(point.x(), point.y()));
                            _mouse_pos_1.setX(coord.x);
                            _mouse_pos_1.setY(coord.y);
                            QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
                            break;
                        }
                    }
                }
                if (!catched_point && GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
                {
                    _editer->auto_aligning(_clicked_obj, real_x1, real_y1, _reflines,
                        GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool());
                }
            }
            _input_line.clear();
            _input_line.hide();
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = true; // view moveable
        break;
    default:
        break;
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        _bool_flags[4] = false; // is obj moveable
        if (is_paintable()) // paintable
        {
            if (_circle_cache.empty() && _AABBRect_cache.empty() && _info_labels[1])
            {
                _info_labels[1]->clear();
            }
        }
        else
        {
            _select_rect.clear();
            _last_point.clear();
            if (_info_labels[1])
            {
                _info_labels[1]->clear();
            }
            _bool_flags[6] = false; // is moving obj
            _last_clicked_obj = _clicked_obj;
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = false; // view moveable
        break;
    default:
        break;
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    const double center_x = size().width() / 2.0, center_y = size().height() / 2.0;
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->localPos();
    double mat[9];
    std::memcpy(mat, _view_ctm, sizeof(double) * 9);
    const double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double real_x0 = _mouse_pos_0.x() * _view_ctm[0] + _mouse_pos_0.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y0 = _mouse_pos_0.x() * _view_ctm[1] + _mouse_pos_0.y() * _view_ctm[4] + _view_ctm[7];
    const double canvas_x0 = real_x0 * _canvas_ctm[0] + real_y0 * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y0 = real_x0 * _canvas_ctm[1] + real_y0 * _canvas_ctm[4] + _canvas_ctm[7];
    const double canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
    if (_info_labels[0])
    {
        _info_labels[0]->setText(std::string("X:").append(std::to_string(static_cast<int>(real_x1))).append(" Y:").append(std::to_string(static_cast<int>(real_y1))).c_str());
    }
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
        case Tool::CIRCLE:
            _circle_cache.radius() = Geo::distance(real_x1, real_y1,
                                                   _circle_cache.center().coord().x, _circle_cache.center().coord().y);
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius())).c_str());
            }
            break;
        case Tool::POLYLINE:
            if (event->modifiers() == Qt::ControlModifier)
            {
                const Geo::Coord &coord =_editer->point_cache().at(_editer->point_cache().size() - 2).coord();
                if (std::abs(real_x1 - coord.x) > std::abs(real_y1 - coord.y))
                {
                    _editer->point_cache().back().coord().x = real_x1;
                    _editer->point_cache().back().coord().y = coord.y;
                }
                else
                {
                    _editer->point_cache().back().coord().x = coord.x;
                    _editer->point_cache().back().coord().y = real_y1;
                }
            }
            else
            {
                _editer->point_cache().back().coord().x = real_x1;
                _editer->point_cache().back().coord().y = real_y1;
            }
            _cache[_cache_count - 3] = _editer->point_cache().back().coord().x;
            _cache[_cache_count - 2] = _editer->point_cache().back().coord().y;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 2 * sizeof(double), &_cache[_cache_count - 3]);
            doneCurrent();
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Length:").append(std::to_string(Geo::distance(_editer->point_cache().back(),
                                                                                                    _editer->point_cache()[_editer->point_cache().size() - 2])))
                                             .c_str());
            }
            break;
        case Tool::RECT:
            _AABBRect_cache = Geo::AABBRect(_last_point, Geo::Point(real_x1, real_y1));
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.coord().y))).c_str());
            }
            break;
        case Tool::CURVE:
            if (_editer->point_cache().size() > _bezier_order && (_editer->point_cache().size() - 2) % _bezier_order == 0) 
            {
                const size_t count = _editer->point_cache().size();
                if (_editer->point_cache()[count - 2].coord().x == _editer->point_cache()[count - 3].coord().x)
                {
                    _editer->point_cache().back().coord().x = _editer->point_cache()[count - 2].coord().x;
                    _editer->point_cache().back().coord().y = real_y1;
                }
                else
                {
                    _editer->point_cache().back().coord().x = real_x1;
                    _editer->point_cache().back().coord().y = (_editer->point_cache()[count - 3].coord().y - _editer->point_cache()[count - 2].coord().y) /
                        (_editer->point_cache()[count - 3].coord().x - _editer->point_cache()[count - 2].coord().x) * 
                        (_mouse_pos_1.x() - _editer->point_cache()[count - 2].coord().x) + _editer->point_cache()[count - 2].coord().y;
                }
            }
            else
            {
                _editer->point_cache().back() = Geo::Point(real_x1, real_y1);
            }
            _cache[_cache_count - 3] = _editer->point_cache().back().coord().x;
            _cache[_cache_count - 2] = _editer->point_cache().back().coord().y;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 2 * sizeof(double), &_cache[_cache_count - 3]);
            doneCurrent();
            break;
        default:
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->clear();
            }
            break;
        }
        update();
    }
    else
    {
        if (is_obj_moveable())
        {
            if (!is_moving_obj())
            {
                _editer->store_backup();
                _bool_flags[6] = true; // is moving obj
            }
            size_t data_len = 513, data_count;
            double *data = new double[data_len];
            double depth;
            size_t selected_count = 0;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
            for (Geo::Geometry *obj : _editer->selected())
            {
                _editer->translate_points(obj, real_x0, real_y0, real_x1, real_y1, event->modifiers() == Qt::ControlModifier);
                data_count = data_len;
                depth = obj->memo()["point_depth"].to_double();
                while (obj->memo()["point_count"].to_ull() * 3 > data_len)
                {
                    data_len *= 2;
                }
                if (data_count < data_len)
                {
                    delete data;
                    data = new double[data_len];
                }
                data_count = 0;
                ++selected_count;
                switch (obj->type())
                {
                case Geo::Type::CONTAINER:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(obj)))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::COMBINATION:
                    for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                    {
                        data_count = 0;
                        depth = item->memo()["point_depth"].to_double();
                        switch (item->type())
                        {
                        case Geo::Type::CONTAINER:
                            for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = depth;
                            }
                            break;
                        case Geo::Type::CIRCLECONTAINER:
                            for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = depth;
                            }
                            break;
                        case Geo::Type::POLYLINE:
                            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = depth;
                            }
                            break;
                        case Geo::Type::BEZIER:
                            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = depth;
                            }
                            break;
                        default:
                            break;
                        }
                        glBufferSubData(GL_ARRAY_BUFFER, item->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
                    }
                    data_count = 0;
                    break;
                case Geo::Type::POLYLINE:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(obj)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    if (selected_count == 1)
                    {
                        _cache_count = 0;
                        for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(obj))
                        {
                            _cache[_cache_count++] = point.coord().x;
                            _cache[_cache_count++] = point.coord().y;
                            _cache[_cache_count++] = depth;
                        }
                    }
                    break;
                default:
                    break;
                }
                if (data_count > 0)
                {
                    glBufferSubData(GL_ARRAY_BUFFER, obj->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
                }
            }
            if (selected_count == 1 && _cache_count > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
                glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
            }
            else
            {
                _cache_count = 0;
            }
            if (event->modifiers() == Qt::ControlModifier)
            {
                refresh_brush_ibo();
            }
            doneCurrent();
            delete data;
            if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
            {
                refresh_text_vbo();
            }
            if (event->modifiers() != Qt::ControlModifier && GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
            {
                _editer->auto_aligning(_clicked_obj, real_x1, real_y1, _reflines,
                    GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool());
            }
            if (_info_labels[1])
            {
                _info_labels[1]->clear();
            }
        }
        else if (!_select_rect.empty())
        {
            _select_rect = Geo::AABBRect(_last_point.coord().x, _last_point.coord().y, real_x1, real_y1);
            if (_info_labels[1])
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.coord().y))).c_str());
            }
        }
        update();
    }

    if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool() && _clicked_obj == nullptr)
    {
        const bool value = GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool();
        Geo::Coord pos(real_x1, real_y1);
        if (_editer->auto_aligning(pos, _reflines, value))
        {
            const double k = mat[0] * mat[4] - mat[3] * mat[1];
            const double x = (mat[4] * (pos.x - mat[6]) - mat[3] * (pos.y - mat[7])) / k;
            const double y = (mat[0] * (pos.y - mat[7]) - mat[1] * (pos.x - mat[6])) / k;
            _mouse_pos_1.setX(x);
            _mouse_pos_1.setY(y);
            QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
        }
    }
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    const double real_x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double canvas_x = real_x * _canvas_ctm[0] + real_y * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y = real_x * _canvas_ctm[1] + real_y * _canvas_ctm[4] + _canvas_ctm[7];
    if (event->angleDelta().y() > 0 && _ratio < 256)
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
    else if (event->angleDelta().y() < 0 && _ratio > (1.0 / 256.0))
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
    makeCurrent();
    glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    double data[12] = {-10 / _ratio, 0, 0, 10 / _ratio, 0, 0, 0, -10 / _ratio, 0, 0, 10 / _ratio, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]); // origin and select rect
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    doneCurrent();
    _editer->set_view_ratio(_ratio);
    _editer->auto_aligning(_clicked_obj, _reflines);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (is_paintable() && is_painting()) // paintable and painting
        {
            _bool_flags[1] = false; // paintable
            _bool_flags[2] = false; // painting
            switch (_tool_flags[0])
            {
            case Tool::CIRCLE:
                _circle_cache.clear();
                update();
                break;
            case Tool::POLYLINE:
                if (_editer != nullptr)
                {
                    _editer->append_points();
                    update();
                }
                _cache_count = 0;
                break;
            case Tool::RECT:
                _AABBRect_cache.clear();
                update();
                break;
            case Tool::CURVE:
                if (_editer != nullptr)
                {
                    _editer->append_bezier(_bezier_order);
                    update();
                }
                _cache_count = 0;
                break;
            default:
                break;
            }
            _tool_flags[1] = _tool_flags[0];
            _tool_flags[0] = Tool::NONE;
            emit tool_changed(_tool_flags[0]);
            refresh_vbo();
        }
        else
        {
            if (is_obj_selected() && (_last_clicked_obj->type() == Geo::Type::TEXT
                || _last_clicked_obj->type() == Geo::Type::CONTAINER
                || _last_clicked_obj->type() == Geo::Type::CIRCLECONTAINER))
            {
                Geo::AABBRect rect(_last_clicked_obj->bounding_rect());
                rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
                _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
                _input_line.move(rect.center().coord().x - _input_line.rect().center().x(),
                                 rect.center().coord().y - _input_line.rect().center().y());
                _input_line.setFocus();
                switch (_last_clicked_obj->type())
                {
                case Geo::Type::TEXT:
                    _input_line.setText(dynamic_cast<Text *>(_last_clicked_obj)->text());
                    break;
                case Geo::Type::CONTAINER:
                    _input_line.setText(dynamic_cast<Container *>(_last_clicked_obj)->text());
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    _input_line.setText(dynamic_cast<CircleContainer *>(_last_clicked_obj)->text());
                    break;
                default:
                    break;
                }
                _input_line.moveCursor(QTextCursor::End);
                _input_line.show();

                if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
                {
                    refresh_text_vbo();
                }
            }
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _last_point = center();
        _editer->set_view_ratio(1.0);
        show_overview();
        makeCurrent();
        glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]); // vec0
        glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
        {
            double data[12] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]); // origin and select rect
            glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
        }
        doneCurrent();
        update();
        break;
    default:
        break;
    }

    QOpenGLWidget::mouseDoubleClickEvent(event);
}

void Canvas::show_overview()
{
    Graph *graph = _editer->graph();
    if (graph->empty())
    {
        update();
        return;
    }
    // 获取graph的边界
    Geo::AABBRect bounding_area = graph->bounding_rect();
    // 整个绘图控件区域作为显示区域
    QRect view_area = this->geometry();
    // 选择合适的缩放倍率
    double height_ratio = view_area.height() / bounding_area.height();
    double width_ratio = view_area.width() / bounding_area.width();
    _ratio = std::min(height_ratio, width_ratio);
    // 缩放减少2%，使其与边界留出一些空间
    _ratio = _ratio * 0.98;

    // 置于控件中间
    double x_offset = (view_area.width() - bounding_area.width() * _ratio) / 2 - bounding_area.left() * _ratio;
    double y_offset = (view_area.height() - bounding_area.height() * _ratio) / 2 - bounding_area.bottom() * _ratio;

    _canvas_ctm[0] = _canvas_ctm[4] = _ratio;
    _canvas_ctm[1] = _canvas_ctm[2] = _canvas_ctm[3] = _canvas_ctm[5] = 0;
    _canvas_ctm[8] = 1;
    _canvas_ctm[6] = x_offset;
    _canvas_ctm[7] = y_offset;

    _view_ctm[0] = _view_ctm[4] = 1 / _ratio;
    _view_ctm[1] = _view_ctm[2] = _view_ctm[3] = _view_ctm[5] = 0;
    _view_ctm[8] = 1;
    _view_ctm[6] = -x_offset / _ratio;
    _view_ctm[7] = -y_offset / _ratio;

    // 可视区域为显示控件区域的反变换
    double x0=0, y0=0, x1=view_area.width(), y1=view_area.height();
    _visible_area = Geo::AABBRect(
        x0 * _view_ctm[0] + y0 * _view_ctm[3] + _view_ctm[6],
        x0 * _view_ctm[1] + y0 * _view_ctm[4] + _view_ctm[7],
        x1 * _view_ctm[0] + y1 * _view_ctm[3] + _view_ctm[6],
        x1 * _view_ctm[1] + y1 * _view_ctm[4] + _view_ctm[7]);

    update();
}


void Canvas::resizeEvent(QResizeEvent *event)
{
    const QRect rect(this->geometry());
    _visible_area = Geo::AABBRect(0, 0, rect.width(), rect.height());
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);
    return QOpenGLWidget::resizeEvent(event);
}



void Canvas::use_tool(const Tool tool)
{
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = tool;
    _bool_flags[1] = (tool != Tool::NONE); // paintable
    _bool_flags[2] = false; // painting
    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    emit tool_changed(_tool_flags[0]);
    update();
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

const bool Canvas::is_catching_cursor() const
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

const size_t Canvas::current_group() const
{
    return _editer->current_group();
}

void Canvas::set_current_group(const size_t index)
{
    assert(index < _editer->graph()->container_groups().size());
    _editer->set_current_group(index);
}

const size_t Canvas::groups_count() const
{
    return _editer->groups_count();
}

void Canvas::set_bezier_order(const size_t order)
{
    _bezier_order = order;
}

const size_t Canvas::bezier_order() const
{
    return _bezier_order;
}




double Canvas::ratio() const
{
    return _ratio;
}

Geo::Point Canvas::center() const
{
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.center().coord().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().coord().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().coord().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().coord().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.coord().x);
            y0 = std::min(y0, point.coord().y);
            x1 = std::max(x1, point.coord().x);
            y1 = std::max(y1, point.coord().y);
        }
    }

    return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
}

Geo::AABBRect Canvas::bounding_rect() const
{
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.center().coord().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().coord().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().coord().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().coord().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::AABBRect(x0, y0, x1, y1);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.coord().x);
            y0 = std::min(y0, point.coord().y);
            x1 = std::max(x1, point.coord().x);
            y1 = std::max(y1, point.coord().y);
        }
    }

    return Geo::AABBRect(x0, y0, x1, y1);
}

Geo::Coord Canvas::mouse_position() const
{
    return Geo::Coord(_mouse_pos_1.x(), _mouse_pos_1.y());
}

const bool Canvas::empty() const
{
    return _circle_cache.empty() && _AABBRect_cache.empty() &&
           (_editer == nullptr || _editer->graph() == nullptr || _editer->graph()->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NONE;
    emit tool_changed(_tool_flags[0]);
    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    update();
}

void Canvas::use_last_tool()
{
    if (is_painting())
    {
        return;
    }
    _tool_flags[0] = _tool_flags[1];
    if (_tool_flags[0] != Tool::NONE)
    {
        _bool_flags[1] = true; // paintable
        emit tool_changed(_tool_flags[0]);
    }
}

void Canvas::set_info_labels(QLabel **labels)
{
    _info_labels = labels;
}

void Canvas::copy()
{
    _stored_mouse_pos = _mouse_pos_1;
    _editer->copy_selected();
}

void Canvas::cut()
{
    _stored_mouse_pos = _mouse_pos_1;
    _editer->cut_selected();
    refresh_vbo();
}

void Canvas::paste()
{
    if (_editer->paste((_mouse_pos_1.x() - _stored_mouse_pos.x()) / _ratio, (_mouse_pos_1.y() - _stored_mouse_pos.y()) / _ratio))
    {
        refresh_vbo();
        refresh_selected_ibo();
        update();
    }
}



bool Canvas::is_visible(const Geo::Point &point) const
{
    return point.coord().x > _visible_area.left() && point.coord().x < _visible_area.right()
        && point.coord().y > _visible_area.bottom() && point.coord().y < _visible_area.top();
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
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (is_visible(polyline[i]) || Geo::distance(center, polyline[i - 1], polyline[i]) < len)
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
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (is_visible(polygon[i - 1]) || Geo::distance(center, polygon[i - 1], polygon[i]) < len)
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
    return circle.center().coord().x > _visible_area.left() - circle.radius() &&
        circle.center().coord().x < _visible_area.right() + circle.radius() &&
        circle.center().coord().y > _visible_area.bottom() - circle.radius() &&
        circle.center().coord().y < _visible_area.top() + circle.radius();
}


Geo::Coord Canvas::real_coord_to_view_coord(const Geo::Coord &input) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (input.x - _view_ctm[6]) - _view_ctm[3] * (input.y - _view_ctm[7])) / k,
        (_view_ctm[0] * (input.y - _view_ctm[7]) - _view_ctm[1] * (input.x - _view_ctm[6])) / k};
}

Geo::Coord Canvas::real_coord_to_view_coord(const double x, const double y) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (x - _view_ctm[6]) - _view_ctm[3] * (y - _view_ctm[7])) / k,
        (_view_ctm[0] * (y - _view_ctm[7]) - _view_ctm[1] * (x - _view_ctm[6])) / k};
}

Geo::Coord Canvas::canvas_coord_to_real_coord(const Geo::Coord &input) const
{
    const double t = (input.y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (input.x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(input.x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}

Geo::Coord Canvas::canvas_coord_to_real_coord(const double x, const double y) const
{
    const double t = (y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}



void Canvas::refresh_vbo()
{
    size_t data_len = 1026, data_count = 0;
    size_t polyline_index_len = 512, polyline_index_count = 0;
    size_t polygon_index_len = 512, polygon_index_count = 0;
    double *data = new double[data_len];
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
    double depth = 1.0;
    Geo::Polygon points;
    Container *container = nullptr;
    Geo::Polyline *polyline = nullptr;
    CircleContainer *circlecontainer = nullptr;

    for (ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            geo->memo()["point_index"] = data_count / 3;
            geo->memo()["point_depth"] = depth;
            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(geo);
                for (size_t i : Geo::ear_cut_to_indexs(container->shape()))
                {
                    polygon_indexs[polygon_index_count++] = data_count / 3 + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                for (const Geo::Point &point : container->shape())
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                container->memo()["point_count"] = container->shape().size();
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<CircleContainer *>(geo);
                points = Geo::circle_to_polygon(circlecontainer->shape());
                for (size_t i : Geo::ear_cut_to_indexs(points))
                {
                    polygon_indexs[polygon_index_count++] = data_count / 3 + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                for (const Geo::Point &point : points)
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                circlecontainer->memo()["point_count"] = data_count / 3 - circlecontainer->memo()["point_index"].to_ull();
                break;
            case Geo::Type::COMBINATION:
                geo->memo()["point_count"] = polyline_index_count;
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(geo))
                {
                    item->memo()["point_index"] = data_count / 3;
                    item->memo()["point_depth"] = depth;
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<Container *>(item);
                        for (size_t i : Geo::ear_cut_to_indexs(container->shape()))
                        {
                            polygon_indexs[polygon_index_count++] = data_count / 3 + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        for (const Geo::Point &point : container->shape())
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        container->memo()["point_count"] = container->shape().size();
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<CircleContainer *>(item);
                        points = Geo::circle_to_polygon(circlecontainer->shape());
                        for (size_t i : Geo::ear_cut_to_indexs(points))
                        {
                            polygon_indexs[polygon_index_count++] = data_count / 3 + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        for (const Geo::Point &point : points)
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        circlecontainer->memo()["point_count"] = data_count / 3 - circlecontainer->memo()["point_index"].to_ull();
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<Geo::Polyline *>(item);
                        for (const Geo::Point &point : *polyline)
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        polyline->memo()["point_count"] = polyline->size();
                        break;
                    case Geo::Type::BEZIER:
                        for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        item->memo()["point_count"] = dynamic_cast<const Geo::Bezier *>(item)->shape().size();
                        break;
                    default:
                        break;
                    }

                    depth -= 1e-6;
                    if (depth <= 0)
                    {
                        depth = 1.0;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                geo->memo()["point_count"] = polyline_index_count - geo->memo()["point_count"].to_ull();
                break;
            case Geo::Type::POLYLINE:
                polyline = dynamic_cast<Geo::Polyline *>(geo);
                for (const Geo::Point &point : *polyline)
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                polyline->memo()["point_count"] = polyline->size();
                break;
            case Geo::Type::BEZIER:
                for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                geo->memo()["point_count"] = dynamic_cast<const Geo::Bezier *>(geo)->shape().size();
                break;
            default:
                break;
            }
            depth -= 1e-6;
            if (depth <= 0)
            {
                depth = 1.0;
            }
            if (polyline_index_count == polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                delete polyline_indexs;
                polyline_indexs = temp;
            }
        }
    }

    _points_count = data_count / 3;
    _indexs_count[0] = polyline_index_count;
    _indexs_count[1] = polygon_index_count;

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
	glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[0]); // polyline
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyline_index_count, polyline_indexs, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]); // polygon
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    doneCurrent();

    _indexs_count[2] = 0;

    delete data;
    delete polyline_indexs;
    delete polygon_indexs;

    if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
    {
        refresh_text_vbo();
    }
}

void Canvas::refresh_vbo(const bool unitary)
{
    size_t data_len = 1026, data_count = 0;
    double *data = new double[data_len];
    double depth;

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            if (!unitary && !geo->is_selected())
            {
                continue;
            }

            data_count = 0;
            depth = geo->memo()["point_depth"].to_double();
            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                for (const Geo::Point &point : dynamic_cast<const Container *>(geo)->shape())
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(geo)))
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    data_count = 0;
                    depth = item->memo()["point_depth"].to_double();
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                        for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::BEZIER:
                        for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                    glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * item->memo()["point_index"].to_ull() * 3,
                        sizeof(double) * data_count, data);
                }
                data_count = 0;
                break;
            case Geo::Type::POLYLINE:
                for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(geo))
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::BEZIER:
                for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                }
                break;
            default:
                break;
            }
            if (data_count > 0)
            {
                glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * geo->memo()["point_index"].to_ull() * 3,
                    sizeof(double) * data_count, data);
            }
        }
    }

    doneCurrent();
    delete data;
}

void Canvas::refresh_selected_ibo()
{
    size_t index_len = 512, index_count = 0;
    unsigned int *indexs = new unsigned int[index_len];

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            if (!geo->is_selected())
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
            case Geo::Type::CIRCLECONTAINER:
            case Geo::Type::POLYLINE:
                for (size_t index = geo->memo()["point_index"].to_ull(), i = 0, count = geo->memo()["point_count"].to_ull(); i < count; ++i)
                {
                    indexs[index_count++] = index + i;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete indexs;
                        indexs = temp;
                    }
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                    case Geo::Type::CIRCLECONTAINER:
                    case Geo::Type::POLYLINE:
                        for (size_t index = item->memo()["point_index"].to_ull(), i = 0, count = item->memo()["point_count"].to_ull(); i < count; ++i)
                        {
                            indexs[index_count++] = index + i;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete indexs;
                                indexs = temp;
                            }
                        }
                        break;
                    case Geo::Type::BEZIER:
                        for (size_t index = item->memo()["point_index"].to_ull(), i = 0, count = item->memo()["point_count"].to_ull(); i < count; ++i)
                        {
                            indexs[index_count++] = index + i;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete indexs;
                                indexs = temp;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                    indexs[index_count++] = UINT_MAX;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete indexs;
                        indexs = temp;
                    }
                }
                break;
            case Geo::Type::BEZIER:
                for (size_t index = geo->memo()["point_index"].to_ull(), i = 0, count = geo->memo()["point_count"].to_ull(); i < count; ++i)
                {
                    indexs[index_count++] = index + i;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete indexs;
                        indexs = temp;
                    }
                }
                break;
            default:
                break;
            }

            indexs[index_count++] = UINT_MAX;
            if (index_count == index_len)
            {
                index_len *= 2;
                unsigned int *temp = new unsigned int[index_len];
                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                delete indexs;
                indexs = temp;
            }
        }
    }

    makeCurrent();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);

    doneCurrent();
    _indexs_count[2] = index_count;
    delete indexs;
}

void Canvas::refresh_selected_vbo()
{
    size_t data_len = 513, data_count;
    double *data = new double[data_len];
    double depth;

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
    for (Geo::Geometry *obj : _editer->selected())
    {
        data_count = data_len;
        depth = obj->memo()["point_depth"].to_double();
        while (obj->memo()["point_count"].to_ull() * 3 > data_len)
        {
            data_len *= 2;
        }
        if (data_count < data_len)
        {
            delete data;
            data = new double[data_len];
        }
        data_count = 0;
        switch (obj->type())
        {
        case Geo::Type::CONTAINER:
            for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = depth;
            }
            break;
        case Geo::Type::CIRCLECONTAINER:
            for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(obj)))
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = depth;
            }
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
            {
                data_count = 0;
                depth = item->memo()["point_depth"].to_double();
                switch (item->type())
                {
                case Geo::Type::CONTAINER:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::POLYLINE:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = depth;
                    }
                    break;
                default:
                    break;
                }
                glBufferSubData(GL_ARRAY_BUFFER, item->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
            }
            data_count = 0;
            break;
        case Geo::Type::POLYLINE:
            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = depth;
            }
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(obj)->shape())
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = depth;
            }
            break;
        default:
            break;
        }
        if (data_count > 0)
        {
            glBufferSubData(GL_ARRAY_BUFFER, obj->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
        }
    }
    doneCurrent();
    delete data;
}

void Canvas::refresh_brush_ibo()
{
    size_t polygon_index_len = 512, polygon_index_count = 0;
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];

    for (ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                for (size_t i : Geo::ear_cut_to_indexs(dynamic_cast<const Container *>(geo)->shape()))
                {
                    polygon_indexs[polygon_index_count++] = geo->memo()["point_index"].to_ull() + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(geo))))
                {
                    polygon_indexs[polygon_index_count++] = geo->memo()["point_index"].to_ull() / 3 + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                        for (size_t i : Geo::ear_cut_to_indexs(dynamic_cast<const Container *>(item)->shape()))
                        {
                            polygon_indexs[polygon_index_count++] = item->memo()["point_index"].to_ull() + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item))))
                        {
                            polygon_indexs[polygon_index_count++] = item->memo()["point_index"].to_ull() + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete polygon_indexs;
                                polygon_indexs = temp;
                            }
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

    _indexs_count[1] = polygon_index_count;

    makeCurrent();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]); // polygon
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    doneCurrent();

    delete polygon_indexs;
}

void Canvas::refresh_text_vbo()
{
    if (!GlobalSetting::get_instance()->setting()["show_text"].toBool())
    {
        _indexs_count[3] = 0;
        return;
    }

    QPainterPath path;
    const QFont font("SimSun", GlobalSetting::get_instance()->setting()["text_size"].toInt());
    const QFontMetrics font_metrics(font);
    QRectF text_rect;

    const Container *container = nullptr;
    const CircleContainer *circlecontainer = nullptr;
    Geo::Coord coord;
    double depth;
    Geo::Polygon points;
    size_t offset;
    int string_index;
    QStringList strings;

    size_t data_len = 4104, data_count = 0;
    double *data = new double[data_len];
    size_t index_len = 1368, index_count = 0;
    unsigned int *indexs = new unsigned int[index_len];
    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                container = dynamic_cast<const Container *>(geo);
                if (container->text().isEmpty())
                {
                    continue;
                }
                coord = container->bounding_rect().center().coord();
                strings = container->text().split('\n');
                string_index = 0;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y + text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                depth = container->memo()["point_depth"].to_double();
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<const CircleContainer *>(geo);
                if (circlecontainer->text().isEmpty())
                {
                    continue;
                }
                coord = circlecontainer->bounding_rect().center().coord();
                strings = circlecontainer->text().split('\n');
                string_index = 0;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y + text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                depth = circlecontainer->memo()["point_depth"].to_double();
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<const Container *>(item);
                        if (container->text().isEmpty())
                        {
                            continue;
                        }
                        coord = container->bounding_rect().center().coord();
                        text_rect = font_metrics.boundingRect(container->text());
                        strings = container->text().split('\n');
                        string_index = 0;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y + text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        depth = container->memo()["point_depth"].to_double();
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<const CircleContainer *>(item);
                        if (circlecontainer->text().isEmpty())
                        {
                            continue;
                        }
                        coord = circlecontainer->bounding_rect().center().coord();
                        text_rect = font_metrics.boundingRect(circlecontainer->text());
                        strings = circlecontainer->text().split('\n');
                        string_index = 0;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y + text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        depth = circlecontainer->memo()["point_depth"].to_double();
                        break;
                    default:
                        break;
                    }
                    for (const QPolygonF &polygon : path.toSubpathPolygons())
                    {
                        offset = data_count / 3;
                        for (const QPointF &point : polygon)
                        {
                            points.append(Geo::Point(point.x(), point.y()));
                            data[data_count++] = point.x();
                            data[data_count++] = point.y();
                            data[data_count++] = depth;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete data;
                                data = temp;
                            }
                        }

                        for (size_t i : Geo::ear_cut_to_indexs(points))
                        {
                            indexs[index_count++] = offset + i;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete indexs;
                                indexs = temp;
                            }
                        }

                        points.clear();
                        indexs[index_count++] = UINT_MAX;
                        if (index_count == index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                            delete indexs;
                            indexs = temp;
                        }
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
                for (const QPointF &point : polygon)
                {
                    points.append(Geo::Point(point.x(), point.y()));
                    data[data_count++] = point.x();
                    data[data_count++] = point.y();
                    data[data_count++] = depth;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete data;
                        data = temp;
                    }
                }

                for (size_t i : Geo::ear_cut_to_indexs(points))
                {
                    indexs[index_count++] = offset + i;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete indexs;
                        indexs = temp;
                    }
                }

                points.clear();
                indexs[index_count++] = UINT_MAX;
                if (index_count == index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                    delete indexs;
                    indexs = temp;
                }
            }
            path.clear();
        }
    }

    _indexs_count[3] = index_count;

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[3]); // text
	glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[3]); // text
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_count, indexs, GL_DYNAMIC_DRAW);
    doneCurrent();

    delete data;
    delete indexs;
}
