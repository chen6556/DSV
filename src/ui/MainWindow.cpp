#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QListView>
#include <QMimeData>

#include "ui/MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "io/File.hpp"
#include "io/PLTParser.hpp"
#include "io/RS274DParser.hpp"
#include "io/DSVParser.hpp"
#include "io/GlobalSetting.hpp"
#include "io/DXFReaderWriter.hpp"


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
}

void MainWindow::init()
{
    GlobalSetting::setting().ui = ui;

    _editer.load_graph(new Graph());
    ui->canvas->bind_editer(&_editer);

    _cmd_widget = new CMDWidget(&_editer, ui->canvas, this);
    _cmd_widget->show();
    QObject::connect(_cmd_widget, &CMDWidget::cmd_changed, this, &MainWindow::refresh_cmd);

    _clock.start(5000);
    QObject::connect(ui->measure_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Measure); });
    QObject::connect(ui->circle_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Circle); });
    QObject::connect(ui->ellipse_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Ellipse); });
    QObject::connect(ui->line_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Polyline); });
    QObject::connect(ui->rect_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Rect); });
    QObject::connect(ui->bspline_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::BSpline); });
    QObject::connect(ui->bezier_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Bezier); });
    QObject::connect(ui->curve_spb, &QSpinBox::valueChanged, ui->canvas, &Canvas::set_curve_order);
    QObject::connect(ui->text_btn, &QPushButton::clicked, [this]() { ui->canvas->use_tool(Canvas::Tool::Text); });
    QObject::connect(ui->split_btn, &QPushButton::clicked, [this]() { _editer.split(_editer.selected()); });
    QObject::connect(&_clock, &QTimer::timeout, this, &MainWindow::auto_save);

    QObject::connect(ui->auto_aligning, &QAction::triggered, [this]() { GlobalSetting::setting().auto_aligning = ui->auto_aligning->isChecked(); });
    QObject::connect(ui->actionadvanced, &QAction::triggered, _setting, &Setting::exec);
    QObject::connect(ui->show_origin, &QAction::triggered, [this]() { ui->show_origin->isChecked() ? ui->canvas->show_origin() : ui->canvas->hide_origin(); });
    QObject::connect(ui->show_cmd_line, &QAction::triggered, [this]() { ui->show_cmd_line->isChecked() ? _cmd_widget->show() : _cmd_widget->hide(); });

    QObject::connect(_setting, &Setting::accepted, this, &MainWindow::refresh_settings);

    for (size_t i = 0; i < 3; ++i)
    {
        _info_labels[i] = new QLabel(this);
        _info_labels[i]->setMinimumWidth(70 + 45 * i);
        ui->statusBar->addWidget(_info_labels[i]);
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::Shape::VLine);
        line->setFrameShadow(QFrame::Shadow::Plain);
        ui->statusBar->addWidget(line);
    }
    ui->canvas->set_info_labels(_info_labels);

    _layers_manager = new LayersManager(this);
    _layers_manager->bind_editer(&_editer);
    _layers_manager->update_layers();
    QObject::connect(_layers_manager, &LayersManager::accepted, this, &MainWindow::hide_layers_manager);

    _layers_btn = new QToolButton(this);
    _layers_btn->setText("Layers");
    _layers_btn->setMinimumHeight(22);
    _layers_btn->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    ui->statusBar->addPermanentWidget(_layers_btn);
    QObject::connect(_layers_btn, &QToolButton::clicked, this, &MainWindow::show_layers_manager);

    _layers_cbx = new QComboBox(this);
    _layers_cbx->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    _layers_cbx->setView(new QListView(_layers_cbx));
    _layers_cbx->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->statusBar->addPermanentWidget(_layers_cbx);
    _layers_cbx->setModel(_layers_manager->model());
    QObject::connect(_layers_cbx, &QComboBox::currentIndexChanged,
        [this](int index) { _editer.set_current_group(_editer.groups_count() - 1 - index); });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (GlobalSetting::setting().graph->modified)
    {
        switch (QMessageBox::question(this, "File is modified", "Save or not?", QMessageBox::StandardButton::Yes,
            QMessageBox::StandardButton::No, QMessageBox::StandardButton::Cancel))
        {
        case QMessageBox::StandardButton::Yes:
            save_file();
            break;
        case QMessageBox::StandardButton::Cancel:
            return event->ignore();
        default:
            break;
        }
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
        ui->canvas->refresh_cache_vbo(0);
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
            ui->canvas->clear_cache();
            ui->canvas->update();
        }
        break;
    case Qt::Key_A:
        if (event->modifiers() == Qt::ControlModifier)
        {
            _editer.reset_selected_mark(true);
            ui->canvas->refresh_selected_ibo();
            ui->canvas->refresh_cache_vbo(0);
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
            ui->canvas->refresh_cache_vbo(0);
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
    const QString suffixs = "dsv DSV plt PLT cut CUT dxf DXF nc NC";
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
    QString path = dialog->getOpenFileName(dialog, nullptr, _editer.path(), "All Files: (*.*);;DSV: (*.dsv *.DSV);;"
        "PLT: (*.plt *.PLT);;RS274D: (*.cut *.CUT *.nc *.NC);;DXF: (*.dxf *.DXF)", &_file_type);
    open_file(path);
    delete dialog;
}

