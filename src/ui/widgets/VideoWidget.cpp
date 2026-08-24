#include "ui/widgets/VideoWidget.h"
#include <QPainter>
#include <QMutexLocker>

namespace SumPlayer
{

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    , m_aspectMode(AspectRatioMode::Fit)
{
    setStyleSheet("background-color: #000000;");
    setAttribute(Qt::WA_OpaquePaintEvent);
}

VideoWidget::~VideoWidget()
{
}

void VideoWidget::displayFrame(
    const uint8_t* data,
    int            width,
    int            height,
    int            linesize)
{
    QMutexLocker locker(&m_frameMutex);

    m_currentFrame = QImage(data, width, height, linesize,
                            QImage::Format_RGB888).copy();

    update();
}

void VideoWidget::setAspectRatioMode(AspectRatioMode mode)
{
    m_aspectMode = mode;
    update();
}

AspectRatioMode VideoWidget::getAspectRatioMode() const
{
    return m_aspectMode;
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), Qt::black);

    if (m_currentFrame.isNull())
        return;

    QMutexLocker locker(&m_frameMutex);

    QSize widgetSize = size();

    // Fit keeps the full frame visible; Fill crops a little so the screen is covered.
    double targetRatio;
    bool expanding = false;

    switch (m_aspectMode)
    {
        case AspectRatioMode::Fill:
            targetRatio = (double)m_currentFrame.width() / m_currentFrame.height();
            expanding = true;
            break;
        case AspectRatioMode::Ratio_16_9:
            targetRatio = 16.0 / 9.0;
            break;
        case AspectRatioMode::Ratio_4_3:
            targetRatio = 4.0 / 3.0;
            break;
        case AspectRatioMode::Ratio_1_1:
            targetRatio = 1.0;
            break;
        case AspectRatioMode::Ratio_2_35_1:
            targetRatio = 2.35;
            break;
        case AspectRatioMode::Fit:
        default:
            targetRatio = (double)m_currentFrame.width() / m_currentFrame.height();
            break;
    }

    double widgetRatio = (double)widgetSize.width() / widgetSize.height();

    int destWidth, destHeight;

    bool widthIsLimiting = expanding
        ? (widgetRatio > targetRatio)
        : (widgetRatio < targetRatio);

    if (widthIsLimiting)
    {
        destWidth  = widgetSize.width();
        destHeight = (int)(destWidth / targetRatio);
    }
    else
    {
        destHeight = widgetSize.height();
        destWidth  = (int)(destHeight * targetRatio);
    }

    int x = (widgetSize.width()  - destWidth)  / 2;
    int y = (widgetSize.height() - destHeight) / 2;

    QRect targetRect(x, y, destWidth, destHeight);

    if (expanding)
    {
        // Fill can draw outside the widget on purpose, so clip it before painting.
        painter.setClipRect(rect());
    }

    painter.drawImage(targetRect, m_currentFrame);
}

}
