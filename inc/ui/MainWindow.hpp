#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QToolButton>
#include <QTimer>
#include <QString>

#include "base/Editer.hpp"
#include "ui/LayersManager.hpp"
#include "ui/Setting.hpp"
#include "ui/CMDWidget.hpp"
#include "ui/DataPanel.hpp"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui = nullptr;
    Editer _editer;
    Setting *_setting = nullptr;
    std::vector<Qt::Key> _keys;
    QTimer _clock;
    QLabel *_info_labels[3] = {nullptr, nullptr, nullptr};
    CMDWidget *_cmd_widget = nullptr;
    DataPanel *_panel = nullptr;

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

    void resizeEvent(QResizeEvent *event);

private slots:
    void open_file();

    void close_file();

    void save_file();

    void auto_save();

    void saveas_file();

    void append_file();

    void set_catch(QAction *action);

    void refresh_tool_label(const Canvas::Tool tool);

    void refresh_tool_label(const Canvas::Operation operation);

    void refresh_cmd(const CMDWidget::CMD cmd);

    void refresh_settings();

    void load_settings();

    void save_settings();

    void show_layers_manager();

    void hide_layers_manager();

    void to_main_page();


    void connect_polylines();

    void close_polyline();

    void combinate();

    void rotate();

    void flip_x();

    void flip_y();

    void mirror();

    void scale();

    void offset();


    void to_array_page();

    void line_array();

    void ring_array();


    void polygon_union();

    void polygon_intersection();

    void polygon_difference();

    void polygon_xor();


    void fillet();


    void show_data_panel();

private:
    void open_file(const QString &path);

    void append_file(const QString &path);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};