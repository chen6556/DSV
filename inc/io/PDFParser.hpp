#include "draw/Graph.hpp"
#include "base/Geometry.hpp"
#include <fstream>
#include <stack>
#include <array>


namespace PDFParser
{

class Importer
{
private:
    std::vector<double> _values;
    std::vector<Geo::Point> _points;
    Graph *_graph = nullptr;

    enum Tool {None, Line, Curve};
    Tool _last_tool = Tool::None, _cur_tool = Tool::None;
    Geo::Coord _start_point;

    std::array<double, 6> _trans_mat = {1, 0, 0, 0, 1, 0};
    std::stack<std::array<double, 6>> _trans_mats;

public:

    void start();

    void store_value(const double value);


    void change_trans_mat();

    void store_trans_mat();

    void pop_trans_mat();


    void line();

    void curve();

    void rect();

    void close_shape();

    void close_and_store_shape();
    

    void store();

    void load_graph(Graph *g);

    void reset();

    void print(const std::string &value);
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);

}
