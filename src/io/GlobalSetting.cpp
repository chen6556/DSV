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

QJsonObject &GlobalSetting::setting()
{
    return _setting;
}

Ui::MainWindow *GlobalSetting::ui()
{
    return _ui;
}

void GlobalSetting::load_ui(Ui::MainWindow *ui)
{
    _ui = ui;
}

void GlobalSetting::load_setting()
{
    QFile file("./config.json");
    file.open(QIODevice::ReadOnly);
    QJsonParseError jerr;
    _instance->_setting = QJsonDocument::fromJson(file.readAll(), &jerr).object();
    file.close();

    if (_instance->_setting.isEmpty())
    {   
        _instance->_setting["active_layer_catch_only"] = true;
        _instance->_setting["auto_aligning"] = true;
        _instance->_setting["auto_layering"] = true;
        _instance->_setting["auto_save"] = false;
        _instance->_setting["backup_times"] = 50;
        _instance->_setting["catch_distance"] = 2;
        _instance->_setting["cursor_catch"] = false;
        _instance->_setting["file_path"] = "/";
        _instance->_setting["file_type"] = "All Files: (*.*)";
        _instance->_setting["multiple_select"] = false;
        _instance->_setting["remember_file_type"] = true;
        _instance->_setting["show_cmd_line"] = false;
        _instance->_setting["show_origin"] = true;
        _instance->_setting["show_points"] = false;
        _instance->_setting["show_text"] = false;
        _instance->_setting["text_size"] = 16;
    }
}

void GlobalSetting::save_setting()
{
    QJsonDocument doc;
    doc.setObject(_instance->_setting);
    QFile file("./config.json");
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}