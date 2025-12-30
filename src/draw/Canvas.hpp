#pragma once
#include <set>
#include <unordered_map>
#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QLabel>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QOpenGLFunctions_4_5_Core>

#include "base/Editer.hpp"
#include "draw/CanvasOperation.hpp"


class Canvas : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    enum class CatchedPointType {Vertex, Center, Foot, Tangency, Intersection};

private:
    Geo::AABBRect _visible_area;
    std::vector<const Geo::Geometry *> _catched_objects;
    Editer *_editer = nullptr;
    QLabel **_info_labels = nullptr;
    QTextEdit _input_line;

    unsigned int _shader_program, _VAO;
    unsigned int _base_VBO[4]; // 0:origin and select rect 1:catched points 2:operation shape 3:operation tool lines
    unsigned int _shape_VBO[7]; // 0:polyline 1:polygon 2:circle 3:curve 4:text 5:circle printable points 6:curve printable points
    unsigned int _shape_IBO[4]; // 0:polyline 1:polygon 2:circle 3:curve
    unsigned int _text_brush_IBO;
    unsigned int _selected_IBO[4]; // 0:polyline 1:polygon 2:circle 3:curve
    int _uniforms[5]; // w, h, vec0, vec1, color
    unsigned int _point_count[4] = {0, 0, 0, 0}; // 0:polyline 1:polygon 2:circle 3:curve
    unsigned int _shape_index_count[4] = {0, 0, 0, 0}; // 0:polyline 1:polygon 2:circle 3:curve
    unsigned int _brush_index_count[3] = {0, 0, 0}; // 0:polygon brush 1:circle brush 2:text brush
    unsigned int _selected_index_count[4] = {0, 0, 0, 0}; // 0:polyline 1:polygon 2:circle 3:curve
    double _catchline_points[24];
    
    double _catch_distance = 0;
    static const int catch_count = 5;
    bool _catch_types[5] = {false, false, false, false, false};

    double _canvas_ctm[9] = {1,0,0, 0,-1,0, 0,0,1}; // 画布坐标变换矩阵(真实坐标变为画布坐标)
    double _view_ctm[9] = {1,0,0, 0,-1,0, 0,0,1}; // 显示坐标变换矩阵(显示坐标变为真实坐标)
    double _ratio = 1; // 缩放系数
    int _canvas_width = 0, _canvas_height = 0;

    // 0:可移动视图 1:显示坐标原点 2:显示捕捉点
    bool _bool_flags[3] = {false, true, false};
    double _select_rect[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    QPointF _mouse_pos_0, _mouse_pos_1;
    std::vector<Geo::Point> _points_cache;
    Text *_edited_text = nullptr;

    QMenu *_menu = nullptr;
    QAction *_up = nullptr;
    QAction *_down = nullptr;
    QAction *_text_to_polylines = nullptr;
    QAction *_bezier_to_bspline = nullptr;
    QAction *_bspline_to_bezier = nullptr;
    QAction *_change_bspline_model = nullptr;

private:
    void init();

    void init_menu();

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
    void tool_changed(const CanvasOperations::Tool);

    void refresh_cmd_parameters_label();

public:
    Canvas(QWidget *parent = nullptr);

    ~Canvas();

    void bind_editer(Editer *editer);

    void use_tool(const CanvasOperations::Tool tool);

    void show_origin();

    void hide_origin();

    void show_overview();

    const bool is_typing() const;

    void set_catch_distance(const double value);

    void set_cursor_catch(const CatchedPointType type, const bool value);

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    Geo::Point center() const;

    Geo::AABBRect bounding_rect() const;

    Geo::Point mouse_position(const bool to_real_coord = true) const;

    const bool empty() const;

    void cancel_painting();

    void set_info_labels(QLabel **labels);

    void add_geometry(Geo::Geometry *object);

    void show_menu(Geo::Geometry *object);

    void show_text_edit(Text *text);

    void hide_text_edit();

    void copy();

    void cut();

    void paste();

    void paste(const double x, const double y);


    Geo::Point real_coord_to_view_coord(const Geo::Point &input) const;

    Geo::Point real_coord_to_view_coord(const double x, const double y) const;

    Geo::Point canvas_coord_to_real_coord(const Geo::Point &input) const;

    Geo::Point canvas_coord_to_real_coord(const double x, const double y) const;

    bool catch_cursor(const double x, const double y, Geo::Point &coord, const double distance, const bool skip_selected);

    bool catch_point(const double x, const double y, Geo::Point &coord, const double distance);


    // 直接更新所有VBO,点数量可能发生变化
    void refresh_vbo(const bool refresh_ibo);

    void refresh_vbo(const Geo::Type type, const bool refresh_ibo);

    void refresh_vbo(const std::set<Geo::Type> &types, const bool refresh_ibo);

    std::tuple<double*, unsigned int, unsigned int*, unsigned int> refresh_polyline_vbo();

    std::tuple<double*, unsigned int, unsigned int*, unsigned int> refresh_polygon_vbo();

    std::tuple<double*, unsigned int, unsigned int*, unsigned int> refresh_circle_vbo();

    std::tuple<double*, unsigned int, unsigned int*, unsigned int> refresh_curve_vbo();

    std::tuple<double *, unsigned int> refresh_circle_printable_points();

    std::tuple<double *, unsigned int> refresh_curve_printable_points();

    void refresh_select_rect(const double x0, const double y0, const double x1, const double y1);

    void refresh_selected_ibo();

    void refresh_selected_ibo(const Geo::Geometry *object);

    void refresh_selected_ibo(const std::vector<Geo::Geometry *> &objects);

    void refresh_selected_vbo();

    void clear_selected_ibo();

    std::tuple<double*, unsigned int, unsigned int*, unsigned int> refresh_text_vbo();


    bool refresh_catached_points(const double x, const double y, const double distance, std::vector<const Geo::Geometry *> &catched_objects, const bool skip_selected, const bool current_group_only = true) const;

    bool refresh_catchline_points(const std::vector<const Geo::Geometry *> &objects, const double distance, Geo::Point &pos);

};