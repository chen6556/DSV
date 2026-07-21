#pragma once
#include "base/UndoStack.hpp"
#include "base/Algorithm.hpp"
#include "draw/QuadTree.hpp"


class Editor
{
private:
    Graph *_graph = nullptr;
    QString _file_path;
    std::vector<Geo::Point> _point_cache;
    UndoStack::CommandStack _backup;
    QuadTree _view_tree;
    std::vector<Geo::DObject *> _paste_table;
    size_t _current_group = 0;
    double _view_ratio = 1.0;

    Geo::DObject *_catched_points = nullptr;

public:
    std::vector<std::tuple<double, double>> edited_shape;
    std::vector<std::tuple<double, double>> edited_path;
    std::vector<double> edited_knots;

private:
    void init();

public:
    Editor() = default;

    ~Editor();

    void load_graph(Graph *graph, const QString &path);

    void load_graph(Graph *graph);

    void delete_graph();

    const QString &path() const;

    void set_path(const QString &path);

    Graph *graph();

    const Graph *graph() const;

    void refresh_visible_objects(const Geo::AABBRectParams &rect);

    const std::vector<Geo::DObject *> &visible_objects() const;

    std::vector<Geo::Point> &point_cache();

    const std::vector<Geo::Point> &point_cache() const;

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    void set_view_ratio(const double value);

    Geo::DObject *select(const Geo::Point &point, const bool reset_others = true, const bool visible_only = true);

    Geo::DObject *select(const double x, const double y, const bool reset_others = true, const bool visible_only = true);

    std::tuple<Geo::DObject *, bool> select_with_state(const Geo::Point &point, const bool reset_others = true);

    std::vector<Geo::DObject *> selected(const bool visible_only = true) const;

    const size_t selected_count() const;

    std::vector<Geo::DObject *> select(const Geo::AABBRectParams &rect, const bool reset_others = true, const bool visible_only = true);

    void reset_selected_mark(const bool value = false);

    const std::vector<Geo::DObject *> &paste_table() const;

    void undo();

    void set_backup_count(const size_t count);

    void push_backup_command(UndoStack::Command *command);

    void moved_objects(const std::vector<Geo::DObject *> &objects, const double dx, const double dy);

    // Layer Operation
    void remove_group(const size_t index);

    void append_group(const size_t index = SIZE_MAX);

    void reorder_group(size_t from, size_t to);

    bool group_is_visible(const size_t index) const;

    void show_group(const size_t index);

    void hide_group(const size_t index);

    QString group_name(const size_t index) const;

    void set_group_name(const size_t index, const QString &name);


    void append(Geo::DObject *object);

    void append(const std::vector<Geo::DObject *> &objects);

    void translate_points(Geo::DObject *points, const double x0, const double y0, const double x1, const double y1,
                          const bool change_shape = true);

    bool remove_selected();

    bool copy_selected();

    bool cut_selected();

    bool paste(const double tx, const double ty);

    bool connect(const std::vector<Geo::DObject *> &objects, const double connect_distance);

    bool blend(const Geo::DObject *object0, const Geo::DObject *object1, const Geo::Point &pos0, const Geo::Point &pos1);

    bool close_polyline(const std::vector<Geo::DObject *> &objects);

    bool combine(const std::vector<Geo::DObject *> &objects);

    bool detach(const std::vector<Geo::DObject *> &objects);

    bool mirror(const std::vector<Geo::DObject *> &objects, const Geo::Point &start, const Geo::Point &end, const bool copy);

    bool offset(const std::vector<Geo::DObject *> &objects, const double distance,
                const Geo::Offset::JoinType join_type = Geo::Offset::JoinType::Round,
                const Geo::Offset::EndType end_type = Geo::Offset::EndType::Polygon);

    bool scale(const std::vector<Geo::DObject *> &objects, const bool unitary, const double k);

    bool shape_union(Geo::DObject *shape0, Geo::DObject *shape1);

    bool shape_intersection(Geo::DObject *shape0, Geo::DObject *shape1);

    bool shape_difference(Geo::DObject *shape0, const Geo::DObject *shape1);

    bool shape_xor(Geo::DObject *shape0, Geo::DObject *shape1);

    bool fillet(Geo::Polyline *polyline0, const Geo::Point &point0, Geo::Polyline *polyline1, const Geo::Point &point1,
                const double radius0, const double radius1);

    bool fillet(Geo::DObject *object0, Geo::DObject *object1, const Geo::Point &start, const Geo::Point &center, const Geo::Point &end,
                const std::vector<std::tuple<size_t, double, double, double>> &tvalues);

    bool fillet(Geo::DObject *object, const Geo::Point &point, const double radius);

    bool fillet(Geo::DObject *object, const Geo::Point &point, const double radius0, const double radius1);

    bool fillet(Geo::DObject *object0, const Geo::Point &point0, Geo::DObject *object1, const Geo::Point &point1, const double radius);

    bool chamfer(Geo::Polygon *shape, const Geo::Point &point, const double distance);

    bool chamfer(Geo::Polyline *polyline, const Geo::Point &point, const double distance);

    bool split(Geo::DObject *object, const Geo::Point &pos);

    bool line_array(const std::vector<Geo::DObject *> &objects, int x, int y, double x_space, double y_space);

    bool ring_array(const std::vector<Geo::DObject *> &objects, const double x, const double y, const int n);

    void up(Geo::DObject *item);

    void down(Geo::DObject *item);

    void rotate(const std::vector<Geo::DObject *> &objects, const double x, const double y, const double rad);

    // true:X false:Y
    void flip(std::vector<Geo::DObject *> objects, const bool direction, const bool unitary, const bool all_layers);

    void trim(Geo::Polyline *polyline, const double x, const double y);

    void trim(Geo::Polygon *polygon, const double x, const double y);

    void trim(Geo::CubicBezier *bezier, const double x, const double y);

    void trim(Geo::BSpline *bspline, const double x, const double y);

    void trim(Geo::Circle *circle, const double x, const double y);

    void trim(Geo::Arc *arc, const double x, const double y);

    void trim(Geo::Ellipse *ellipse, const double x, const double y);

    void extend(Geo::Polyline *polyline, const double x, const double y);

    void extend(Geo::CubicBezier *bezier, const double x, const double y);

    void extend(Geo::BSpline *bspline, const double x, const double y);

    bool divide_points_n(const std::vector<Geo::DObject *> &objects, const size_t n);

    bool divide_parts_n(const std::vector<Geo::DObject *> &objects, const size_t n);

    bool divide_points_measure(const std::vector<Geo::DObject *> &objects, const double length);

    bool divide_parts_measure(const std::vector<Geo::DObject *> &objects, const double length);

    void reverse(const std::vector<Geo::DObject *> &objects);


    void auto_combine();

    void auto_layering();


    void text_to_polylines(Text *text);

    void bezier_to_bspline(Geo::CubicBezier *bezier);

    void bspline_to_bezier(Geo::BSpline *bspline);

};