void MainWindow::close_file()
{
    if (GlobalSetting::setting().graph->modified && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }

    _editer.delete_graph();
    _editer.load_graph(new Graph());
    GlobalSetting::setting().graph->modified = false;
    ui->canvas->refresh_vbo();
    _info_labels[2]->clear();
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());
    ui->canvas->update();
    GlobalSetting::setting().save_setting();
}

void MainWindow::save_file()
{
    if (GlobalSetting::setting().graph == nullptr || ui->canvas->empty())
    {
        return;
    }

    if (const QString label_path = _info_labels[2]->text(); label_path.isEmpty() || !(label_path.toLower().endsWith(".dsv")
        || label_path.toLower().endsWith(".plt") || label_path.toLower().endsWith(".dxf")))
    {
        QFileDialog *dialog = new QFileDialog();
        dialog->setModal(true);
        QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path(), "DSV: (*.dsv);;PLT: (*.plt);;DXF: (*.dxf)");
        if (!path.isEmpty())
        {
            bool flag = false;
            if (path.toLower().endsWith(".dsv"))
            {
                File::write(path, GlobalSetting::setting().graph, File::DSV);
                flag = true;
            }
            else if (path.toLower().endsWith(".plt"))
            {
                File::write(path, GlobalSetting::setting().graph, File::PLT);
                flag = true;
            }
            else if (path.toLower().endsWith(".dxf"))
            {
                dxfRW dxfRW(path.toLocal8Bit());
                DXFReaderWriter dxf_interface(GlobalSetting::setting().graph, &dxfRW);
                dxfRW.write(&dxf_interface, DRW::Version::AC1018, false);
                flag = true;
            }
            if (flag)
            {
                _editer.set_path(path);
                GlobalSetting::setting().graph->modified = false;
                _info_labels[2]->setText(path);
            }
        }
        delete dialog;
    }
    else
    {
        if (label_path.toLower().endsWith(".dsv"))
        {
            File::write(label_path, GlobalSetting::setting().graph, File::DSV);
            GlobalSetting::setting().graph->modified = false;
        }
        else if (label_path.toLower().endsWith(".plt"))
        {
            File::write(label_path, GlobalSetting::setting().graph, File::PLT);
            GlobalSetting::setting().graph->modified = false;
        }
        else if (label_path.toLower().endsWith(".dxf"))
        {
            dxfRW dxfRW(label_path.toLocal8Bit());
            DXFReaderWriter dxf_interface(GlobalSetting::setting().graph, &dxfRW);
            dxfRW.write(&dxf_interface, DRW::Version::AC1018, false);
            GlobalSetting::setting().graph->modified = false;
        }
    }
}

void MainWindow::auto_save()
{
    if (!ui->auto_save->isChecked() || _editer.path().isEmpty() || !(_editer.path().toLower().endsWith(".dsv")
        || _editer.path().toLower().endsWith(".plt") || _editer.path().toLower().endsWith(".dxf")))
    {
        return;
    }
    if (GlobalSetting::setting().graph->modified)
    {
        if (_editer.path().toLower().endsWith(".dsv"))
        {
            File::write(_editer.path(), GlobalSetting::setting().graph, File::DSV);
        }
        else if (_editer.path().toLower().endsWith(".plt"))
        {
            File::write(_editer.path(), GlobalSetting::setting().graph, File::PLT);
        }
        else if (_editer.path().toLower().endsWith(".dxf"))
        {
            dxfRW dxfRW(_editer.path().toLocal8Bit());
            DXFReaderWriter dxf_interface(GlobalSetting::setting().graph, &dxfRW);
            dxfRW.write(&dxf_interface, DRW::Version::AC1018, false);
        }
        GlobalSetting::setting().graph->modified = false;
    }
}

