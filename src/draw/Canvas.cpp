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
    delete []_cache;
    delete _menu;
    delete _up;
    delete _down;
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

    _menu = new QMenu(this);
    _up = new QAction("Up");
    _down = new QAction("Down");
    _menu->addAction(_up);
    _menu->addAction(_down);
    _menu->setStyleSheet("color: rgb(230, 230, 230);"
        "background-color: rgb(50, 50, 51);"
        "selection-color: rgb(230, 230, 230);"
        "selection-background-color: rgb(0, 85, 127);");
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 抗锯齿
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

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

    glGenVertexArrays(1, &_VAO);
    glGenBuffers(5, _VBO);

    glBindVertexArray(_VAO);
    glVertexAttribLFormat(0, 3, GL_DOUBLE, 0);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[4]); // reflines
    glBufferData(GL_ARRAY_BUFFER, 30 * sizeof(double), _refline_points, GL_DYNAMIC_DRAW);

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

    _canvas_ctm[7] += (h - _canvas_height);
    glUniform3d(_uniforms[3], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]); // vec1
    _view_ctm[7] += (h - _canvas_height) / _ratio;
    _canvas_width = w, _canvas_height = h;

    _visible_area = Geo::AABBRect(0, 0, w, h);
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);    
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
            switch (obj->type())
            {
            case Geo::Type::CONTAINER:
            case Geo::Type::CIRCLECONTAINER:
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
                for (size_t index = obj->point_index, i = 0, count = obj->point_count; i < count; ++i)
                {
                    indexs[index_count++] = index++;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete []indexs;
                        indexs = temp;
                    }
                }
                indexs[index_count++] = UINT_MAX;
                if (index_count == index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                    delete []indexs;
                    indexs = temp;
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                {
                    if (item->type() == Geo::Type::TEXT)
                    {
                        continue;
                    }
                    for (size_t index = item->point_index, i = 0, count = item->point_count; i < count; ++i)
                    {
                        indexs[index_count++] = index++;
                        if (index_count == index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                            delete []indexs;
                            indexs = temp;
                        }
                    }
                    indexs[index_count++] = UINT_MAX;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete []indexs;
                        indexs = temp;
                    }
                }
                break;
            default:
                continue;
            }
        }
        _indexs_count[2] = index_count;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
        delete []indexs;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (_indexs_count[0] > 0) // polyline
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[0]); // polyline
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线 normal
        glDrawElements(GL_LINE_STRIP, _indexs_count[0], GL_UNSIGNED_INT, NULL);
    }

    if (_indexs_count[2] > 0) // selected
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
        glUniform4f(_uniforms[4], 1.0f, 0.0f, 0.0f, 1.0f); // color 绘制线 selected
        glDrawElements(GL_LINE_STRIP, _indexs_count[2], GL_UNSIGNED_INT, NULL);
    }

    if (_editer->point_cache().empty() && _cache_count > 0) // cache
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glDrawArrays(GL_POINTS, 0, _cache_count / 3);
    }
    else if (_measure_flags[0])
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(double), _cache);
        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 2);
        glLineWidth(1.4f);
    }

    if (_indexs_count[3] > 0) // text
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[3]); // text
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[3]); // text
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color

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

    if (!_reflines.empty()) // reflines
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
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[4]); // reflines
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);
        glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(double), _refline_points);

        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, i / 3);
        glLineWidth(1.4f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);
    if (_indexs_count[1] > 0) // polygon
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[1]); // polygon
        glUniform4f(_uniforms[4], 0.831372f, 0.843137f, 0.850980f, 0.078431f); // color 绘制填充色
        glDrawElements(GL_TRIANGLES, _indexs_count[1], GL_UNSIGNED_INT, NULL);
    }

    if (GlobalSetting::get_instance()->setting()["show_points"].toBool())
    {
        glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
        glDrawArrays(GL_POINTS, 0, _points_count);
    }

    if (!_editer->point_cache().empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        if (_tool_flags[0] != Tool::CURVE)
        {
            glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color
        }
        else
        {
            glUniform4f(_uniforms[4], 1.0f, 0.549f, 0.0f, 1.0f); // color
        }
        glDrawArrays(GL_LINE_STRIP, 0, _cache_count / 3);
        if (_tool_flags[0] == Tool::CURVE || GlobalSetting::get_instance()->setting()["show_points"].toBool())
        {
            glUniform4f(_uniforms[4], 0.031372f, 0.572549f, 0.815686f, 1.0f); // color
            glDrawArrays(GL_POINTS, 0, _cache_count / 3);
        }
    }
    else if (!_AABBRect_cache.empty())
    {
        for (int i = 0; i < 4; ++i)
        {
            _cache[i * 3] = _AABBRect_cache[i].x;
            _cache[i * 3 + 1] = _AABBRect_cache[i].y;
            _cache[i * 3 + 2] = 0;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * sizeof(double), _cache);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.831372f, 0.843137f, 0.850980f, 0.078431f); // color 绘制填充色
        glDrawArrays(GL_POLYGON, 0, 4);

        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
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
            delete []_cache;
            _cache = new double[_cache_len];
            _cache_count = 0;
            for (const Geo::Point &point : points)
            {
                _cache[_cache_count++] = point.x;
                _cache[_cache_count++] = point.y;
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
                _cache[_cache_count++] = point.x;
                _cache[_cache_count++] = point.y;
                _cache[_cache_count++] = 0;
            }
            
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
            glEnableVertexAttribArray(0);
            glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
        }

        glUniform4f(_uniforms[4], 0.831372f, 0.843137f, 0.850980f, 0.078431f); // color 绘制填充色
        glDrawArrays(GL_TRIANGLE_FAN, 0, _cache_count / 3);

        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 绘制线
        glDrawArrays(GL_LINE_LOOP, 0, _cache_count / 3);
        _cache_count = 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, _VBO[1]); // origin and select rect
    glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
    glEnableVertexAttribArray(0);

    if (_bool_flags[7]) // origin
    {
        glUniform4f(_uniforms[4], 1.0f, 1.0f, 1.0f, 1.0f); // color 画原点
        glDrawArrays(GL_LINES, 0, 4);
    }

    if (!_select_rect.empty())
    {
        double data[12];
        for (int i = 0; i < 4; ++i)
        {
            data[i * 3] = _select_rect[i].x;
            data[i * 3 + 1] = _select_rect[i].y;
            data[i * 3 + 2] = 0;
        }
        
        glBufferSubData(GL_ARRAY_BUFFER, 12 * sizeof(double), 12 * sizeof(double), data);
        glVertexAttribLPointer(0, 3, GL_DOUBLE, 3 * sizeof(double), NULL);
        glEnableVertexAttribArray(0);

        glUniform4f(_uniforms[4], 0.0f, 0.47f, 0.843f, 0.1f); // color
        glDrawArrays(GL_POLYGON, 4, 4);

        glUniform4f(_uniforms[4], 0.0f, 1.0f, 0.0f, 0.549f); // color
        glDrawArrays(GL_LINE_LOOP, 4, 4);
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
                    _tool_flags[0] = Tool::NOTOOL;
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
                        delete []_cache;
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
                    _last_point.x = real_x1;
                    _last_point.y = real_y1;
                    _AABBRect_cache = Geo::AABBRect(real_x1, real_y1, real_x1 + 2, real_y1 + 2);
                }
                else
                {
                    _editer->append(_AABBRect_cache);
                    _AABBRect_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NOTOOL;
                    _bool_flags[1] = false; // paintable
                    emit tool_changed(_tool_flags[0]);
                    refresh_vbo();
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::TEXT:
                _editer->append_text(real_x1, real_y1);
                _tool_flags[0] = Tool::NOTOOL;
                _bool_flags[1] = _bool_flags[2] = false;
                emit tool_changed(_tool_flags[0]);
                refresh_vbo();
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
            case Operation::RINGARRAY:
                _operation = Operation::NOOPERATION;
                if (_editer->ring_array(_object_cache, real_x1, real_y1,
                        GlobalSetting::get_instance()->ui()->array_item->value()))
                {
                    refresh_vbo();
                    refresh_selected_ibo();
                }
                emit tool_changed(Tool::NOTOOL);
                _object_cache.clear();
                return update();
            default:
                break;
            }

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
                _last_point.x = real_x1;
                _last_point.y = real_y1;
                _bool_flags[5] = false; // is obj selected

                switch (_operation)
                {
                case Operation::MIRROR:
                    _operation = Operation::NOOPERATION;
                    emit tool_changed(Tool::NOTOOL);
                    _object_cache.clear();
                    break;
                default:
                    break;
                }

                switch (_tool_flags[0])
                {
                case Tool::MEASURE:
                    if (!_measure_flags[0] || _measure_flags[1])
                    {
                        Geo::Point coord(real_x1, real_y1);
                        catch_point(real_x1, real_y1, coord, 12.0 / _ratio);
                        _measure_flags[0] = true;
                        _measure_flags[1] = false;
                        _cache[2] = _cache[5] = 0.51;
                        _cache[0] = _cache[3] = coord.x;
                        _cache[1] = _cache[4] = coord.y;
                    }
                    else
                    {
                        Geo::Point coord(real_x1, real_y1);
                        catch_point(real_x1, real_y1, coord, 12.0 / _ratio);
                        _measure_flags[1] = true;
                        _cache[3] = coord.x;
                        _cache[4] = coord.y;
                        _info_labels[1]->setText("Length:" +
                            QString::number(Geo::distance(_cache[0], _cache[1],
                                _cache[3], _cache[4])));
                    }
                    return update();
                default:
                    break;
                }

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
                    refresh_text_vbo();
                    _input_line.clear();
                    _input_line.hide();
                }
            }
            else
            {
                _pressed_obj = _clicked_obj;
                if (std::find(selected_objs.begin(), selected_objs.end(), _clicked_obj) == selected_objs.end())
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->is_selected = true;
                }

                switch (_operation)
                {
                case Operation::MIRROR:
                    if (_editer->mirror(_object_cache, _clicked_obj, event->modifiers() == Qt::ControlModifier))
                    {
                        refresh_vbo();
                        refresh_selected_ibo();
                    }
                    _object_cache.clear();
                    _operation = Operation::NOOPERATION;
                    emit tool_changed(Tool::NOTOOL);
                    return update();
                default:
                    break;
                }

                switch (_tool_flags[0])
                {
                case Tool::MEASURE:
                    if (!_measure_flags[0] || _measure_flags[1])
                    {
                        Geo::Point coord(real_x1, real_y1);
                        if (catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_flags[0] = true;
                            _measure_flags[1] = false;
                            _cache[2] = _cache[5] = 0.51;
                            _cache[0] = _cache[3] = coord.x;
                            _cache[1] = _cache[4] = coord.y;
                            return update();
                        }
                    }
                    else
                    {
                        Geo::Point coord(real_x1, real_y1);
                        if (catch_point(real_x1, real_y1, coord, 12.0 / _ratio))
                        {
                            _measure_flags[1] = true;
                            _cache[3] = coord.x;
                            _cache[4] = coord.y;
                            _info_labels[1]->setText("Length:" +
                                QString::number(Geo::distance(_cache[0], _cache[1],
                                    _cache[3], _cache[4])));
                            return update();
                        }
                    }
                    break;
                default:
                    break;
                }

                size_t index_len = 512, index_count = 0;
                unsigned int *indexs = new unsigned int[index_len];
                for (const Geo::Geometry *obj : selected_objs)
                {
                    if (obj->is_selected)
                    {
                        if (obj->type() == Geo::Type::COMBINATION)
                        {
                            for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                            {
                                for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                                {
                                    indexs[index_count++] = index++;
                                    if (index_count == index_len)
                                    {
                                        index_len *= 2;
                                        unsigned int *temp = new unsigned int[index_len];
                                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                        delete []indexs;
                                        indexs = temp;
                                    }
                                }
                                indexs[index_count++] = UINT_MAX;
                                if (index_count == index_len)
                                {
                                    index_len *= 2;
                                    unsigned int *temp = new unsigned int[index_len];
                                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                    delete []indexs;
                                    indexs = temp;
                                }
                            }
                        }
                        else
                        {
                            for (size_t i = 0, index = obj->point_index, count = obj->point_count; i < count; ++i)
                            {
                                indexs[index_count++] = index++;
                                if (index_count == index_len)
                                {
                                    index_len *= 2;
                                    unsigned int *temp = new unsigned int[index_len];
                                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                    delete []indexs;
                                    indexs = temp;
                                }
                            }
                            indexs[index_count++] = UINT_MAX;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete []indexs;
                                indexs = temp;
                            }

                            if (obj->type() == Geo::Type::BEZIER)
                            {
                                _cache_count = 0;
                                for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(obj))
                                {
                                    _cache[_cache_count++] = point.x;
                                    _cache[_cache_count++] = point.y;
                                    _cache[_cache_count++] = 0.5;
                                }
                            }
                        }
                    }
                }
                _indexs_count[2] = index_count;
                makeCurrent();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
                if (selected_objs.size() == 1 && _cache_count > 0)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
                    glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
                }
                else
                {
                    _cache_count = 0;
                }
                doneCurrent();
                delete []indexs;

                _bool_flags[4] = true; // is obj moveable
                _bool_flags[5] = true; // is obj selected

                switch (_tool_flags[0])
                {
                case Tool::MEASURE:
                    _measure_flags[0] = _measure_flags[1] = false;
                    switch (_clicked_obj->type())
                    {
                    case Geo::Type::TEXT:
                        _info_labels[1]->setText("X:" + QString::number(dynamic_cast<const Text*>(_clicked_obj)->center().x) +
                            "Y:" + QString::number(dynamic_cast<const Text*>(_clicked_obj)->center().y));
                        break;
                    case Geo::Type::CONTAINER:
                        _info_labels[1]->setText("X:" + QString::number(dynamic_cast<const Container*>(_clicked_obj)->center().x) +
                            " Y:" + QString::number(dynamic_cast<const Container*>(_clicked_obj)->center().y) +
                            " Length:" + QString::number(_clicked_obj->length()) +
                            " Area:" + QString::number(dynamic_cast<const Container*>(_clicked_obj)->area()));
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        _info_labels[1]->setText("X:" + QString::number(dynamic_cast<const CircleContainer*>(_clicked_obj)->center().x) +
                            " Y:" + QString::number(dynamic_cast<const CircleContainer*>(_clicked_obj)->center().y) +
                            " Radius:" + QString::number(dynamic_cast<const CircleContainer*>(_clicked_obj)->radius()));
                        break;
                    case Geo::Type::POLYLINE:
                        _info_labels[1]->setText("Length:" + QString::number(dynamic_cast<const Geo::Polyline*>(_clicked_obj)->length()));
                        break;
                    case Geo::Type::BEZIER:
                        _info_labels[1]->setText("Order:" + QString::number(dynamic_cast<const Geo::Bezier*>(_clicked_obj)->order()) +
                            " Length:" + QString::number(dynamic_cast<const Geo::Bezier*>(_clicked_obj)->length()));
                        break;
                    default:
                        break;
                    }
                    return update();
                default:
                    break;
                }

                Geo::Point coord;
                if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool() &&
                    catch_cursor(real_x1, real_y1, coord, GlobalSetting::get_instance()->setting()["catch_distance"].toDouble()))
                {
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
        if (!is_paintable())
        {
            _operation = Operation::NOOPERATION;
            _object_cache.clear();
            _clicked_obj = _editer->select(real_x1, real_y1, true);
            if (_clicked_obj != nullptr)
            {
                size_t index_len = 512, index_count = 0;
                unsigned int *indexs = new unsigned int[index_len];
                if (_clicked_obj->type() == Geo::Type::COMBINATION)
                {
                    for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(_clicked_obj))
                    {
                        for (size_t i = 0, index = item->point_index, count = item->point_count; i < count; ++i)
                        {
                            indexs[index_count++] = index++;
                            if (index_count == index_len)
                            {
                                index_len *= 2;
                                unsigned int *temp = new unsigned int[index_len];
                                std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                                delete []indexs;
                                indexs = temp;
                            }
                        }
                        indexs[index_count++] = UINT_MAX;
                        if (index_count == index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                            delete []indexs;
                            indexs = temp;
                        }
                    }
                }
                else
                {
                    for (size_t i = 0, index = _clicked_obj->point_index, count = _clicked_obj->point_count; i < count; ++i)
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
                makeCurrent();
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);
                doneCurrent();
                delete []indexs;

                const QAction *a = _menu->exec(QCursor::pos());
                if (a == _up)
                {
                    _editer->up(_clicked_obj);
                    refresh_vbo();
                }
                else if (a == _down)
                {
                    _editer->down(_clicked_obj);
                    refresh_vbo();
                }
            }
            use_tool(Tool::NOTOOL);
        }
        else
        {
            cancel_painting();
            _editer->reset_selected_mark();
            refresh_selected_ibo();
        }
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
            if (_circle_cache.empty() && _AABBRect_cache.empty())
            {
                _info_labels[1]->clear();
            }
        }
        else
        {
            _select_rect.clear();
            _last_point.clear();
            if (_tool_flags[0] != Tool::MEASURE)
            {
                _info_labels[1]->clear();
            }
            _bool_flags[6] = false; // is moving obj
            _last_clicked_obj = _clicked_obj;
            _pressed_obj = nullptr;
            update();
        }
        refresh_catached_points();
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

    _info_labels[0]->setText(std::string("X:").append(std::to_string(static_cast<int>(real_x1))).append(" Y:").append(std::to_string(static_cast<int>(real_y1))).c_str());

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
                                                   _circle_cache.center().x, _circle_cache.center().y);

            _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius())).c_str());
            break;
        case Tool::POLYLINE:
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
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 2 * sizeof(double), &_cache[_cache_count - 3]);
            doneCurrent();
            _info_labels[1]->setText(std::string("Length:").append(std::to_string(
                Geo::distance(_editer->point_cache().back(), _editer->point_cache()[_editer->point_cache().size() - 2]))).c_str());
            break;
        case Tool::RECT:
            _AABBRect_cache = Geo::AABBRect(_last_point, Geo::Point(real_x1, real_y1));
            _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.x)))
                .append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.y))).c_str());
            break;
        case Tool::CURVE:
            if (_editer->point_cache().size() > _bezier_order && (_editer->point_cache().size() - 2) % _bezier_order == 0) 
            {
                const size_t count = _editer->point_cache().size();
                if (_editer->point_cache()[count - 2].x == _editer->point_cache()[count - 3].x)
                {
                    _editer->point_cache().back().x = _editer->point_cache()[count - 2].x;
                    _editer->point_cache().back().y = real_y1;
                }
                else
                {
                    _editer->point_cache().back().x = real_x1;
                    _editer->point_cache().back().y = (_editer->point_cache()[count - 3].y - _editer->point_cache()[count - 2].y) /
                        (_editer->point_cache()[count - 3].x - _editer->point_cache()[count - 2].x) * 
                        (_mouse_pos_1.x() - _editer->point_cache()[count - 2].x) + _editer->point_cache()[count - 2].y;
                }
            }
            else
            {
                _editer->point_cache().back() = Geo::Point(real_x1, real_y1);
            }
            _cache[_cache_count - 3] = _editer->point_cache().back().x;
            _cache[_cache_count - 2] = _editer->point_cache().back().y;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 3) * sizeof(double), 2 * sizeof(double), &_cache[_cache_count - 3]);
            doneCurrent();
            break;
        default:
            _info_labels[1]->clear();
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
            bool update_vbo = false;
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
            std::list<Geo::Geometry *> objs = _editer->selected();
            const bool only_one_selected = objs.size() == 1;
            for (Geo::Geometry *obj : objs)
            {
                _editer->translate_points(obj, real_x0, real_y0, real_x1, real_y1, event->modifiers() == Qt::ControlModifier && only_one_selected);
                data_count = data_len;
                while (obj->point_count * 3 > data_len)
                {
                    data_len *= 2;
                }
                if (data_count < data_len)
                {
                    delete []data;
                    data = new double[data_len];
                }
                data_count = 0;

                switch (obj->type())
                {
                case Geo::Type::CONTAINER:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(obj)))
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                        if (event->modifiers() == Qt::ControlModifier && data_len == data_count)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            delete []data;
                            data = temp;
                        }
                    }
                    update_vbo = (event->modifiers() == Qt::ControlModifier && only_one_selected);
                    break;
                case Geo::Type::COMBINATION:
                    for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
                    {
                        data_count = 0;
                        switch (item->type())
                        {
                        case Geo::Type::CONTAINER:
                            for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                            {
                                data[data_count++] = point.x;
                                data[data_count++] = point.y;
                                data[data_count++] = 0.5;
                            }
                            break;
                        case Geo::Type::CIRCLECONTAINER:
                            for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                            {
                                data[data_count++] = point.x;
                                data[data_count++] = point.y;
                                data[data_count++] = 0.5;
                            }
                            break;
                        case Geo::Type::POLYLINE:
                            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                            {
                                data[data_count++] = point.x;
                                data[data_count++] = point.y;
                                data[data_count++] = 0.5;
                            }
                            break;
                        case Geo::Type::BEZIER:
                            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                            {
                                data[data_count++] = point.x;
                                data[data_count++] = point.y;
                                data[data_count++] = 0.5;
                            }
                            break;
                        default:
                            break;
                        }
                        glBufferSubData(GL_ARRAY_BUFFER, item->point_index * 3 * sizeof(double), data_count * sizeof(double), data);
                    }
                    data_count = 0;
                    break;
                case Geo::Type::POLYLINE:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(obj)->shape())
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                        if (event->modifiers() == Qt::ControlModifier && data_len == data_count)
                        {
                            data_len *= 2;
                            double *temp = new double[data_len];
                            delete []data;
                            data = temp;
                        }
                    }
                    if (only_one_selected)
                    {
                        _cache_count = 0;
                        for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(obj))
                        {
                            _cache[_cache_count++] = point.x;
                            _cache[_cache_count++] = point.y;
                            _cache[_cache_count++] = 0.5;
                        }
                    }
                    update_vbo = (event->modifiers() == Qt::ControlModifier && only_one_selected);
                    break;
                default:
                    break;
                }
                if (data_count > 0)
                {
                    glBufferSubData(GL_ARRAY_BUFFER, obj->point_index * 3 * sizeof(double), data_count * sizeof(double), data);
                }
            }
            if (only_one_selected && _cache_count > 0)
            {
                glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
                glBufferSubData(GL_ARRAY_BUFFER, 0, _cache_count * sizeof(double), _cache);
            }
            else
            {
                _cache_count = 0;
            }
            doneCurrent();
            if (only_one_selected && event->modifiers() == Qt::ControlModifier)
            {
                if (update_vbo)
                {
                    refresh_vbo();
                    refresh_selected_ibo();
                }
                else
                {
                    refresh_brush_ibo();
                }
            }
            delete []data;
            if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
            {
                refresh_text_vbo(false);
            }
            if (event->modifiers() != Qt::ControlModifier && GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
            {
                _reflines.clear();
                if (_editer->auto_aligning(_pressed_obj, real_x1, real_y1, _reflines, true))
                {
                    refresh_selected_vbo();
                }
            }
            _info_labels[1]->clear();
        }
        else if (!_select_rect.empty())
        {
            _select_rect = Geo::AABBRect(_last_point.x, _last_point.y, real_x1, real_y1);
            _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.x)))
                .append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.y))).c_str());
        }
        else if (_measure_flags[0] && !_measure_flags[1])
        {
            _cache[3] = real_x1;
            _cache[4] = real_y1;
            _info_labels[1]->setText("Length:" + QString::number(
                Geo::distance(_cache[0], _cache[1], real_x1, real_y1)));
        }
        update();
    }

    Geo::Point coord;
    if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool() && _clicked_obj == nullptr &&
        catch_cursor(real_x1, real_y1, coord, GlobalSetting::get_instance()->setting()["catch_distance"].toDouble()))
    {
        _mouse_pos_1.setX(coord.x);
        _mouse_pos_1.setY(coord.y);
        QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
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

    _reflines.clear();
    _editer->auto_aligning(_pressed_obj, _reflines);
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
                break;
            case Tool::POLYLINE:
                _editer->append_points();
                _cache_count = 0;
                break;
            case Tool::RECT:
                _AABBRect_cache.clear();
                break;
            case Tool::CURVE:
                _editer->append_bezier(_bezier_order);
                _cache_count = 0;
                break;
            default:
                break;
            }
            _tool_flags[1] = _tool_flags[0];
            _tool_flags[0] = Tool::NOTOOL;
            emit tool_changed(_tool_flags[0]);
            refresh_vbo();
            update();
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
                _input_line.move(rect.center().x - _input_line.rect().center().x(),
                                 rect.center().y - _input_line.rect().center().y());
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

    update();
}




