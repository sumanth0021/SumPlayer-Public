#pragma once
#include <QFrame>

class QLabel;

namespace SumPlayer
{
    class ActionCard : public QFrame
    {
        Q_OBJECT
    public:
        ActionCard(const QString& iconFile, const QString& title, const QString& subtitle, QWidget* parent = nullptr);

    signals:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
    };
}