#pragma once
#include <QObject>
#include "GlobalSetting.hpp"


class GUISetting : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ActiveLayerCatchOnly READ getActiveLayerCatchOnly WRITE setActiveLayerCatchOnly);
    Q_PROPERTY(bool AutoAligning READ getAutoAligning WRITE setAutoAligning);
    Q_PROPERTY(bool AutoLayering READ getAutoLayering WRITE setAutoLayering);
    Q_PROPERTY(bool AutoSave READ getAutoSave WRITE setAutoSave);
    Q_PROPERTY(int BackupTimes READ getBackupTimes WRITE setBackupTimes);
    Q_PROPERTY(double CatchDistance READ getCatchDistance WRITE setCatchDistance);
    Q_PROPERTY(bool CursorCatch READ getCursorCatch WRITE setCursorCatch);
    Q_PROPERTY(QString FilePath READ getFilePath WRITE setFilePath);
    Q_PROPERTY(QString FileType READ getFileType WRITE setFileType);
    Q_PROPERTY(bool RememberFileType READ getRememberFileType WRITE setRememberFileType);
    Q_PROPERTY(bool ScaleText READ getScaleText WRITE setScaleText);
    Q_PROPERTY(bool ShowPoints READ getShowPoints WRITE setShowPoints);

private:
    bool bool_value;
    int int_value;
    double double_value;
    QString string_value;

public:

    GUISetting(QObject *parent = nullptr)
        : QObject(parent){}

    ~GUISetting()
    {
        GlobalSetting::get_instance()->save_setting();
    }

    bool getActiveLayerCatchOnly()
    {
        bool_value = GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool();
        return bool_value;
    }

    void setActiveLayerCatchOnly(const bool value)
    {
        GlobalSetting::get_instance()->setting()["active_layer_catch_only"] = value;
        bool_value = value;
    }

    bool getAutoAligning()
    {
        bool_value = GlobalSetting::get_instance()->setting()["auto_aligning"].toBool();
        return bool_value;
    }

    void setAutoAligning(const bool value)
    {
        GlobalSetting::get_instance()->setting()["auto_aligning"] = value;
        bool_value = value;
    }

    bool getAutoLayering()
    {
        bool_value = GlobalSetting::get_instance()->setting()["auto_layering"].toBool();
        return bool_value;
    }

    void setAutoLayering(const bool value)
    {
        GlobalSetting::get_instance()->setting()["auto_layering"] = value;
        bool_value = value;
    }

    bool getAutoSave()
    {
        bool_value = GlobalSetting::get_instance()->setting()["auto_save"].toBool();
        return bool_value;
    }

    void setAutoSave(const bool value)
    {
        GlobalSetting::get_instance()->setting()["auto_save"] = value;
        bool_value = value;
    }

    int getBackupTimes()
    {
        int_value = GlobalSetting::get_instance()->setting()["backup_times"].toInt();
        return int_value;
    }

    void setBackupTimes(const int value)
    {
        GlobalSetting::get_instance()->setting()["backup_times"] = value;
        int_value = value;
    }

    double getCatchDistance()
    {
        double_value = GlobalSetting::get_instance()->setting()["catch_distance"].toDouble();
        return double_value;
    }

    void setCatchDistance(const double value)
    {
        GlobalSetting::get_instance()->setting()["catch_distance"] = value;
        double_value = value;
    }

    bool getCursorCatch()
    {
        bool_value = GlobalSetting::get_instance()->setting()["cursor_catch"].toBool();
        return bool_value;
    }

    void setCursorCatch(const bool value)
    {
        GlobalSetting::get_instance()->setting()["cursor_catch"] = value;
        bool_value = value;
    }

    QString &getFilePath()
    {
        string_value = GlobalSetting::get_instance()->setting()["file_path"].toString();
        return string_value;
    }

    void setFilePath(const QString &value)
    {
        GlobalSetting::get_instance()->setting()["file_path"] = value;
        string_value = value;
    }

    QString &getFileType()
    {
        string_value = GlobalSetting::get_instance()->setting()["file_type"].toString();
        return string_value;
    }

    void setFileType(const QString &value)
    {
        GlobalSetting::get_instance()->setting()["file_type"] = value;
        string_value = value;
    }

    bool getRememberFileType()
    {
        bool_value = GlobalSetting::get_instance()->setting()["remember_file_type"].toBool();
        return bool_value;
    }

    void setRememberFileType(const bool value)
    {
        GlobalSetting::get_instance()->setting()["remember_file_type"] = value;
        bool_value = value;
    }

    bool getScaleText()
    {
        bool_value = GlobalSetting::get_instance()->setting()["scale_text"].toBool();
        return bool_value;
    }

    void setScaleText(const bool value)
    {
        GlobalSetting::get_instance()->setting()["scale_text"] = value;
        bool_value = value;
    }

    bool getShowPoints()
    {
        bool_value = GlobalSetting::get_instance()->setting()["show_points"].toBool();
        return bool_value;
    }

    void setShowPoints(const bool value)
    {
        GlobalSetting::get_instance()->setting()["show_points"] = value;
        bool_value = value;
    }
};