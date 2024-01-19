#pragma once

#include <QDialog>
#include "draw/Graph.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class DataPanel;}
QT_END_NAMESPACE

class DataPanel : public QDialog
{
    Q_OBJECT

private:
    Ui::DataPanel *ui = nullptr;

public:
    DataPanel(QWidget *parent);
    ~DataPanel();

    void load_draw_data(const Graph *graph, const size_t point_count);
};