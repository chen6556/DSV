#pragma once

#include <QDialog>
#include "base/Container.hpp"


QT_BEGIN_NAMESPACE
namespace Ui
{
class PropertyWidget;
}
QT_END_NAMESPACE


class PropertyWidget : public QDialog
{
private:
    Ui::PropertyWidget *ui = nullptr;
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

public:
    PropertyWidget(QWidget *parent);

    ~PropertyWidget() override;

    void show(Geo::Geometry *object);

private:
    void init();

    void show(Geo::Arc *arc);

    void show(Geo::CubicBezier *bezier);

    void show(Geo::BSpline *bspline);

    void show(Geo::Circle *circle);

    void show(Combination *combination);

    void show(Geo::Ellipse *ellipse);

    void show(Geo::Point *point);

    void show(Geo::Polygon *polygon);

    void show(Geo::Polyline *polyline);

    void show(Text *text);
};