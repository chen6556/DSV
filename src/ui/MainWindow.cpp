#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QAbstractItemView>
#include <QMimeData>

#include "ui/MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "io/File.hpp"
#include "io/PLTParser.hpp"
#include "io/RS274DParser.hpp"
#include "io/DSVParser.hpp"
#include "io/GlobalSetting.hpp"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), _setting(new Setting(this)),
    _panel(new DataPanel(this))
{
    ui->setupUi(this);
    init();
    load_settings();
}

MainWindow::~MainWindow()
{
    save_settings();
    delete ui;
    _editer.delete_graph();
    for (size_t i = 0; i < 3; ++i)
    {
        delete _info_labels[i];
    }
    delete _layers_cbx;
    delete _layers_btn;
    delete _layers_manager;
    delete _cmd_widget;
    delete _setting;
    delete _panel;

    GlobalSetting::release();
}

void MainWindow::init()
{
    GlobalSetting::get_instance()->ui = ui;

    _editer.load_graph(new Graph());
    ui->canvas->bind_editer(&_editer);

    _cmd_widget = new CMDWidget(&_editer, ui->canvas, this);
    _cmd_widget->show();
    QObject::connect(_cmd_widget, &CMDWidget::cmd_changed, this, &MainWindow::refresh_cmd);

    _clock.start(5000);
    QObject::connect(ui->measure_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::MEASURE); });
    QObject::connect(ui->circle_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::CIRCLE); });
    QObject::connect(ui->line_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::POLYLINE); });
    QObject::connect(ui->rect_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::RECT); });
    QObject::connect(ui->curve_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::CURVE); ui->canvas->set_bezier_order(ui->curve_sbx->value()); });
    QObject::connect(ui->text_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::TEXT); });
    QObject::connect(ui->split_btn, &QPushButton::clicked, [this]() { _editer.split(_editer.selected()); });
    QObject::connect(&_clock, &QTimer::timeout, this, &MainWindow::auto_save);

    QObject::connect(ui->auto_aligning, &QAction::triggered, [this]() { GlobalSetting::get_instance()->setting["auto_aligning"] = ui->auto_aligning->isChecked(); });
    QObject::connect(ui->actionadvanced, &QAction::triggered, _setting, &Setting::exec);
    QObject::connect(ui->show_origin, &QAction::triggered, [this]() { ui->show_origin->isChecked() ? ui->canvas->show_origin() : ui->canvas->hide_origin(); });
    QObject::connect(ui->show_cmd_line, &QAction::triggered, [this]() { ui->show_cmd_line->isChecked() ? _cmd_widget->show() : _cmd_widget->hide(); });

    QObject::connect(_setting, &Setting::accepted, ui->canvas, static_cast<void(Canvas::*)(void)>(&Canvas::refresh_text_vbo));
    QObject::connect(_setting, &Setting::accepted, this, &MainWindow::refresh_settings);

    for (size_t i = 0; i < 3; ++i)
    {
        _info_labels[i] = new QLabel(this);
        _info_labels[i]->setMinimumWidth(70 + 45 * i);
        ui->statusBar->addWidget(_info_labels[i]);
    }
    ui->canvas->set_info_labels(_info_labels);

    _layers_manager = new LayersManager(this);
    _layers_manager->bind_editer(&_editer);
    _layers_manager->update_layers();
    QObject::connect(_layers_manager, &LayersManager::accepted, this, &MainWindow::hide_layers_manager);

    _layers_btn = new QToolButton(this);
    _layers_btn->setText("Layers");
    _layers_btn->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    ui->statusBar->addPermanentWidget(_layers_btn);
    QObject::connect(_layers_btn, &QToolButton::clicked, this, &MainWindow::show_layers_manager);

    _layers_cbx = new QComboBox(this);
    _layers_cbx->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    _layers_cbx->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->statusBar->addPermanentWidget(_layers_cbx);
    _layers_cbx->setModel(_layers_manager->model());
    QObject::connect(_layers_cbx, &QComboBox::currentIndexChanged,
        [this](int index) { _editer.set_current_group(_editer.groups_count() - 1 - index); });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (GlobalSetting::get_instance()->graph->modified && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }
    QMainWindow::closeEvent(event);
}





