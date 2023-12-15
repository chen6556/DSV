#include "draw/Canvas.hpp"
#include "draw/GLSL.hpp"
#include <QPalette>
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
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    _cache = new double[_cache_len];
    _input_line.hide();

    QObject::connect(&_input_line, &QTextEdit::textChanged, this, [this]()
                     {
                                                            if (_last_clicked_obj)
                                                            {
                                                                if (dynamic_cast<Container*>(_last_clicked_obj))
                                                                {
                                                                    dynamic_cast<Container*>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                                                                }
                                                                else
                                                                {
                                                                    dynamic_cast<CircleContainer*>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                                                                }
                                                            } });
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
}

void Canvas::paint_cache(QPainter &painter)
{
    if (_int_flags[0] == 3)
    {
        painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
    }
    else
    {
        // painter.setPen(QPen(shape_color, 3));
    }
    painter.setBrush(QColor(250, 250, 250));

    if (!_circle_cache.empty())
    {
        Geo::Circle circle(_circle_cache);
        circle.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
        painter.drawEllipse(circle.center().coord().x - circle.radius(),
                            circle.center().coord().y - circle.radius(),
                            circle.radius() * 2, circle.radius() * 2);
    }

    QPolygonF points;
    if (!_rectangle_cache.empty())
    {
        for (const Geo::Point &point : _rectangle_cache)
        {
            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6], 
                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
        }
        points.pop_back();
        painter.drawPolygon(points);
    }
    if (_editer != nullptr && !_editer->point_cache().empty())
    {
        points.clear();
        for (const Geo::Point &point : _editer->point_cache())
        {
            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6], 
                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
        }
        painter.drawPolyline(points);
        if (_int_flags[0] == 3)
        {
            painter.setPen(QPen(Qt::blue, 6));
            painter.drawPoints(points);
        }
    }

    if (!_reflines.empty())
    {
        painter.setPen(QPen(QColor(0, 140, 255), 3));
        for (const QLineF &line : _reflines)
        {
            painter.drawLine(line.x1() * _canvas_ctm[0] + line.y1() * _canvas_ctm[3] + _canvas_ctm[6],
                line.x1() * _canvas_ctm[1] + line.y1() * _canvas_ctm[4] + _canvas_ctm[7],
                line.x2() * _canvas_ctm[0] + line.y2() * _canvas_ctm[3] + _canvas_ctm[6],
                line.x2() * _canvas_ctm[1] + line.y2() * _canvas_ctm[4] + _canvas_ctm[7]);
        }
        _reflines.clear();
    }
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
    glUniform3d(_uniforms[2], 1.0, 0.0, 0.0);
    glUniform3d(_uniforms[3], 0.0, 1.0, 0.0);

    glGenVertexArrays(1, &_VAO);
    glGenBuffers(3, _VBO);

    glBindVertexArray(_VAO);
    glVertexAttribLFormat(0, 3, GL_DOUBLE, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_DYNAMIC_DRAW);

    double data[24] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(double), data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    glGenBuffers(3, _IBO);

}

