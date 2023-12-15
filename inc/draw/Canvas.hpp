#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QLabel>
#include <QTextEdit>
#include "base/Editer.hpp"
#include "base/Geometry.hpp"


class Canvas : public QWidget
{
    Q_OBJECT

private:
    Geo::Circle _circle_cache;
    Geo::Rectangle _rectangle_cache, _visible_area;
    Geo::AxisAlignedBoundingBox _select_rect;
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

    // current_tool:[-1:no-tool 0:circle 1:polyline 2:rectangle 3:curve], last_tool
    int _int_flags[2] = {-1, -1};

    QPointF _mouse_pos_0, _mouse_pos_1, _stored_mouse_pos;
    Geo::Point _last_point;
    Geo::Geometry *_clicked_obj = nullptr, *_last_clicked_obj = nullptr;

    const static Qt::GlobalColor shape_color = Qt::green, selected_shape_color = Qt::red, text_color = Qt::black;

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
    void tool_changed(const int &);

public:
    Canvas(QLabel **labels = nullptr, QWidget *parent = nullptr);

    void bind_editer(Editer *editer);

    void use_tool(const int value);

    void show_origin();

    void hide_origin();

    void show_overview();

    bool origin_visible() const;

    const bool is_painting() const;

    const bool is_typing() const;

    const bool is_moving() const;

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    void set_bezier_order(const size_t order);
    
    const size_t bezier_order() const;

    double ratio() const;

    Geo::Point center() const;

    Geo::AxisAlignedBoundingBox bounding_box() const;

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