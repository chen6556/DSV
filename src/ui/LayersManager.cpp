#include "ui/LayersManager.hpp"
#include "./ui_layersmanager.h"


LayersManager::LayersManager(QWidget *parent)
    : QDialog(parent), ui(new Ui::LayersManager)
{
    ui->setupUi(this);
    init();
}

LayersManager::~LayersManager()
{
    delete ui;
    delete _menu;
    delete _show;
    delete _add;
    delete _del;
}


void LayersManager::init()
{
    _menu = new QMenu(this);
    _show = new QAction("ON/OFF");
    _add = new QAction("Add");
    _insert = new QAction("Insert");
    _del = new QAction("Remove");
    _menu->addAction(_show);
    _menu->addAction(_add);
    _menu->addAction(_insert);
    _menu->addAction(_del);
    connect(_show, &QAction::triggered, this, &LayersManager::show_layer);
    connect(_add, &QAction::triggered, this, &LayersManager::add_layer);
    connect(_insert, &QAction::triggered, this, &LayersManager::insert_layer);
    connect(_del, &QAction::triggered, this, &LayersManager::remove_layer);
    
    _layers_model = new QStringListModel(this);
    ui->layers_view->setModel(_layers_model);
    connect(ui->layers_view, &QListView::customContextMenuRequested, this, [this](const QPoint &pos){_menu->exec(QCursor::pos());});
    connect(_layers_model, &QStringListModel::dataChanged, this, &LayersManager::change_layer_name);
}


void LayersManager::load_layers(Graph *graph)
{
    _layers.clear();
    int index = 0;
    for (ContainerGroup &group : graph->container_groups())
    {
        if (!group.memo().has("layer_name"))
        {
            group.memo()["layer_name"] = std::to_string(index++);
        }
        _layers.append((group.visible() ? QString("O ") : QString("X ")) + group.memo()["layer_name"].to_string().c_str());
    }
    _layers_model->setStringList(_layers);
    _graph = graph;
}

void LayersManager::closeEvent(QCloseEvent *event)
{
    emit accepted();
    QDialog::closeEvent(event);
}


void LayersManager::show_layer()
{
    const int index = ui->layers_view->currentIndex().row();
    if (_graph->container_group(index).visible())
    {
        _graph->container_group(index).hide();
    }
    else
    {
        _graph->container_group(index).show();
    }
    _layers[index] = (_graph->container_group(index).visible() ? QString("O ") : QString("X ")) + 
        _graph->container_group(index).memo()["layer_name"].to_string().c_str();
    _layers_model->setStringList(_layers);
}

void LayersManager::add_layer()
{
    _graph->append_group();
    _graph->container_groups().back().memo()["layer_name"] = std::to_string(_layers.size());
    _layers.append(QString("O ") + std::to_string(_layers.size()).c_str());
    _layers_model->setStringList(_layers);
}

void LayersManager::insert_layer()
{
    const int index = ui->layers_view->currentIndex().row();
    _graph->insert_group(index);
    _graph->container_group(index).memo()["layer_name"] = std::to_string(_layers.size());
    _layers.insert(index, QString("O ") + std::to_string(_layers.size()).c_str());
    _layers_model->setStringList(_layers);
}

void LayersManager::remove_layer()
{
    int index = ui->layers_view->currentIndex().row();
    _graph->remove_group(index);
    _layers.remove(index);
    _layers_model->setStringList(_layers);
}

void LayersManager::change_layer_name(const QModelIndex &row, const QModelIndex &col, const QList<int> &roles)
{
    QString new_name(_layers_model->index(row.row()).data().toString());
    if (new_name[1] == ' ' && (new_name.front() == 'O' || new_name.front() == 'X'))
    {
        new_name = QString::fromStdString(new_name.toStdString().substr(2));
    }

    _layers[row.row()] = (_graph->container_group(row.row()).visible() ? QString("O ") : QString("X ")) + new_name;
    _graph->container_group(row.row()).memo()["layer_name"] = new_name.toStdString();
    _layers_model->setStringList(_layers);
}




QStringListModel *LayersManager::model()
{
    return _layers_model;
}