#pragma once
#include <tuple>
#include <vector>
#include <QDialog>
#include <QFont>
#include "base/Container.hpp"
#include "base/Dimension.hpp"


QT_BEGIN_NAMESPACE
namespace Ui
{
class PropertyWidget;
}
QT_END_NAMESPACE


class Canvas;

class PropertyWidget : public QDialog
{
private:
    Ui::PropertyWidget *ui = nullptr;
    Canvas *_canvas = nullptr;
    Geo::Arc *_arc = nullptr;
    Geo::CubicBezier *_bezier = nullptr;
    Geo::BSpline *_bspline = nullptr;
    Geo::Circle *_circle = nullptr;
    Combination *_combination = nullptr;
    Geo::Ellipse *_ellipse = nullptr;
    Geo::Point *_point = nullptr;
    Geo::Polygon *_polygon = nullptr;
    Geo::Polyline *_polyline = nullptr;
    Text *_text = nullptr;
    Dim::Dimension *_dim = nullptr;
    QFont _font;
    QString _txt;
    std::vector<std::tuple<double, double>> _shape, _path_points;
    std::vector<double> _knots;

public:
    PropertyWidget(Canvas *canvas);

    ~PropertyWidget() override;

    void show(Geo::Geometry *object);

private:
    void init();

    void init_arc_widget();

    void init_bezier_widget();

    void init_bspline_widget();

    void init_circle_widget();

    void init_combination_widget();

    void init_ellipse_widget();

    void init_point_widget();

    void init_polygon_widget();

    void init_polyline_widget();

    void init_text_widget();

    void init_dimension_widget();

    void read(Geo::Geometry *object);

    void read(Geo::Arc *arc);

    void read(Geo::CubicBezier *bezier);

    void read(Geo::BSpline *bspline);

    void read(Geo::Circle *circle);

    void read(Combination *combination);

    void read(Geo::Ellipse *ellipse);

    void read(Geo::Point *point);

    void read(Geo::Polygon *polygon);

    void read(Geo::Polyline *polyline);

    void read(Text *text);

    void read(Dim::Dimension *dim);

    void check(Geo::Geometry *object);

    void check(Geo::Arc *arc);

    void check(Geo::CubicBezier *bezier);

    void check(Geo::BSpline *bspline);

    void check(Geo::Circle *circle);

    void check(Combination *combination);

    void check(Geo::Ellipse *ellipse);

    void check(Geo::Point *point);

    void check(Geo::Polygon *polygon);

    void check(Geo::Polyline *polyline);

    void check(Text *text);

    void move_bezier_point(int index, double x0, double y0);

    void move_bspline_point(int index, double x0, double y0);
};