void MainWindow::saveas_file()
{
    if (GlobalSetting::setting().graph == nullptr || ui->canvas->empty())
    {
        return;
    }
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path().isEmpty() ? "D:/output.dsv" : _editer.path(), "DSV: (*.dsv);;PLT: (*.plt);;DXF: (*.dxf)");
    if (!path.isEmpty())
    {
        if (path.toLower().endsWith(".dsv"))
        {
            File::write(path, GlobalSetting::setting().graph, File::DSV);
        }
        else if (path.toLower().endsWith(".plt"))
        {
            File::write(path, GlobalSetting::setting().graph, File::PLT);
        }
        else if (path.toLower().endsWith(".dxf"))
        {
            dxfRW dxfRW(path.toLocal8Bit());
            DXFReaderWriter dxf_interface(GlobalSetting::setting().graph, &dxfRW);
            dxfRW.write(&dxf_interface, DRW::Version::AC1018, false);
        }
        GlobalSetting::setting().graph->modified = false;
    }
    delete dialog;
}

void MainWindow::append_file()
{
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    QString path = dialog->getOpenFileName(dialog, nullptr, _editer.path(), "All Files: (*.*);;DSV: (*.dsv *.DSV);;"
        "PLT: (*.plt *.PLT);;RS274D: (*.cut *.CUT *.nc *.NC);;DXF: (*.dxf *.DXF)", &_file_type);
    append_file(path);
    delete dialog;
}

void MainWindow::set_catch(QAction *action)
{
    switch (ui->menuCursorCatch->actions().indexOf(action))
    {
    case 0:
        ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Vertex, action->isChecked());
        break;
    case 1:
        ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Center, action->isChecked());
        break;
    case 2:
        ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Foot, action->isChecked());
        break;
    case 3:
        ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Tangency, action->isChecked());
        break;
    case 4:
        ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Intersection, action->isChecked());
        break;
    default:
        break;
    }
}

void MainWindow::refresh_tool_label(const Canvas::Tool tool)
{
    switch (tool)
    {
    case Canvas::Tool::Measure:
        ui->current_tool->setText("Length");
        break;
    case Canvas::Tool::Circle:
        ui->current_tool->setText("Circle");
        break;
    case Canvas::Tool::Ellipse:
        ui->current_tool->setText("Ellipse");
        break;
    case Canvas::Tool::Polyline:
        ui->current_tool->setText("Polyline");
        break;
    case Canvas::Tool::Rect:
        ui->current_tool->setText("Rectangle");
        break;
    case Canvas::Tool::BSpline:
        ui->current_tool->setText("BSpline");
        break;
    case Canvas::Tool::Bezier:
        ui->current_tool->setText("Bezier");
        break;
    case Canvas::Tool::Text:
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
    case Canvas::Operation::Mirror:
        ui->current_tool->setText("Mirror");
        break;
    case Canvas::Operation::PolygonDifference:
        ui->current_tool->setText("Difference");
        break;
    case Canvas::Operation::RingArray:
        ui->current_tool->setText("Ring Array");
        break;
    case Canvas::Operation::Fillet:
        ui->current_tool->setText("Fillet");
        break;
    case Canvas::Operation::Rotate:
        ui->current_tool->setText("Rotate");
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
    case CMDWidget::CMD::Open_CMD:
        return open_file();
    case CMDWidget::CMD::Append_CMD:
        return append_file();
    case CMDWidget::CMD::Save_CMD:
        return save_file();
    case CMDWidget::CMD::Exit_CMD:
        this->close();
        break;
    case CMDWidget::CMD::Main_CMD:
        return ui->tool_widget->setCurrentIndex(0);
    case CMDWidget::CMD::Mirror_CMD:
        ui->current_tool->setText("Mirror");
        return ui->canvas->set_operation(Canvas::Operation::Mirror);
    case CMDWidget::CMD::Array_CMD:
        return ui->tool_widget->setCurrentIndex(1);
    case CMDWidget::CMD::RingArray_CMD:
        ui->array_tool->setText("Ring Array");
        break;
    case CMDWidget::CMD::Difference_CMD:
        ui->current_tool->setText("Difference");
        break;
    default:
        break;
    }
}

