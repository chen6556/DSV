#pragma once
#include "draw/Graph.hpp"
#include <array>
#include <mutex>


class Pretreatment
{
private:
    Graph *_graph = nullptr;
    std::vector<Container *> _all_containers;
    std::vector<CircleContainer *> _all_circles;
    std::vector<Link *> _all_links;
    std::vector<Geo::Polyline *> _all_polylines;
    std::vector<size_t> _removed_polyliens;
    std::vector<Geo::Bezier *> _all_beziers, _removed_beziers;
    std::mutex _mtx;

    void split();

    void connect_lines(const double value);

    void connect_lines(const double value, const size_t cores);

    void connect_lines_subfunc(const double value, const size_t start, const size_t end);

    void finish();

public:
    Pretreatment(Graph *graph);
};