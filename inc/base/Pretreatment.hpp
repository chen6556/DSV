#pragma once
#include "draw/Graph.hpp"
#include <array>


class Pretreatment
{
private:
    Graph *_graph = nullptr;
    std::vector<Container *> _all_containers;
    std::vector<CircleContainer *> _all_circles;
    std::vector<Link *> _all_links;
    std::vector<Geo::Polyline *> _all_polylines;
    std::vector<Geo::Bezier *> _all_beziers;
    std::vector<Combination *> _all_combinations; 

    void split();

    void connect_lines(const double value);

    void combine();

    void finish();

public:
    Pretreatment(Graph *graph);
};