#pragma once

#include <QJsonObject>
#include "./ui_MainWindow.h"


class GlobalSetting
{
private:
    QJsonObject _setting;
    Ui::MainWindow *_ui = nullptr;
    static std::unique_ptr<GlobalSetting> _instance;
    
private:
    GlobalSetting(){}

public:
    static std::unique_ptr<GlobalSetting> &get_instance();

    QJsonObject &setting();

    Ui::MainWindow *ui();

    void load_ui(Ui::MainWindow *ui);

    void load_setting();
    
    void save_setting();
};