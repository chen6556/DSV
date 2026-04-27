#include <QFont>
#include "PropertyWidget.hpp"
#include "./ui_PropertyWidget.h"
#include "WinUITool.hpp"
#include "base/Algorithm.hpp"
#include "draw/Canvas.hpp"


PropertyWidget::PropertyWidget(Canvas *canvas) : QDialog(canvas), _canvas(canvas), ui(new Ui::PropertyWidget)
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
                p->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
                p->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            }
            else if (QLineEdit *p = dynamic_cast<QLineEdit *>(item->widget()))
            {
                p->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
            }
        }
    }

    init_arc_widget();
    init_bezier_widget();
    init_bspline_widget();
    init_circle_widget();
    init_combination_widget();
    init_ellipse_widget();
    init_point_widget();
    init_polygon_widget();
    init_polyline_widget();
    init_text_widget();
    init_dimension_widget();
}

void PropertyWidget::init_arc_widget()
{
    ui->arc_radius->setRange(0, DBL_MAX);
    connect(ui->arc_centerX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _arc->translate(value - _arc->x, 0);
                ui->arc_startX->setValue(_arc->control_points[0].x);
                ui->arc_startY->setValue(_arc->control_points[0].y);
                ui->arc_endX->setValue(_arc->control_points[2].x);
                ui->arc_endY->setValue(_arc->control_points[2].y);
                _canvas->refresh_vbo(true, Geo::Type::ARC);
                _canvas->update();
            });
    connect(ui->arc_centerY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _arc->translate(0, value - _arc->y);
                ui->arc_startX->setValue(_arc->control_points[0].x);
                ui->arc_startY->setValue(_arc->control_points[0].y);
                ui->arc_endX->setValue(_arc->control_points[2].x);
                ui->arc_endY->setValue(_arc->control_points[2].y);
                _canvas->refresh_vbo(true, Geo::Type::ARC);
                _canvas->update();
            });
    connect(ui->arc_startAngle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _arc->control_points[0].rotate(
                    _arc->x, _arc->y, Geo::degree_to_rad(value) - Geo::angle(Geo::Point(_arc->x, _arc->y), _arc->control_points[0]));
                _arc->update_shape(Geo::Circle::default_down_sampling_value);
                ui->arc_startX->setValue(_arc->control_points[0].x);
                ui->arc_startY->setValue(_arc->control_points[0].y);
                ui->arc_length->setValue(_arc->length());
                _canvas->refresh_vbo(true, Geo::Type::ARC);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
    connect(ui->arc_endAngle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _arc->control_points[2].rotate(
                    _arc->x, _arc->y, Geo::degree_to_rad(value) - Geo::angle(Geo::Point(_arc->x, _arc->y), _arc->control_points[2]));
                _arc->update_shape(Geo::Circle::default_down_sampling_value);
                ui->arc_endX->setValue(_arc->control_points[2].x);
                ui->arc_endY->setValue(_arc->control_points[2].y);
                ui->arc_length->setValue(_arc->length());
                _canvas->refresh_vbo(true, Geo::Type::ARC);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
    connect(ui->arc_radius, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const Geo::Point center(_arc->x, _arc->y);
                for (int i = 0; i < 3; ++i)
                {
                    Geo::Point vec(_arc->control_points[i] - center);
                    vec *= (value / _arc->radius - 1);
                    _arc->control_points[i] += vec;
                }
                _arc->radius = value;
                _arc->update_shape(Geo::Circle::default_down_sampling_value);
                ui->arc_length->setValue(_arc->length());
                _canvas->refresh_vbo(true, Geo::Type::ARC);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
}

void PropertyWidget::init_bezier_widget()
{
    connect(ui->bezier_point, &QSpinBox::valueChanged,
            [this](int index)
            {
                ui->bezier_pointX->blockSignals(true);
                ui->bezier_pointY->blockSignals(true);
                ui->bezier_pointX->setValue(_bezier->at(index).x);
                ui->bezier_pointY->setValue(_bezier->at(index).y);
                ui->bezier_pointX->blockSignals(false);
                ui->bezier_pointY->blockSignals(false);
            });
    connect(ui->bezier_pointX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const int index = ui->bezier_point->value();
                const double x0 = _bezier->at(index).x;
                _bezier->at(index).x = value;
                move_bezier_point(index, x0, _bezier->at(index).y);
            });
    connect(ui->bezier_pointY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const int index = ui->bezier_point->value();
                const double y0 = _bezier->at(index).y;
                _bezier->at(index).y = value;
                move_bezier_point(index, _bezier->at(index).x, y0);
            });
}

