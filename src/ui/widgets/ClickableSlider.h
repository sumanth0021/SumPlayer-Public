#pragma once
#include <QSlider>

namespace SumPlayer
{
    class ClickableSlider : public QSlider
    {
        Q_OBJECT
    public:
        explicit ClickableSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
    };
}