void Canvas::use_tool(const Tool tool)
{
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = tool;
    _bool_flags[1] = (tool != Tool::NOTOOL && tool != Tool::MEASURE); // paintable
    _bool_flags[2] = false; // painting

    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    _cache_count = 0;

    _measure_flags[0] = _measure_flags[1] = false;
    _info_labels[1]->clear();

    emit tool_changed(_tool_flags[0]);
    update();
}

void Canvas::set_operation(const Operation operation)
{
    _operation = operation;
    switch (operation)
    {
    case Operation::MIRROR:
        _object_cache = _editer->selected();
        break;
    case Operation::RINGARRAY:
        _object_cache = _editer->selected();
        break;
    default:
        _object_cache.clear();
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
        x0 = std::min(x0, point.x);
        y0 = std::min(y0, point.y);
        x1 = std::max(x1, point.x);
        y1 = std::max(y1, point.y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.center().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
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
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
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
        x0 = std::min(x0, _circle_cache.center().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::AABBRect(x0, y0, x1, y1);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
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
           (_editer->graph() == nullptr || _editer->graph()->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NOTOOL;

    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    _cache_count = 0;

    _measure_flags[0] = _measure_flags[1] = false;
    _info_labels[1]->clear();

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
    _measure_flags[0] = _measure_flags[1] = false;
    _info_labels[1]->clear();
    _bool_flags[1] = _tool_flags[0] != Tool::MEASURE; // paintable
    if (_tool_flags[0] == Tool::NOTOOL)
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
    refresh_vbo();
}

void Canvas::paste()
{
    const double x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    if (_editer->paste(x - _stored_coord.x, y - _stored_coord.y))
    {
        refresh_vbo();
        refresh_selected_ibo();
        update();
    }
}

void Canvas::paste(const double x, const double y)
{
    if (_editer->paste(x - _stored_coord.x, y - _stored_coord.y))
    {
        refresh_vbo();
        refresh_selected_ibo();
        update();
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
        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        if (_cache_count == _cache_len)
        {
            _cache_len *= 2;
            double *temp = new double[_cache_len];
            std::memmove(temp, _cache, _cache_count * sizeof(double));
            delete []_cache;
            _cache = temp;
            glBufferData(GL_ARRAY_BUFFER, _cache_len * sizeof(double), _cache, GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, (_cache_count - 6) * sizeof(double), 6 * sizeof(double), &_cache[_cache_count - 6]);
        }
        doneCurrent();
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
        makeCurrent();
        glBindBuffer(GL_ARRAY_BUFFER, _VBO[2]); // cache
        glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(double), _cache);
        doneCurrent();
    }
    update();
}

void Canvas::polyline_cmd()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    if (_tool_flags[0] == Tool::POLYLINE)
    {
        _editer->append_points();
    }
    else
    {
        _editer->append_bezier(_bezier_order);
    }
    _cache_count = 0;
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NOTOOL;
    emit tool_changed(_tool_flags[0]);
    refresh_vbo();
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
        _tool_flags[0] = Tool::NOTOOL;
        _bool_flags[1] = false; // paintable
        emit tool_changed(_tool_flags[0]);
        refresh_vbo();
    }
    else
    {
        _last_point.x = x;
        _last_point.y = y;
        _AABBRect_cache = Geo::AABBRect(x, y,
            _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6],
            _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7]);
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
        _tool_flags[0] = Tool::NOTOOL;
        _bool_flags[1] = false; // paintable
        emit tool_changed(_tool_flags[0]);
        refresh_vbo();
    }
    else
    {
        _last_point.x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
        _last_point.y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
        _AABBRect_cache = Geo::AABBRect(_last_point.x, _last_point.y,
            _last_point.x + 2, _last_point.y + 2);
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
    _tool_flags[0] = Tool::NOTOOL;
    _bool_flags[1] = false; // moveable
    emit tool_changed(_tool_flags[0]);
    refresh_vbo();
    _bool_flags[2] = !_bool_flags[2]; // painting
    update();
}

