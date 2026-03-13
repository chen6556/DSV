#include <QFont>
#include <QSpinBox>
#include "PropertyWidget.hpp"
#include "./ui_PropertyWidget.h"
#include "WinUITool.hpp"
#include "base/Algorithm.hpp"


PropertyWidget::PropertyWidget(QWidget *parent) : QDialog(parent), ui(new Ui::PropertyWidget)
{
    ui->setupUi(this);
    init();
}

PropertyWidget::~PropertyWidget()
{
    delete ui;
}

void PropertyWidget::init()
{
    QFont font = ui->stackedWidget->font();
    font.setPointSize(10);
    for (int i = 0, count = ui->stackedWidget->count(); i < count; ++i)
    {
        QScrollArea *scrollarea = static_cast<QScrollArea *>(ui->stackedWidget->widget(i)->layout()->itemAt(0)->widget());
        QFormLayout *layout = static_cast<QFormLayout *>(scrollarea->widget()->layout()->itemAt(0)->layout());
        for (int j = 0, rowcount = layout->rowCount(); j < rowcount; ++j)
        {
            QLayoutItem *item = layout->itemAt(j, QFormLayout::ItemRole::FieldRole);
            item->widget()->setMinimumHeight(24);
            item->widget()->setFont(font);
            if (QSpinBox *p = dynamic_cast<QSpinBox *>(item->widget()))
            {
                p->setMaximum(INT_MAX);
                p->setReadOnly(true);
                p->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
                p->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            }
            else if (QDoubleSpinBox *p = dynamic_cast<QDoubleSpinBox *>(item->widget()))
            {
                if (p->suffix().isEmpty())
                {
                    QLabel *label = static_cast<QLabel *>(layout->itemAt(j, QFormLayout::ItemRole::LabelRole)->widget());
                    if (label->text().back() == 'X' || label->text().back() == 'Y')
                    {
                        p->setMinimum(-DBL_MAX);
                    }
                    p->setMaximum(DBL_MAX);
                }
                p->setDecimals(4);
                p->setReadOnly(true);
                p->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
                p->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            }
            else if (QLineEdit *p = dynamic_cast<QLineEdit *>(item->widget()))
            {
                p->setReadOnly(true);
                p->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            }
        }
    }
}

void PropertyWidget::show(Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::ARC:
        show(static_cast<Geo::Arc *>(object));
        break;
    case Geo::Type::BEZIER:
        show(static_cast<Geo::CubicBezier *>(object));
        break;
    case Geo::Type::BSPLINE:
        show(static_cast<Geo::BSpline *>(object));
        break;
    case Geo::Type::CIRCLE:
        show(static_cast<Geo::Circle *>(object));
        break;
    case Geo::Type::COMBINATION:
        show(static_cast<Combination *>(object));
        break;
    case Geo::Type::ELLIPSE:
        show(static_cast<Geo::Ellipse *>(object));
        break;
    case Geo::Type::POINT:
        show(static_cast<Geo::Point *>(object));
        break;
    case Geo::Type::POLYGON:
        show(static_cast<Geo::Polygon *>(object));
        break;
    case Geo::Type::POLYLINE:
        show(static_cast<Geo::Polyline *>(object));
        break;
    case Geo::Type::TEXT:
        show(static_cast<Text *>(object));
        break;
    default:
        break;
    }

#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
    QDialog::exec();
}

void PropertyWidget::show(Geo::Arc *arc)
{
    _arc = arc;
    ui->stackedWidget->setCurrentIndex(0);
    ui->object_type_lb->setText("Arc");
    ui->arc_startX->setValue(arc->control_points[0].x);
    ui->arc_startY->setValue(arc->control_points[0].y);
    ui->arc_centerX->setValue(arc->x);
    ui->arc_centerY->setValue(arc->y);
    ui->arc_endX->setValue(arc->control_points[2].x);
    ui->arc_endY->setValue(arc->control_points[2].y);
    ui->arc_startAngle->setValue(Geo::rad_to_degree(Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[0])));
    ui->arc_endAngle->setValue(Geo::rad_to_degree(Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[2])));
    ui->arc_clockwise->setText(arc->is_cw() ? "true" : "false");
    ui->arc_radius->setValue(arc->radius);
    ui->arc_length->setValue(arc->length());
}

void PropertyWidget::show(Geo::CubicBezier *bezier)
{
    _bezier = bezier;
    ui->stackedWidget->setCurrentIndex(1);
    ui->object_type_lb->setText("Cubic Bezier");
    ui->bezier_controlPoint->setValue(bezier->size());
    ui->bezier_controlPoint->setValue(0);
    ui->bezier_controlPoint->setRange(0, bezier->size() - 1);
    ui->bezier_controlPointX->setValue(bezier->at(0).x);
    ui->bezier_controlPointY->setValue(bezier->at(0).y);
    ui->bezier_pathPointCount->setValue(bezier->size() / 3 + 1);
    ui->bezier_pathPoint->setValue(0);
    ui->bezier_pathPoint->setRange(0, bezier->size() / 3);
    ui->bezier_pathPointX->setValue(bezier->at(0).x);
    ui->bezier_pathPointY->setValue(bezier->at(0).y);
    ui->bezier_length->setValue(bezier->length());
}

