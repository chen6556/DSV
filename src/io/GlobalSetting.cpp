#include "io/GlobalSetting.hpp"
#include <QFile>
#include <QIODevice>
#include <QJsonObject>
#include <QJsonDocument>

std::unique_ptr<GlobalSetting> GlobalSetting::_instance = std::make_unique<GlobalSetting>(GlobalSetting());

std::unique_ptr<GlobalSetting> &GlobalSetting::get_instance()
{
    return _instance;
}

Memo &GlobalSetting::setting()
{
    return _setting;
}

void GlobalSetting::load_setting()
{
    QFile file("./config.json");
    file.open(QIODevice::ReadOnly);
    QJsonParseError jerr;
    QJsonObject obj = QJsonDocument::fromJson(file.readAll(), &jerr).object();
    file.close();

    Memo &setting = _instance->_setting;
    if (obj.isEmpty())
    {
        setting["file_path"] = "D:/";
        setting["auto_save"] = false;
        setting["auto_layering"] = true;
        setting["auto_aligning"] = false;
        setting["remember_file_type"] = true;
        setting["file_type"] = "All File: (*.*)";
    }
    else
    {
        setting["file_path"] = obj["file_path"].toString().toStdString();
        setting["auto_save"] = obj["auto_save"].toBool();
        setting["auto_layering"] = obj["auto_layering"].toBool();
        setting["auto_aligning"] = obj["auto_aligning"].toBool();
        setting["remember_file_type"] = obj["remember_file_type"].toBool();
        setting["file_type"] = obj["file_type"].toString().toStdString();
    }
}

void GlobalSetting::save_setting()
{
    Memo &setting = _instance->_setting;
    QJsonObject obj;
    obj.insert("file_path", QString::fromStdString(setting["file_path"].to_string()));
    obj.insert("auto_save", setting["auto_save"].to_bool());
    obj.insert("auto_layering", setting["auto_layering"].to_bool());
    obj.insert("auto_aligning", setting["auto_aligning"].to_bool());
    obj.insert("remember_file_type", setting["remember_file_type"].to_bool());
    obj.insert("file_type", QString::fromStdString(setting["file_type"].to_string()));

    QJsonDocument doc;
    doc.setObject(obj);
    QFile file("./config.json");
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}