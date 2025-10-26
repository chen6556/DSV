#pragma once
#include "../ui/ui_MainWindow.h"


class GlobalSetting
{
private:
    GlobalSetting() {}

    GlobalSetting(const GlobalSetting &) = delete;

    GlobalSetting &operator=(const GlobalSetting &) = delete;

    ~GlobalSetting() {};

public:
    Graph *graph = nullptr;
    Ui::MainWindow *ui = nullptr;

    bool translated_points = false;
    bool auto_aligning = false;
    bool auto_connect = false;
    bool auto_layering = false;
    bool auto_combinate = false;
    bool auto_save = false;

    bool catch_center = false;
    bool catch_foot = false;
    bool catch_tangency = false;
    bool catch_vertex = false;
    bool catch_intersection = false;

    bool ignore_M19 = true;
    bool multiple_select = false;
    bool remember_file_type = true;
    bool show_cmd_line = true;
    bool show_origin = true;
    bool show_points = false;
    bool show_text = true;

    int backup_times = 50;
    int text_size = 16;
    int offset_join_type = 2;
    int offset_end_type = 0;

    double catch_distance = 2.0;
    double sampling_step = 0.01;
    double down_sampling = 0.02;

    QString file_path = "/";
    QString file_type = "All Files: (*.*)";

public:
    static GlobalSetting &setting();

    void load_setting();
    
    void save_setting();
};