void Canvas::text_cmd(const double x, const double y)
{
    _editer->append_text(x, y);
    _tool_flags[0] = Tool::NOTOOL;
    _bool_flags[1] = _bool_flags[2] = false;
    emit tool_changed(_tool_flags[0]);
    refresh_vbo();
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
    return circle.center().x > _visible_area.left() - circle.radius() &&
        circle.center().x < _visible_area.right() + circle.radius() &&
        circle.center().y > _visible_area.bottom() - circle.radius() &&
        circle.center().y < _visible_area.top() + circle.radius();
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

bool Canvas::catch_cursor(const double x, const double y, Geo::Point &coord, const double distance)
{
    double min_distance = DBL_MAX, temp;
    Geo::Point pos;
    for (const Geo::Point &point : _catched_points)
    {
        temp = Geo::distance(point.x, point.y, x, y);
        if (temp < distance / _ratio && temp < min_distance)
        {
            min_distance = temp;
            pos = point;
        }
    }
    if (min_distance < DBL_MAX)
    {
        coord = real_coord_to_view_coord(pos.x, pos.y);
        return true;
    }
    else
    {
        return false;
    }
}

bool Canvas::catch_point(const double x, const double y, Geo::Point &coord, const double distance)
{
    double min_distance = DBL_MAX, temp;
    Geo::Point pos;
    for (const Geo::Point &point : _catched_points)
    {
        temp = Geo::distance(point.x, point.y, x, y);
        if (temp < distance / _ratio && temp < min_distance)
        {
            min_distance = temp;
            pos = point;
        }
    }
    if (min_distance < DBL_MAX)
    {
        coord.x = pos.x;
        coord.y = pos.y;
        return true;
    }
    else
    {
        return false;
    }
}



void Canvas::refresh_vbo()
{
    size_t data_len = 1026, data_count = 0;
    size_t polyline_index_len = 512, polyline_index_count = 0;
    size_t polygon_index_len = 512, polygon_index_count = 0;
    double *data = new double[data_len];
    unsigned int *polyline_indexs = new unsigned int[polyline_index_len];
    unsigned int *polygon_indexs = new unsigned int[polygon_index_len];
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
            geo->point_index = data_count / 3;
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
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                for (const Geo::Point &point : container->shape())
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                container->point_count = container->shape().size();
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
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                for (const Geo::Point &point : points)
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                circlecontainer->point_count = data_count / 3 - circlecontainer->point_index;
                break;
            case Geo::Type::COMBINATION:
                geo->point_count = polyline_index_count;
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(geo))
                {
                    item->point_index = data_count / 3;
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
                                delete []polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        for (const Geo::Point &point : container->shape())
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete []polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        container->point_count = container->shape().size();
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
                                delete []polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        for (const Geo::Point &point : points)
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete []polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        circlecontainer->point_count = data_count / 3 - circlecontainer->point_index;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<Geo::Polyline *>(item);
                        for (const Geo::Point &point : *polyline)
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete []polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        polyline->point_count = polyline->size();
                        break;
                    case Geo::Type::BEZIER:
                        for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        {
                            polyline_indexs[polyline_index_count++] = data_count / 3;
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                            if (polyline_index_count == polyline_index_len)
                            {
                                polyline_index_len *= 2;
                                unsigned int *temp = new unsigned int[polyline_index_len];
                                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                                delete []polyline_indexs;
                                polyline_indexs = temp;
                            }
                        }
                        polyline_indexs[polyline_index_count++] = UINT_MAX;
                        item->point_count = dynamic_cast<const Geo::Bezier *>(item)->shape().size();
                        break;
                    default:
                        break;
                    }

                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                geo->point_count = polyline_index_count - geo->point_count;
                break;
            case Geo::Type::POLYLINE:
                polyline = dynamic_cast<Geo::Polyline *>(geo);
                for (const Geo::Point &point : *polyline)
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                polyline->point_count = polyline->size();
                break;
            case Geo::Type::BEZIER:
                for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                {
                    polyline_indexs[polyline_index_count++] = data_count / 3;
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                    if (polyline_index_count == polyline_index_len)
                    {
                        polyline_index_len *= 2;
                        unsigned int *temp = new unsigned int[polyline_index_len];
                        std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                        delete []polyline_indexs;
                        polyline_indexs = temp;
                    }
                }
                polyline_indexs[polyline_index_count++] = UINT_MAX;
                geo->point_count = dynamic_cast<const Geo::Bezier *>(geo)->shape().size();
                break;
            default:
                break;
            }

            if (polyline_index_count == polyline_index_len)
            {
                polyline_index_len *= 2;
                unsigned int *temp = new unsigned int[polyline_index_len];
                std::memmove(temp, polyline_indexs, polyline_index_count * sizeof(unsigned int));
                delete []polyline_indexs;
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

    delete []data;
    delete []polyline_indexs;
    delete []polygon_indexs;

    refresh_catached_points();
    if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
    {
        refresh_text_vbo();
    }
}

