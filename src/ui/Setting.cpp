#include <QListView>
#include "ui/Setting.hpp"
#include "./ui_Setting.h"
#include "ui/WinUITool.hpp"
#include "io/GlobalSetting.hpp"


Setting::Setting(QWidget *parent) : QDialog(parent), ui(new Ui::Setting)
{
    ui->setupUi(this);
}

Setting::~Setting()
{
    delete ui;
}

void Setting::init()
{
    ui->offset_join_type->setView(new QListView(ui->offset_join_type));
    ui->offset_join_type->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->offset_end_type->setView(new QListView(ui->offset_end_type));
    ui->offset_end_type->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->catch_distance->setValue(GlobalSetting::setting().catch_distance);
    ui->backup_times->setValue(GlobalSetting::setting().backup_times);
    ui->text_size->setValue(GlobalSetting::setting().text_size);
    ui->sampling_step->setValue(GlobalSetting::setting().sampling_step);
    ui->down_sampling->setValue(GlobalSetting::setting().down_sampling);
    ui->offset_join_type->setCurrentIndex(GlobalSetting::setting().offset_join_type);
    ui->offset_end_type->setCurrentIndex(GlobalSetting::setting().offset_end_type);

    _text_size = ui->text_size->value();
    _down_sampling_value = ui->down_sampling->value();

    ui->show_text->setChecked(GlobalSetting::setting().show_text);
    ui->show_points->setChecked(GlobalSetting::setting().show_points);
    ui->ignroe_M19->setChecked(GlobalSetting::setting().ignore_M19);

#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
}

void Setting::accept()
{
    GlobalSetting::setting().catch_distance = ui->catch_distance->value();
    GlobalSetting::setting().backup_times = ui->backup_times->value();
    GlobalSetting::setting().text_size = ui->text_size->value();
    GlobalSetting::setting().sampling_step = ui->sampling_step->value();
    GlobalSetting::setting().down_sampling = ui->down_sampling->value();
    GlobalSetting::setting().offset_join_type = ui->offset_join_type->currentIndex();
    GlobalSetting::setting().offset_end_type = ui->offset_end_type->currentIndex();

    GlobalSetting::setting().show_text = ui->show_text->isChecked();
    GlobalSetting::setting().show_points = ui->show_points->isChecked();
    GlobalSetting::setting().ignore_M19 = ui->ignroe_M19->isChecked();

    QDialog::accept();
}


void Setting::show()
{
    init();
    return QDialog::show();
}

int Setting::exec()
{
    init();
    return QDialog::exec();
}

bool Setting::update_text_vbo() const
{
    return _text_size == ui->text_size->value();
}

bool Setting::update_curve_vbo() const
{
    return _down_sampling_value == ui->down_sampling->value();
}