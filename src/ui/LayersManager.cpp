#include "ui/LayersManager.hpp"
#include "./ui_LayersManager.h"


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
    delete _up;
    delete _down;
    delete _add;
    delete _insert;
    delete _del;
}


void LayersManager::init()
{
    _menu = new QMenu(this);
    _show = new QAction("ON/OFF");
    _up = new QAction("Up");
    _down = new QAction("Down");
    _add = new QAction("Add");
    _insert = new QAction("Insert");
    _del = new QAction("Remove");
    _menu->addAction(_show);
    _menu->addAction(_up);
    _menu->addAction(_down);
    _menu->addAction(_add);
    _menu->addAction(_insert);
    _menu->addAction(_del);
    QObject::connect(_show, &QAction::triggered, this, &LayersManager::show_layer);
    QObject::connect(_up, &QAction::triggered, this, &LayersManager::layer_up);
    QObject::connect(_down, &QAction::triggered, this, &LayersManager::layer_down);
    QObject::connect(_add, &QAction::triggered, this, &LayersManager::add_layer);
    QObject::connect(_insert, &QAction::triggered, this, &LayersManager::insert_layer);
    QObject::connect(_del, &QAction::triggered, this, &LayersManager::remove_layer);
    
    _layers_model = new QStringListModel(this);
    ui->layers_view->setModel(_layers_model);
    QObject::connect(ui->layers_view, &QListView::customContextMenuRequested, this, [this](const QPoint &pos){_menu->exec(QCursor::pos());});
    QObject::connect(_layers_model, &QStringListModel::dataChanged, this, &LayersManager::change_layer_name);
}


void LayersManager::load_layers(Graph *graph)
{
    _layers.clear();
    int index = 0;
    for (ContainerGroup &group : graph->container_groups())
    {
        if (group.name.isEmpty())
        {
            group.name = QString::number(index++);
        }
        _layers.append((group.visible() ? QString("O ") : QString("X ")) + group.name);
    }
    std::reverse(_layers.begin(), _layers.end());
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
    const int count = _layers.length();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    if (_graph->container_group(index).visible())
    {
        _graph->container_group(index).hide();
    }
    else
    {
        _graph->container_group(index).show();
    }
    _layers[count - 1 - index] = (_graph->container_group(index).visible() ? QString("O ") : QString("X ")) + 
        _graph->container_group(index).name;
    _layers_model->setStringList(_layers);
}

void LayersManager::layer_up()
{
    const int index = _layers.length() - 1 - ui->layers_view->currentIndex().row();
    if (index == _layers.length() - 1)
    {
        return;
    }

    const int count = _layers.length();
    if (index == count - 2)
    {
        _graph->append_group();
        _graph->container_group(index).transfer(_graph->back());
        if (!_graph->container_group(index).visible())
        {
            _graph->back().hide();
        }
        _graph->remove_group(index);
        
        _layers.push_front(_layers[count - 1 - index]);
    }
    else
    {
        _graph->insert_group(index + 2);
        _graph->container_group(index).transfer(_graph->container_group(index + 2));
        if (!_graph->container_group(index).visible())
        {
            _graph->container_group(index + 2).hide();
        }
        _graph->remove_group(index);

        _layers.insert(count - 2 - index, _layers[count - 1 - index]);
    }
    _layers.remove(count - index);
    _layers_model->setStringList(_layers);
    _graph->modified = true;
}

void LayersManager::layer_down()
{
    const int index = _layers.length() - 1 - ui->layers_view->currentIndex().row();
    if (index == 0)
    {
        return;
    }

    const int count = _layers.length();
    _graph->insert_group(index - 1);
    _graph->container_group(index + 1).transfer(_graph->container_group(index - 1));
    if (!_graph->container_group(index + 1).visible())
    {
        _graph->container_group(index - 1).hide();
    }
    _graph->remove_group(index + 1);

    if (index > 1)
    {
        _layers.insert(count - index + 1, _layers[count - 1 - index]);
    }
    else
    {
        _layers.append(_layers[count - 1 - index]);
    }
    _layers.remove(count - 1 - index);
    _layers_model->setStringList(_layers);
    _graph->modified = true;
}

void LayersManager::add_layer()
{
    _graph->append_group();
    _graph->container_groups().back().name = QString::number(_layers.size());
    _layers.push_front(QString("O ") + QString::number(_layers.size()));
    _layers_model->setStringList(_layers);
    _graph->modified = true;
}

void LayersManager::insert_layer()
{
    const int count = _layers.length();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _graph->insert_group(index);
    _graph->container_group(index).name = QString::number(_layers.size());
    _layers.insert(count - 1 - index, QString("O ") + QString::number(_layers.size()));
    _layers_model->setStringList(_layers);
    _graph->modified = true;
}

void LayersManager::remove_layer()
{
    const int count = _layers.length();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _graph->remove_group(index);
    _layers.remove(count - 1 - index);
    if (_layers.empty())
    {
        _layers.append("O 0");
        _graph->append_group();
        _graph->back().name = '0';
    }
    _layers_model->setStringList(_layers);
    _graph->modified = true;  
}

void LayersManager::change_layer_name(const QModelIndex &row, const QModelIndex &col, const QList<int> &roles)
{
    QString new_name(_layers_model->index(row.row()).data().toString());
    if (new_name[1] == ' ' && (new_name.front() == 'O' || new_name.front() == 'X'))
    {
        new_name = QString::fromStdString(new_name.toStdString().substr(2));
    }

    const int count = _layers.length();
    _layers[row.row()] = (_graph->container_group(count - 1 - row.row()).visible() ? QString("O ") : QString("X ")) + new_name;
    _graph->container_group(count - 1 - row.row()).name = new_name;
    _layers_model->setStringList(_layers);
    _graph->modified = true;
}




QStringListModel *LayersManager::model()
{
    return _layers_model;
}