void PropertyWidget::init_bspline_widget()
{
    connect(ui->bspline_pointTypeCbx, &QComboBox::currentIndexChanged,
            [this](int index)
            {
                _bspline->controls_model = index == 1;
                CanvasOperations::CanvasOperation::refresh_tool_lines(_bspline);
                _canvas->update();
                ui->bspline_point->blockSignals(true);
                ui->bspline_pointX->blockSignals(true);
                ui->bspline_pointY->blockSignals(true);
                ui->bspline_point->setValue(0);
                if (index == 0)
                {
                    ui->bspline_pointCount->setValue(_bspline->path_points.size());
                    ui->bspline_point->setRange(0, _bspline->path_points.size() - 1);
                    ui->bspline_pointX->setValue(_bspline->path_points[0].x);
                    ui->bspline_pointX->setValue(_bspline->path_points[0].y);
                }
                else
                {
                    ui->bspline_pointCount->setValue(_bspline->control_points.size());
                    ui->bspline_point->setRange(0, _bspline->control_points.size() - 1);
                    ui->bspline_pointX->setValue(_bspline->control_points[0].x);
                    ui->bspline_pointX->setValue(_bspline->control_points[0].y);
                }
                ui->bspline_pointX->blockSignals(false);
                ui->bspline_pointY->blockSignals(false);
                ui->bspline_point->blockSignals(false);
            });
    connect(ui->bspline_point, &QSpinBox::valueChanged,
            [this](int index)
            {
                ui->bspline_pointX->blockSignals(true);
                ui->bspline_pointY->blockSignals(true);
                if (ui->bspline_pointTypeCbx->currentIndex() == 0)
                {
                    ui->bspline_pointX->setValue(_bspline->path_points[index].x);
                    ui->bspline_pointY->setValue(_bspline->path_points[index].y);
                }
                else
                {
                    ui->bspline_pointX->setValue(_bspline->control_points[index].x);
                    ui->bspline_pointY->setValue(_bspline->control_points[index].y);
                }
                ui->bspline_pointX->blockSignals(false);
                ui->bspline_pointY->blockSignals(false);
            });
    connect(ui->bspline_pointX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {

            });
    connect(ui->bspline_pointX, &QDoubleSpinBox::valueChanged,
            [this](double value) { move_bspline_point(ui->bspline_point->value(), value, ui->bspline_pointY->value()); });
    connect(ui->bspline_pointY, &QDoubleSpinBox::valueChanged,
            [this](double value) { move_bspline_point(ui->bspline_point->value(), ui->bspline_pointX->value(), value); });
}

void PropertyWidget::init_circle_widget()
{
    ui->circle_radius->setRange(0, DBL_MAX);
    connect(ui->circle_centerX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _circle->x = value;
                _circle->update_shape(Geo::Circle::default_down_sampling_value);
                _canvas->refresh_vbo(true, Geo::Type::CIRCLE);
                CanvasOperations::CanvasOperation::refresh_tool_lines(_circle);
                _canvas->update();
            });
    connect(ui->circle_centerY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _circle->y = value;
                _circle->update_shape(Geo::Circle::default_down_sampling_value);
                _canvas->refresh_vbo(true, Geo::Type::CIRCLE);
                CanvasOperations::CanvasOperation::refresh_tool_lines(_circle);
                _canvas->update();
            });
    connect(ui->circle_radius, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _circle->radius = value;
                _circle->update_shape(Geo::Circle::default_down_sampling_value);
                ui->circle_area->setValue(_circle->area());
                ui->circle_length->setValue(_circle->length());
                _canvas->refresh_vbo(true, Geo::Type::CIRCLE);
                _canvas->refresh_selected_ibo();
                CanvasOperations::CanvasOperation::refresh_tool_lines(_circle);
                _canvas->update();
            });
}

void PropertyWidget::init_combination_widget()
{
    connect(ui->combination_centerX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const Geo::AABBRectParams rect = _combination->aabbrect_params();
                _combination->translate(value - (rect.left + rect.right) / 2, 0);
                _canvas->refresh_vbo(true, Geo::Type::COMBINATION);
                _canvas->update();
            });
    connect(ui->combination_centerY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const Geo::AABBRectParams rect = _combination->aabbrect_params();
                _combination->translate(0, value - (rect.top + rect.bottom) / 2);
                _canvas->refresh_vbo(true, Geo::Type::COMBINATION);
                _canvas->update();
            });
}