void Canvas::refresh_vbo(const bool unitary)
{
    size_t data_len = 1026, data_count = 0;
    double *data = new double[data_len];

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
            if (!unitary && !geo->is_selected)
            {
                continue;
            }

            data_count = 0;
            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                for (const Geo::Point &point : dynamic_cast<const Container *>(geo)->shape())
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(geo)))
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    data_count = 0;
                    switch (item->type())
                    {
                    case Geo::Type::CONTAINER:
                        for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                        }
                        break;
                    case Geo::Type::BEZIER:
                        for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                        {
                            data[data_count++] = point.x;
                            data[data_count++] = point.y;
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                    glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * item->point_index * 3,
                        sizeof(double) * data_count, data);
                }
                data_count = 0;
                break;
            case Geo::Type::POLYLINE:
                for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(geo))
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                }
                break;
            case Geo::Type::BEZIER:
                for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                {
                    data[data_count++] = point.x;
                    data[data_count++] = point.y;
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                }
                break;
            default:
                break;
            }
            if (data_count > 0)
            {
                glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * geo->point_index * 3,
                    sizeof(double) * data_count, data);
            }
        }
    }

    doneCurrent();
    delete []data;

    refresh_catached_points();
    if (GlobalSetting::get_instance()->setting()["show_text"].toBool())
    {
        refresh_text_vbo(unitary);
    }
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
            if (!geo->is_selected)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
            case Geo::Type::CIRCLECONTAINER:
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
                for (size_t index = geo->point_index, i = 0, count = geo->point_count; i < count; ++i)
                {
                    indexs[index_count++] = index++;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete []indexs;
                        indexs = temp;
                    }
                }
                indexs[index_count++] = UINT_MAX;
                if (index_count == index_len)
                {
                    index_len *= 2;
                    unsigned int *temp = new unsigned int[index_len];
                    std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                    delete []indexs;
                    indexs = temp;
                }
                break;
            case Geo::Type::COMBINATION:
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    if (item->type() == Geo::Type::TEXT)
                    {
                        continue;
                    }
                    for (size_t index = item->point_index, i = 0, count = item->point_count; i < count; ++i)
                    {
                        indexs[index_count++] = index++;
                        if (index_count == index_len)
                        {
                            index_len *= 2;
                            unsigned int *temp = new unsigned int[index_len];
                            std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                            delete []indexs;
                            indexs = temp;
                        }
                    }
                    indexs[index_count++] = UINT_MAX;
                    if (index_count == index_len)
                    {
                        index_len *= 2;
                        unsigned int *temp = new unsigned int[index_len];
                        std::memmove(temp, indexs, index_count * sizeof(unsigned int));
                        delete []indexs;
                        indexs = temp;
                    }
                }
                break;
            default:
                continue;
            }
        }
    }

    makeCurrent();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBO[2]); // selected
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indexs, GL_DYNAMIC_DRAW);

    doneCurrent();
    _indexs_count[2] = index_count;
    delete []indexs;
}

