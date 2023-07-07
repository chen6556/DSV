#pragma once
#include "Spirit.hpp"
#include "draw/Graph.hpp"
#include "base/Geometry.hpp"


class PLTSpirit : public Spirit<PLTSpirit>
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    Geo::Coord _last_coord;
    double _radius = -1;

    enum Command {NONE, PU, PD, CI, AA, AR};
    Command _last_cmd = Command::NONE, _cur_cmd = Command::NONE;
    bool _relative_coord = false;


    void pu(const std::string &value);

    void pd(const std::string &value);

    void ci(const std::string &value);

    void pa(const std::string &value);

    void pr(const std::string &value);

    void coord(const std::string &value);

    void radius(const std::string &value);

    void aa(const std::string &value);

    void ar(const std::string &value);

    
    void exec(const std::string &value);

    
    void store_points();

    void store_circle();

    void store_arc();

protected:
    void exec();

public:
    PLTSpirit();

    void load_graph(Graph *g);

};