void PropertyWidget::init_ellipse_widget()
{
    connect(ui->ellipse_centerX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->translate(value - _ellipse->center().x, 0);
                CanvasOperations::CanvasOperation::refresh_tool_lines(_ellipse);
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->update();
            });
    connect(ui->ellipse_centerY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->translate(0, value - _ellipse->center().y);
                CanvasOperations::CanvasOperation::refresh_tool_lines(_ellipse);
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->update();
            });
    connect(ui->ellipse_startAngle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->update_angle_param(Geo::degree_to_rad(value), Geo::degree_to_rad(ui->ellipse_endAngle->value()), false);
                _ellipse->update_shape(Geo::Ellipse::default_down_sampling_value);
                ui->ellipse_length->setValue(_ellipse->length());
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
    connect(ui->ellipse_endAngle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->update_angle_param(Geo::degree_to_rad(ui->ellipse_startAngle->value()), Geo::degree_to_rad(value), false);
                _ellipse->update_shape(Geo::Ellipse::default_down_sampling_value);
                ui->ellipse_length->setValue(_ellipse->length());
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
    connect(ui->ellipse_angle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                const Geo::Point center(_ellipse->center());
                _ellipse->rotate(center.x, center.y, Geo::degree_to_rad(value) - _ellipse->angle());
                CanvasOperations::CanvasOperation::refresh_tool_lines(_ellipse);
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->update();
            });
    connect(ui->ellipse_radiusA, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->set_lengtha(value);
                _ellipse->update_shape(Geo::Ellipse::default_down_sampling_value);
                ui->ellipse_ratio->setValue(ui->ellipse_radiusB->value() / value);
                ui->ellipse_length->setValue(_ellipse->length());
                ui->ellipse_area->setValue(_ellipse->area());
                CanvasOperations::CanvasOperation::refresh_tool_lines(_ellipse);
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
    connect(ui->ellipse_radiusB, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _ellipse->set_lengthb(value);
                _ellipse->update_shape(Geo::Ellipse::default_down_sampling_value);
                ui->ellipse_ratio->setValue(value / ui->ellipse_radiusA->value());
                ui->ellipse_length->setValue(_ellipse->length());
                ui->ellipse_area->setValue(_ellipse->area());
                CanvasOperations::CanvasOperation::refresh_tool_lines(_ellipse);
                _canvas->refresh_vbo(true, Geo::Type::ELLIPSE);
                _canvas->refresh_selected_ibo();
                _canvas->update();
            });
}

void PropertyWidget::init_point_widget()
{
    connect(ui->point_x, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _point->x = value;
                _canvas->refresh_vbo(true, Geo::Type::POINT);
                _canvas->update();
            });
    connect(ui->point_y, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _point->y = value;
                _canvas->refresh_vbo(true, Geo::Type::POINT);
                _canvas->update();
            });
}

void PropertyWidget::init_polygon_widget()
{
    connect(ui->polygon_point, &QSpinBox::valueChanged,
            [this](int index)
            {
                ui->polygon_pointX->blockSignals(true);
                ui->polygon_pointY->blockSignals(true);
                ui->polygon_pointX->setValue(_polygon->at(index).x);
                ui->polygon_pointY->setValue(_polygon->at(index).y);
                ui->polygon_pointX->blockSignals(false);
                ui->polygon_pointY->blockSignals(false);
            });
    connect(ui->polygon_pointX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                if (const int index = ui->polygon_point->value(); index == 0 || index == _polygon->size() - 1)
                {
                    _polygon->front().x = _polygon->back().x = value;
                }
                else
                {
                    _polygon->at(index).x = value;
                }
                ui->polygon_length->setValue(_polygon->length());
                ui->polygon_area->setValue(_polygon->area());
                _canvas->refresh_vbo(true, Geo::Type::POLYGON);
                _canvas->update();
            });
    connect(ui->polygon_pointY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                if (const int index = ui->polygon_point->value(); index == 0 || index == _polygon->size() - 1)
                {
                    _polygon->front().y = _polygon->back().y = value;
                }
                else
                {
                    _polygon->at(index).y = value;
                }
                ui->polygon_length->setValue(_polygon->length());
                ui->polygon_area->setValue(_polygon->area());
                _canvas->refresh_vbo(true, Geo::Type::POLYGON);
                _canvas->update();
            });
}

