#include "ui/DataPanel.hpp"
#include "./ui_DataPanel.h"
#include "ui/WinUITool.hpp"


DataPanel::DataPanel(QWidget *parent) : QDialog(parent), ui(new Ui::DataPanel)
{
    ui->setupUi(this);
}

DataPanel::~DataPanel()
{
    delete ui;
}


void DataPanel::load_draw_data(const Graph *graph)
{
    size_t point_count = 0, polyline_count = 0, bezier_count = 0, bspline_count = 0, polygon_count = 0, circle_count = 0, ellipse_count = 0,
           text_count = 0;
    for (const ContainerGroup &group : *graph)
    {
        for (const Geo::Geometry *object : group)
        {
            switch (object->type())
            {
            case Geo::Type::TEXT:
                ++text_count;
                break;
            case Geo::Type::POLYGON:
                ++polygon_count;
                point_count += static_cast<const Geo::Polygon *>(object)->size();
                break;
            case Geo::Type::CIRCLE:
                ++circle_count;
                point_count += static_cast<const Geo::Circle *>(object)->shape().size();
                break;
            case Geo::Type::ELLIPSE:
                ++ellipse_count;
                point_count += static_cast<const Geo::Ellipse *>(object)->shape().size();
                break;
            case Geo::Type::POLYLINE:
                ++polyline_count;
                point_count += static_cast<const Geo::Polyline *>(object)->size();
                break;
            case Geo::Type::BEZIER:
                ++bezier_count;
                point_count += static_cast<const Geo::Bezier *>(object)->shape().size();
                break;
            case Geo::Type::BSPLINE:
                ++bspline_count;
                point_count += static_cast<const Geo::BSpline *>(object)->shape().size();
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *obj : *dynamic_cast<const Combination *>(object))
                {
                    switch (obj->type())
                    {
                    case Geo::Type::TEXT:
                        ++text_count;
                        break;
                    case Geo::Type::POLYGON:
                        ++polygon_count;
                        point_count += static_cast<const Geo::Polygon *>(obj)->size();
                        break;
                    case Geo::Type::CIRCLE:
                        ++circle_count;
                        point_count += static_cast<const Geo::Circle *>(obj)->shape().size();
                        break;
                    case Geo::Type::ELLIPSE:
                        ++ellipse_count;
                        point_count += static_cast<const Geo::Ellipse *>(obj)->shape().size();
                        break;
                    case Geo::Type::POLYLINE:
                        ++polyline_count;
                        point_count += static_cast<const Geo::Polyline *>(obj)->size();
                        break;
                    case Geo::Type::BEZIER:
                        ++bezier_count;
                        point_count += static_cast<const Geo::Bezier *>(obj)->shape().size();
                        break;
                    case Geo::Type::BSPLINE:
                        ++bspline_count;
                        point_count += static_cast<const Geo::BSpline *>(obj)->shape().size();
                        break;
                    default:
                        break;
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    ui->point_label->setText(QString::number(point_count));
    ui->polyline_label->setText(QString::number(polyline_count));
    ui->polygon_label->setText(QString::number(polygon_count));
    ui->bezier_label->setText(QString::number(bezier_count));
    ui->bspline_label->setText(QString::number(bspline_count));
    ui->circle_label->setText(QString::number(circle_count));
    ui->ellipse_label->setText(QString::number(ellipse_count));
    ui->text_label->setText(QString::number(text_count));

    const Geo::AABBRect rect(graph->bounding_rect());
    ui->width_label->setText(QString::number(rect.width()));
    ui->height_label->setText(QString::number(rect.height()));
    ui->area_label->setText(QString::number(rect.area(), 'f', 4));
}

int DataPanel::exec()
{
#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
    return QDialog::exec();
}