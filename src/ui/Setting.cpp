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
    const QJsonObject &setting = GlobalSetting::get_instance()->setting;

    ui->catch_distance->setValue(setting["catch_distance"].toDouble());
    ui->backup_times->setValue(setting["backup_times"].toInt());
    ui->text_size->setValue(setting["text_size"].toInt());
    ui->down_sampling->setValue(setting["down_sampling"].toDouble());

    _text_size = ui->text_size->value();
    _down_sampling_value = ui->down_sampling->value();

    ui->show_text->setChecked(setting["show_text"].toBool());
    ui->multiple_select->setChecked(setting["multiple_select"].toBool());
    ui->show_points->setChecked(setting["show_points"].toBool());
    ui->ignroe_M19->setChecked(setting["ignore_M19"].toBool());
}

void Setting::accept()
{
    QJsonObject &setting = GlobalSetting::get_instance()->setting;

    setting["catch_distance"] = ui->catch_distance->value();
    setting["backup_times"] = ui->backup_times->value();
    setting["text_size"] = ui->text_size->value();
    setting["down_sampling"] = ui->down_sampling->value();

    setting["show_text"] = ui->show_text->isChecked();
    setting["multiple_select"] = ui->multiple_select->isChecked();
    setting["show_points"] = ui->show_points->isChecked();
    setting["ignore_M19"] = ui->ignroe_M19->isChecked();

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