void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->canvas->is_typing())
    {
        return;
    }
    switch (event->key())
    {
    case Qt::Key_Escape:
        ui->canvas->cancel_painting();
        _editer.reset_selected_mark();
        ui->canvas->refresh_selected_ibo();
        _cmd_widget->clear();
        break;
    case Qt::Key_Space:
        if (_cmd_widget->empty())
        {
            ui->canvas->use_last_tool();
        }
        break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        if (_editer.remove_selected())
        {
            ui->canvas->refresh_vbo();
            ui->canvas->update();
        }
        break;
    case Qt::Key_A:
        if (event->modifiers() == Qt::ControlModifier)
        {
            _editer.reset_selected_mark(true);
            ui->canvas->refresh_selected_ibo();
            ui->canvas->update();
        }
        break;
    case Qt::Key_S:
        if (event->modifiers() == Qt::ControlModifier)
        {
            save_file();
        }
        break;
    case Qt::Key_C:
        if (event->modifiers() == Qt::ControlModifier)
        {
            ui->canvas->copy();
        }
        break;
    case Qt::Key_X:
        if (event->modifiers() == Qt::ControlModifier)
        {
            ui->canvas->cut();
            ui->canvas->update();
        }
        break;
    case Qt::Key_V:
        if (event->modifiers() == Qt::ControlModifier)
        {
            ui->canvas->paste();
            ui->canvas->update();
        }
        break;
    case Qt::Key_Z:
        if (event->modifiers() == Qt::ControlModifier && !ui->canvas->is_painting())
        {
            const size_t layers_count = _editer.groups_count();
            _editer.undo();
            if (_editer.groups_count() != layers_count)
            {
                if (_editer.groups_count() == 0)
                {
                    _editer.append_group();
                }
                _editer.set_current_group(std::min(_editer.current_group(), _editer.groups_count() - 1));
                _layers_manager->update_layers();
                _layers_cbx->setModel(_layers_manager->model());
            }
            ui->canvas->refresh_vbo();
            ui->canvas->refresh_selected_ibo();
            ui->canvas->update();
        }
        break;
    default:
        break;
    }

    if (event->modifiers() != Qt::ControlModifier &&
        0x41 <= event->key() && event->key() <= 0x5A)
    {
        _cmd_widget->activate(event->key());
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString suffixs = "dsv DSV plt PLT cut CUT";
    QFileInfo file_info(event->mimeData()->urls().front().toLocalFile());
    if( file_info.isFile() && suffixs.contains(file_info.suffix()))
    {
        open_file(file_info.absoluteFilePath());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    _cmd_widget->move(_cmd_widget->mapToParent(QPoint((width() - _cmd_widget->width()) / 2,
        height() - 33 - _cmd_widget->height()) - _cmd_widget->pos()));
}


void MainWindow::open_file()
{
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    QString path = dialog->getOpenFileName(dialog, nullptr, _editer.path(), "All Files: (*.*);;DSV: (*.dsv *.DSV);;PLT: (*.plt *.PLT);;RS274D: (*.cut *.CUT *.nc *.NC)", &_file_type);
    open_file(path);
    delete dialog;
}

void MainWindow::close_file()
{
    if (GlobalSetting::get_instance()->graph->modified && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }

    _editer.delete_graph();
    _editer.load_graph(new Graph());
    GlobalSetting::get_instance()->graph->modified = false;
    ui->canvas->refresh_vbo();
    _info_labels[2]->clear();
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());
    ui->canvas->update();
}