void Canvas::resizeGL(int w, int h)
{
    glUniform1i(_uniforms[0], w / 2);
    glUniform1i(_uniforms[1], h / 2);
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
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
        delete indexs;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_indexs_count[1] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]);
        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // 绘制填充色
        glDrawElements(GL_TRIANGLES, _indexs_count[1], GL_UNSIGNED_INT, NULL);
    }

    if (_indexs_count[0] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[0]);
        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _indexs_count[0], GL_UNSIGNED_INT, NULL);
    }

    if (_indexs_count[2] > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]);
        glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // 绘制线 selected
        glDrawElements(GL_LINE_STRIP, _indexs_count[2], GL_UNSIGNED_INT, NULL);
    }

    if (GlobalSetting::get_instance()->setting()["show_points"].toBool())
    {
        glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 1.0f);
        glDrawArrays(GL_POINTS, 0, _points_count);
    }

    if (!_bool_flags[7] && _select_rect.empty() && _editer->point_cache().empty()
        && _rectangle_cache.empty() && _circle_cache.empty())
    {
        return;
    }

    if (!_editer->point_cache().empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count * sizeof(double));
        if (GlobalSetting::get_instance()->setting()["show_points"].toBool())
        {
            glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 1.0f);
            glDrawArrays(GL_POINTS, 0, _cache_count * sizeof(double));
        }
    }
    else if (!_rectangle_cache.empty())
    {
        for (int i = 0; i < 4; ++i)
        {
            _cache[i * 3] = _rectangle_cache[i].coord().x;
            _cache[i * 3 + 1] = _rectangle_cache[i].coord().y;
            _cache[i * 3 + 2] = 0;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), _cache);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // 绘制填充色
        glDrawArrays(GL_POLYGON, 0, 4);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // 绘制线
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

            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
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
            
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        }

        _cache_count /= 3;
        glUniform4f(_uniforms[4], 0.9765f, 0.9765f, 0.9765f, 1.0f); // 绘制填充色
        glDrawArrays(GL_TRIANGLE_FAN, 0, _cache_count);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 1.0f); // 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, _cache_count);
        _cache_count = 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]);
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    if (_bool_flags[7])
    {
        glUniform4f(_uniforms[4], 0.0f, 0.0f, 0.0f, 1.0f); // 画原点
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

        glUniform4f(_uniforms[4], 0.0f, 0.47f, 0.843f, 0.1f);
        glDrawArrays(GL_POLYGON, 4, 4);

        glUniform4f(_uniforms[4], 0.0f, 0.0f, 1.0f, 0.549f);
        glDrawArrays(GL_LINE_LOOP, 4, 4);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    const double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    switch (event->button())
    {
    case Qt::LeftButton:
        if (_bool_flags[1]) // paintable
        {
            switch (_int_flags[0])
            {
            case 0:
                if (!_bool_flags[2]) // not painting
                {
                    _circle_cache = Geo::Circle(real_x1, real_y1, 10);
                }
                else
                {
                    _editer->append(_circle_cache);
                    _circle_cache.clear();
                    _int_flags[1] = _int_flags[0];
                    _int_flags[0] = -1;
                    _bool_flags[1] = false;
                    emit tool_changed(_int_flags[0]);
                    refresh_vbo();
                }
                _bool_flags[2] = !_bool_flags[2];
                break;
            case 1:
            case 3:
                if (_bool_flags[2])
                {
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _cache[_cache_count++] = real_x1;
                    _cache[_cache_count++] = real_y1;
                    _cache[_cache_count++] = 0;
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
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
                    _bool_flags[2] = true;

                    _cache_count = 6;
                    _cache[0] = _cache[3] = real_x1;
                    _cache[1] = _cache[4] = real_y1;
                    _cache[2] = _cache[5] = 0;
                    makeCurrent();
                    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(double), _cache);
                    doneCurrent();
                }
                break;
            case 2:
                if (!_bool_flags[2])
                {
                    _last_point.coord().x = real_x1;
                    _last_point.coord().y = real_y1;
                    _rectangle_cache = Geo::Rectangle(real_x1, real_y1, real_x1 + 2, real_y1 + 2);
                }
                else
                {
                    _editer->append(_rectangle_cache);
                    _rectangle_cache.clear();
                    _int_flags[1] = _int_flags[0];
                    _int_flags[0] = -1;
                    _bool_flags[1] = false;
                    emit tool_changed(_int_flags[0]);
                    refresh_vbo();
                }
                _bool_flags[2] = !_bool_flags[2];
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
                makeCurrent();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
                doneCurrent();
                _select_rect = Geo::Rectangle(real_x1, real_y1, real_x1 + 1, real_y1 + 1);
                _last_point.coord().x = real_x1;
                _last_point.coord().y = real_y1;
                _bool_flags[5] = false;
                _input_line.hide();
            }
            else
            {
                if (std::find(selected_objs.begin(), selected_objs.end(), _clicked_obj) == selected_objs.end())
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->memo()["is_selected"] = true;
                }

                size_t index_len = 512, index_count = 0;
                unsigned int *indexs = new unsigned int[index_len];
                for (const Geo::Geometry *obj : selected_objs)
                {
                    if (obj->memo()["is_selected"].to_bool())
                    {
                        if (obj->memo()["Type"].to_int() == 3)
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
                        }
                    }
                }
                _indexs_count[2] = index_count;
                makeCurrent();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]);
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
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = true;
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
        _bool_flags[4] = false;
        if (_bool_flags[1]) // paintable
        {
            if (_circle_cache.empty() && _rectangle_cache.empty() && _info_labels[1])
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
            _bool_flags[6] = false;
            _last_clicked_obj = _clicked_obj;
            _clicked_obj = nullptr;
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = false;
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
    if (_bool_flags[0]) // 视图可移动
    {
        _canvas_ctm[6] += (canvas_x1 - canvas_x0), _canvas_ctm[7] += (canvas_y1 - canvas_y0);
        _view_ctm[6] -= (real_x1 - real_x0), _view_ctm[7] -= (real_y1 - real_y0);
        _visible_area.translate(real_x0 - real_x1, real_y0 - real_y1);
        makeCurrent();
        glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]);
        glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
        doneCurrent();
        update();
    }
    if (_bool_flags[1] && _bool_flags[2]) // painting
    {
        switch (_int_flags[0])
        {
        case 0:
            _circle_cache.radius() = Geo::distance(real_x1, real_y1,
                                                   _circle_cache.center().coord().x, _circle_cache.center().coord().y);
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius())).c_str());
            }
            break;
        case 1:
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
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]);
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 2 * sizeof(double), &_cache[_cache_count - 3]);
            doneCurrent();
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Length:").append(std::to_string(Geo::distance(_editer->point_cache().back(),
                                                                                                    _editer->point_cache()[_editer->point_cache().size() - 2])))
                                             .c_str());
            }
            break;
        case 2:
            _rectangle_cache = Geo::Rectangle(_last_point, Geo::Point(real_x1, real_y1));
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.coord().y))).c_str());
            }
            break;
        case 3:
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
        if (_bool_flags[4])
        {
            if (!_bool_flags[6])
            {
                _editer->store_backup();
                _bool_flags[6] = true;
            }
            size_t data_len = 513, data_count;
            double *data = new double[data_len];
            double deepth;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);
            for (Geo::Geometry *obj : _editer->selected())
            {
                _editer->translate_points(obj, real_x0, real_y0, real_x1, real_y1, event->modifiers() == Qt::ControlModifier);
                data_count = data_len;
                deepth = obj->memo()["point_deepth"].to_double();
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
                switch (obj->memo()["Type"].to_int())
                {
                case 0:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
                    }
                    break;
                case 1:
                    for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(obj)->shape()))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
                    }
                    break;
                case 3:
                    for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                    {
                        data_count = 0;
                        deepth = item->memo()["point_deepth"].to_double();
                        switch (item->memo()["Type"].to_int())
                        {
                        case 0:
                            for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = deepth;
                            }
                            break;
                        case 1:
                            for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(item)->shape()))
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = deepth;
                            }
                            break;
                        case 20:
                            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                            {
                                data[data_count++] = point.coord().x;
                                data[data_count++] = point.coord().y;
                                data[data_count++] = deepth;
                            }
                            break;
                        default:
                            break;
                        }
                        glBufferSubData(GL_ARRAY_BUFFER, item->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
                    }
                    data_count = 0;
                    break;
                case 20:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
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
            if (event->modifiers() == Qt::ControlModifier)
            {
                refresh_brush_ibo();
            }
            doneCurrent();
            delete data;
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
            _select_rect = Geo::Rectangle(_last_point.coord().x, _last_point.coord().y, real_x1, real_y1);
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
    glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]);
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
    double data[12] = {-10 / _ratio, 0, 0, 10 / _ratio, 0, 0, 0, -10 / _ratio, 0, 0, 10 / _ratio, 0};
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), data);
    doneCurrent();
    _editer->auto_aligning(_clicked_obj, _reflines);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (_bool_flags[1] && _bool_flags[2]) // paintable and painting
        {
            _bool_flags[1] = false;
            _bool_flags[2] = false;
            switch (_int_flags[0])
            {
            case 0:
                _circle_cache.clear();
                update();
                break;
            case 1:
                if (_editer != nullptr)
                {
                    _editer->append_points();
                    update();
                }
                _cache_count = 0;
                break;
            case 2:
                _rectangle_cache.clear();
                update();
                break;
            case 3:
                if (_editer != nullptr)
                {
                    _editer->append_bezier(_bezier_order);
                    update();
                }
                break;
            default:
                break;
            }
            _int_flags[1] = _int_flags[0];
            _int_flags[0] = -1;
            emit tool_changed(_int_flags[0]);
            refresh_vbo();
        }
        else
        {
            if (_bool_flags[5] && _last_clicked_obj->memo()["Type"].to_int() < 2)
            {
                Geo::Rectangle rect(_last_clicked_obj->bounding_rect());
                rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
                _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
                _input_line.move(rect.center().coord().x - _input_line.rect().center().x(),
                                 rect.center().coord().y - _input_line.rect().center().y());
                _input_line.setFocus();
                if (dynamic_cast<Container *>(_last_clicked_obj) != nullptr)
                {
                    _input_line.setText(dynamic_cast<Container *>(_last_clicked_obj)->text());
                    _input_line.moveCursor(QTextCursor::End);
                    _input_line.show();
                }
                else
                {
                    _input_line.setText(dynamic_cast<CircleContainer *>(_last_clicked_obj)->text());
                    _input_line.moveCursor(QTextCursor::End);
                    _input_line.show();
                }
            }
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _last_point = center();
        _canvas_ctm[0] = _canvas_ctm[4] = _canvas_ctm[8] = 1;
        _canvas_ctm[1] = _canvas_ctm[2] = _canvas_ctm[3] = _canvas_ctm[5] = _canvas_ctm[6] = _canvas_ctm[7] = 0;
        _view_ctm[0] = _view_ctm[4] = _view_ctm[8] = 1;
        _view_ctm[1] = _view_ctm[2] = _view_ctm[3] = _view_ctm[5] = _view_ctm[6] = _view_ctm[7] = 0;
        _visible_area = Geo::Rectangle(0, 0, this->geometry().width(), this->geometry().height());
        _ratio = 1;
        makeCurrent();
        glUniform3d(_uniforms[2], _canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6]);
        glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
        {
            double data[12] = {-10, 0, 0, 10, 0, 0, 0, -10, 0, 0, 10, 0};
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]);
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

