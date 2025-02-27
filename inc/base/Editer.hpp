#pragma once

#include <QLineF>
#include <QPolygonF>

#include "base/UndoStack.hpp"


class Editer
{
private:
    Graph *_graph = nullptr;
    QString _file_path;
    std::vector<Geo::Point> _point_cache;
    UndoStack::CommandStack _backup;
    std::list<Geo::Geometry *> _paste_table;
    size_t _current_group = 0;
    double _view_ratio = 1.0;

    Geo::Geometry *_catched_points = nullptr;
    std::vector<std::tuple<double, double>> _edited_shape;

private:
    void init();

public:
    Editer(){};

    Editer(Graph *graph);

    Editer(Graph *graph, const QString &path);

    ~Editer();

    void load_graph(Graph *graph, const QString &path);

    void load_graph(Graph *graph);

    void delete_graph();

    const QString &path() const;

    void set_path(const QString &path);

    std::vector<Geo::Point> &point_cache();

    const std::vector<Geo::Point> &point_cache() const;

    std::vector<std::tuple<double, double>> &edited_shape();

    const size_t current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;

    void set_view_ratio(const double value);
    
    Geo::Geometry *select(const Geo::Point &point, const bool reset_others = true);

    Geo::Geometry *select(const double x, const double y, const bool reset_others = true);

    std::list<Geo::Geometry *> selected() const;

    const size_t selected_count() const;

    std::vector<Geo::Geometry *> select(const Geo::AABBRect &rect);

    void reset_selected_mark(const bool value = false);

    void undo();

    void set_backup_count(const size_t count);

    void push_backup_command(UndoStack::Command *command);

    // Layer Operation
    void remove_group(const size_t index);

    void append_group(const size_t index = SIZE_MAX);

    void reorder_group(size_t from, size_t to);

    bool group_is_visible(const size_t index) const;

    void show_group(const size_t index);

    void hide_group(const size_t index);

    QString group_name(const size_t index) const;

    void set_group_name(const size_t index, const QString &name);


    void append_points();

    void append(const Geo::Circle &circle);

    void append(const Geo::AABBRect &rect);

    void append_bezier(const size_t order);

    void append_text(const double x, const double y);

    void translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape = true);

    bool remove_selected();

    bool copy_selected();

    bool cut_selected();

    bool paste(const double tx, const double ty);

    bool connect(std::list<Geo::Geometry *> objects, const double connect_distance);

    bool close_polyline(std::list<Geo::Geometry *> objects);

    bool combinate(std::list<Geo::Geometry *> objects);

    bool split(std::list<Geo::Geometry *> objects);

    bool mirror(std::list<Geo::Geometry *> objects, const Geo::Geometry *line, const bool copy);

    bool offset(std::list<Geo::Geometry *> objects, const double distance);

    bool scale(std::list<Geo::Geometry *> objects, const bool unitary, const double k);

    bool polygon_union(Container<Geo::Polygon> *container0, Container<Geo::Polygon> *container1);

    bool polygon_intersection(Container<Geo::Polygon> *container0, Container<Geo::Polygon> *container1);

    bool polygon_difference(Container<Geo::Polygon> *container0, const Container<Geo::Polygon> *container1); 

    bool fillet(Container<Geo::Polygon> *container, const Geo::Point &point, const double radius);

    bool fillet(Geo::Polyline *polyline, const Geo::Point &point, const double radius);

    bool line_array(std::list<Geo::Geometry *> objects, int x, int y, double x_space, double y_space);

    bool ring_array(std::list<Geo::Geometry *> objects, const double x, const double y, const int n);

    void up(Geo::Geometry *item);

    void down(Geo::Geometry *item);

	void rotate(std::list<Geo::Geometry *> objects, const double angle, const bool unitary, const bool all_layers);

    // true:X false:Y
    void flip(std::list<Geo::Geometry *> objects, const bool direction, const bool unitary, const bool all_layers);


    bool auto_aligning(Geo::Geometry *src, const Geo::Geometry *dst, std::list<QLineF> &reflines);

    bool auto_aligning(Geo::Point &coord, const Geo::Geometry *dst, std::list<QLineF> &reflines);

    bool auto_aligning(Geo::Geometry *points, std::list<QLineF> &reflines, const bool current_group_only = true);

    bool auto_aligning(Geo::Geometry *points, const double x, const double y, std::list<QLineF> &reflines, const bool current_group_only = true);

    bool auto_aligning(Geo::Point &coord, std::list<QLineF> &reflines, const bool current_group_only = true);

    void auto_combinate();

	void auto_layering();

    void auto_connect();
};
