#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QToolButton>
#include "ui/LayersManager.hpp"
#include "draw/Canvas.hpp"
#include "base/Editer.hpp"
#include "ui/Setting.hpp"
#include <QTimer>
#include <QString>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui = nullptr;
    Canvas _painter;
    Editer _editer;
    Setting *_setting = nullptr;
    std::vector<Qt::Key> _keys;
    QTimer _clock;
    QLabel *_info_labels[3] = {nullptr, nullptr, nullptr};

    QComboBox *_layers_cbx = nullptr;
    LayersManager *_layers_manager = nullptr;
    QToolButton *_layers_btn = nullptr;
    QString _file_type = "All Files: (*.*)";

private:
    void init();

protected:
    void keyPressEvent(QKeyEvent *event);

    void closeEvent(QCloseEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);

    void dropEvent(QDropEvent *event);

private slots:
    void open_file();

    void save_file();

    void auto_save();

    void saveas_file();

    void refresh_tool_label(const int &value);

    void load_settings();

    void save_settings();

    void show_layers_manager();


    void rotate();

    void flip_x();

    void flip_y();

private:
    void open_file(const QString &path);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};