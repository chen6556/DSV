#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QLabel>
#include <QTextEdit>
#include "base/Editer.hpp"
#include <QMenu>
#include <QAction>


class Canvas : public QWidget
{
    Q_OBJECT

public:
    enum Tool {NONE, CIRCLE, POLYLINE, RECT, CURVE, TEXT};

private:
    Geo::Circle _circle_cache;
    Geo::AABBRect _AABBRect_cache, _select_rect, _visible_area;
    std::list<QLineF> _reflines;
    QPolygonF _catched_points;
    Editer *_editer = nullptr;
    QLabel **_info_labels = nullptr;
    QTextEdit _input_line;

    double _canvas_ctm[9] = {1,0,0, 0,1,0, 0,0,1}; // 画布坐标变换矩阵(真实坐标变为画布坐标)
    double _view_ctm[9] = {1,0,0, 0,1,0, 0,0,1}; // 显示坐标变换矩阵(显示坐标变为真实坐标)
    double _ratio = 1; // 缩放系数
    size_t _bezier_order = 3; // 贝塞尔曲线阶数

    // 可移动视图, 可绘图, 正在绘图, 光标追踪, 可移动单个object, 选中一个obj, 正在移动obj, 显示坐标原点
    bool _bool_flags[8] = {false, false, false, false, false, false, false, true};

    // current_tool, last_tool
    Tool _tool_flags[2] = {Tool::NONE, Tool::NONE};

    QPointF _mouse_pos_0, _mouse_pos_1, _stored_mouse_pos;
    Geo::Point _last_point;
    Geo::Geometry *_clicked_obj = nullptr, *_last_clicked_obj = nullptr;
    Geo::Geometry *_pressed_obj = nullptr; 

    QMenu *_menu = nullptr;
    QAction *_up = nullptr;
    QAction *_down = nullptr;

    const QColor point_color = QColor(8, 146, 208);
    const Qt::GlobalColor shape_color = Qt::white, selected_shape_color = Qt::red, text_color = Qt::white;
    const int point_size = 5, line_size = 1, text_size = 2;

private:
    void init();

    void paint_cache();

    void paint_graph();

    void paint_select_rect();

protected:
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *event);

    void mouseReleaseEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void wheelEvent(QWheelEvent *event);

    void mouseDoubleClickEvent(QMouseEvent *event);

    void resizeEvent(QResizeEvent *event);

public:
signals:
    void tool_changed(const Tool);

public:
    Canvas(QLabel **labels = nullptr, QWidget *parent = nullptr);

    ~Canvas();

    void bind_editer(Editer *editer);

    void use_tool(const Tool tool);

    void show_origin();

    void hide_origin();

    void show_overview();

    bool origin_visible() const;

    const bool is_view_moveable() const;

    const bool is_paintable() const;

    const bool is_painting() const;

    const bool is_typing() const;

    const bool is_catching_cursor() const;

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
};