void PropertyWidget::init_polyline_widget()
{
    connect(ui->polyline_point, &QSpinBox::valueChanged,
            [this](int index)
            {
                ui->polyline_pointX->blockSignals(true);
                ui->polyline_pointY->blockSignals(true);
                ui->polyline_pointX->setValue(_polyline->at(index).x);
                ui->polyline_pointY->setValue(_polyline->at(index).y);
                ui->polyline_pointX->blockSignals(false);
                ui->polyline_pointY->blockSignals(false);
            });
    connect(ui->polyline_pointX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _polyline->at(ui->polyline_point->value()).x = value;
                ui->polyline_length->setValue(_polyline->length());
                _canvas->refresh_vbo(true, Geo::Type::POLYLINE);
                _canvas->update();
            });
    connect(ui->polyline_pointY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _polyline->at(ui->polyline_point->value()).y = value;
                ui->polyline_length->setValue(_polyline->length());
                _canvas->refresh_vbo(true, Geo::Type::POLYLINE);
                _canvas->update();
            });
}

void PropertyWidget::init_text_widget()
{
    connect(ui->text_anchorX, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _text->translate(value - _text->anchor().x, 0);
                _canvas->update();
            });
    connect(ui->text_anchorY, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _text->translate(0, value - _text->anchor().y);
                _canvas->update();
            });
    connect(ui->text_fontSize, &QSpinBox::valueChanged,
            [this](int value)
            {
                QFont font(_text->font());
                font.setPointSize(value);
                _text->set_font(font);
                _canvas->update();
            });
    connect(ui->text_angle, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _text->rotate(_text->anchor().x, _text->anchor().y, Geo::degree_to_rad(value) - _text->angle());
                _canvas->update();
            });
}

void PropertyWidget::init_dimension_widget()
{
    connect(ui->dim_fontSize, &QSpinBox::valueChanged,
            [this](int value)
            {
                _dim->font_size = value;
                _canvas->update();
            });
    connect(ui->dim_arrowSize, &QDoubleSpinBox::valueChanged,
            [this](double value)
            {
                _dim->arrow_size = value;
                _canvas->refresh_vbo(true, Geo::Type::DIMENSION);
                _canvas->update();
            });
}

void PropertyWidget::show(Geo::Geometry *object)
{
    read(object);

#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
    QDialog::exec();

    check(object);
}

void PropertyWidget::read(Geo::Geometry *object)
{
    _shape.clear();
    CanvasOperations::CanvasOperation::refresh_tool_lines(object);
    switch (object->type())
    {
    case Geo::Type::ARC:
        read(static_cast<Geo::Arc *>(object));
        break;
    case Geo::Type::BEZIER:
        read(static_cast<Geo::CubicBezier *>(object));
        break;
    case Geo::Type::BSPLINE:
        _path_points.clear();
        _knots.clear();
        read(static_cast<Geo::BSpline *>(object));
        break;
    case Geo::Type::CIRCLE:
        read(static_cast<Geo::Circle *>(object));
        break;
    case Geo::Type::COMBINATION:
        read(static_cast<Combination *>(object));
        break;
    case Geo::Type::ELLIPSE:
        read(static_cast<Geo::Ellipse *>(object));
        break;
    case Geo::Type::POINT:
        read(static_cast<Geo::Point *>(object));
        break;
    case Geo::Type::POLYGON:
        read(static_cast<Geo::Polygon *>(object));
        break;
    case Geo::Type::POLYLINE:
        read(static_cast<Geo::Polyline *>(object));
        break;
    case Geo::Type::TEXT:
        read(static_cast<Text *>(object));
        break;
    case Geo::Type::DIMENSION:
        read(static_cast<Dim::Dimension *>(object));
        break;
    default:
        break;
    }
    static_cast<QScrollArea *>(ui->stackedWidget->currentWidget()->layout()->itemAt(0)->widget())->widget()->adjustSize();
}

void PropertyWidget::read(Geo::Arc *arc)
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

    for (const Geo::Point &point : arc->control_points)
    {
        _shape.emplace_back(point.x, point.y);
    }
}

void PropertyWidget::read(Geo::CubicBezier *bezier)
{
    _bezier = bezier;
    ui->stackedWidget->setCurrentIndex(1);
    ui->object_type_lb->setText("Cubic Bezier");
    ui->bezier_pointCount->setValue(bezier->size());
    ui->bezier_point->setValue(0);
    ui->bezier_point->setRange(0, bezier->size() - 1);
    ui->bezier_pointX->blockSignals(true);
    ui->bezier_pointY->blockSignals(true);
    ui->bezier_pointX->setValue(bezier->at(0).x);
    ui->bezier_pointY->setValue(bezier->at(0).y);
    ui->bezier_pointX->blockSignals(false);
    ui->bezier_pointY->blockSignals(false);
    ui->bezier_length->setValue(bezier->length());

    for (const Geo::Point &point : *bezier)
    {
        _shape.emplace_back(point.x, point.y);
    }
}

