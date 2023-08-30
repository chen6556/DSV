#include "ui/Setting.hpp"
#include "./ui_setting.h"
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
    const QJsonObject &setting = GlobalSetting::get_instance()->setting();

    ui->catch_distance->setValue(setting["catch_distance"].toDouble());
    ui->backup_times->setValue(setting["backup_times"].toInt());

    ui->scale_text->setChecked(setting["scale_text"].toBool());
    ui->catch_only->setChecked(setting["active_layer_catch_only"].toBool());
    ui->multiple_select->setChecked(setting["multiple_select"].toBool());
    ui->cursor_catch->setChecked(setting["cursor_catch"].toBool());
    ui->show_points->setChecked(setting["show_points"].toBool());
}

void Setting::accept()
{
    QJsonObject &setting = GlobalSetting::get_instance()->setting();

    setting["catch_distance"] = ui->catch_distance->value();
    setting["backup_times"] = ui->backup_times->value();

    setting["scale_text"] = ui->scale_text->isChecked();
    setting["active_layer_catch_only"] = ui->catch_only->isChecked();
    setting["multiple_select"] = ui->multiple_select->isChecked();
    setting["cursor_catch"] = ui->cursor_catch->isChecked();
    setting["show_points"] = ui->show_points->isChecked();

    QDialog::accept();
}


void Setting::show()
{
    init();
    QDialog::show();
}