#include <QFile>
#include <QIODevice>
#include <QJsonObject>
#include <QJsonDocument>

#include "io/GlobalSetting.hpp"


GlobalSetting &GlobalSetting::setting()
{
    static GlobalSetting instance;
    return instance;
}

void GlobalSetting::load_setting()
{
    QFile file("./config.json");
    file.open(QIODevice::ReadOnly);
    QJsonParseError jerr;
    const QJsonObject values = QJsonDocument::fromJson(file.readAll(), &jerr).object();
    file.close();

    if (values.contains("auto_aligning"))
    {
        this->auto_aligning = values.value("auto_aligning").toBool();
    }
    if (values.contains("auto_connect"))
    {
        this->auto_connect = values.value("auto_connect").toBool();
    }
    if (values.contains("auto_layering"))
    {
        this->auto_layering = values.value("auto_layering").toBool();
    }
    if (values.contains("auto_save"))
    {
        this->auto_save = values.value("auto_save").toBool();
    }
    if (values.contains("backup_times"))
    {
        this->backup_times = values.value("backup_times").toInt();
    }
    if (values.contains("catch_distance"))
    {
        this->catch_distance = values.value("catch_distance").toDouble();
    }
    if (values.contains("catch_center"))
    {
        this->catch_center = values.value("catch_center").toBool();
    }
    if (values.contains("catch_foot"))
    {
        this->catch_foot = values.value("catch_foot").toBool();
    }
    if (values.contains("catch_tangency"))
    {
        this->catch_tangency = values.value("catch_tangency").toBool();
    }
    if (values.contains("catch_vertex"))
    {
        this->catch_vertex = values.value("catch_vertex").toBool();
    }
    if (values.contains("catch_intersection"))
    {
        this->catch_intersection = values.value("catch_intersection").toBool();
    }
    if (values.contains("down_sampling"))
    {
        this->down_sampling = values.value("down_sampling").toDouble();
    }
    if (values.contains("file_path"))
    {
        this->file_path = values.value("file_path").toString();
    }
    if (values.contains("file_type"))
    {
        this->file_type = values.value("file_type").toString();
    }
    if (values.contains("ignore_M19"))
    {
        this->ignore_M19 = values.value("ignore_M19").toBool();
    }
    if (values.contains("multiple_select"))
    {
        this->multiple_select = values.value("multiple_select").toBool();
    }
    if (values.contains("offset_end_type"))
    {
        this->offset_end_type = values.value("offset_end_type").toInt();
    }
    if (values.contains("offset_join_type"))
    {
        this->offset_join_type = values.value("offset_join_type").toInt();
    }
    if (values.contains("remember_file_type"))
    {
        this->remember_file_type = values.value("remember_file_type").toBool();
    }
    if (values.contains("show_cmd_line"))
    {
        this->show_cmd_line = values.value("show_cmd_line").toBool();
    }
    if (values.contains("show_origin"))
    {
        this->show_origin = values.value("show_origin").toBool();
    }
    if (values.contains("show_points"))
    {
        this->show_points = values.value("show_points").toBool();
    }
    if (values.contains("show_text"))
    {
        this->show_text = values.value("show_text").toBool();
    }
    if (values.contains("text_size"))
    {
        this->text_size = values.value("text_size").toInt();
    }
}

void GlobalSetting::save_setting()
{
    QJsonObject values;
    values.insert("auto_aligning", this->auto_aligning);
    values.insert("auto_connect", this->auto_connect);
    values.insert("auto_layering", this->auto_layering);
    values.insert("auto_save", this->auto_save);
    values.insert("backup_times", this->backup_times);
    values.insert("catch_distance", this->catch_distance);
    values.insert("catch_center", this->catch_center);
    values.insert("catch_foot", this->catch_foot);
    values.insert("catch_tangency", this->catch_tangency);
    values.insert("catch_vertex", this->catch_vertex);
    values.insert("catch_intersection", this->catch_intersection);
    values.insert("down_sampling", this->down_sampling);
    values.insert("file_path", this->file_path);
    values.insert("file_type", this->file_type);
    values.insert("ignore_M19", this->ignore_M19);
    values.insert("multiple_select", this->multiple_select);
    values.insert("offset_end_type", this->offset_end_type);
    values.insert("offset_join_type", this->offset_join_type);
    values.insert("remember_file_type", this->remember_file_type);
    values.insert("show_cmd_line", this->show_cmd_line);
    values.insert("show_origin", this->show_origin);
    values.insert("show_points", this->show_points);
    values.insert("show_text", this->show_text);
    values.insert("text_size", this->text_size);

    QJsonDocument doc;
    doc.setObject(values);
    QFile file("./config.json");
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}