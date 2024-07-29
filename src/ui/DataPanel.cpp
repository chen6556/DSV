#include "ui/DataPanel.hpp"
#include "./ui_DataPanel.h"


DataPanel::DataPanel(QWidget *parent)
    : QDialog(parent), ui(new Ui::DataPanel)
{
    ui->setupUi(this);
}

DataPanel::~DataPanel()
{
    delete ui;
}


void DataPanel::load_draw_data(const Graph *graph, const size_t point_count)
{
    ui->point_label->setText(QString::number(point_count));

    size_t polyline_count = 0, bezier_count = 0,
        polygon_count = 0, circle_count = 0, text_count = 0;
    for (const ContainerGroup &group : *graph)
    {
        for (const Geo::Geometry *object : group)
        {
            switch (object->type())
            {
            case Geo::Type::TEXT:
                ++text_count;
                break;
            case Geo::Type::CONTAINER:
                ++polygon_count;
                break;
            case Geo::Type::CIRCLECONTAINER:
                ++circle_count;
                break;
            case Geo::Type::POLYLINE:
                ++polyline_count;
                break;
            case Geo::Type::BEZIER:
                ++bezier_count;
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *obj : *dynamic_cast<const Combination *>(object))
                {
                    switch (obj->type())
                    {
                    case Geo::Type::TEXT:
                        ++text_count;
                        break;
                    case Geo::Type::CONTAINER:
                        ++polygon_count;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        ++circle_count;
                        break;
                    case Geo::Type::POLYLINE:
                        ++polyline_count;
                        break;
                    case Geo::Type::BEZIER:
                        ++bezier_count;
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

    ui->polyline_label->setText(QString::number(polyline_count));
    ui->bezier_label->setText(QString::number(bezier_count));
    ui->polygon_label->setText(QString::number(polygon_count));
    ui->circle_label->setText(QString::number(circle_count));
    ui->text_label->setText(QString::number(text_count));

    const Geo::AABBRect rect(graph->bounding_rect());
    ui->width_label->setText(QString::number(rect.width()));
    ui->height_label->setText(QString::number(rect.height()));
    ui->area_label->setText(QString::number(rect.area(), 'f', 4));
}