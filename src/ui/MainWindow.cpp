#include <QFileDialog>
#include <QAbstractItemView>
#include <QListView>
#include <QMimeData>

#include "ui/MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "ui/WinUITool.hpp"
#include "ui/MessageBox.hpp"
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
    delete _actiongroup;
    _editer.delete_graph();
    for (size_t i = 0; i < 3; ++i)
    {
        delete _info_labels[i];
    }
    delete _layers_cbx;
    delete _layers_btn;
    delete _layers_manager;
    delete _setting;
    delete _panel;
}

void MainWindow::init()
{
    GlobalSetting::setting().ui = ui;
    _actiongroup = new ActionGroup(ui, std::bind(&MainWindow::actiongroup_callback,
        this, std::placeholders::_1, std::placeholders::_2));

    _editer.load_graph(new Graph());
    ui->canvas->bind_editer(&_editer);

    ui->canvas->installEventFilter(ui->cmd_widget);

    _clock.start(5000);
    connect_btn_to_cmd();
    connect(&_clock, &QTimer::timeout, this, &MainWindow::auto_save);

    connect(ui->auto_aligning, &QAction::triggered, [this]() { GlobalSetting::setting().auto_aligning = ui->auto_aligning->isChecked(); });
    connect(ui->actionadvanced, &QAction::triggered, _setting, &Setting::exec);
    connect(ui->show_origin, &QAction::triggered, [this]() { ui->show_origin->isChecked() ? ui->canvas->show_origin() : ui->canvas->hide_origin(); });
    connect(ui->show_cmdline, &QAction::triggered, [this]() { ui->show_cmdline->isChecked() ? ui->cmd_widget->show() : ui->cmd_widget->hide(); });

    connect(_setting, &Setting::accepted, this, &MainWindow::refresh_settings);

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
    connect(_layers_manager, &LayersManager::accepted, this, &MainWindow::hide_layers_manager);

    _layers_btn = new QToolButton(this);
    _layers_btn->setText("Layers");
    _layers_btn->setMinimumHeight(22);
    _layers_btn->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    ui->statusBar->addPermanentWidget(_layers_btn);
    connect(_layers_btn, &QToolButton::clicked, this, &MainWindow::show_layers_manager);

    _layers_cbx = new QComboBox(this);
    _layers_cbx->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    _layers_cbx->setView(new QListView(_layers_cbx));
    _layers_cbx->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->statusBar->addPermanentWidget(_layers_cbx);
    _layers_cbx->setModel(_layers_manager->model());
    connect(_layers_cbx, &QComboBox::currentIndexChanged,
        [this](int index) { _editer.set_current_group(_editer.groups_count() - 1 - index); });

#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
}

void MainWindow::connect_btn_to_cmd()
{
    connect(ui->measure_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Length_CMD); });
    connect(ui->angle_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Angle_CMD); });

    connect(ui->text_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Text_CMD); });
    connect(ui->close_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Close_CMD); });
    connect(ui->combinate_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Combinate_CMD); });
    connect(ui->detach_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Detach_CMD); });

    connect(ui->trim_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Trim_CMD); });
    connect(ui->extend_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Extend_CMD); });
    connect(ui->split_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Split_CMD); });
    connect(ui->mirror_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Mirror_CMD); });
    connect(ui->flipx_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::FlipX_CMD); });
    connect(ui->flipy_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::FlipY_CMD); });
    connect(ui->rotate_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Rotate_CMD); });
    connect(ui->scale_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Scale_CMD); });
    connect(ui->offset_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Offset_CMD); });

    connect(ui->intersection_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Intersection_CMD); });
    connect(ui->union_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Union_CMD); });
    connect(ui->xor_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::XOR_CMD); });
    connect(ui->difference_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::Difference_CMD); });

    connect(ui->line_array_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::LineArray_CMD); });
    connect(ui->ring_array_btn, &QPushButton::clicked, [this]() { ui->cmd_widget->work(CMDWidget::CMD::RingArray_CMD); });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (GlobalSetting::setting().graph->modified)
    {
        switch (MessageBox::question(this, "File is modified", "Save or not?", QDialogButtonBox::StandardButton::Yes |
            QDialogButtonBox::StandardButton::No | QDialogButtonBox::StandardButton::Cancel))
        {
        case QDialogButtonBox::StandardButton::Yes:
            save_file();
            break;
        case QDialogButtonBox::StandardButton::Cancel:
            return event->ignore();
        default:
            break;
        }
    }
    QMainWindow::closeEvent(event);
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
    switch (event->button())
    {
    case Qt::MouseButton::BackButton:
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
            ui->canvas->refresh_vbo(true);
            ui->canvas->refresh_selected_ibo();
            ui->canvas->update();
        }
        break;
    default:
        break;
    }

    return QMainWindow::mousePressEvent(event);
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
        ui->cmd_widget->clear();
        break;
    case Qt::Key_Space:
        ui->cmd_widget->work_last_cmd();
        break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        {
            std::set<Geo::Type> types;
            for (const Geo::Geometry *object : _editer.selected())
            {
                if (const Combination *combination = dynamic_cast<const Combination *>(object))
                {
                    for (const Geo::Geometry *item : *combination)
                    {
                        types.insert(item->type());
                    }
                }
                else
                {
                    types.insert(object->type());
                }
            }
            if (!types.empty())
            {
                CanvasOperations::CanvasOperation::operation().clear();
                _editer.remove_selected();
                ui->canvas->refresh_vbo(types, true);
                ui->canvas->refresh_selected_ibo();
                ui->canvas->update();
            }
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
        if (event->modifiers() == Qt::ControlModifier)
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
            ui->canvas->refresh_vbo(true);
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
        ui->cmd_widget->activate(event->key());
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
    ui->cmd_widget->move(ui->cmd_widget->mapToParent(QPoint((width() - ui->cmd_widget->width()) / 2,
        height() - 33 - ui->cmd_widget->height()) - ui->cmd_widget->pos()));
}


