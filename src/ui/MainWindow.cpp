#include "ui/MainWindow.hpp"
#include "./ui_mainwindow.h"
#include "io/File.hpp"
#include "io/PLTSpirit.hpp"
#include "io/PDFSpirit.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QAbstractItemView>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include "io/GlobalSetting.hpp"
 

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), _builder(new Graph, &_painter), _setting(new Setting(this))
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
}

void MainWindow::init()
{
    setWindowIcon(QIcon("./DSV2.ico"));

    _painter.resize(800, 600);
    ui->horizontalLayout->addWidget(&_painter);
    _editer.load_graph(_builder.graph());
    _painter.bind_editer(&_editer);

    _clock.start(5000);
    QObject::connect(&_painter, &Canvas::tool_changed, this, &MainWindow::refresh_tool_label);
    QObject::connect(ui->view_btn, &QPushButton::clicked, &_painter, &Canvas::cancel_painting);
    QObject::connect(ui->circle_btn, &QPushButton::clicked, this, [this]() { _painter.use_tool(0); });
    QObject::connect(ui->line_btn, &QPushButton::clicked, this, [this]() { _painter.use_tool(1); });
    QObject::connect(ui->rect_btn, &QPushButton::clicked, this, [this]() { _painter.use_tool(2); });
    QObject::connect(ui->curve_btn, &QPushButton::clicked, this, [this]() { _painter.use_tool(3); _painter.set_bezier_order(ui->curve_sbx->value());});
    QObject::connect(&_clock, &QTimer::timeout, this, &MainWindow::auto_save);

    QObject::connect(ui->auto_aligning, &QAction::triggered, this, [this]() {GlobalSetting::get_instance()->setting()["auto_aligning"] = ui->auto_aligning->isChecked();});
    QObject::connect(ui->actionadvanced, &QAction::triggered, this, [this]() { _setting->show(); });

    for (size_t i = 0; i < 3; ++i)
    {
        _info_labels[i] = new QLabel(this);
        _info_labels[i]->setMinimumWidth(70 + 45 * i);
        ui->statusBar->addWidget(_info_labels[i]);
    }
    _painter.set_info_labels(_info_labels);

    _layers_manager = new LayersManager(this);
    _layers_manager->load_layers(_editer.graph());
    connect(_layers_manager, &LayersManager::accepted, this, [this](){_layers_cbx->setModel(_layers_manager->model());});

    _layers_btn = new QToolButton(this);
    _layers_btn->setText("Layers");
    ui->statusBar->addPermanentWidget(_layers_btn);
    connect(_layers_btn, &QToolButton::clicked, this, &MainWindow::show_layers_manager);

    _layers_cbx = new QComboBox(this);
    _layers_cbx->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->statusBar->addPermanentWidget(_layers_cbx);
    _layers_cbx->setModel(_layers_manager->model());
    connect(_layers_cbx, &QComboBox::currentIndexChanged, this, [this](const int index){_editer.set_current_group(_editer.groups_count() - 1 - index);});
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (_editer.modified() && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }
    QMainWindow::closeEvent(event);
}