void MainWindow::save_file()
{
    if (GlobalSetting::get_instance()->graph == nullptr || ui->canvas->empty())
    {
        return;
    }

    if (_info_labels[2]->text().isEmpty() || !(_info_labels[2]->text().toLower().endsWith(".dsv") 
        || _info_labels[2]->text().toLower().endsWith(".plt")))
    {
        QFileDialog *dialog = new QFileDialog();
        dialog->setModal(true);
        QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path(), "DSV: (*.dsv);;PLT: (*.plt)");
        if (!path.isEmpty())
        {
            bool flag = false;
            if (path.toLower().endsWith(".dsv"))
            {
                File::write(path, GlobalSetting::get_instance()->graph, File::DSV);
                flag = true;
            }
            else if (path.toLower().endsWith(".plt"))
            {
                File::write(path, GlobalSetting::get_instance()->graph, File::PLT);
                flag = true;
            }
            if (flag)
            {
                _editer.set_path(path);
                GlobalSetting::get_instance()->graph->modified = false;
                _info_labels[2]->setText(path);
            }
        }
        delete dialog;
    }
    else
    {
        if (_info_labels[2]->text().toLower().endsWith(".dsv"))
        {
            File::write(_info_labels[2]->text(), GlobalSetting::get_instance()->graph, File::DSV);
            GlobalSetting::get_instance()->graph->modified = false;
        }
        else if (_info_labels[2]->text().toLower().endsWith(".plt"))
        {
            File::write(_info_labels[2]->text(), GlobalSetting::get_instance()->graph, File::PLT);
            GlobalSetting::get_instance()->graph->modified = false;
        }
    }
}

void MainWindow::auto_save()
{
    if (!ui->auto_save->isChecked() || _editer.path().isEmpty() ||
        !(_editer.path().toLower().endsWith(".dsv") || _editer.path().toLower().endsWith(".plt")))
    {
        return;
    }
    if (GlobalSetting::get_instance()->graph->modified)
    {
        if (_editer.path().toLower().endsWith(".dsv"))
        {
            File::write(_editer.path(), GlobalSetting::get_instance()->graph, File::DSV);
        }
        else if (_editer.path().toLower().endsWith(".plt"))
        {
            File::write(_editer.path(), GlobalSetting::get_instance()->graph, File::PLT);
        }
        GlobalSetting::get_instance()->graph->modified = false;
    }
}

void MainWindow::saveas_file()
{
    if (GlobalSetting::get_instance()->graph == nullptr || ui->canvas->empty())
    {
        return;
    }
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path().isEmpty() ? "D:/output.dsv" : _editer.path(), "DSV: (*.dsv);;PLT: (*.plt)");
    if (!path.isEmpty())
    {
        if (path.toLower().endsWith(".dsv"))
        {
            File::write(path, GlobalSetting::get_instance()->graph, File::DSV);
        }
        else if (path.toLower().endsWith(".plt"))
        {
            File::write(path, GlobalSetting::get_instance()->graph, File::PLT);
        }
        GlobalSetting::get_instance()->graph->modified = false;
    }
    delete dialog;
}

void MainWindow::append_file()
{
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    QString path = dialog->getOpenFileName(dialog, nullptr, _editer.path(), "All Files: (*.*);;DSV: (*.dsv *.DSV);;PLT: (*.plt *.PLT);;RS274D: (*.cut *.CUT *.nc *.NC)", &_file_type);
    append_file(path);
    delete dialog;
}

void MainWindow::refresh_tool_label(const Canvas::Tool tool)
{
    switch (tool)
    {
    case Canvas::Tool::MEASURE:
        ui->current_tool->setText("Length");
        break;
    case Canvas::Tool::CIRCLE:
        ui->current_tool->setText("Circle");
        break;
    case Canvas::Tool::POLYLINE:
        ui->current_tool->setText("Polyline");
        break;
    case Canvas::Tool::RECT:
        ui->current_tool->setText("Rectangle");
        break;
    case Canvas::Tool::CURVE:
        ui->current_tool->setText("Bezier Curve");
        break;
    case Canvas::Tool::TEXT:
        ui->current_tool->setText("Text");
        break;
    default:
        ui->current_tool->clear();
        ui->array_tool->clear();
        break;
    }
}

