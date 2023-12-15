#pragma once

#include "draw/Graph.hpp"
#include "base/Geometry.hpp"
#include <fstream>
#include <stack>
#include <array>
#include <cfloat>


namespace PDFParser
{

class Function
{
public:
    int _type;
    std::vector<double> _domain;
    std::vector<double> _range;

public:
    static double interpolate(double x, double xmin, double xmax, double ymin, double ymax)
    {
        double value = (ymax - ymin) / (xmax - xmin);
        value *= (x - xmin);
        value += ymin;
        return value;
    };

    int get_num_inputs() const
    {
        return _domain.size() / 2;
    };

    int get_num_outputs() const
    {
        return _range.size() / 2;
    }

    double get_range(const int i)
    {
        if (i >= _range.size())
        {
            return i % 2 == 0 ? DBL_MIN : DBL_MAX;
        }
        else
        {
            return _range[i];
        }
    }

    virtual void do_function(std::vector<double> &inputs, const int inputOffset, std::vector<double> &outputs, const int outputOffset) = 0;

    virtual std::vector<double> &calculate(std::vector<double> &inputs, const int inputOffset, std::vector<double> &outputs, const int outputOffset) = 0;

    virtual std::vector<double> calculate(std::vector<double> &inputs) = 0;
};

class FunctionType0 final: public Function
{
public:
    std::vector<int> _size;
    int _bits_per_sample;
    int _order = 1;
    std::vector<double> _encoded;
    std::vector<double> _decoded;
    std::vector<std::vector<int>> _samples;

private:
    virtual void do_function(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        std::vector<double> encoded(get_num_inputs());
        for (int i = 0, count = get_num_inputs(); i < count; ++i)
        {
            encoded[i] = interpolate(inputs[i + input_offset], _domain[2 * i], _decoded[2 * i + i], _encoded[2 * i], _encoded[2 * i + 1]);
            encoded[i] = std::max(encoded[i], 0.0);
            encoded[i] = std::min(encoded[i], _size[i] - 1.0);
        }

        for (int i = 0, count = get_num_outputs(); i < count; ++i)
        {
            outputs[i] = _order == 1 ? multilinear_interpolate(encoded, i) : multicubic_interpolate(encoded, i);
        }

        for (int i = 0, count = outputs.size(); i < count; ++i)
        {
            outputs[i + output_offset] = interpolate(outputs[i + output_offset], 0, std::pow(2, _bits_per_sample) - 1, _decoded[2 * i], _decoded[2 * i + 1]);
        }
    }

    double get_sample(std::vector<int> &values, int od)
    {
        int mult = 1, index = 0;
        for (int i = 0, count = values.size(); i < count; ++i)
        {
            index += (mult * values[i]);
            mult *= _size[i];
        }
        return _samples[index][od];
    }

    double get_sample(std::vector<double> &encoded, int map, int od)
    {
        std::vector<int> controls(encoded.size(), 0);
        for (int i = 0, count = controls.size(); i < count; ++i)
        {
            controls[i] = ((map & (0x1 << i)) == 0) ? std::floor(encoded[i]) : std::ceil(encoded[i]);
        }
        return get_sample(controls, od);
    };

    double multilinear_interpolate(std::vector<double> &encoded, int od)
    {
        std::vector<double> dists(encoded);
        for (int i = 0, count = dists.size(); i < count; ++i)
        {
            dists[i] = encoded[i] - std::floor(encoded[i]);
        }
        int map = 0;
        double val = get_sample(encoded, map, od);
        double prev = val;
        for (int idx, i = 0, count = dists.size(); i < count; ++i)
        {
            idx = 0;
            double largest = -1;
            for (int c = 0; c < count; ++c)
            {
                if (dists[c] > largest)
                {
                    largest = dists[c];
                    idx = c;
                }
            }

            map |= (0x1 << idx);
            double cur = get_sample(encoded, map, od);

            val += (dists[idx] * (cur - prev));
            prev = val;

            dists[idx] = -1;
        }
        return val;
    };

    double multicubic_interpolate(std::vector<double> &encoded, int od)
    {
        return multilinear_interpolate(encoded, od);
    };

public:
    void read_samples(const std::string &text)
    {
        int size = 1;
        for (int i = 0, count = get_num_inputs(); i < count; ++i)
        {
            size *= _size[i];
        }
        _samples.assign(size, std::vector<int>(get_num_outputs(), 0));

        int bitLoc = 0, byteLoc = 0, index = 0;
        for (int i = 0, inputNum = get_num_inputs(), outputNum = get_num_outputs(); i < inputNum; ++i)
        {
            for (int j = 0; j < _size[i]; ++j)
            {
                for (int k = 0; k < outputNum; ++k)
                {
                    int value = 0;
                    int toRead = _bits_per_sample;
                    char curByte = text[byteLoc];

                    while (toRead > 0)
                    {
                        int nextBit = ((curByte >> (7 - bitLoc)) & 0x1);
                        value |= (nextBit << (toRead - 1));

                        if (++bitLoc == 8)
                        {
                            bitLoc = 0;
                            ++byteLoc;

                            if (toRead > 1)
                            {
                                curByte = text[byteLoc];
                            }
                        }

                        --toRead;
                    }

                    _samples[index][k] = value;
                }
                ++index;
            }
        }
    };

