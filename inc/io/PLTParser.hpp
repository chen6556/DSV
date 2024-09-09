#pragma once

#include <fstream>

#include "base/Geometry.hpp"
#include "draw/Graph.hpp"


namespace PLTParser
{

class Importer
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    std::vector<double> _parameters;
    Geo::Point _last_coord;

    bool _relative_coord = false;
    double _x_ratio = 0.025, _y_ratio = 0.025;
    double _ip[6], _sc[4];

    struct Txt
    {
        std::string txt;
        Geo::Point pos;
        bool marked = false;

        Txt(const std::string &text, const Geo::Point &position)
            : txt(text), pos(position) {};
    };
    std::list<Txt> _texts;

public:
    static const double plotter_unit;

private:
    void store_points();

    void store_arc();

public:

    void x_coord(const double value);

    void y_coord(const double value);

    void parameter(const double value);

    void ip();

    void sc();

    void pu();

    void sp(const int value);

    void ci();

    void pa();

    void pr();

    void aa();

    void ar();

    void store_text(const std::string &text);

    void end();


    void load_graph(Graph *g);

    void reset();
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);
}