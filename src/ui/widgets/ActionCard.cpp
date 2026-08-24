#include "ui/widgets/ActionCard.h"
#include "core/app/IconPath.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace SumPlayer
{
    ActionCard::ActionCard(const QString& iconFile, const QString& title, const QString& subtitle, QWidget* parent)
        : QFrame(parent)
    {
        setStyleSheet(
            "QFrame {"
            "  background-color: #161616;"
            "  border: 1px solid #2a2a2a;"
            "  border-radius: 10px;"
            "}"
            "QFrame:hover {"
            "  background-color: #1c1c1c;"
            "  border: 1px solid #3a3a3a;"
            "}"
        );
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(84);

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(20, 15, 20, 15);

        QLabel* icon = new QLabel(this);
        icon->setFixedSize(44, 44);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(QIcon(iconPath(iconFile)).pixmap(20, 20));
        icon->setStyleSheet(
            "background-color: #262626; border: none; border-radius: 10px;"
        );

        QVBoxLayout* textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);

        QLabel* titleLabel = new QLabel(title, this);
        titleLabel->setStyleSheet(
            "color: #ffffff; font-size: 15px; font-weight: 600; border: none; background: transparent;"
        );

        QLabel* subtitleLabel = new QLabel(subtitle, this);
        subtitleLabel->setStyleSheet(
            "color: #888888; font-size: 12px; border: none; background: transparent;"
        );

        textLayout->addWidget(titleLabel);
        textLayout->addWidget(subtitleLabel);

        layout->addWidget(icon);
        layout->addSpacing(15);
        layout->addLayout(textLayout);
        layout->addStretch();
    }

    void ActionCard::mousePressEvent(QMouseEvent* event)
    {
        emit clicked();
        QFrame::mousePressEvent(event);
    }
}