void MainWindow::refresh_settings()
{
    if (_setting->update_curve_vbo())
    {
        const double value = GlobalSetting::setting().down_sampling;
        Geo::BSpline::default_down_sampling_value = Geo::Bezier::default_down_sampling_value = value;
        GlobalSetting::setting().graph->update_curve_shape(0.2, value);
        ui->canvas->refresh_vbo();
    }
    if (_setting->update_text_vbo())
    {
        ui->canvas->refresh_text_vbo();
    }
    _editer.set_backup_count(GlobalSetting::setting().backup_times);
    ui->canvas->set_catch_distance(GlobalSetting::setting().catch_distance);
    GlobalSetting::setting().save_setting();
}

void MainWindow::load_settings()
{
    GlobalSetting::setting().load_setting();

    Geo::Bezier::default_down_sampling_value = Geo::BSpline::default_down_sampling_value =
        Geo::Circle::default_down_sampling_value = Geo::Ellipse::default_down_sampling_value =
        GlobalSetting::setting().down_sampling;

    _editer.set_path(GlobalSetting::setting().file_path);
    _editer.set_backup_count(GlobalSetting::setting().backup_times);
    ui->auto_save->setChecked(GlobalSetting::setting().auto_save);
    ui->auto_layering->setChecked(GlobalSetting::setting().auto_layering);
    ui->auto_connect->setChecked(GlobalSetting::setting().auto_connect);
    ui->auto_aligning->setChecked(GlobalSetting::setting().auto_aligning);
    ui->remember_file_type->setChecked(GlobalSetting::setting().remember_file_type);
    ui->show_cmd_line->setChecked(GlobalSetting::setting().show_cmd_line);
    ui->show_origin->setChecked(GlobalSetting::setting().show_origin);
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
       _file_type = GlobalSetting::setting().file_type;
    }

    ui->canvas->set_catch_distance(GlobalSetting::setting().catch_distance);
    ui->actionVertex->setChecked(GlobalSetting::setting().catch_vertex);
    ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Vertex, ui->actionVertex->isChecked());
    ui->actionCenter->setChecked(GlobalSetting::setting().catch_center);
    ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Center, ui->actionCenter->isChecked());
    ui->actionFoot->setChecked(GlobalSetting::setting().catch_foot);
    ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Foot, ui->actionFoot->isChecked());
    ui->actionTangency->setChecked(GlobalSetting::setting().catch_tangency);
    ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Tangency, ui->actionTangency->isChecked());
    ui->actionIntersection->setChecked(GlobalSetting::setting().catch_intersection);
    ui->canvas->set_cursor_catch(Canvas::CatchedPointType::Intersection, ui->actionIntersection->isChecked());
}

void MainWindow::save_settings()
{
    GlobalSetting::setting().auto_save = ui->auto_save->isChecked();
    GlobalSetting::setting().auto_layering = ui->auto_layering->isChecked();
    GlobalSetting::setting().auto_connect = ui->auto_connect->isChecked();
    GlobalSetting::setting().auto_aligning = ui->auto_aligning->isChecked();
    GlobalSetting::setting().remember_file_type = ui->remember_file_type->isChecked();
    GlobalSetting::setting().show_cmd_line = ui->show_cmd_line->isChecked();
    GlobalSetting::setting().show_origin = ui->show_origin->isChecked();
    if (ui->remember_file_type->isChecked())
    {
        GlobalSetting::setting().file_type = _file_type;
    }
    GlobalSetting::setting().catch_vertex = ui->actionVertex->isChecked();
    GlobalSetting::setting().catch_center = ui->actionCenter->isChecked();
    GlobalSetting::setting().catch_foot = ui->actionFoot->isChecked();
    GlobalSetting::setting().catch_tangency = ui->actionTangency->isChecked();
    GlobalSetting::setting().catch_intersection = ui->actionIntersection->isChecked();

    GlobalSetting::setting().save_setting();
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
    if (_editer.connect(_editer.selected(), GlobalSetting::setting().catch_distance))
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
    if (objects.size() == 1)
    {
        ui->canvas->refresh_selected_ibo(objects.front());
        ui->canvas->refresh_cache_vbo(0);
    }
    ui->canvas->update();
}

