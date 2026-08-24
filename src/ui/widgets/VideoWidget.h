#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>

namespace SumPlayer
{

enum class AspectRatioMode
{
    Fit,        
    Fill,      
    Ratio_16_9,
    Ratio_4_3,
    Ratio_1_1,
    Ratio_2_35_1
};

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget* parent = nullptr);
    ~VideoWidget();

    void displayFrame(const uint8_t* data, int width, int height, int linesize);
    void setAspectRatioMode(AspectRatioMode mode);
    AspectRatioMode getAspectRatioMode() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_currentFrame;
    QMutex m_frameMutex;
    AspectRatioMode m_aspectMode;
};

}