void PropertyWidget::read(Geo::BSpline *bspline)
{
    _bspline = bspline;
    ui->stackedWidget->setCurrentIndex(2);
    ui->object_type_lb->setText(dynamic_cast<Geo::CubicBSpline *>(bspline) == nullptr ? "Quad BSpline" : "Cubic BSpline");
    ui->bspline_pointTypeCbx->blockSignals(true);
    ui->bspline_pointTypeCbx->setCurrentIndex(bspline->controls_model ? 1 : 0);
    ui->bspline_pointTypeCbx->blockSignals(false);
    ui->bspline_pointX->blockSignals(true);
    ui->bspline_pointY->blockSignals(true);
    if (bspline->controls_model)
    {
        ui->bspline_pointCount->setValue(bspline->control_points.size());
        ui->bspline_point->setValue(0);
        ui->bspline_point->setRange(0, bspline->control_points.size() - 1);
        ui->bspline_pointX->setValue(bspline->control_points[0].x);
        ui->bspline_pointY->setValue(bspline->control_points[0].y);
    }
    else
    {
        ui->bspline_pointCount->setValue(bspline->path_points.size());
        ui->bspline_point->setValue(0);
        ui->bspline_point->setRange(0, bspline->path_points.size());
        ui->bspline_pointX->setValue(bspline->path_points[0].x);
        ui->bspline_pointY->setValue(bspline->path_points[0].y);
    }
    ui->bspline_pointX->blockSignals(false);
    ui->bspline_pointY->blockSignals(false);
    ui->bspline_length->setValue(bspline->length());

    for (const Geo::Point &point : bspline->control_points)
    {
        _shape.emplace_back(point.x, point.y);
    }
    for (const Geo::Point &point : bspline->path_points)
    {
        _path_points.emplace_back(point.x, point.y);
    }
    _knots.assign(bspline->knots().cbegin(), bspline->knots().cend());
}

void PropertyWidget::read(Geo::Circle *circle)
{
    _circle = circle;
    ui->stackedWidget->setCurrentIndex(3);
    ui->object_type_lb->setText("Circle");
    ui->circle_centerX->blockSignals(true);
    ui->circle_centerY->blockSignals(true);
    ui->circle_radius->blockSignals(true);
    ui->circle_centerX->setValue(circle->x);
    ui->circle_centerY->setValue(circle->y);
    ui->circle_radius->setValue(circle->radius);
    ui->circle_centerX->blockSignals(false);
    ui->circle_centerY->blockSignals(false);
    ui->circle_radius->blockSignals(false);
    ui->circle_length->setValue(circle->length());
    ui->circle_area->setValue(circle->area());

    _shape.emplace_back(circle->x, circle->y);
    _shape.emplace_back(circle->radius, 0);
}

