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
}

void Setting::accept()
{
    QJsonObject &setting = GlobalSetting::get_instance()->setting();

    setting["catch_distance"] = ui->catch_distance->value();
    setting["backup_times"] = ui->backup_times->value();

    setting["scale_text"] = ui->scale_text->isChecked();

    QDialog::accept();
}


void Setting::show()
{
    init();
    QDialog::show();
}