void MainWindow::open_file()
{
    if (_editer.modified() && QMessageBox::question(this, "File is modified", "Save or not?") == QMessageBox::Yes)
    {
        save_file();
    }

    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    QString path = dialog->getOpenFileName(dialog, nullptr, _editer.path(), "All Files: (*.*);;JSON: (*.json *.JSON);;PLT: (*.plt *.PLT);;PDF: (*.pdf *.PDF)", &_file_type);
    if (!path.isEmpty())
    {
        _editer.delete_graph();
        Graph *g = new Graph;
        if (path.endsWith(".json") || path.endsWith(".JSON"))
        {
            File::read(path, g);
            
            if (ui->remember_file_type->isChecked())
            {
                _file_type = "JSON: (*.json *.JSON)";
            }
        }
        else if (path.endsWith(".plt") || path.endsWith(".PLT"))
        {
            PLTSpirit spirit;
            spirit.load_graph(g);
            std::fstream file(path.toStdString(), std::ios_base::in);
            spirit.parse(file);
            file.close();
            
            if (ui->remember_file_type->isChecked())
            {
                _file_type = "PLT: (*.plt *.PLT)";
            }
        }
		else if (path.endsWith(".pdf") || path.endsWith(".PDF"))
        {
            QPDF pdf;
            pdf.processFile(path.toStdString().c_str());
            for (int i = 0, count = pdf.getObjectCount(); i < count; ++i)
            {
                if (pdf.getObject(i, 0).isImage() || pdf.getObject(i, 0).isFormXObject())
                {
                    pdf.replaceObject(i, 0, QPDFObjectHandle::newNull());
                }
            }

            QPDFWriter outpdf(pdf);
            outpdf.setStreamDataMode(qpdf_stream_data_e::qpdf_s_uncompress);
            outpdf.setDecodeLevel(qpdf_stream_decode_level_e::qpdf_dl_all);
            outpdf.setOutputMemory();
            outpdf.setNewlineBeforeEndstream(true);
            outpdf.write();
            std::shared_ptr<Buffer> buffer = outpdf.getBufferSharedPointer();

            std::stringstream ss(reinterpret_cast<char *>(buffer->getBuffer()), std::ios::in);
            PDFSpirit spirit;
            spirit.load_graph(g);
            spirit.parse(ss);

			if (ui->remember_file_type->isChecked())
            {
                _file_type = "PDF: (*.pdf *.PDF)";
            }
        }
        _editer.load_graph(g, path);
        if (ui->auto_layering->isChecked())
        {
            _editer.auto_layering();
        }
        _editer.reset_modified();
        _info_labels[2]->setText(path);
        _layers_manager->load_layers(g);
        _layers_cbx->setModel(_layers_manager->model());
        g = nullptr;
    }
    delete dialog;
}

void MainWindow::save_file()
{
    if (_editer.graph() == nullptr || _painter.empty())
    {
        return;
    }

    if (_info_labels[2]->text().isEmpty() || !(_info_labels[2]->text().endsWith(".json") 
        || _info_labels[2]->text().endsWith(".JSON")))
    {
        QFileDialog *dialog = new QFileDialog();
        dialog->setModal(true);
        QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path(), "Files: (*.json *.JSON)");
        if (!path.isEmpty())
        {
            File::write(path, _editer.graph());
            _editer.set_path(path);
            _editer.reset_modified();
            _info_labels[2]->setText(path);
        }
        delete dialog;
    }
    else
    {
        File::write(_info_labels[2]->text(), _editer.graph());
        _editer.reset_modified();
    }
}

void MainWindow::auto_save()
{
    if (!ui->auto_save->isChecked() || _editer.path().isEmpty() ||
        !(_editer.path().endsWith(".json") || _editer.path().endsWith(".JSON")))
    {
        return;
    }
    if (_editer.modified())
    {
        File::write(_editer.path(), _editer.graph());
        _editer.reset_modified();
    }
}

