#pragma once

#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QLabel>
#include <QTextEdit>
#include <QOpenGLFunctions_4_5_Core>
#include "base/Editer.hpp"
#include <QMenu>
#include <QAction>


class Canvas : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    enum Tool {NOTOOL, MEASURE, CIRCLE, POLYLINE, RECT, CURVE, TEXT};
    enum Operation {NOOPERATION, MIRROR, RINGARRAY};

private:
    Geo::Circle _circle_cache;
    Geo::AABBRect _AABBRect_cache, _select_rect, _visible_area;
    std::list<QLineF> _reflines;
    std::vector<Geo::Coord> _catched_points;
    Editer *_editer = nullptr;
    QLabel **_info_labels = nullptr;
    QTextEdit _input_line;

    unsigned int _shader_program, _VAO;
    unsigned int _VBO[5]; //0:points 1:origin and select rect 2:cache 3:text 4:reflines
    unsigned int _IBO[4]; //0:polyline 1:polygon 2:selected 3:text
    int _uniforms[5]; // w, h, vec0, vec1, color
    size_t _points_count;
    size_t _indexs_count[4] = {0, 0, 0, 0}; //0:polyline 1:polygon 2:selected 3:text
    double *_cache = nullptr;
    size_t _cache_len = 513, _cache_count = 0;
    double _refline_points[30];

    double _canvas_ctm[9] = {1,0,0, 0,1,0, 0,0,1}; // 画布坐标变换矩阵(真实坐标变为画布坐标)
    double _view_ctm[9] = {1,0,0, 0,1,0, 0,0,1}; // 显示坐标变换矩阵(显示坐标变为真实坐标)
    double _ratio = 1; // 缩放系数
    size_t _bezier_order = 3; // 贝塞尔曲线阶数

    // 可移动视图, 可绘图, 正在绘图, 测量, 可移动单个object, 选中一个obj, 正在移动obj, 显示坐标原点
    bool _bool_flags[8] = {false, false, false, false, false, false, false, true};

    // First point and second point
    bool _measure_flags[2] = {false, false};

    // current_tool, last_tool
    Tool _tool_flags[2] = {Tool::NOTOOL, Tool::NOTOOL};
    Operation _operation = Operation::NOOPERATION;

    QPointF _mouse_pos_0, _mouse_pos_1, _stored_mouse_pos;
    Geo::Point _last_point;
    Geo::Geometry *_clicked_obj = nullptr, *_last_clicked_obj = nullptr;
    Geo::Geometry *_pressed_obj = nullptr;
    std::list<Geo::Geometry *> _object_cache;

    QMenu *_menu = nullptr;
    QAction *_up = nullptr;
    QAction *_down = nullptr;

private:
    void init();

protected:
    void initializeGL();

    void resizeGL(int w, int h);

    void paintGL();

    void mousePressEvent(QMouseEvent *event);

    void mouseReleaseEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void wheelEvent(QWheelEvent *event);

    void mouseDoubleClickEvent(QMouseEvent *event);

public:
signals:
    void tool_changed(const Tool);

public:
    Canvas(QLabel **labels = nullptr, QWidget *parent = nullptr);

    ~Canvas();

    void bind_editer(Editer *editer);

    void use_tool(const Tool tool);

    void set_operation(const Operation operation);

    void show_origin();

    void hide_origin();

    void show_overview();

    bool origin_visible() const;

    const bool is_view_moveable() const;

    const bool is_paintable() const;

    const bool is_painting() const;

    const bool is_typing() const;

    const bool is_measureing() const;

    const bool is_obj_moveable() const;

    const bool is_obj_selected() const;

    const bool is_moving_obj() const;

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    void set_bezier_order(const size_t order);
    
    const size_t bezier_order() const;

    double ratio() const;

    Geo::Point center() const;

    Geo::AABBRect bounding_rect() const;

    Geo::Coord mouse_position() const;

    const bool empty() const;

    void cancel_painting();

    void use_last_tool();

    void set_info_labels(QLabel **labels);

    void copy();

    void cut();

    void paste();


    bool is_visible(const Geo::Point &point) const;

    bool is_visible(const Geo::Polyline &polyline) const;

    bool is_visible(const Geo::Polygon &polygon) const;

    bool is_visible(const Geo::Circle &circle) const;


    Geo::Coord real_coord_to_view_coord(const Geo::Coord &input) const;

    Geo::Coord real_coord_to_view_coord(const double x, const double y) const;

    Geo::Coord canvas_coord_to_real_coord(const Geo::Coord &input) const;

    Geo::Coord canvas_coord_to_real_coord(const double x, const double y) const;

    bool catch_cursor(const double x, const double y, Geo::Coord &coord, const double distance);

    bool catch_point(const double x, const double y, Geo::Coord &coord, const double distance);



    void refresh_vbo();

    void refresh_vbo(const bool unitary);

    void refresh_selected_ibo();

    void refresh_selected_vbo();

    void refresh_brush_ibo();

    void refresh_text_vbo();

    void refresh_text_vbo(const bool unitary);


    void refresh_catached_points(const bool current_group_only = true);

};