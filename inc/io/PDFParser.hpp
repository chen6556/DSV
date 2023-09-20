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

    std::map<std::string, std::string> _encoding_map;
    std::string _text;
    struct Text
    {
        std::string txt;
        Geo::Point pos;

        Text(const std::string &text, const Geo::Point &position)
            : txt(text), pos(position) {};
    };
    std::list<Text> _texts;

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


    void store_text(const std::string &value);

    void store_encoding(const std::string &value);

    void BT();

    void ET();

    void Tm();
    
    void end();
    

    void store();

    void load_graph(Graph *g);

    void reset();
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);

}