void MainWindow::refresh_tool_label(const Canvas::Operation operation)
{
    switch (operation)
    {
    case Canvas::Operation::FILLET:
        ui->current_tool->setText("Fillet");
        break;
    default:
        ui->current_tool->clear();
        break;
    }
}

void MainWindow::refresh_cmd(const CMDWidget::CMD cmd)
{
    switch (cmd)
    {
    case CMDWidget::CMD::OPEN_CMD:
        return open_file();
    case CMDWidget::CMD::SAVE_CMD:
        return save_file();
    case CMDWidget::CMD::EXIT_CMD:
        this->close();
        break;
    case CMDWidget::CMD::MAIN_CMD:
        return ui->tool_widget->setCurrentIndex(0);
    case CMDWidget::CMD::MIRROR_CMD:
        ui->current_tool->setText("Mirror");
        return ui->canvas->set_operation(Canvas::Operation::MIRROR);
    case CMDWidget::CMD::ARRAY_CMD:
        return ui->tool_widget->setCurrentIndex(1);
    case CMDWidget::CMD::RINGARRAY_CMD:
        ui->array_tool->setText("Ring Array");
        break;
    case CMDWidget::CMD::DIFFERENCE_CMD:
        ui->current_tool->setText("Difference");
        break;
    default:
        break;
    }
}

void MainWindow::refresh_settings()
{
    _editer.set_backup_count(GlobalSetting::get_instance()->setting["backup_times"].toInt());
}

void MainWindow::load_settings()
{
    GlobalSetting::get_instance()->load_setting();
    const QJsonObject &setting = GlobalSetting::get_instance()->setting;

    _editer.set_path(setting["file_path"].toString());
    _editer.set_backup_count(setting["backup_times"].toInt());
    ui->auto_save->setChecked(setting["auto_save"].toBool());
    ui->auto_layering->setChecked(setting["auto_layering"].toBool());
    ui->auto_connect->setChecked(setting["auto_connect"].toBool());
    ui->auto_aligning->setChecked(setting["auto_aligning"].toBool());
    ui->remember_file_type->setChecked(setting["remember_file_type"].toBool());
    ui->show_cmd_line->setChecked(setting["show_cmd_line"].toBool());
    ui->show_origin->setChecked(setting["show_origin"].toBool());
    if (ui->show_cmd_line->isChecked())
    {
        _cmd_widget->show();
    }
    else
    {
        _cmd_widget->hide();
    }
    if (ui->show_origin->isChecked())
    {
        ui->canvas->show_origin();
    }
    else
    {
        ui->canvas->hide_origin();
    }
    if (ui->remember_file_type->isChecked())
    {
       _file_type = setting["file_type"].toString();
    }
}

void MainWindow::save_settings()
{
    QJsonObject &setting = GlobalSetting::get_instance()->setting;

    setting["auto_save"] = ui->auto_save->isChecked();
    setting["auto_layering"] = ui->auto_layering->isChecked();
    setting["auto_connect"] = ui->auto_connect->isChecked();
    setting["auto_aligning"] = ui->auto_aligning->isChecked();
    setting["remember_file_type"] = ui->remember_file_type->isChecked();
    setting["show_cmd_line"] = ui->show_cmd_line->isChecked();
    setting["show_origin"] = ui->show_origin->isChecked();
    if (ui->remember_file_type->isChecked())
    {
        setting["file_type"] = _file_type;
    }

    GlobalSetting::get_instance()->save_setting();
}

void MainWindow::show_layers_manager()
{
    _layers_manager->update_layers();
    _layers_manager->exec();
}

void MainWindow::hide_layers_manager()
{
    _layers_cbx->setModel(_layers_manager->model());
    ui->canvas->refresh_vbo();
    _editer.reset_selected_mark();
}