void MainWindow::flip_x()
{
    std::list<Geo::Geometry *> objects = _editer.selected();
    _editer.flip(objects, true, QApplication::keyboardModifiers() != Qt::ControlModifier, ui->to_all_layers->isChecked());
    ui->canvas->refresh_vbo(objects.empty());
    if (objects.size() == 1)
    {
        ui->canvas->refresh_selected_ibo(objects.front());
        ui->canvas->refresh_cache_vbo(0);
    }
    ui->canvas->update();
}

void MainWindow::flip_y()
{
    std::list<Geo::Geometry *> objects = _editer.selected();
    _editer.flip(objects, false, QApplication::keyboardModifiers() != Qt::ControlModifier, ui->to_all_layers->isChecked());
    ui->canvas->refresh_vbo(objects.empty());
    if (objects.size() == 1)
    {
        ui->canvas->refresh_selected_ibo(objects.front());
        ui->canvas->refresh_cache_vbo(0);
    }
    ui->canvas->update();
}

void MainWindow::mirror()
{
    ui->current_tool->setText("Mirror");
    ui->canvas->set_operation(Canvas::Operation::Mirror);
}

void MainWindow::scale()
{
    if (std::list<Geo::Geometry *> objects = _editer.selected();
        _editer.scale(objects, QApplication::keyboardModifiers() != Qt::ControlModifier, ui->scale_sbx->value()))
    {
        ui->canvas->refresh_vbo();
        if (objects.size() == 1)
        {
            ui->canvas->refresh_selected_ibo(objects.front());
            ui->canvas->refresh_cache_vbo(0);
        }
        ui->canvas->update();
    }
}

void MainWindow::offset()
{
    if (_editer.offset(_editer.selected(), ui->offset_sbx->value(),
        static_cast<Geo::Offset::JoinType>(GlobalSetting::setting().offset_join_type),
        static_cast<Geo::Offset::EndType>(GlobalSetting::setting().offset_end_type)))
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
    ui->canvas->set_operation(Canvas::Operation::RingArray);
}



void MainWindow::polygon_union()
{
    Geo::Polygon *polygon0 = nullptr, *polygon1 = nullptr;
    for (Geo::Geometry *object : _editer.selected())
    {
        if (dynamic_cast<Geo::Polygon *>(object) == nullptr)
        {
            continue;
        }
        if (polygon0 == nullptr)
        {
            polygon0 = dynamic_cast<Geo::Polygon *>(object);
        }
        else
        {
            polygon1 = dynamic_cast<Geo::Polygon *>(object);
            break;
        }
    }

    if (_editer.polygon_union(polygon0, polygon1))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::polygon_intersection()
{
    Geo::Polygon *polygon0 = nullptr, *polygon1 = nullptr;
    for (Geo::Geometry *object : _editer.selected())
    {
        if (dynamic_cast<Geo::Polygon *>(object) == nullptr)
        {
            continue;
        }
        if (polygon0 == nullptr)
        {
            polygon0 = dynamic_cast<Geo::Polygon *>(object);
        }
        else
        {
            polygon1 = dynamic_cast<Geo::Polygon *>(object);
            break;
        }
    }

    if (_editer.polygon_intersection(polygon0, polygon1))
    {
        ui->canvas->refresh_vbo();
        ui->canvas->refresh_selected_ibo();
        ui->canvas->update();
    }
}

void MainWindow::polygon_difference()
{
    ui->current_tool->setText("Difference");
    ui->canvas->set_operation(Canvas::Operation::PolygonDifference);
}



void MainWindow::fillet()
{
    ui->canvas->set_operation(Canvas::Operation::Fillet);
}



void MainWindow::show_data_panel()
{
    _panel->load_draw_data(GlobalSetting::setting().graph, ui->canvas->points_count());
    _panel->exec();
}



void MainWindow::open_file(const QString &path)
{
    if (!QFileInfo(path).isFile())
    {
        return;
    }
    else if (GlobalSetting::setting().graph->modified && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }

    GlobalSetting::setting().file_path = path;

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
    else if (path.toUpper().endsWith(".DXF"))
    {
        DXFReaderWriter dxf_interface(g);
        dxfRW dxfRW(path.toLocal8Bit());
        dxfRW.read(&dxf_interface, false);
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
    GlobalSetting::setting().graph->modified = false;

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
    else if (path.toUpper().endsWith(".DXF"))
    {
        DXFReaderWriter dxf_interface(g);
        dxfRW dxfRW(path.toLocal8Bit());
        dxfRW.read(&dxf_interface, false);
    }

    Graph *graph = GlobalSetting::setting().graph;
    graph->modified = true;
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

