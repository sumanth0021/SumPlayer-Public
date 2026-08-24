#include "core/app/AppSettings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

namespace SumPlayer
{
    AppSettings::AppSettings()
        : m_subtitleFontScale(1.0)
        , m_defaultVolume(200)
    {
        load();
    }

    std::string AppSettings::getSettingsFilePath() const
    {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        return (appDataDir + "/settings.json").toStdString();
    }

    void AppSettings::load()
    {
        QFile file(QString::fromStdString(getSettingsFilePath()));
        if (!file.open(QIODevice::ReadOnly))
        {
            std::cout << "[AppSettings] No settings file found, using defaults." << std::endl;
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject())
        {
            std::cout << "[AppSettings] Settings file is corrupted, using defaults." << std::endl;
            return;
        }

        QJsonObject obj = doc.object();
        m_subtitleFontScale = obj.value("subtitleFontScale").toDouble(1.0);
        m_defaultVolume = obj.value("defaultVolume").toInt(200);

        std::cout << "[AppSettings] Loaded settings from " << getSettingsFilePath() << std::endl;
    }

    void AppSettings::save()
    {
        QJsonObject obj;
        obj["subtitleFontScale"] = m_subtitleFontScale;
        obj["defaultVolume"] = m_defaultVolume;

        QJsonDocument doc(obj);

        QFile file(QString::fromStdString(getSettingsFilePath()));
        if (!file.open(QIODevice::WriteOnly))
        {
            std::cout << "[AppSettings] Failed to save settings." << std::endl;
            return;
        }

        file.write(doc.toJson());
        file.close();

        std::cout << "[AppSettings] Settings saved." << std::endl;
    }

    double AppSettings::getSubtitleFontScale() const { return m_subtitleFontScale; }
    void AppSettings::setSubtitleFontScale(double scale) { m_subtitleFontScale = scale; save(); }

    int AppSettings::getDefaultVolume() const { return m_defaultVolume; }
    void AppSettings::setDefaultVolume(int volume) { m_defaultVolume = volume; save(); }
}