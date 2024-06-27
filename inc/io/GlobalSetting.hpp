#pragma once

#include <QJsonObject>

#include "./ui_MainWindow.h"


class GlobalSetting
{
private:
    QJsonObject _setting;
    Ui::MainWindow *_ui = nullptr;
    static GlobalSetting *_instance;
    
private:
    GlobalSetting() {}

    GlobalSetting(const GlobalSetting &) = delete;

    GlobalSetting &operator=(const GlobalSetting &) = delete;

    ~GlobalSetting() {};

public:
    static GlobalSetting *get_instance();

    static void release();

    QJsonObject &setting();

    Ui::MainWindow *ui();

    void load_ui(Ui::MainWindow *ui);

    void load_setting();
    
    void save_setting();
};