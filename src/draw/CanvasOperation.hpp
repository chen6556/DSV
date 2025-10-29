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
        Ellipse
    };

    class CanvasOperation
    {
    public:
        static const double *matrix;
        static double *shape;
        static double *tool_lines;
        static unsigned int shape_len;
        static unsigned int shape_count;
        static unsigned int tool_lines_len;
        static unsigned int tool_lines_count;
        static float tool_line_width;
        static double real_pos[2];
        static QString info;

    private:
        CanvasOperation *operations[3] = {nullptr};
    
    public:
        void init(const double *mat);

        void clear();

        virtual ~CanvasOperation();

        CanvasOperation *operator[](const Tool tool);

        static void calc_real_pos(const double x, const double y);

        virtual bool mouse_press(QMouseEvent *event);

        virtual bool mouse_release(QMouseEvent *event);

        virtual bool mouse_move(QMouseEvent *event);

        virtual bool mouse_double_click(QMouseEvent *event);

        virtual Geo::Geometry *geometry();

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
}