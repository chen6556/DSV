#include <set>
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

    ui->layers_view->setModel(&_layers_model);
    QObject::connect(ui->layers_view, &QListView::customContextMenuRequested, [this](const QPoint &pos) { _menu->exec(QCursor::pos()); });
    QObject::connect(&_layers_model, &QStandardItemModel::dataChanged, this, &LayersManager::change_layer_name);
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
    const int row = ui->layers_view->currentIndex().row();
    if (const int index = _editer->groups_count() - 1 - row; _editer->group_is_visible(index))
    {
        _editer->hide_group(index);
        _layers_model.item(row)->setCheckState(Qt::CheckState::Unchecked);
    }
    else
    {
        _editer->show_group(index);
        _layers_model.item(row)->setCheckState(Qt::CheckState::Checked);
    }
}

void LayersManager::layer_up()
{
    if (const int row = ui->layers_view->currentIndex().row(); row > 0)
    {
        const int index = _editer->groups_count() - 1 - row;
        _editer->reorder_group(index, index + 1);
        QList<QStandardItem *> items = _layers_model.takeRow(row);
        _layers_model.insertRow(row - 1, items);
    }
}

void LayersManager::layer_down()
{
    if (const int row = ui->layers_view->currentIndex().row(); row < _layers_model.rowCount() - 1)
    {
        const int index = _editer->groups_count() - 1 - row;
        _editer->reorder_group(index, index - 1);
        QList<QStandardItem *> items = _layers_model.takeRow(row);
        _layers_model.insertRow(row + 1, items);
    }
}

void LayersManager::add_layer()
{
    std::set<QString> names;
    for (int i = 0, count = _editer->groups_count(); i < count; ++i)
    {
        if (QString name = _editer->group_name(i); !name.isEmpty())
        {
            names.insert(name);
        }
    }
    _editer->append_group();
    int index = 0;
    while (names.find(QString::number(index)) != names.cend())
    {
        ++index;
    }
    _editer->set_group_name(_editer->groups_count() - 1, QString::number(index));

    QStandardItem *item = new QStandardItem(QString::number(index));
    item->setCheckState(Qt::CheckState::Checked);
    _layers_model.insertRow(0, item);
}

void LayersManager::insert_layer()
{
    std::set<QString> names;
    for (int i = 0, count = _editer->groups_count(); i < count; ++i)
    {
        if (QString name = _editer->group_name(i); !name.isEmpty())
        {
            names.insert(name);
        }
    }
    const int count = _editer->groups_count();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _editer->append_group(index);
    int j = 0;
    while (names.find(QString::number(j)) != names.cend())
    {
        ++j;
    }
    _editer->set_group_name(index, QString::number(j));

    QStandardItem *item = new QStandardItem(QString::number(j));
    item->setCheckState(Qt::CheckState::Checked);
    _layers_model.insertRow(count - index, item);
}

void LayersManager::remove_layer()
{
    const int count = _editer->groups_count();
    const int index = count - 1 - ui->layers_view->currentIndex().row();
    _layers_model.removeRow(count - 1 - index);
    _editer->remove_group(index);
    if (_editer->groups_count() == 0)
    {
        _editer->append_group();
        _editer->set_group_name(0, "0");
        QStandardItem *item = new QStandardItem("0");
        item->setCheckState(Qt::CheckState::Checked);
        _layers_model.appendRow(item);
    }
}

void LayersManager::change_layer_name(const QModelIndex &row, const QModelIndex &col, const QList<int> &roles)
{
    if (const QString name(_layers_model.index(row.row(), 0).data().toString()); name.isEmpty())
    {
        _layers_model.setData(_layers_model.index(row.row(), 0), _editer->group_name(_editer->groups_count() - 1 - row.row()));
    }
    else
    {
        std::set<QString> names;
        const int index = _editer->groups_count() - 1 - row.row();
        for (int i = 0, count = _editer->groups_count(); i < count; ++i)
        {
            if (QString str = _editer->group_name(i); index != i && !str.isEmpty())
            {
                names.insert(str);
            }
        }
        if (names.find(name) == names.cend())
        {
            _editer->set_group_name(index, name);
            _layers_model.setData(_layers_model.index(row.row(), 0), name);
        }
        else
        {
            _layers_model.setData(_layers_model.index(row.row(), 0), _editer->group_name(index));
        }
    }
}

void LayersManager::update_layers()
{
    std::set<QString> names;
    for (int i = 0, count = _editer->groups_count(); i < count; ++i)
    {
        if (QString name = _editer->group_name(i); !name.isEmpty())
        {
            names.insert(name);
        }
    }
    _layers_model.clear();
    if (_editer->groups_count() == 0)
    {
        _editer->append_group(0);
    }
    for (int i = _editer->groups_count() - 1, index = 0; i >= 0; --i)
    {
        if (_editer->group_name(i).isEmpty())
        {
            while (names.find(QString::number(index)) != names.cend())
            {
                ++index;
            }
            _editer->set_group_name(i, QString::number(index++));
        }
        QStandardItem *item = new QStandardItem(_editer->group_name(i));
        item->setCheckState(_editer->group_is_visible(i) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        _layers_model.appendRow(item);
    }
}

QStandardItemModel *LayersManager::model()
{
    return &_layers_model;
}

int LayersManager::exec()
{
#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
    return QDialog::exec();
}