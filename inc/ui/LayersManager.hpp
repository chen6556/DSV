#pragma once

#include <QDialog>
#include <QMenu>
#include <QAction>
#include <QStringListModel>
#include <QStringList>

#include "base/Editer.hpp"


QT_BEGIN_NAMESPACE
namespace Ui { class LayersManager;}
QT_END_NAMESPACE

class LayersManager : public QDialog
{
    Q_OBJECT

private:
    Ui::LayersManager *ui = nullptr;
    QMenu *_menu = nullptr;
    QAction *_show = nullptr;
    QAction *_up = nullptr;
    QAction *_down = nullptr;
    QAction *_add = nullptr;
    QAction *_insert = nullptr;
    QAction *_del = nullptr;

    Editer *_editer = nullptr;
    QStringList _layers;
    QStringListModel *_layers_model = nullptr;
    bool _append_to_last = true;

private:
    void init();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void show_layer();

    void layer_up();

    void layer_down();

    void add_layer();

    void insert_layer();

    void remove_layer();

    void change_layer_name(const QModelIndex &row, const QModelIndex &col, const QList<int> &roles);

public:
    LayersManager(QWidget *parent);
    ~LayersManager();

    void bind_editer(Editer *editer);

    void update_layers();

    QStringListModel *model();
};

