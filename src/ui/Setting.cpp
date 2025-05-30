#include "ui/Setting.hpp"
#include "./ui_Setting.h"
#include "io/GlobalSetting.hpp"


Setting::Setting(QWidget *parent)
    :QDialog(parent), ui(new Ui::Setting)
{
    ui->setupUi(this);
}

Setting::~Setting()
{
    delete ui;
}

void Setting::init()
{
    ui->catch_distance->setValue(GlobalSetting::setting().catch_distance);
    ui->backup_times->setValue(GlobalSetting::setting().backup_times);
    ui->text_size->setValue(GlobalSetting::setting().text_size);
    ui->down_sampling->setValue(GlobalSetting::setting().down_sampling);

    _text_size = ui->text_size->value();
    _down_sampling_value = ui->down_sampling->value();

    ui->show_text->setChecked(GlobalSetting::setting().show_text);
    ui->multiple_select->setChecked(GlobalSetting::setting().multiple_select);
    ui->show_points->setChecked(GlobalSetting::setting().show_points);
    ui->ignroe_M19->setChecked(GlobalSetting::setting().ignore_M19);
}

void Setting::accept()
{
    GlobalSetting::setting().catch_distance = ui->catch_distance->value();
    GlobalSetting::setting().backup_times = ui->backup_times->value();
    GlobalSetting::setting().text_size = ui->text_size->value();
    GlobalSetting::setting().down_sampling = ui->down_sampling->value();

    GlobalSetting::setting().show_text = ui->show_text->isChecked();
    GlobalSetting::setting().multiple_select = ui->multiple_select->isChecked();
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