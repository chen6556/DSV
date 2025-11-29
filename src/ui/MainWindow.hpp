#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QToolButton>
#include <QTimer>
#include <QString>

#include "base/Editer.hpp"
#include "ui/LayersManager.hpp"
#include "ui/Setting.hpp"
#include "ui/DataPanel.hpp"
#include "ui/ActionGroup.hpp"


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
    DataPanel *_panel = nullptr;

    QComboBox *_layers_cbx = nullptr;
    LayersManager *_layers_manager = nullptr;
    QToolButton *_layers_btn = nullptr;
    ActionGroup *_actiongroup = nullptr;
    QString _file_type = "All Files: (*.*)";

private:
    void init();

    void connect_btn_to_cmd();

protected:
    void mousePressEvent(QMouseEvent *event);

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

    void refresh_cmd(const CMDWidget::CMD cmd);

    void refresh_settings();

    void load_settings();

    void save_settings();

    void show_layers_manager();

    void hide_layers_manager();

    void to_main_page();

    void to_array_page();

    void show_data_panel();

private:
    void open_file(const QString &path);

    void append_file(const QString &path);

    void actiongroup_callback(const ActionGroup::MenuType menu, const int index);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};