void MainWindow::saveas_file()
{
    if (_editer.graph() == nullptr || _painter.empty())
    {
        return;
    }
    QFileDialog *dialog = new QFileDialog();
    dialog->setModal(true);
    QString path = dialog->getSaveFileName(dialog, nullptr, _editer.path().isEmpty() ? "D:/output.json" : _editer.path(), "Files: (*.json *.JSON)");
    if (!path.isEmpty())
    {
        File::write(path, _editer.graph());
        _editer.reset_modified();
    }
    delete dialog;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (_painter.is_typing())
    {
        return;
    }
    switch (event->key())
    {
    /* case Qt::Key_T:
        test();
        _painter.update();
        break; */
    case Qt::Key_Escape:
        _painter.cancel_painting();
        _editer.reset_selected_mark();
        break;
    case Qt::Key_Space:
        _painter.use_last_tool();
        break;
    case Qt::Key_D:
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        if (_editer.remove_selected())
        {
            _painter.update();
        }
        break;
    case Qt::Key_A:
        if (event->modifiers() == Qt::ControlModifier)
        {
            _editer.reset_selected_mark(true);
            _painter.update();
        }
        break;
    case Qt::Key_B:
        _painter.use_tool(3);
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
            _painter.copy();
        }
        else
        {
            _painter.use_tool(0);
        }
        break;
    case Qt::Key_X:
        if (event->modifiers() == Qt::ControlModifier)
        {
            _painter.cut();
            _painter.update();
        }
        break;
    case Qt::Key_V:
        if (event->modifiers() == Qt::ControlModifier)
        {
            _painter.paste();
            _painter.update();
        }
        break;
    case Qt::Key_L:
        _painter.use_tool(1);
        break;
    case Qt::Key_R:
        _painter.use_tool(2);
        break;
    case Qt::Key_Z:
        if (event->modifiers() == Qt::ControlModifier && !_painter.is_painting())
        {
            _editer.load_backup();
            _painter.update();
        }
    default:
        break;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::refresh_tool_label(const int &value)
{
    switch (value)
    {
    case -1:
        ui->current_tool->clear();
        break;
    case 0:
        ui->current_tool->setText("Circle");
        break;
    case 1:
        ui->current_tool->setText("Polyline");
        break;
    case 2:
        ui->current_tool->setText("Polygon");
        break;
    case 3:
        ui->current_tool->setText("Bezier Curve");
        break;
    default:
        break;
    }
}

void MainWindow::load_settings()
{   
    GlobalSetting::get_instance()->load_setting();
    const QJsonObject &setting = GlobalSetting::get_instance()->setting();

    _editer.set_path(setting["file_path"].toString());
    ui->auto_save->setChecked(setting["auto_save"].toBool());
    ui->auto_layering->setChecked(setting["auto_layering"].toBool());
    ui->auto_aligning->setChecked(setting["auto_aligning"].toBool());
    ui->remember_file_type->setChecked(setting["remember_file_type"].toBool());
    if (ui->remember_file_type->isChecked())
    {
       _file_type = setting["file_type"].toString();
    }
}

void MainWindow::save_settings()
{
    QJsonObject &setting = GlobalSetting::get_instance()->setting();
    
    setting["file_path"] = _editer.path();
    setting["auto_save"] = ui->auto_save->isChecked();
    setting["auto_layering"] = ui->auto_layering->isChecked();
    setting["auto_aligning"] = ui->auto_aligning->isChecked();
    setting["remember_file_type"] = ui->remember_file_type->isChecked();
    if (ui->remember_file_type->isChecked())
    {
        setting["file_type"] = _file_type;
    }

    GlobalSetting::get_instance()->save_setting();
}

void MainWindow::show_layers_manager()
{
    _layers_manager->load_layers(_editer.graph());
    _layers_manager->show();
}





void MainWindow::rotate()
{
    _editer.rotate(ui->rotate_angle->value(), _editer.selected_count() == 0);
    _painter.update();
}

void MainWindow::flip_x()
{
    _editer.flip(true, _editer.selected_count() == 0);
    _painter.update();
}

void MainWindow::flip_y()
{
    _editer.flip(false, _editer.selected_count() == 0);
    _painter.update();
}


/* void MainWindow::test()
{
    std::list<Geo::Geometry *> objs = _editer.selected();

    if (objs.empty())
    {
        // _builder.store_linkedlist({"1", "2", "3", "3", "4"});
        std::list<QString> values = {"1", "2", QString(), "3", "4", QString(), "5", "6"};
        BinaryTreeNode<QString> *tree = BinaryTree::create_tree_preorder(values, QString());

        _builder.store_binarytree(tree);

        BinaryTree::delete_tree(tree);
    }
    else
    {
        for (Geo::Geometry *obj : objs)
        {
            if (dynamic_cast<Container *>(obj))
            {
                StructureMover::move(reinterpret_cast<Container *>(obj), 40, 0);
                break;
            }
        }
    }
} */