void Canvas::refresh_selected_vbo()
{
    size_t data_len = 513, data_count;
    double *data = new double[data_len];

    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[0]); // points
    for (Geo::Geometry *obj : _editer->selected())
    {
        data_count = data_len;
        while (obj->point_count * 3 > data_len)
        {
            data_len *= 2;
        }
        if (data_count < data_len)
        {
            delete []data;
            data = new double[data_len];
        }
        data_count = 0;
        switch (obj->type())
        {
        case Geo::Type::CONTAINER:
            for (const Geo::Point &point : dynamic_cast<const Container *>(obj)->shape())
            {
                data[data_count++] = point.x;
                data[data_count++] = point.y;
                data[data_count++] = 0.5;
            }
            break;
        case Geo::Type::CIRCLECONTAINER:
            for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(obj)))
            {
                data[data_count++] = point.x;
                data[data_count++] = point.y;
                data[data_count++] = 0.5;
            }
            break;
        case Geo::Type::COMBINATION:
            for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(obj))
            {
                data_count = 0;
                switch (item->type())
                {
                case Geo::Type::CONTAINER:
                    for (const Geo::Point &point : dynamic_cast<const Container *>(item)->shape())
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    for (const Geo::Point &point : Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item)))
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                case Geo::Type::POLYLINE:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(item))
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                case Geo::Type::BEZIER:
                    for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                    {
                        data[data_count++] = point.x;
                        data[data_count++] = point.y;
                        data[data_count++] = 0.5;
                    }
                    break;
                default:
                    break;
                }
                glBufferSubData(GL_ARRAY_BUFFER, item->point_index * 3 * sizeof(double), data_count * sizeof(double), data);
            }
            data_count = 0;
            break;
        case Geo::Type::POLYLINE:
            for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(obj))
            {
                data[data_count++] = point.x;
                data[data_count++] = point.y;
                data[data_count++] = 0.5;
            }
            break;
        case Geo::Type::BEZIER:
            for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(obj)->shape())
            {
                data[data_count++] = point.x;
                data[data_count++] = point.y;
                data[data_count++] = 0.5;
            }
            break;
        default:
            break;
        }
        if (data_count > 0)
        {
            glBufferSubData(GL_ARRAY_BUFFER, obj->point_index * 3 * sizeof(double), data_count * sizeof(double), data);
        }
    }
    doneCurrent();
    delete []data;

    refresh_catached_points();
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
                    polygon_indexs[polygon_index_count++] = geo->point_index + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete []polygon_indexs;
                        polygon_indexs = temp;
                    }
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(geo))))
                {
                    polygon_indexs[polygon_index_count++] = geo->point_index / 3 + i;
                    if (polygon_index_count == polygon_index_len)
                    {
                        polygon_index_len *= 2;
                        unsigned int *temp = new unsigned int[polygon_index_len];
                        std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                        delete []polygon_indexs;
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
                            polygon_indexs[polygon_index_count++] = item->point_index + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete []polygon_indexs;
                                polygon_indexs = temp;
                            }
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        for (size_t i : Geo::ear_cut_to_indexs(Geo::circle_to_polygon(*dynamic_cast<const Geo::Circle *>(item))))
                        {
                            polygon_indexs[polygon_index_count++] = item->point_index + i;
                            if (polygon_index_count == polygon_index_len)
                            {
                                polygon_index_len *= 2;
                                unsigned int *temp = new unsigned int[polygon_index_len];
                                std::memmove(temp, polygon_indexs, polygon_index_count * sizeof(unsigned int));
                                delete []polygon_indexs;
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

    delete []polygon_indexs;
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

    Text *text = nullptr;
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Geo::Point coord;
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

        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<Text *>(geo);
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
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(geo);
                if (container->text().isEmpty())
                {
                    continue;
                }
                coord = container->bounding_rect().center();
                strings = container->text().split('\n');
                string_index = 1;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                container->text_index = data_count;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<CircleContainer *>(geo);
                if (circlecontainer->text().isEmpty())
                {
                    continue;
                }
                coord = circlecontainer->bounding_rect().center();
                strings = circlecontainer->text().split('\n');
                string_index = 1;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                circlecontainer->text_index = data_count;
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<Text *>(item);
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
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<Container *>(item);
                        if (container->text().isEmpty())
                        {
                            continue;
                        }
                        coord = container->bounding_rect().center();
                        text_rect = font_metrics.boundingRect(container->text());
                        strings = container->text().split('\n');
                        string_index = 1;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        container->text_index = data_count;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<CircleContainer *>(item);
                        if (circlecontainer->text().isEmpty())
                        {
                            continue;
                        }
                        coord = circlecontainer->bounding_rect().center();
                        text_rect = font_metrics.boundingRect(circlecontainer->text());
                        strings = circlecontainer->text().split('\n');
                        string_index = 1;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        circlecontainer->text_index = data_count;
                        break;
                    default:
                        break;
                    }
                    for (const QPolygonF &polygon : path.toSubpathPolygons())
                    {
                        offset = data_count / 3;
                        for (const QPointF &point : polygon)
                        {
                            points.append(Geo::Point(point.x(), coord.y * 2 - point.y()));
                            data[data_count++] = point.x();
                            data[data_count++] = coord.y * 2 - point.y();
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
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
                                delete []indexs;
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
                            delete []indexs;
                            indexs = temp;
                        }
                    }
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text->text_count = data_count - text->text_index;
                        break;
                    case Geo::Type::CONTAINER:
                        container->text_count = data_count - container->text_index;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer->text_count = data_count - circlecontainer->text_index;
                        break; 
                    default:
                        break;
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
                    points.append(Geo::Point(point.x(),  coord.y * 2 - point.y()));
                    data[data_count++] = point.x();
                    data[data_count++] = coord.y * 2 - point.y();
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
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
                        delete []indexs;
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
                    delete []indexs;
                    indexs = temp;
                }
            }
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text->text_count = data_count - text->text_index;
                break;
            case Geo::Type::CONTAINER:
                container->text_count = data_count - container->text_index;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer->text_count = data_count - circlecontainer->text_index;
                break;
            default:
                break;
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

    delete []data;
    delete []indexs;
}

