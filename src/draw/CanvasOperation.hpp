#pragma once
#include <QString>
#include <QMouseEvent>
#include "base/Geometry.hpp"


namespace CanvasOperations
{
    enum class Tool
    {
        NoTool,
        Measure, 
        Angle, 
        Circle0, // Center-Radius
        Circle1, // 2-Point
        Circle2, // 3-Point
        Polyline, 
        Rect, 
        BSpline, 
        Bezier, 
        Text, 
        Ellipse,

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
        static double real_pos[2];
        static double view_ratio;
        static bool finish;
        static QString info;
        static std::function<void(Geo::Geometry *)> add_geometry;

    private:
        CanvasOperation *operations[static_cast<int>(Tool::End)] = {nullptr};
    
    public:
        void init(std::function<void(Geo::Geometry *)> func);

        void clear();

        virtual ~CanvasOperation();

        CanvasOperation *operator[](const Tool tool);

        static void check_shape_size();

        static void check_tool_lines_size();

        virtual bool mouse_press(QMouseEvent *event);

        virtual bool mouse_release(QMouseEvent *event);

        virtual bool mouse_move(QMouseEvent *event);

        virtual bool mouse_double_click(QMouseEvent *event);

        virtual void reset();
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
    };


    class Circle1Operation : public CanvasOperation
    {
    private:
        double _parameters[4]; // x0, y0, x1, y1
        bool _set_first_point = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;
    };


    class Circle2Operation : public CanvasOperation
    {
    private:
        double _parameters[6]; // x0, y0, x1, y1, x2, y2
        int _index = 0;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;
    };


    class PolythingOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;
    };


    class RectOperation : public CanvasOperation
    {
    private:
        double _parameters[4];
        bool _set_first_point = true;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        void reset() override;
    };


    class BSplineOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;
        int _order = 3;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;
    };


    class BezierOperation : public CanvasOperation
    {
    private:
        std::vector<Geo::Point> _points;
        int _order = 3;

    public:
        bool mouse_press(QMouseEvent *event) override;

        bool mouse_move(QMouseEvent *event) override;

        bool mouse_double_click(QMouseEvent *event) override;

        void reset() override;
    };


    class TextOperation : public CanvasOperation
    {
    public:
        bool mouse_press(QMouseEvent *event) override;
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
    };
}