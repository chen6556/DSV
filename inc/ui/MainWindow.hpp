#pragma once
#include <QQuickPaintedItem>
// #include "ui/LayersManager.hpp"
#include "draw/Canvas.hpp"
#include "base/Editer.hpp"
#include "simulation/StructureBuilder.hpp"
#include "simulation/StructureMover.hpp"
// #include "ui/Setting.hpp"
#include <QTimer>
#include <QString>


class MainWindow : public QQuickPaintedItem
{
    Q_OBJECT

private:
    Canvas _painter;
    Editer _editer;
    // Setting *_setting = nullptr;
    StructureBuilder _builder;
    std::vector<Qt::Key> _keys;
    QTimer _clock;

    // LayersManager *_layers_manager = nullptr;
    // QToolButton *_layers_btn = nullptr;
    QString _file_type = "All Files: (*.*)";

private:
    void init();

    // void test();

protected:
    void keyPressEvent(QKeyEvent *event);

    void closeEvent(QCloseEvent *event);

public:
    Q_INVOKABLE void open_file(const QString &path);

    Q_INVOKABLE void save_file();

    Q_INVOKABLE void auto_save();

    Q_INVOKABLE void saveas_file();

    Q_INVOKABLE void refresh_tool_label(const int &value);

    Q_INVOKABLE void load_settings();

    Q_INVOKABLE void save_settings();

    Q_INVOKABLE void show_layers_manager();


    Q_INVOKABLE void rotate();

    Q_INVOKABLE void flip_x();

    Q_INVOKABLE void flip_y();

public:
    MainWindow(QQuickPaintedItem *parent = nullptr);
    ~MainWindow();

    void paint(QPainter *p) Q_DECL_OVERRIDE;
};