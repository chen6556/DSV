#pragma once

#include <QJsonObject>

#include "./ui_MainWindow.h"


class GlobalSetting
{
private:
    static GlobalSetting *_instance;

private:
    GlobalSetting() {}

    GlobalSetting(const GlobalSetting &) = delete;

    GlobalSetting &operator=(const GlobalSetting &) = delete;

    ~GlobalSetting() {};

public:
    Graph *graph = nullptr;
    Ui::MainWindow *ui = nullptr;
    QJsonObject setting;

    bool translated_points = false;

public:
    static GlobalSetting *get_instance();

    static void release();

    void load_setting();
    
    void save_setting();
};