void Canvas::resizeEvent(QResizeEvent *event)
{
    const QRect rect(this->geometry());
    _visible_area = Geo::Rectangle(0, 0, rect.width(), rect.height());
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);
    return QOpenGLWidget::resizeEvent(event);
}



void Canvas::use_tool(const int value)
{
    switch (value)
    {
    case -1:
    case 0:
    case 1:
    case 2:
    case 3:
        _int_flags[1] = _int_flags[0];
        _int_flags[0] = value;
        _bool_flags[1] = (value != -1);
        _bool_flags[2] = false;
        _editer->point_cache().clear();
        _circle_cache.clear();
        _rectangle_cache.clear();
        // setMouseTracking(value != -1);
        emit tool_changed(_int_flags[0]);
        update();
        break;
    default:
        break;
    }
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

const bool Canvas::is_painting() const
{
    return _bool_flags[2];
}

const bool Canvas::is_typing() const
{
    return _input_line.isVisible();
}

const bool Canvas::is_moving() const
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
    if (_circle_cache.empty() && _rectangle_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _rectangle_cache)
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

Geo::Rectangle Canvas::bounding_rect() const
{
    if (_circle_cache.empty() && _rectangle_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Rectangle();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _rectangle_cache)
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
        return Geo::Rectangle(x0, y0, x1, y1);
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

    return Geo::Rectangle(x0, y0, x1, y1);
}

Geo::Coord Canvas::mouse_position() const
{
    return Geo::Coord(_mouse_pos_1.x(), _mouse_pos_1.y());
}

const bool Canvas::empty() const
{
    return _circle_cache.empty() && _rectangle_cache.empty() &&
           (_editer == nullptr || _editer->graph() == nullptr || _editer->graph()->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false;
    _bool_flags[2] = false;
    _int_flags[1] = _int_flags[0];
    _int_flags[0] = -1;
    emit tool_changed(_int_flags[0]);
    _editer->point_cache().clear();
    _circle_cache.clear();
    _rectangle_cache.clear();
    update();
}

void Canvas::use_last_tool()
{
    if (_bool_flags[2])
    {
        return;
    }
    _int_flags[0] = _int_flags[1];
    if (_int_flags[0] >= 0)
    {
        _bool_flags[1] = true;
        emit tool_changed(_int_flags[0]);
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
    return point.coord().x > _visible_area[0].coord().x && point.coord().x < _visible_area[2].coord().x
        && point.coord().y < _visible_area[2].coord().y && point.coord().y > _visible_area[0].coord().y;
}

bool Canvas::is_visible(const Geo::Polyline &polyline) const
{
    const Geo::Point center(_visible_area.center());
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if ((polyline[i].coord().x > _visible_area[0].coord().x && polyline[i].coord().x < _visible_area[2].coord().x
            && polyline[i].coord().y < _visible_area[2].coord().y && polyline[i].coord().y > _visible_area[0].coord().y)
            || Geo::distance(center, polyline[i - 1], polyline[i]) < len)
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Polygon &polygon) const
{
    const Geo::Point center(_visible_area.center());
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if ((polygon[i].coord().x > _visible_area[0].coord().x && polygon[i].coord().x < _visible_area[2].coord().x
            && polygon[i].coord().y < _visible_area[2].coord().y && polygon[i].coord().y > _visible_area[0].coord().y)
            || Geo::distance(center, polygon[i - 1], polygon[i]) < len)
        {
            return true;
        }
    }

    const Geo::Rectangle rect(polygon.bounding_rect());
    for (const Geo::Point &point : _visible_area)
    {
        if (point.coord().x > rect[0].coord().x && point.coord().x < rect[2].coord().x
            && point.coord().y < rect[2].coord().y && point.coord().y > rect[0].coord().y)
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Circle &circle) const
{
    return circle.center().coord().x > _visible_area[0].coord().x - circle.radius() &&
        circle.center().coord().x < _visible_area[2].coord().x + circle.radius() &&
        circle.center().coord().y < _visible_area[2].coord().y + circle.radius() &&
        circle.center().coord().y > _visible_area[0].coord().y - circle.radius();
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
    double deepth = 1.0;
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
            geo->memo()["point_deepth"] = deepth;
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
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
                    data[data_count++] = deepth;
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
                // if (!container->text().isEmpty())
                // {
                //     painter.setPen(QPen(text_color, 2));
                //     text_rect = font_metrics.boundingRect(container->text());
                //     text_rect.setWidth(text_rect.width() + suffix_text_width);
                //     text_rect.setHeight(4 * text_heigh_ratio + 14 * (container->text().count('\n') + 1) * text_heigh_ratio);
                //     text_rect.translate(points.boundingRect().center() - text_rect.center());
                //     painter.drawText(text_rect, container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                // }
                break;
            case 1:
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
                    data[data_count++] = deepth;
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
                // if (!circlecontainer->text().isEmpty())
                // {
                //     painter.setPen(QPen(text_color, 2));
                //     text_rect = font_metrics.boundingRect(circlecontainer->text());
                //     text_rect.setWidth(text_rect.width() + suffix_text_width);
                //     text_rect.setHeight(4 * text_heigh_ratio + 14 * (circlecontainer->text().count('\n') + 1) * text_heigh_ratio);
                //     text_rect.translate(circlecontainer->center().coord().x - text_rect.center().x(), 
                //         circlecontainer->center().coord().y - text_rect.center().y());
                //     painter.drawText(text_rect, circlecontainer->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                // }
                break;
            case 3:
                geo->memo()["point_count"] = polyline_index_count;
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(geo))
                {
                    item->memo()["point_index"] = data_count / 3;
                    item->memo()["point_deepth"] = deepth;
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
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
                            data[data_count++] = deepth;
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
                    case 1:
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
                            data[data_count++] = deepth;
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
                    case 20:
                        polyline = dynamic_cast<Geo::Polyline *>(item);
                        for (const Geo::Point &point : *polyline)
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = deepth;
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
                    case 21:
                        // if (!is_visible(dynamic_cast<const Geo::Bezier *>(item)->shape()))
                        // {
                        //     continue;
                        // }
                        // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        // {
                        //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        // }
                        // painter.drawPolyline(points);
                        // if (show_points)
                        // {
                        //     painter.setRenderHint(QPainter::Antialiasing);
                        //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                        //     painter.drawPoint(points.front());
                        //     painter.drawPoint(points.back());
                        // }
                        break;
                    default:
                        break;
                    }

                    deepth -= 1e-6;
                    if (deepth <= 0)
                    {
                        deepth = 1.0;
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
            case 20:
                polyline = dynamic_cast<Geo::Polyline *>(geo);
                for (const Geo::Point &point : *polyline)
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = deepth;
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
            case 21:
                // if (geo->memo()["is_selected"].to_bool())
                // {
                //     painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
                //     for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(geo))
                //     {
                //         points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //             point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                //     }
                //     painter.drawPolyline(points);
                //     painter.setPen(QPen(Qt::blue, 6));
                //     painter.drawPoints(points);
                //     painter.setPen(pen_selected);
                //     points.clear();
                // }
                // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                // {
                //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                // }
                // painter.drawPolyline(points);
                // if (show_points)
                // {
                //     painter.setRenderHint(QPainter::Antialiasing);
                //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                //     painter.drawPoint(points.front());
                //     painter.drawPoint(points.back());
                // }
                break;
            default:
                break;
            }
            deepth -= 1e-6;
            if (deepth <= 0)
            {
                deepth = 1.0;
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
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(double) * data_count, data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyline_index_count, polyline_indexs, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    doneCurrent();

    _indexs_count[2] = 0;

    delete data;
    delete polyline_indexs;
    delete polygon_indexs;
}

void Canvas::refresh_vbo(const bool unitary)
{
    size_t data_len = 1026, data_count = 0;
    double *data = new double[data_len];
    double deepth;

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            if (!unitary && !geo->memo()["is_selected"].to_bool())
            {
                continue;
            }

            data_count = 0;
            deepth = geo->memo()["point_deepth"].to_double();
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                for (const Geo::Point &point : dynamic_cast<const Container *>(geo)->shape())
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = deepth;
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
            case 1:
                for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(geo)->shape()))
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = deepth;
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
            case 3:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    data_count = 0;
                    deepth = item->memo()["point_deepth"].to_double();
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
                        for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = deepth;
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
                    case 1:
                        for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(item)->shape()))
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = deepth;
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
                    case 20:
                        for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                        {
                            data[data_count++] = point.coord().x;
                            data[data_count++] = point.coord().y;
                            data[data_count++] = deepth;
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
                    case 21:
                        // if (!is_visible(dynamic_cast<const Geo::Bezier *>(item)->shape()))
                        // {
                        //     continue;
                        // }
                        // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        // {
                        //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        // }
                        // painter.drawPolyline(points);
                        // if (show_points)
                        // {
                        //     painter.setRenderHint(QPainter::Antialiasing);
                        //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                        //     painter.drawPoint(points.front());
                        //     painter.drawPoint(points.back());
                        // }
                        break;
                    default:
                        break;
                    }
                    glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * item->memo()["point_index"].to_ull() * 3,
                        sizeof(double) * data_count, data);
                }
                data_count = 0;
                break;
            case 20:
                for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(geo))
                {
                    data[data_count++] = point.coord().x;
                    data[data_count++] = point.coord().y;
                    data[data_count++] = deepth;
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
            case 21:
                // if (geo->memo()["is_selected"].to_bool())
                // {
                //     painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
                //     for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(geo))
                //     {
                //         points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //             point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                //     }
                //     painter.drawPolyline(points);
                //     painter.setPen(QPen(Qt::blue, 6));
                //     painter.drawPoints(points);
                //     painter.setPen(pen_selected);
                //     points.clear();
                // }
                // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                // {
                //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                // }
                // painter.drawPolyline(points);
                // if (show_points)
                // {
                //     painter.setRenderHint(QPainter::Antialiasing);
                //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                //     painter.drawPoint(points.front());
                //     painter.drawPoint(points.back());
                // }
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

    makeCurrent();
    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            if (!geo->memo()["is_selected"].to_bool())
            {
                continue;
            }

            switch (geo->memo()["Type"].to_int())
            {
            case 0:
            case 1:
            case 20:
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
            case 3:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
                    case 1:
                    case 20:
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
                    case 21:
                        // if (!is_visible(dynamic_cast<const Geo::Bezier *>(item)->shape()))
                        // {
                        //     continue;
                        // }
                        // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        // {
                        //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        // }
                        // painter.drawPolyline(points);
                        // if (show_points)
                        // {
                        //     painter.setRenderHint(QPainter::Antialiasing);
                        //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                        //     painter.drawPoint(points.front());
                        //     painter.drawPoint(points.back());
                        // }
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
            case 21:
                // if (geo->memo()["is_selected"].to_bool())
                // {
                //     painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
                //     for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(geo))
                //     {
                //         points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //             point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                //     }
                //     painter.drawPolyline(points);
                //     painter.setPen(QPen(Qt::blue, 6));
                //     painter.drawPoints(points);
                //     painter.setPen(pen_selected);
                //     points.clear();
                // }
                // for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                // {
                //     points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                //         point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                // }
                // painter.drawPolyline(points);
                // if (show_points)
                // {
                //     painter.setRenderHint(QPainter::Antialiasing);
                //     painter.setPen(QPen(QColor(0, 140, 255), 6));
                //     painter.drawPoint(points.front());
                //     painter.drawPoint(points.back());
                // }
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);

    doneCurrent();
    _indexs_count[2] = index_count;
    delete indexs;
}

