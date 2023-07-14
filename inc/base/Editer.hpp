#pragma once
#include "draw/Graph.hpp"
#include "Structure.hpp"


class Editer
{
private:
    Graph *_graph = nullptr;
    QString _file_path;
    std::vector<Geo::Point> _point_cache;
    bool _modified = false;
    std::list<Graph *> _backup;
    std::list<Geo::Geometry *> _paste_table;
    const static size_t backup_times = 50;
    size_t _current_group = 0;

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

    Graph *graph();

    const Graph *graph() const;

    const bool &modified() const;

    void reset_modified();

    const QString &path() const;

    void set_path(const QString &path);

    std::vector<Geo::Point> &point_cache();

    const std::vector<Geo::Point> &point_cache() const;

    const size_t &current_group() const;

    void set_current_group(const size_t index);

    const size_t groups_count() const;
    


    void append_points();

    void append(const Geo::Circle &circle);

    void append(const Geo::Rectangle &rect);

    void append_bezier(const size_t order);

    void translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape = true);

    bool remove_selected();

    bool copy_selected();

    bool cut_selected();

    bool paste(const double tx, const double ty);



    Geo::Geometry *select(const Geo::Point &point, const bool reset_others = true);

    Geo::Geometry *select(const double x, const double y, const bool reset_others = true);

    std::list<Geo::Geometry *> selected() const;

    const size_t selected_count() const;

    std::vector<Geo::Geometry *> select(const Geo::Rectangle &rect);

    void reset_selected_mark(const bool value = false);

    void store_backup();

    void load_backup();



    void remove_group(const size_t index);

    void append_group(const size_t index = SIZE_MAX);





    void rotate(const double angle, const bool unitary);

    // true:X false:Y
    void flip(const bool direction, const bool unitary);
};
