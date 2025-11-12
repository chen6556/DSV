#pragma once
#include <set>
#include <QString>
#include <QMouseEvent>
#include "base/Geometry.hpp"
#include "base/Editer.hpp"


class Canvas;

namespace CanvasOperations
{
    enum class Tool
    {
        Select, 
        Move, 
        Measure, 
        Angle, 
        Circle0, // Center-Radius
        Circle1, // 2-Point
        Circle2, // 3-Point
        Polyline, 
        Arc0, // 3-Point
        Arc1, // Start-Center-End
        Arc2, // Start-End-Angle
        Arc3, // Start-End-Radius
        Rectangle,
        Polygon0,
        Polygon1, 
        BSpline, 
        Bezier, 
        Text, 
        Ellipse,
 
        Mirror, 
        RingArray, 
        PolygonDifference, 
        Fillet, 
        Chamfer, 
        Rotate, 
        Trim, 
        Extend, 
        Split, 

        End
    };

    class CanvasOperation
    {
    public:
        static double *shape;
        static double *tool_lines;
        static unsigned int shape_len;
        static unsigned int shape_count;
        static unsigned int tool_lines_len;
        static unsigned int tool_lines_count;
        static float tool_line_width;
        static float tool_line_color[4];
        static double real_pos[4]; // current(x,y), last(x,y)
        static double press_pos[2];
        static double release_pos[2];
        static Tool tool[2]; // current, last
        static double view_ratio;
        static QString info;
        static Editer *editer;
        static Canvas *canvas;
        static Geo::Geometry *clicked_object;
        static bool absolute_coord;

    protected:
        enum class ParamType {LengthAngle, Coord};

    private:
        CanvasOperation *operations[static_cast<int>(Tool::End)] = {nullptr};

    protected:
        CanvasOperation() = default;

        CanvasOperation(const CanvasOperation &) = delete;

        CanvasOperation &operator=(const CanvasOperation &) = delete;

        virtual ~CanvasOperation();
    
    public:
        static CanvasOperation &operation();

        void init();

        void clear();

        CanvasOperation *operator[](const Tool tool);

        static void check_shape_size();

        static void check_shape_size(const size_t count);

        static void check_tool_lines_size();

        static void check_tool_lines_size(const size_t count);

        virtual bool mouse_press(QMouseEvent *event);

        virtual bool mouse_release(QMouseEvent *event);

        virtual bool mouse_move(QMouseEvent *event);

        virtual bool mouse_double_click(QMouseEvent *event);

        virtual void reset();

        virtual bool read_parameters(const double *params, const int count);

        virtual QString cmd_tips() const;

        virtual void switch_parameters_type();
    };


    class SelectOperation : public CanvasOperation
    {
    private:
        double _pos[2];
        bool _select = false;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_release(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;
    };


    class MoveOperation : public CanvasOperation
    {
    public:
        bool mouse_release(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;
    };


    class MeasureOperation : public CanvasOperation
    {
    private:
        Geo::Point _pos[2];
        bool _is_head = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;
    };


    class AngleOperation : public CanvasOperation
    {
    private:
        Geo::Point _pos[3];
        int _index = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;
    };


    class Circle0Operation : public CanvasOperation
    {
    private:
        double _parameters[3]; // x, y, r
        bool _set_center = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class Circle1Operation : public CanvasOperation
    {
    private:
        double _parameters[4]; // x0, y0, x1, y1
        bool _set_first_point = true;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class Circle2Operation : public CanvasOperation
    {
    private:
        double _parameters[6]; // x0, y0, x1, y1, x2, y2
        int _index = 0;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class PolythingOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class Arc0Operation : public CanvasOperation
    {
    private:
        double _parameters[6];
        int _index = 0;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class Arc1Operation : public CanvasOperation
    {
    private:
        double _parameters[5];
        int _index = 0;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;    
    };


    class Arc2Operation : public CanvasOperation
    {
    private:
        double _parameters[5];
        int _index = 0;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class Arc3Operation : public CanvasOperation
    {
    private:
        double _parameters[5];
        int _index = 0;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class RectangleOperation : public CanvasOperation
    {
    private:
        double _parameters[4];
        bool _set_first_point = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class CircumscribedPolygonOperation : public CanvasOperation
    {
    private:
        double _parameters[4]; // x, y, r, angle
        int _n = 5;
        bool _set_n = true, _set_center = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class InscribedPolygonOperation : public CanvasOperation
    {
    private:
        double _parameters[4]; // x, y, r, angle
        int _n = 5;
        bool _set_n = true, _set_center = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class BSplineOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;
        int _order = 3;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class BezierOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;
        int _order = 3;
        ParamType _param_type = ParamType::LengthAngle;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;

        void switch_parameters_type() override;
    };


    class TextOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class EllipseOperation : public CanvasOperation
    {
    private:
        double _parameters[5]; // x, y, a, b, angle
        int _index = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class MirrorOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
    };


    class RingArrayOperation : public CanvasOperation
    {
    private:
        int _count = 4;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class PolygonDifferenceOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
    };


    class FilletOperation : public CanvasOperation
    {
    private:
        double _radius = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class ChamferOperation : public CanvasOperation
    {
    private:
        double _distance = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class RotateOperation : public CanvasOperation
    {
    private:
        Geo::Point _pos[3];
        int _index = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;

        bool read_parameters(const double *params, const int count) override;

        QString cmd_tips() const override;
    };


    class TrimOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
    };


    class ExtendOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
    };


    class SplitOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
    };
}