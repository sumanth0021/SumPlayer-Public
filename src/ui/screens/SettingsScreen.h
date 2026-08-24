#pragma once
#include <QWidget>

class QSlider;
class QLabel;

namespace SumPlayer
{
    class AppSettings;

    class SettingsScreen : public QWidget
    {
        Q_OBJECT
    public:
        explicit SettingsScreen(AppSettings* settings, QWidget* parent = nullptr);

    signals:
        void backRequested();
        void subtitleFontScaleChanged(double scale);
        void defaultVolumeChanged(int volume);

    private:
        void onFontScaleChanged(int sliderValue);
        void onVolumeChanged(int sliderValue);

        AppSettings* m_settings;
        QSlider* m_fontScaleSlider;
        QLabel* m_fontScaleLabel;
        QSlider* m_volumeSlider;
        QLabel* m_volumeLabel;
    };
}
