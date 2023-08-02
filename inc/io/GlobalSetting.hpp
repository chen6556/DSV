#pragma once
#include <QJsonObject>


class GlobalSetting
{
private:
    QJsonObject _setting;
    static std::unique_ptr<GlobalSetting> _instance;
    
private:
    GlobalSetting(){}

public:
    static std::unique_ptr<GlobalSetting> &get_instance();

    QJsonObject &setting();

    void load_setting();
    
    void save_setting();
};