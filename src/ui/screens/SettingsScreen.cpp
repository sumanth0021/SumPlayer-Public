#include "ui/screens/SettingsScreen.h"
#include "core/app/AppSettings.h"

#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace SumPlayer
{
    SettingsScreen::SettingsScreen(AppSettings* settings, QWidget* parent)
        : QWidget(parent)
        , m_settings(settings)
    {
        setStyleSheet("background-color: #111111;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(40, 25, 40, 25);

        QPushButton* backButton = new QPushButton("< Back to Home", this);
        backButton->setFlat(true);
        backButton->setCursor(Qt::PointingHandCursor);
        backButton->setStyleSheet("color: #999999; background: transparent; border: none; font-size: 13px; text-align: left;");
        connect(backButton, &QPushButton::clicked, this, &SettingsScreen::backRequested);

        QLabel* title = new QLabel("Settings", this);
        title->setStyleSheet("color: #ffffff; font-size: 28px; font-weight: bold;");

        QLabel* fontScaleTitle = new QLabel("Subtitle Text Size", this);
        fontScaleTitle->setStyleSheet("color: #ffffff; font-size: 15px; font-weight: bold;");

        QHBoxLayout* fontScaleRow = new QHBoxLayout();
        m_fontScaleSlider = new QSlider(Qt::Horizontal, this);
        m_fontScaleSlider->setRange(50, 200);
        m_fontScaleSlider->setValue((int)(settings->getSubtitleFontScale() * 100));
        m_fontScaleSlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 4px; background: #444444; border-radius: 2px; }"
            "QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0; background: #ffffff; border-radius: 7px; }"
            "QSlider::sub-page:horizontal { background: #6a6a6a; border-radius: 2px; }"
        );

        m_fontScaleLabel = new QLabel(QString::number(settings->getSubtitleFontScale(), 'f', 2) + "x", this);
        m_fontScaleLabel->setStyleSheet("color: #aaaaaa; font-size: 13px;");
        m_fontScaleLabel->setFixedWidth(50);

        fontScaleRow->addWidget(m_fontScaleSlider);
        fontScaleRow->addWidget(m_fontScaleLabel);

        connect(m_fontScaleSlider, &QSlider::valueChanged, this, &SettingsScreen::onFontScaleChanged);

        QLabel* volumeTitle = new QLabel("Default Volume", this);
        volumeTitle->setStyleSheet("color: #ffffff; font-size: 15px; font-weight: bold; margin-top: 20px;");

        QHBoxLayout* volumeRow = new QHBoxLayout();
        m_volumeSlider = new QSlider(Qt::Horizontal, this);
        m_volumeSlider->setRange(0, 400);
        m_volumeSlider->setValue(settings->getDefaultVolume());
        m_volumeSlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 4px; background: #444444; border-radius: 2px; }"
            "QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0; background: #ffffff; border-radius: 7px; }"
            "QSlider::sub-page:horizontal { background: #6a6a6a; border-radius: 2px; }"
        );

        m_volumeLabel = new QLabel(QString::number(settings->getDefaultVolume() * 100 / 200) + "%", this);
        m_volumeLabel->setStyleSheet("color: #aaaaaa; font-size: 13px;");
        m_volumeLabel->setFixedWidth(50);

        volumeRow->addWidget(m_volumeSlider);
        volumeRow->addWidget(m_volumeLabel);

        connect(m_volumeSlider, &QSlider::valueChanged, this, &SettingsScreen::onVolumeChanged);

        mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(title);
        mainLayout->addSpacing(30);
        mainLayout->addWidget(fontScaleTitle);
        mainLayout->addLayout(fontScaleRow);
        mainLayout->addWidget(volumeTitle);
        mainLayout->addLayout(volumeRow);
        mainLayout->addStretch();
    }

    void SettingsScreen::onFontScaleChanged(int sliderValue)
    {
        double scale = sliderValue / 100.0;
        m_fontScaleLabel->setText(QString::number(scale, 'f', 2) + "x");
        m_settings->setSubtitleFontScale(scale);
        emit subtitleFontScaleChanged(scale);
    }

    void SettingsScreen::onVolumeChanged(int sliderValue)
    {
        int percent = sliderValue * 100 / 200;
        m_volumeLabel->setText(QString::number(percent) + "%");
        m_settings->setDefaultVolume(sliderValue);
        emit defaultVolumeChanged(sliderValue);
    }
}