void Canvas::refresh_selected_vbo()
{
    size_t data_len = 513, data_count;
    double *data = new double[data_len];
    double deepth;
    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]);
    for (Geo::Geometry *obj : _editer->selected())
    {
        data_count = data_len;
        deepth = obj->memo()["point_deepth"].to_double();
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
        switch (obj->memo()["Type"].to_int())
        {
        case 0:
            for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = deepth;
            }
            break;
        case 1:
            for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(obj)->shape()))
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = deepth;
            }
            break;
        case 3:
            for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
            {
                data_count = 0;
                deepth = item->memo()["point_deepth"].to_double();
                switch (item->memo()["Type"].to_int())
                {
                case 0:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
                    }
                    break;
                case 1:
                    for (const Geo::Point &point : Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(item)->shape()))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
                    }
                    break;
                case 20:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                    {
                        data[data_count++] = point.coord().x;
                        data[data_count++] = point.coord().y;
                        data[data_count++] = deepth;
                    }
                    break;
                default:
                    break;
                }
                glBufferSubData(GL_ARRAY_BUFFER, item->memo()["point_index"].to_ull() * 3 * sizeof(double), data_count * sizeof(double), data);
            }
            data_count = 0;
            break;
        case 20:
            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
            {
                data[data_count++] = point.coord().x;
                data[data_count++] = point.coord().y;
                data[data_count++] = deepth;
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
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
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
            case 1:
                for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(geo)->shape())))
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
            case 3:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
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
                    case 1:
                        for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(dynamic_cast<const CircleContainer *>(item)->shape())))
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polygon_index_count, polygon_indexs, GL_DYNAMIC_DRAW);
    doneCurrent();

    delete polygon_indexs;
}