void Canvas::refresh_text_vbo(const bool unitary)
{
    if (!GlobalSetting::get_instance()->setting()["show_text"].toBool())
    {
        return;
    }

    QPainterPath path;
    const QFont font("SimSun", GlobalSetting::get_instance()->setting()["text_size"].toInt());
    const QFontMetrics font_metrics(font);
    QRectF text_rect;

    Text *text = nullptr;
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Geo::Point coord;
    int string_index;
    QStringList strings;

    size_t data_len = 4104, data_count = 0;
    double *data = new double[data_len];
    makeCurrent();
    glBindBuffer(GL_ARRAY_BUFFER, _VBO[3]); // text
    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (Geo::Geometry *geo : group)
        {
            if (!unitary && !geo->is_selected)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<Text *>(geo);
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
                break;
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(geo);
                if (container->text().isEmpty())
                {
                    continue;
                }
                coord = container->bounding_rect().center();
                strings = container->text().split('\n');
                string_index = 1;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<CircleContainer *>(geo);
                if (circlecontainer->text().isEmpty())
                {
                    continue;
                }
                coord = circlecontainer->bounding_rect().center();
                strings = circlecontainer->text().split('\n');
                string_index = 1;
                for (const QString &string : strings)
                {
                    text_rect = font_metrics.boundingRect(string);
                    path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                        * (strings.length() / 2.0 - string_index++), font, string);
                }
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<Text *>(item);
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
                        break;
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<Container *>(item);
                        if (container->text().isEmpty())
                        {
                            continue;
                        }
                        coord = container->bounding_rect().center();
                        text_rect = font_metrics.boundingRect(container->text());
                        strings = container->text().split('\n');
                        string_index = 1;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<CircleContainer *>(item);
                        if (circlecontainer->text().isEmpty())
                        {
                            continue;
                        }
                        coord = circlecontainer->bounding_rect().center();
                        text_rect = font_metrics.boundingRect(circlecontainer->text());
                        strings = circlecontainer->text().split('\n');
                        string_index = 1;
                        for (const QString &string : strings)
                        {
                            text_rect = font_metrics.boundingRect(string);
                            path.addText(coord.x - text_rect.width() / 2, coord.y - text_rect.height()
                                * (strings.length() / 2.0 - string_index++), font, string);
                        }
                        break;
                    default:
                        break;
                    }
                    data_count = 0;
                    for (const QPolygonF &polygon : path.toSubpathPolygons())
                    {
                        for (const QPointF &point : polygon)
                        {
                            data[data_count++] = point.x();
                            data[data_count++] = coord.y * 2 - point.y();
                            data[data_count++] = 0.5;
                            if (data_count == data_len)
                            {
                                data_len *= 2;
                                double *temp = new double[data_len];
                                std::memmove(temp, data, data_count * sizeof(double));
                                delete []data;
                                data = temp;
                            }
                        }
                    }
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * text->text_index, sizeof(double) * data_count, data);
                        break;
                    case Geo::Type::CONTAINER:
                        glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * container->text_index, sizeof(double) * data_count, data);
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * circlecontainer->text_index, sizeof(double) * data_count, data);
                        break; 
                    default:
                        break;
                    }
                    path.clear();
                }
                break;
            default:
                break;
            }
            data_count = 0;
            for (const QPolygonF &polygon : path.toSubpathPolygons())
            {
                for (const QPointF &point : polygon)
                {
                    data[data_count++] = point.x();
                    data[data_count++] = coord.y * 2 - point.y();
                    data[data_count++] = 0.5;
                    if (data_count == data_len)
                    {
                        data_len *= 2;
                        double *temp = new double[data_len];
                        std::memmove(temp, data, data_count * sizeof(double));
                        delete []data;
                        data = temp;
                    }
                }
            }
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * text->text_index, sizeof(double) * data_count, data);
                break;
            case Geo::Type::CONTAINER:
                glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * container->text_index, sizeof(double) * data_count, data);
                break;
            case Geo::Type::CIRCLECONTAINER:
                glBufferSubData(GL_ARRAY_BUFFER, sizeof(double) * circlecontainer->text_index, sizeof(double) * data_count, data);
                break;
            default:
                break;
            }
            path.clear();
        }
    }
    doneCurrent();
    delete []data;
}