void MainWindow::to_main_page()
{
    ui->tool_widget->setCurrentIndex(0);
}




void MainWindow::connect_polylines()
{
    if (_editer.connect(_editer.selected(), GlobalSetting::get_instance()->setting["catch_distance"].toDouble()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
    }
}

void MainWindow::close_polyline()
{
    if (_editer.close_polyline(_editer.selected()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
    }
}

void MainWindow::combinate()
{
    if (_editer.combinate(_editer.selected()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
    }
}

void MainWindow::rotate()
{
    std::list<Geo::Geometry *> objects = _editer.selected();
    _editer.rotate(objects, ui->rotate_angle->value(), QApplication::keyboardModifiers() != Qt::ControlModifier, ui->to_all_layers->isChecked());
    ui->canvas->refresh_vbo(objects.empty());
    ui->canvas->update();
}

void MainWindow::flip_x()
{
    std::list<Geo::Geometry *> objects = _editer.selected();
    _editer.flip(objects, true, QApplication::keyboardModifiers() != Qt::ControlModifier, ui->to_all_layers->isChecked());
    ui->canvas->refresh_vbo(objects.empty());
    ui->canvas->update();
}

void MainWindow::flip_y()
{
    std::list<Geo::Geometry *> objects = _editer.selected();
    _editer.flip(objects, false, QApplication::keyboardModifiers() != Qt::ControlModifier, ui->to_all_layers->isChecked());
    ui->canvas->refresh_vbo(objects.empty());
    ui->canvas->update();
}

void MainWindow::mirror()
{
    ui->current_tool->setText("Mirror");
    ui->canvas->set_operation(Canvas::Operation::MIRROR);
}

void MainWindow::scale()
{
    if (_editer.scale(_editer.selected(), QApplication::keyboardModifiers() != Qt::ControlModifier, ui->scale_sbx->value()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::offset()
{
    if (_editer.offset(_editer.selected(), ui->offset_sbx->value()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->update();
    }
}



void MainWindow::to_array_page()
{
    ui->tool_widget->setCurrentIndex(1);
}

void MainWindow::line_array()
{
    if (_editer.line_array(_editer.selected(), ui->array_x_item->value(), ui->array_y_item->value(),
            ui->array_x_space->value(), ui->array_y_space->value()))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::ring_array()
{
    ui->array_tool->setText("Ring Array");
    ui->canvas->set_operation(Canvas::Operation::RINGARRAY);
}



void MainWindow::polygon_union()
{
    Container *container0 = nullptr, *container1 = nullptr;
    for (Geo::Geometry *object : _editer.selected())
    {
        if (object->type() == Geo::Type::CONTAINER)
        {
            if (container0 == nullptr)
            {
                container0 = dynamic_cast<Container *>(object);
            }
            else
            {
                container1 = dynamic_cast<Container *>(object);
                break;
            }
        }
    }

    if (_editer.polygon_union(container0, container1))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::polygon_intersection()
{
    Container *container0 = nullptr, *container1 = nullptr;
    for (Geo::Geometry *object : _editer.selected())
    {
        if (object->type() == Geo::Type::CONTAINER)
        {
            if (container0 == nullptr)
            {
                container0 = dynamic_cast<Container *>(object);
            }
            else
            {
                container1 = dynamic_cast<Container *>(object);
                break;
            }
        }
    }

    if (_editer.polygon_intersection(container0, container1))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::polygon_difference()
{
    ui->current_tool->setText("Difference");
    ui->canvas->set_operation(Canvas::Operation::POLYGONDIFFERENCE);
}



void MainWindow::fillet()
{
    ui->canvas->set_operation(Canvas::Operation::FILLET);
}



void MainWindow::show_data_panel()
{
    _panel->load_draw_data(GlobalSetting::get_instance()->graph, ui->canvas->points_count());
    _panel->exec();
}



void MainWindow::open_file(const QString &path)
{
    if (!QFileInfo(path).isFile())
    {
        return;
    }
    else if (GlobalSetting::get_instance()->graph->modified && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }

    GlobalSetting::get_instance()->setting["file_path"] = path;

    _editer.delete_graph();
    Graph *g = new Graph;
    if (path.toUpper().endsWith(".DSV"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios::in | std::ios::binary);
        DSVParser::parse(file, g);
        file.close();
        
        if (ui->remember_file_type->isChecked())
        {
            _file_type = "DSV: (*.dsv *.DSV)";
        }
    }
    else if (path.toUpper().endsWith(".PLT"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios::in | std::ios::binary);
        PLTParser::parse(file, g);
        file.close();
        
        if (ui->remember_file_type->isChecked())
        {
            _file_type = "PLT: (*.plt *.PLT)";
        }
    }
    else if(path.toUpper().endsWith(".CUT") || path.toUpper().endsWith(".NC"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios::in | std::ios::binary);
        RS274DParser::parse(file, g);
        file.close();

        if (ui->remember_file_type->isChecked())
        {
            _file_type = "RS274D: (*.cut *.CUT *.nc *NC)";
        }
    }
    else
    {
        std::ifstream file(path.toLocal8Bit(), std::ios_base::in | std::ios::binary);
        std::string line;
        std::getline(file, line);
        while (file)
        {
            if (line.find("IN") != std::string::npos || line.find("PU") != std::string::npos || line.find("PD") != std::string::npos)
            {
                file.clear();
                file.seekg(0, std::ios::beg);
                PLTParser::parse(file, g);
                break;
            }
            else if (line.find("M15") != std::string::npos || line.find('X') != std::string::npos || line.find('Y') != std::string::npos)
            {
                file.clear();
                file.seekg(0, std::ios::beg);
                RS274DParser::parse(file, g);
                break;
            }
            else if (line.find("END") != std::string::npos)
            {
                file.clear();
                file.seekg(0, std::ios::beg);
                DSVParser::parse(file, g);
                break;
            }
        }
        file.close();
    }

    _editer.load_graph(g, path);
    if (ui->auto_connect->isChecked())
    {
        _editer.auto_connect();
    }
    if (ui->auto_layering->isChecked())
    {
        _editer.auto_layering();
    }
    GlobalSetting::get_instance()->graph->modified = false;

    ui->canvas->refresh_vbo();
    _info_labels[2]->setText(path);
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());
    g = nullptr;

    ui->canvas->show_overview();
    ui->canvas->update();
}

void MainWindow::append_file(const QString &path)
{
    if (!QFileInfo(path).isFile())
    {
        return;
    }

    Graph *g = new Graph;
    if (path.toUpper().endsWith(".DSV"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios_base::in);
        DSVParser::parse(file, g);
        file.close();
    }
    else if (path.toUpper().endsWith(".PLT"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios_base::in);
        PLTParser::parse(file, g);
        file.close();
    }
    else if(path.toUpper().endsWith(".CUT") || path.toUpper().endsWith(".NC"))
    {
        std::ifstream file(path.toLocal8Bit(), std::ios_base::in);
        RS274DParser::parse(file,g);
        file.close();
    }

    GlobalSetting::get_instance()->graph->modified = true;
    Graph *graph = GlobalSetting::get_instance()->graph;
    _editer.load_graph(g);
    if (ui->auto_connect->isChecked())
    {
        _editer.auto_connect();
    }
    if (ui->auto_layering->isChecked())
    {
        _editer.auto_layering();
    }
    Geo::AABBRect rect0(graph->bounding_rect()), rect1(g->bounding_rect());
    g->translate(rect0.right() + 10 - rect1.left(), rect0.bottom() - rect1.bottom());
    graph->merge(*g);
    _editer.load_graph(graph);
    delete g;
    ui->canvas->refresh_vbo();
    ui->canvas->refresh_selected_ibo();
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());

    ui->canvas->show_overview();
    ui->canvas->update();
}

