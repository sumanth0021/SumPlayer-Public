#include "ui/widgets/ClickableSlider.h"
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

namespace SumPlayer
{
    ClickableSlider::ClickableSlider(Qt::Orientation orientation, QWidget* parent)
        : QSlider(orientation, parent)
    {}

    void ClickableSlider::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            QStyleOptionSlider opt;
            initStyleOption(&opt);
            QRect handleRect = style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

            if (!handleRect.contains(event->pos()))
            {
                int newValue = QStyle::sliderValueFromPosition(
                    minimum(), maximum(), event->pos().x(), width());
                setValue(newValue);
                emit sliderMoved(newValue);
            }
        }
        QSlider::mousePressEvent(event);
    }
}