void PropertyWidget::read(Combination *combination)
{
    _combination = combination;
    ui->stackedWidget->setCurrentIndex(4);
    ui->object_type_lb->setText("Combination");
    ui->combination_itemCount->setValue(combination->size());
    const Geo::AABBRectParams rect = combination->aabbrect_params();
    ui->combination_centerX->blockSignals(true);
    ui->combination_centerY->blockSignals(true);
    ui->combination_centerX->setValue((rect.left + rect.right) / 2);
    ui->combination_centerY->setValue((rect.top + rect.bottom) / 2);
    ui->combination_centerX->blockSignals(false);
    ui->combination_centerY->blockSignals(false);
    ui->combination_width->setValue(rect.right - rect.left);
    ui->combination_height->setValue(rect.top - rect.bottom);

    _shape.emplace_back((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
}

void PropertyWidget::read(Geo::Ellipse *ellipse)
{
    _ellipse = ellipse;
    ui->stackedWidget->setCurrentIndex(5);
    ui->object_type_lb->setText("Ellipse");
    ui->ellipse_startX->setValue(ellipse->arc_point0().x);
    ui->ellipse_startY->setValue(ellipse->arc_point0().y);
    ui->ellipse_centerX->blockSignals(true);
    ui->ellipse_centerY->blockSignals(true);
    ui->ellipse_centerX->setValue(ellipse->center().x);
    ui->ellipse_centerY->setValue(ellipse->center().y);
    ui->ellipse_centerX->blockSignals(false);
    ui->ellipse_centerY->blockSignals(false);
    ui->ellipse_endX->setValue(ellipse->arc_point1().x);
    ui->ellipse_endY->setValue(ellipse->arc_point1().y);
    ui->ellipse_startAngle->blockSignals(true);
    ui->ellipse_endAngle->blockSignals(true);
    ui->ellipse_startAngle->setValue(Geo::rad_to_degree(ellipse->arc_angle0()));
    ui->ellipse_endAngle->setValue(Geo::rad_to_degree(ellipse->arc_angle1()));
    ui->ellipse_startAngle->blockSignals(false);
    ui->ellipse_endAngle->blockSignals(false);
    ui->ellipse_angle->blockSignals(true);
    ui->ellipse_angle->setValue(Geo::rad_to_degree(ellipse->angle()));
    ui->ellipse_angle->blockSignals(false);
    ui->ellipse_radiusA->blockSignals(true);
    ui->ellipse_radiusB->blockSignals(true);
    {
        const double a = ellipse->lengtha(), b = ellipse->lengthb();
        ui->ellipse_radiusA->setValue(a);
        ui->ellipse_radiusB->setValue(b);
        ui->ellipse_ratio->setValue(b / a);
    }
    ui->ellipse_radiusA->blockSignals(false);
    ui->ellipse_radiusB->blockSignals(false);
    ui->ellipse_length->setValue(ellipse->length());
    ui->ellipse_area->setValue(ellipse->area());

    _shape.emplace_back(ellipse->a0().x, ellipse->a0().y);
    _shape.emplace_back(ellipse->a1().x, ellipse->a1().y);
    _shape.emplace_back(ellipse->b0().x, ellipse->b0().y);
    _shape.emplace_back(ellipse->b1().x, ellipse->b1().y);
    _shape.emplace_back(ellipse->arc_angle0(), ellipse->arc_angle1());
}

void PropertyWidget::read(Geo::Point *point)
{
    _point = point;
    ui->stackedWidget->setCurrentIndex(6);
    ui->object_type_lb->setText("Point");
    ui->point_x->blockSignals(true);
    ui->point_y->blockSignals(true);
    ui->point_x->setValue(point->x);
    ui->point_y->setValue(point->y);
    ui->point_x->blockSignals(false);
    ui->point_y->blockSignals(false);
    ui->point_length->setValue(point->length());

    _shape.emplace_back(point->x, point->y);
}

void PropertyWidget::read(Geo::Polygon *polygon)
{
    _polygon = polygon;
    ui->stackedWidget->setCurrentIndex(7);
    ui->object_type_lb->setText("Polygon");
    ui->polygon_pointCount->setValue(polygon->size());
    ui->polygon_point->setValue(0);
    ui->polygon_point->setRange(0, polygon->size() - 1);
    ui->polygon_pointX->blockSignals(true);
    ui->polygon_pointY->blockSignals(true);
    ui->polygon_pointX->setValue(polygon->at(0).x);
    ui->polygon_pointY->setValue(polygon->at(0).y);
    ui->polygon_pointX->blockSignals(false);
    ui->polygon_pointY->blockSignals(false);
    ui->polygon_clockwise->setText(polygon->is_cw() ? "true" : "false");
    ui->polygon_length->setValue(polygon->length());
    ui->polygon_area->setValue(polygon->area());

    for (const Geo::Point &point : *polygon)
    {
        _shape.emplace_back(point.x, point.y);
    }
}

void PropertyWidget::read(Geo::Polyline *polyline)
{
    _polyline = polyline;
    ui->stackedWidget->setCurrentIndex(8);
    ui->object_type_lb->setText("Polyline");
    ui->polyline_pointCount->setValue(polyline->size());
    ui->polyline_point->setValue(0);
    ui->polyline_point->setRange(0, polyline->size() - 1);
    ui->polyline_pointX->blockSignals(true);
    ui->polyline_pointY->blockSignals(true);
    ui->polyline_pointX->setValue(polyline->at(0).x);
    ui->polyline_pointY->setValue(polyline->at(0).y);
    ui->polyline_pointX->blockSignals(false);
    ui->polyline_pointY->blockSignals(false);
    ui->polyline_length->setValue(polyline->length());

    for (const Geo::Point &point : *polyline)
    {
        _shape.emplace_back(point.x, point.y);
    }
}

void PropertyWidget::read(Text *text)
{
    _text = text;
    ui->stackedWidget->setCurrentIndex(9);
    ui->object_type_lb->setText("Text");
    ui->text_anchorX->blockSignals(true);
    ui->text_anchorY->blockSignals(true);
    ui->text_anchorX->setValue(text->anchor().x);
    ui->text_anchorY->setValue(text->anchor().y);
    ui->text_anchorX->blockSignals(false);
    ui->text_anchorY->blockSignals(false);
    ui->text_width->setValue(text->width());
    ui->text_height->setValue(text->height());
    ui->text_font->setText(text->font().family());
    ui->text_fontSize->setValue(text->font().pointSize());
    ui->text_angle->setValue(Geo::rad_to_degree(text->angle()));

    _font = text->font();
    _txt = text->text();
}

void PropertyWidget::read(Dim::Dimension *dim)
{
    _dim = dim;
    ui->stackedWidget->setCurrentIndex(10);
    switch (dim->dim_type())
    {
    case Dim::Type::ALIGNED:
        ui->object_type_lb->setText("Aligned Dimension");
        break;
    case Dim::Type::LINEAR:
        ui->object_type_lb->setText("Linear Dimension");
        break;
    case Dim::Type::RADIUS:
        ui->object_type_lb->setText("Radius Dimension");
        break;
    case Dim::Type::DIAMETER:
        ui->object_type_lb->setText("Diameter Dimension");
        break;
    case Dim::Type::ANGLE:
        ui->object_type_lb->setText("Angle Dimension");
        break;
    case Dim::Type::ARC:
        ui->object_type_lb->setText("Arc Dimension");
        break;
    case Dim::Type::ORDINATE:
        ui->object_type_lb->setText("Ordinate Dimension");
        break;
    default:
        break;
    }

    ui->dim_value->setText(dim->txt);
    ui->dim_fontSize->blockSignals(true);
    ui->dim_arrowSize->blockSignals(true);
    ui->dim_fontSize->setValue(dim->font_size);
    ui->dim_arrowSize->setValue(dim->arrow_size);
    ui->dim_fontSize->blockSignals(false);
    ui->dim_arrowSize->blockSignals(false);
}

void PropertyWidget::check(Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::ARC:
        check(static_cast<Geo::Arc *>(object));
        break;
    case Geo::Type::BEZIER:
        check(static_cast<Geo::CubicBezier *>(object));
        break;
    case Geo::Type::BSPLINE:
        check(static_cast<Geo::BSpline *>(object));
        break;
    case Geo::Type::CIRCLE:
        check(static_cast<Geo::Circle *>(object));
        break;
    case Geo::Type::COMBINATION:
        check(static_cast<Combination *>(object));
        break;
    case Geo::Type::ELLIPSE:
        check(static_cast<Geo::Ellipse *>(object));
        break;
    case Geo::Type::POINT:
        check(static_cast<Geo::Point *>(object));
        break;
    case Geo::Type::POLYGON:
        check(static_cast<Geo::Polygon *>(object));
        break;
    case Geo::Type::POLYLINE:
        check(static_cast<Geo::Polyline *>(object));
        break;
    case Geo::Type::TEXT:
        check(static_cast<Text *>(object));
        break;
    default:
        break;
    }
}

void PropertyWidget::check(Geo::Arc *arc)
{
    std::vector<std::tuple<double, double>> data;
    for (const Geo::Point &point : arc->control_points)
    {
        data.emplace_back(point.x, point.y);
    }
    if (_shape == data)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(arc, _shape);
    cmd->updated.push_back(arc);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::CubicBezier *bezier)
{
    std::vector<std::tuple<double, double>> data;
    for (const Geo::Point &point : *bezier)
    {
        data.emplace_back(point.x, point.y);
    }
    if (_shape == data)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(bezier, _shape);
    cmd->updated.push_back(bezier);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::BSpline *bspline)
{
    std::vector<std::tuple<double, double>> data0, data1;
    for (const Geo::Point &point : bspline->path_points)
    {
        data0.emplace_back(point.x, point.y);
    }
    for (const Geo::Point &point : bspline->control_points)
    {
        data1.emplace_back(point.x, point.y);
    }
    if (_shape == data1 && _path_points == data0 && _knots == bspline->knots())
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(bspline, _shape, _path_points, _knots);
    cmd->updated.push_back(bspline);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::Circle *circle)
{
    if (std::get<0>(_shape.front()) == circle->x && std::get<1>(_shape.front()) == circle->y &&
        std::get<0>(_shape.back()) == circle->radius)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(circle, _shape);
    cmd->updated.push_back(circle);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Combination *combination)
{
    const Geo::AABBRectParams rect = combination->aabbrect_params();
    const double x = (rect.left + rect.right) / 2, y = (rect.top + rect.bottom) / 2;
    if (std::get<0>(_shape.front()) == x && std::get<1>(_shape.front()) == y)
    {
        return;
    }

    UndoStack::TranslateCommand *cmd =
        new UndoStack::TranslateCommand(combination, x - std::get<0>(_shape.front()), y - std::get<1>(_shape.front()));
    cmd->updated.push_back(combination);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::Ellipse *ellipse)
{
    std::vector<std::tuple<double, double>> data;
    data.emplace_back(ellipse->a0().x, ellipse->a0().y);
    data.emplace_back(ellipse->a1().x, ellipse->a1().y);
    data.emplace_back(ellipse->b0().x, ellipse->b0().y);
    data.emplace_back(ellipse->b1().x, ellipse->b1().y);
    data.emplace_back(ellipse->arc_angle0(), ellipse->arc_angle1());
    if (_shape == data)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(ellipse, _shape);
    cmd->updated.push_back(ellipse);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::Point *point)
{
    if (std::get<0>(_shape.front()) == point->x && std::get<1>(_shape.front()) == point->y)
    {
        return;
    }

    UndoStack::TranslateCommand *cmd =
        new UndoStack::TranslateCommand(point, point->x - std::get<0>(_shape.front()), point->y - std::get<1>(_shape.front()));
    cmd->updated.push_back(point);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::Polygon *polygon)
{
    std::vector<std::tuple<double, double>> data;
    for (const Geo::Point &point : *polygon)
    {
        data.emplace_back(point.x, point.y);
    }
    if (_shape == data)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(polygon, _shape);
    cmd->updated.push_back(polygon);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Geo::Polyline *polyline)
{
    std::vector<std::tuple<double, double>> data;
    for (const Geo::Point &point : *polyline)
    {
        data.emplace_back(point.x, point.y);
    }
    if (_shape == data)
    {
        return;
    }

    UndoStack::ChangeShapeCommand *cmd = new UndoStack::ChangeShapeCommand(polyline, _shape);
    cmd->updated.push_back(polyline);
    _canvas->editor().push_backup_command(cmd);
}

void PropertyWidget::check(Text *text)
{
    if (text->font() != _font || text->text() != _txt)
    {
        UndoStack::TextChangedCommand *cmd = new UndoStack::TextChangedCommand(text, _txt, _font);
        cmd->updated.push_back(text);
        _canvas->editor().push_backup_command(cmd);
    }
}

void PropertyWidget::move_bezier_point(int index, double x0, double y0)
{
    const double x1 = _bezier->at(index).x;
    const double y1 = _bezier->at(index).y;
    if (const int order = 3; index > 2 && index % order == 1)
    {
        (*_bezier)[index - 2] = (*_bezier)[index - 1] + ((*_bezier)[index - 1] - (*_bezier)[index]).normalize() *
                                                            Geo::distance((*_bezier)[index - 2], (*_bezier)[index - 1]);
    }
    else if (index + 2 < _bezier->size() && index % order == order - 1)
    {
        (*_bezier)[index + 2] = (*_bezier)[index + 1] + ((*_bezier)[index + 1] - (*_bezier)[index]).normalize() *
                                                            Geo::distance((*_bezier)[index + 1], (*_bezier)[index + 2]);
    }
    else if (index % order == 0 && index > 0 && index < _bezier->size() - 1)
    {
        (*_bezier)[index - 1].translate(x1 - x0, y1 - y0);
        (*_bezier)[index + 1].translate(x1 - x0, y1 - y0);
    }
    _bezier->update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);

    ui->bezier_length->setValue(_bezier->length());
    CanvasOperations::CanvasOperation::refresh_tool_lines(_bezier);
    _canvas->refresh_vbo(true, Geo::Type::BEZIER);
    _canvas->refresh_selected_ibo();
    _canvas->update();
}

void PropertyWidget::move_bspline_point(int index, double x0, double y0)
{
    if (_bspline->controls_model)
    {
        _bspline->control_points[index].x = x0;
        _bspline->control_points[index].y = y0;
    }
    else
    {
        _bspline->path_points[index].x = x0;
        _bspline->path_points[index].y = y0;
        _bspline->update_control_points();
    }
    _bspline->update_shape(Geo::BSpline::default_step, Geo::BSpline::default_down_sampling_value);
    ui->bspline_length->setValue(_bspline->length());
    CanvasOperations::CanvasOperation::refresh_tool_lines(_bspline);
    _canvas->refresh_vbo(true, Geo::Type::BSPLINE);
    _canvas->refresh_selected_ibo();
    _canvas->update();
}