    virtual std::vector<double> &calculate(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        for (int i = 0, count = inputs.size(); i < count; ++i)
        {
            inputs[i] = std::max(inputs[i], _domain[2 * i]);
            inputs[i] = std::min(inputs[i], _domain[2 * i + 1]);
        }

        do_function(inputs, input_offset, outputs, output_offset);

        for (int i = 0, count = outputs.size(); i < count; ++i)
        {
            outputs[i] = std::max(outputs[i], _range[2 * i]);
            outputs[i] = std::min(outputs[i], _range[2 * i + 1]);
        }

        return outputs;
    };

    virtual std::vector<double> calculate(std::vector<double> &inputs)
    {
        std::vector<double> outputs(get_num_outputs(), 0.0);
        calculate(inputs, 0, outputs, 0);
        return outputs;
    };
};

class FunctionType2 final : public Function
{
public:
    double _N = 1.0;
    std::vector<double> _C0;
    std::vector<double> _C1;

private:
    virtual void do_function(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        const double input = inputs[input_offset];

        for (int i = 0, count = get_num_outputs(); i < count; ++i)
        {
            outputs[i + output_offset] = _C0[i] + std::pow(input, _N) * (_C1[i] - _C0[i]);
        }
    };

public:
    virtual std::vector<double> &calculate(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        for (int i = 0, count = inputs.size(); i < count; ++i)
        {
            inputs[i] = std::max(inputs[i], _domain[2 * i]);
            inputs[i] = std::min(inputs[i], _domain[2 * i + 1]);
        }

        do_function(inputs, input_offset, outputs, output_offset);

        for (int i = 0, count = outputs.size(); i < count; ++i)
        {
            outputs[i] = std::max(outputs[i], _range[2 * i]);
            outputs[i] = std::min(outputs[i], _range[2 * i + 1]);
        }

        return outputs;
    };

    virtual std::vector<double> calculate(std::vector<double> &inputs)
    {
        std::vector<double> outputs(get_num_outputs(), 0.0);
        calculate(inputs, 0, outputs, 0);
        return outputs;
    };
};

class FunctionType3 final  : public Function
{
public:
    std::vector<std::shared_ptr<Function>> _functions;
    std::vector<double> _bounds;
    std::vector<double> _encode;

private:
    virtual void do_function(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        double x = inputs[input_offset];
        int p = _bounds.size() - 2;
        while (x < _bounds[p])
        {
            --p;
        }

        x = interpolate(x, _bounds[p], _bounds[p + 1], _encode[2 * p], _encode[2 * p + 1]);
        std::vector<double> value(1, x);
        std::vector<double> out = _functions[p]->calculate(value);
        
        for (int i = 0, count = out.size(); i < count; ++i)
        {
            outputs[i + output_offset] = out[i];
        }
    }

public:
    virtual std::vector<double> &calculate(std::vector<double> &inputs, const int input_offset, std::vector<double> &outputs, const int output_offset)
    {
        for (int i = 0, count = inputs.size(); i < count; ++i)
        {
            inputs[i] = std::max(inputs[i], _domain[2 * i]);
            inputs[i] = std::min(inputs[i], _domain[2 * i + 1]);
        }

        do_function(inputs, input_offset, outputs, output_offset);

        for (int i = 0, count = outputs.size(); i < count; ++i)
        {
            outputs[i] = std::max(outputs[i], _range[2 * i]);
            outputs[i] = std::min(outputs[i], _range[2 * i + 1]);
        }

        return outputs;
    };

    virtual std::vector<double> calculate(std::vector<double> &inputs)
    {
        std::vector<double> outputs(get_num_outputs(), 0.0);
        calculate(inputs, 0, outputs, 0);
        return outputs;
    };
};

enum ColorSpace {Gray, RGB, CMYK};

class DeviceColor
{
public:
    ColorSpace _color_space = ColorSpace::RGB;
    std::shared_ptr<Function> _func = nullptr;
    std::vector<int> _function_index;
    
    std::vector<double> operator()(std::vector<double> inputs) const
    {
        return _func->calculate(inputs);
    }

    std::vector<double> operator()(double input) const
    {
        std::vector<double> inputs(1, input);
        return _func->calculate(inputs);
    }
};

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

    double _stroking_color[4] = { 0.0, 0.0, 0.0, 0.0 };
    ColorSpace _stroking_color_space = ColorSpace::RGB;
    double _nonstroking_color[4] = { 0.0, 0.0, 0.0, 0.0 };
    ColorSpace _nonstroking_color_space = ColorSpace::RGB;
    int _cur_color_map_index = -1;
	std::map<std::string, int> _color_map_index; // 颜色编码索引
	std::map<int, DeviceColor> _color_map; // 颜色编码
    std::vector<std::string> _keys;

    int _cur_object = 0;
	std::vector<int> _objects;
        
public:

    void start();

    void store_value(const double value);

    void store_key(const std::string &value);

    void CS();

    void cs();

    void SCN();

    void G();

    void g();

    void RG();

    void rg();

    void K();

    void k();


    void change_trans_mat();

    void store_trans_mat();

    void pop_trans_mat();

    void store_object(const int value);


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


    void get_color_map_index(const std::string_view &stream);

    void get_color_map(const std::string_view &stream);
    

    void store();

    void load_graph(Graph *g);

    void reset();
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);

}