void MainWindow::open_file()
{
    if (GlobalSetting::setting().graph->modified && MessageBox::question(this, "File is modified", "Save or not?") == QDialogButtonBox::StandardButton::Yes)
    {
        save_file();
    }
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
    if (GlobalSetting::setting().graph->modified && MessageBox::question(this, "File is modified", "Save or not?") == QDialogButtonBox::StandardButton::Yes)
    {
        save_file();
    }

    _editer.delete_graph();
    _editer.load_graph(new Graph());
    GlobalSetting::setting().graph->modified = false;
    ui->canvas->refresh_vbo(true);
    _info_labels[2]->clear();
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());
    ui->canvas->update();
    GlobalSetting::setting().save_setting();
}

void MainWindow::save_file()
{
    if (ui->canvas->empty())
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
    if (ui->canvas->empty())
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

void MainWindow::refresh_cmd(const CMDWidget::CMD cmd)
{
    switch (cmd)
    {
    case CMDWidget::CMD::Main_CMD:
        return ui->tool_widget->setCurrentIndex(0);
    case CMDWidget::CMD::Array_CMD:
        return ui->tool_widget->setCurrentIndex(1);
    default:
        break;
    }
}

void MainWindow::refresh_settings()
{
    if (_setting->update_curve_vbo())
    {
        const double value = GlobalSetting::setting().down_sampling;
        Geo::BSpline::default_down_sampling_value = Geo::Bezier::default_down_sampling_value =
            Geo::Circle::default_down_sampling_value = Geo::Ellipse::default_down_sampling_value = value;
        const double step = GlobalSetting::setting().sampling_step;
        Geo::BSpline::default_step = step * 2, Geo::Bezier::default_step = step;
        GlobalSetting::setting().graph->update_curve_shape(step, value);
        ui->canvas->refresh_vbo(true);
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
    ui->auto_combinate->setChecked(GlobalSetting::setting().auto_combinate);
    ui->auto_connect->setChecked(GlobalSetting::setting().auto_connect);
    ui->auto_aligning->setChecked(GlobalSetting::setting().auto_aligning);
    ui->remember_file_type->setChecked(GlobalSetting::setting().remember_file_type);
    ui->show_cmdline->setChecked(GlobalSetting::setting().show_cmdline);
    ui->show_cmdline->isChecked() ? ui->cmd_widget->show() : ui->cmd_widget->hide();
    ui->show_origin->setChecked(GlobalSetting::setting().show_origin);
    ui->show_origin->isChecked() ? ui->canvas->show_origin() : ui->canvas->hide_origin();
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
    GlobalSetting::setting().auto_combinate = ui->auto_combinate->isChecked();
    GlobalSetting::setting().auto_connect = ui->auto_connect->isChecked();
    GlobalSetting::setting().auto_aligning = ui->auto_aligning->isChecked();
    GlobalSetting::setting().remember_file_type = ui->remember_file_type->isChecked();
    GlobalSetting::setting().show_cmdline = ui->show_cmdline->isChecked();
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
    ui->canvas->refresh_vbo(true);
    _editer.reset_selected_mark();
}

void MainWindow::to_main_page()
{
    ui->tool_widget->setCurrentIndex(0);
}

void MainWindow::to_array_page()
{
    ui->tool_widget->setCurrentIndex(1);
}

void MainWindow::show_data_panel()
{
    _panel->load_draw_data(GlobalSetting::setting().graph);
    _panel->exec();
}


void MainWindow::open_file(const QString &path)
{
    if (!QFileInfo(path).isFile())
    {
        return;
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
    else if (ui->auto_combinate->isChecked())
    {
        _editer.auto_combinate();
    }
    GlobalSetting::setting().graph->modified = false;

    ui->canvas->refresh_vbo(true);
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
    else if (ui->auto_combinate->isChecked())
    {
        _editer.auto_combinate();
    }
    Geo::AABBRect rect0(graph->bounding_rect()), rect1(g->bounding_rect());
    g->translate(rect0.right() + 10 - rect1.left(), rect0.bottom() - rect1.bottom());
    graph->merge(*g);
    _editer.load_graph(graph);
    delete g;
    ui->canvas->refresh_vbo(true);
    ui->canvas->refresh_selected_ibo();
    _layers_manager->update_layers();
    _layers_cbx->setModel(_layers_manager->model());

    ui->canvas->show_overview();
    ui->canvas->update();
}

void MainWindow::actiongroup_callback(const ActionGroup::MenuType menu, const int index)
{
    switch (menu)
    {
    case ActionGroup::MenuType::PolygonMenu:
        switch (index)
        {
        case 0: // polyline
            ui->cmd_widget->work(CMDWidget::CMD::Polyline_CMD);
            break;
        case 1: // rectangle
            ui->cmd_widget->work(CMDWidget::CMD::Rectangle_CMD);
            break;
        case 2: // circumscribed polygon
            ui->cmd_widget->work(CMDWidget::CMD::CPolygon_CMD);
            break;
        case 3: // inscribed polygon
            ui->cmd_widget->work(CMDWidget::CMD::IPolygon_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::ArcMenu:
        switch (index)
        {
        case 0: // 3-Point Arc
            ui->cmd_widget->work(CMDWidget::CMD::PArc_CMD);
            break;
        case 1: // Start-Center-Angle Arc
            ui->cmd_widget->work(CMDWidget::CMD::SCAArc_CMD);
            break;
        case 2: // Start-End-Angle Arc
            ui->cmd_widget->work(CMDWidget::CMD::SEAArc_CMD);
            break;
        case 3: // Start-End-Radius Arc
            ui->cmd_widget->work(CMDWidget::CMD::SERArc_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::CircleMenu:
        switch (index)
        {
        case 0: // Center-Radius
            ui->cmd_widget->work(CMDWidget::CMD::CCircle_CMD);
            break;
        case 1: // 2-Point
            ui->cmd_widget->work(CMDWidget::CMD::DCircle_CMD);
            break;
        case 2: // 3-Point
            ui->cmd_widget->work(CMDWidget::CMD::PCircle_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::EllipseMenu:
        switch (index)
        {
        case 0: // Ellipse
            ui->cmd_widget->work(CMDWidget::CMD::Ellipse_CMD);
            break;
        case 1: // Ellipse arc
            ui->cmd_widget->work(CMDWidget::CMD::EllipseArc_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::CurveMenu:
        switch (index)
        {
        case 0: // BSpline
            ui->cmd_widget->work(CMDWidget::CMD::BSpline_CMD);
            break;
        case 1: // Bezier
            ui->cmd_widget->work(CMDWidget::CMD::Bezier_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::FilletMenu:
        switch (index)
        {
        case 0: // fillet
            ui->cmd_widget->work(CMDWidget::CMD::Fillet_CMD);
            break;
        case 1: // free fillet
            ui->cmd_widget->work(CMDWidget::CMD::FreeFillet_CMD);
            break;
        case 2: // chamfer
            ui->cmd_widget->work(CMDWidget::CMD::Chamfer_CMD);
            break;
        default:
            break;
        }
        break;
    case ActionGroup::MenuType::ConnectMenu:
        switch (index)
        {
        case 0: // connect
            ui->cmd_widget->work(CMDWidget::CMD::Connect_CMD);
            break;
        case 1: // blend
            ui->cmd_widget->work(CMDWidget::CMD::Blend_CMD);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}