void PropertyWidget::show(Geo::BSpline *bspline)
{
    _bspline = bspline;
    ui->stackedWidget->setCurrentIndex(2);
    ui->object_type_lb->setText(dynamic_cast<Geo::CubicBSpline *>(bspline) == nullptr ? "Quad BSpline" : "Cubic BSpline");
    ui->bspline_controlPoint->setValue(bspline->control_points.size());
    ui->bspline_controlPoint->setValue(0);
    ui->bspline_controlPoint->setRange(0, bspline->control_points.size() - 1);
    ui->bspline_controlPointX->setValue(bspline->control_points[0].x);
    ui->bspline_controlPointY->setValue(bspline->control_points[0].y);
    ui->bspline_pathPointCount->setValue(bspline->path_points.size());
    ui->bspline_pathPoint->setValue(0);
    ui->bspline_pathPoint->setRange(0, bspline->path_points.size());
    ui->bspline_pathPointX->setValue(bspline->path_points[0].x);
    ui->bspline_pathPointY->setValue(bspline->path_points[0].y);
    ui->bspline_length->setValue(bspline->length());
}

void PropertyWidget::show(Geo::Circle *circle)
{
    _circle = circle;
    ui->stackedWidget->setCurrentIndex(3);
    ui->object_type_lb->setText("Circle");
    ui->circle_centerX->setValue(circle->x);
    ui->circle_centerY->setValue(circle->y);
    ui->circle_radius->setValue(circle->radius);
    ui->circle_length->setValue(circle->length());
    ui->circle_area->setValue(circle->area());
}

void PropertyWidget::show(Combination *combination)
{
    _combination = combination;
    ui->stackedWidget->setCurrentIndex(4);
    ui->object_type_lb->setText("Combination");
    ui->combination_itemCount->setValue(combination->size());
    const Geo::AABBRectParams rect = combination->aabbrect_params();
    ui->combination_centerX->setValue((rect.left + rect.right) / 2);
    ui->combination_centerY->setValue((rect.top + rect.bottom) / 2);
    ui->combination_width->setValue(rect.right - rect.left);
    ui->combination_height->setValue(rect.top - rect.bottom);
}

void PropertyWidget::show(Geo::Ellipse *ellipse)
{
    _ellipse = ellipse;
    ui->stackedWidget->setCurrentIndex(5);
    ui->object_type_lb->setText("Ellipse");
    ui->ellipse_startX->setValue(ellipse->arc_point0().x);
    ui->ellipse_startY->setValue(ellipse->arc_point0().y);
    ui->ellipse_centerX->setValue(ellipse->center().x);
    ui->ellipse_centerY->setValue(ellipse->center().y);
    ui->ellipse_endX->setValue(ellipse->arc_point1().x);
    ui->ellipse_endY->setValue(ellipse->arc_point1().y);
    ui->ellipse_startAngle->setValue(Geo::rad_to_degree(ellipse->arc_angle0()));
    ui->ellipse_endAngle->setValue(Geo::rad_to_degree(ellipse->arc_angle1()));
    if (const double a = ellipse->lengtha(), b = ellipse->lengthb(); a > b)
    {
        ui->ellipse_radiusA->setValue(a);
        ui->ellipse_radiusB->setValue(b);
        ui->ellipse_ratio->setValue(b / a);
    }
    else
    {
        ui->ellipse_radiusA->setValue(b);
        ui->ellipse_radiusB->setValue(a);
        ui->ellipse_ratio->setValue(a / b);
    }
    ui->ellipse_length->setValue(ellipse->length());
    ui->ellipse_area->setValue(ellipse->area());
}

void PropertyWidget::show(Geo::Point *point)
{
    _point = point;
    ui->stackedWidget->setCurrentIndex(6);
    ui->object_type_lb->setText("Point");
    ui->point_x->setValue(point->x);
    ui->point_y->setValue(point->y);
    ui->point_length->setValue(point->length());
}

void PropertyWidget::show(Geo::Polygon *polygon)
{
    _polygon = polygon;
    ui->stackedWidget->setCurrentIndex(7);
    ui->object_type_lb->setText("Polygon");
    ui->polygon_pointCount->setValue(polygon->size());
    ui->polygon_point->setValue(0);
    ui->polygon_point->setRange(0, polygon->size() - 1);
    ui->polygon_pointX->setValue(polygon->at(0).x);
    ui->polygon_pointY->setValue(polygon->at(0).y);
    ui->polygon_clockwise->setText(polygon->is_cw() ? "true" : "false");
    ui->polygon_length->setValue(polygon->length());
    ui->polygon_area->setValue(polygon->area());
}

void PropertyWidget::show(Geo::Polyline *polyline)
{
    _polyline = polyline;
    ui->stackedWidget->setCurrentIndex(8);
    ui->object_type_lb->setText("Polyline");
    ui->polyline_pointCount->setValue(polyline->size());
    ui->polyline_point->setValue(0);
    ui->polyline_point->setRange(0, polyline->size() - 1);
    ui->polyline_pointX->setValue(polyline->at(0).x);
    ui->polyline_pointY->setValue(polyline->at(0).y);
    ui->polyline_length->setValue(polyline->length());
}

void PropertyWidget::show(Text *text)
{
    _text = text;
    ui->stackedWidget->setCurrentIndex(9);
    ui->object_type_lb->setText("Text");
    ui->text_centerX->setValue(text->center().x);
    ui->text_centerY->setValue(text->center().y);
    ui->text_width->setValue(text->width());
    ui->text_height->setValue(text->height());
}