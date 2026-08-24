#pragma once
#include <string>

namespace SumPlayer
{
    class AppSettings
    {
    public:
        AppSettings();

        void load();
        void save();

        double getSubtitleFontScale() const;
        void setSubtitleFontScale(double scale);

        int getDefaultVolume() const;
        void setDefaultVolume(int volume);

    private:
        std::string getSettingsFilePath() const;

        double m_subtitleFontScale;
        int m_defaultVolume;
    };
}