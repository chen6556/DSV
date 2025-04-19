#include <QFile>
#include <QIODevice>
#include <QJsonDocument>

#include "io/GlobalSetting.hpp"


GlobalSetting *GlobalSetting::_instance = new GlobalSetting();

GlobalSetting *GlobalSetting::get_instance()
{
    return _instance;
}

void GlobalSetting::release()
{
    delete _instance;
}

void GlobalSetting::load_setting()
{
    QFile file("./config.json");
    file.open(QIODevice::ReadOnly);
    QJsonParseError jerr;
    _instance->setting = QJsonDocument::fromJson(file.readAll(), &jerr).object();
    file.close();

    if (_instance->setting.isEmpty())
    {
        _instance->setting["auto_aligning"] = true;
        _instance->setting["auto_connect"] = true;
        _instance->setting["auto_layering"] = true;
        _instance->setting["auto_save"] = false;
        _instance->setting["backup_times"] = 50;
        _instance->setting["catch_distance"] = 2;
        _instance->setting["catch_center"] = false;
        _instance->setting["catch_foot"] = false;
        _instance->setting["catch_tangency"] = false;
        _instance->setting["catch_vertex"] = false;
        _instance->setting["file_path"] = "/";
        _instance->setting["file_type"] = "All Files: (*.*)";
        _instance->setting["ignore_M19"] = true;
        _instance->setting["multiple_select"] = false;
        _instance->setting["remember_file_type"] = true;
        _instance->setting["show_cmd_line"] = false;
        _instance->setting["show_origin"] = true;
        _instance->setting["show_points"] = false;
        _instance->setting["show_text"] = false;
        _instance->setting["text_size"] = 16;
    }
}

void GlobalSetting::save_setting()
{
    QJsonDocument doc;
    doc.setObject(_instance->setting);
    QFile file("./config.json");
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}