#pragma once

#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QLabel>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QOpenGLFunctions_4_5_Core>

#include "base/Editer.hpp"


class Canvas : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    enum class Tool {NoTool, Measure, Circle, Polyline, Rect, BSpline, Bezier, Text, Ellipse};
    enum class Operation {NoOperation, Mirror, RingArray, PolygonDifference, Fillet, Rotate};
    enum class CatchedPointType {Vertex, Center, Foot, Tangency, Intersection};

private:
    Geo::Circle _circle_cache;
    Geo::Ellipse _ellipse_cache;
    Geo::AABBRect _AABBRect_cache, _select_rect, _visible_area;
    std::list<QLineF> _reflines;
    std::vector<const Geo::Geometry *> _catched_objects;
    Editer *_editer = nullptr;
    QLabel **_info_labels = nullptr;
    QTextEdit _input_line;

    unsigned int _shader_program, _VAO;
    unsigned int _VBO[6]; //0:points 1:origin and select rect 2:cache 3:text 4:reflines 5:catched point
    unsigned int _IBO[4]; //0:polyline 1:polygon 2:selected 3:text
    int _uniforms[5]; // w, h, vec0, vec1, color
    size_t _points_count = 0;
    size_t _indexs_count[4] = {0, 0, 0, 0}; //0:polyline 1:polygon 2:selected 3:text
    double *_cache = nullptr;
    size_t _cache_len = 513, _cache_count = 0;
    double _refline_points[30];
    double _catchline_points[24];
    
    double _catch_distance = 0;
    static const int catch_count = 5;
    bool _catch_types[5] = {false, false, false, false, false};

    double _canvas_ctm[9] = {1,0,0, 0,-1,0, 0,0,1}; // 画布坐标变换矩阵(真实坐标变为画布坐标)
    double _view_ctm[9] = {1,0,0, 0,-1,0, 0,0,1}; // 显示坐标变换矩阵(显示坐标变为真实坐标)
    double _ratio = 1; // 缩放系数
    int _canvas_width = 0, _canvas_height = 0;
    size_t _curve_order = 3; // 曲线次数

    // 0:可移动视图, 1:可绘图, 2:正在绘图, 3:测量, 4:可移动单个object, 5:选中一个obj, 6:正在移动obj, 7:显示坐标原点, 8:显示捕捉点
    bool _bool_flags[9] = {false, false, false, false, false, false, false, true, false};

    // First point and second point
    bool _measure_flags[2] = {false, false};

    // 0:current_tool, 1:last_tool
    Tool _tool_flags[2] = {Tool::NoTool, Tool::NoTool};
    Operation _operation = Operation::NoOperation;

    QPointF _mouse_pos_0, _mouse_pos_1;
    Geo::Point _mouse_press_pos, _mouse_release_pos;
    Geo::Point _stored_coord;
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

    void operation_changed(const Operation);

public:
    Canvas(QWidget *parent = nullptr);

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

    void set_catch_distance(const double value);

    void set_cursor_catch(const CatchedPointType type, const bool value);

    const bool is_catching(const CatchedPointType type) const;

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    void set_curve_order(const size_t order);

    const size_t curve_order() const;

    double ratio() const;

    Geo::Point center() const;

    Geo::AABBRect bounding_rect() const;

    Geo::Point mouse_position(const bool to_real_coord = true) const;

    const bool empty() const;

    void cancel_painting();

    void use_last_tool();

    void set_info_labels(QLabel **labels);

    void copy();

    void cut();

    void paste();

    void paste(const double x, const double y);

    void polyline_cmd(const double x, const double y);

    void polyline_cmd();

    void rect_cmd(const double x, const double y);

    void rect_cmd();

    void circle_cmd(const double x, const double y);

    void circle_cmd(const double x, const double y, const double r);

    void ellipse_cmd(const double x, const double y);

    void ellipse_cmd(const double x, const double y, const double rad, const double a);

    void ellipse_cmd(const double x, const double y, const double rad,  const double a, const double b);

    void text_cmd(const double x, const double y);


    bool is_visible(const Geo::Point &point) const;

    bool is_visible(const Geo::Polyline &polyline) const;

    bool is_visible(const Geo::Polygon &polygon) const;

    bool is_visible(const Geo::Circle &circle) const;


    Geo::Point real_coord_to_view_coord(const Geo::Point &input) const;

    Geo::Point real_coord_to_view_coord(const double x, const double y) const;

    Geo::Point canvas_coord_to_real_coord(const Geo::Point &input) const;

    Geo::Point canvas_coord_to_real_coord(const double x, const double y) const;

    bool catch_cursor(const double x, const double y, Geo::Point &coord, const double distance, const bool skip_selected);

    bool catch_point(const double x, const double y, Geo::Point &coord, const double distance);


    void check_cache();


    // 直接更新所有VBO,点数量可能发生变化
    void refresh_vbo();

    // 更新被选中或所有VBO,点数量可能发生变化
    void refresh_vbo(const bool unitary);

    void refresh_cache_vbo(const unsigned int count);

    void clear_cache();

    void refresh_selected_ibo();

    void refresh_selected_ibo(const Geo::Geometry *object);

    void refresh_selected_vbo();

    // 数量发生变化时更新IBO数组
    void refresh_brush_ibo();

    // 数量不发生变化时更新部分IBO数组
    void refresh_brush_ibo(const unsigned int index, const unsigned int offset, const std::vector<size_t> &values);

    void refresh_text_vbo();

    void refresh_text_vbo(const bool unitary);


    bool refresh_catached_points(const double x, const double y, const double distance, std::vector<const Geo::Geometry *> &catched_objects, const bool skip_selected, const bool current_group_only = true) const;

    bool refresh_catchline_points(const std::vector<const Geo::Geometry *> &objects, const double distance, Geo::Point &pos);


    size_t points_count() const;
};