void Canvas::refresh_catached_points(const bool current_group_only)
{
    const CircleContainer *c = nullptr;
    _catched_points.clear();
    if (current_group_only)
    {
        for (const Geo::Geometry *geo : _editer->graph()->container_group(_editer->current_group()))
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                _catched_points.emplace_back(dynamic_cast<const Text*>(geo)->center());
                break;
            case Geo::Type::CONTAINER:
                _catched_points.emplace_back(dynamic_cast<const Container *>(geo)->shape().bounding_rect().center());
                for (const Geo::Point &point : dynamic_cast<const Container *>(geo)->shape())
                {
                    _catched_points.emplace_back(point);
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                c = dynamic_cast<const CircleContainer *>(geo);
                _catched_points.emplace_back(c->center());
                _catched_points.emplace_back(c->center().x + c->radius(), c->center().y);
                _catched_points.emplace_back(c->center().x, c->center().y - c->radius());
                _catched_points.emplace_back(c->center().x - c->radius(), c->center().y);
                _catched_points.emplace_back(c->center().x, c->center().y + c->radius());
                break;
            case Geo::Type::POLYLINE:
                for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(geo))
                {
                    _catched_points.emplace_back(point);
                }
                break;
            default:
                break;
            }
        }
    }
    else
    {
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
                case Geo::Type::TEXT:
                    _catched_points.emplace_back(dynamic_cast<const Text*>(geo)->center());
                    break;
                case Geo::Type::CONTAINER:
                    _catched_points.emplace_back(dynamic_cast<const Container *>(geo)->shape().bounding_rect().center());
                    for (const Geo::Point &point : dynamic_cast<const Container *>(geo)->shape())
                    {
                        _catched_points.emplace_back(point);
                    }
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    c = dynamic_cast<const CircleContainer *>(geo);
                    _catched_points.emplace_back(c->center());
                    _catched_points.emplace_back(c->center().x + c->radius(), c->center().y);
                    _catched_points.emplace_back(c->center().x, c->center().y - c->radius());
                    _catched_points.emplace_back(c->center().x - c->radius(), c->center().y);
                    _catched_points.emplace_back(c->center().x, c->center().y + c->radius());
                    break;
                case Geo::Type::POLYLINE:
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Polyline *>(geo))
                    {
                        _catched_points.emplace_back(point);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    } 
}



size_t Canvas::points_count() const
{
    return _points_count;
}





