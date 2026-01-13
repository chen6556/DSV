#include "ui/LayersManager.hpp"
#include "./ui_LayersManager.h"
#include "ui/WinUITool.hpp"


LayersManager::LayersManager(QWidget *parent) : QDialog(parent), ui(new Ui::LayersManager)
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
    QObject::connect(ui->layers_view, &QListView::customContextMenuRequested, [this](const QPoint &pos) { _menu->exec(QCursor::pos()); });
    QObject::connect(_layers_model, &QStringListModel::dataChanged, this, &LayersManager::change_layer_name);
}


void LayersManager::bind_editer(Editer *editer)
{
    _editer = editer;
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
    if (_editer->group_is_visible(index))
    {
        _editer->hide_group(index);
        _layers[count - 1 - index] = "x " + _editer->group_name(index);
    }
    else
    {
        _editer->show_group(index);
        _layers[count - 1 - index] = "O " + _editer->group_name(index);
    }
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
    _editer->reorder_group(index, index + 1);
    if (index == count - 2)
    {
        _layers.push_front(_layers[count - 1 - index]);
    }
    else
    {
        _layers.insert(count - 2 - index, _layers[count - 1 - index]);
    }
    _layers.remove(count - index);
    _layers_model->setStringList(_layers);
}

void LayersManager::layer_down()
{
    const int index = _layers.length() - 1 - ui->layers_view->currentIndex().row();
    if (index == 0)
    {
        return;
    }

    const int count = _layers.length();
    _editer->reorder_group(index, index - 1);

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
}

void LayersManager::add_layer()
{
    _editer->append_group();
    _editer->set_group_name(_editer->groups_count() - 1, QString::number(_layers.size()));
    _layers.push_front("O " + QString::number(_layers.size()));
    _layers_model->setStringList(_layers);
}

void LayersManager::insert_layer()
{
    const int count = _layers.length();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _editer->append_group(index);
    _editer->set_group_name(index, QString::number(_layers.size()));
    _layers.insert(count - index, "O " + QString::number(_layers.size()));
    _layers_model->setStringList(_layers);
}

void LayersManager::remove_layer()
{
    const int count = _layers.length();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _editer->remove_group(index);
    _layers.remove(count - 1 - index);
    if (_layers.empty())
    {
        _layers.append("O 0");
        _editer->append_group();
        _editer->set_group_name(0, QString::number(0));
    }
    _layers_model->setStringList(_layers);
}

void LayersManager::change_layer_name(const QModelIndex &row, const QModelIndex &col, const QList<int> &roles)
{
    QString new_name(_layers_model->index(row.row()).data().toString());
    if (new_name[1] == ' ' && (new_name.front() == 'O' || new_name.front() == 'X'))
    {
        new_name = QString::fromStdString(new_name.toStdString().substr(2));
    }

    const int count = _layers.length();
    _layers[row.row()] = (_editer->group_is_visible(count - 1 - row.row()) ? "O " : "X ") + new_name;
    _editer->set_group_name(count - 1 - row.row(), new_name);
    _layers_model->setStringList(_layers);
}

void LayersManager::update_layers()
{
    _layers.clear();
    int index = 0;
    for (size_t i = 0, count = _editer->groups_count(); i < count; ++i)
    {
        if (_editer->group_name(i).isEmpty())
        {
            _editer->set_group_name(i, QString::number(index++));
        }
        _layers.append((_editer->group_is_visible(i) ? "O " : "X ") + _editer->group_name(i));
    }
    std::reverse(_layers.begin(), _layers.end());
    _layers_model->setStringList(_layers);
}

QStringListModel *LayersManager::model()
{
    return _layers_model;
}

int LayersManager::exec()
{
#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
    return QDialog::exec();
}