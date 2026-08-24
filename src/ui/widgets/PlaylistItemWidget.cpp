#include "ui/widgets/PlaylistItemWidget.h"
#include "core/app/IconPath.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QPixmap>

namespace SumPlayer{

    static const QString kMenuStyle =
    "QMenu {"
    "  background-color: rgba(30, 30, 30, 245);"
    "  border: 1px solid rgba(255, 255, 255, 25);"
    "  border-radius: 12px;"
    "  padding: 6px 4px;"
    "}"
    "QMenu::item {"
    "  padding: 8px 16px 8px 10px;"
    "  border-radius: 6px;"
    "  font-size: 13px;"
    "  color: rgba(255, 255, 255, 230);"
    "  min-width: 160px;"
    "  icon-size: 17px;"
    "}"
    "QMenu::item:selected {"
    "  background-color: rgba(255, 255, 255, 30);"
    "  color: #ffffff;"
    "}"
    "QMenu::item:disabled {"
    "  color: rgba(255, 255, 255, 70);"
    "}"
    "QMenu::separator {"
    "  height: 1px;"
    "  background: rgba(255, 255, 255, 22);"
    "  margin: 6px 10px;"
    "}"
    "QMenu::icon {"
    "  padding-left: 6px;"
    "  padding-right: 4px;"
    "}"
    "QMenu::indicator {"
    "  width: 16px;"
    "  height: 16px;"
    "  margin-left: 6px;"
    "}"
    "QMenu::right-arrow {"
    "  width: 9px;"
    "  height: 9px;"
    "}";

    PlaylistItemWidget::PlaylistItemWidget(const QString& displayName,
                                        const QString& thumbnailPath,
                                        QWidget* parent)
        : QFrame(parent)
    

    {
        setStyleSheet(
            "QFrame { background-color: #161616; border: 1px solid #2a2a2a; border-radius: 8px; }"
            "QFrame:hover { background-color: #1e1e1e; }"
        );
        setCursor(Qt::PointingHandCursor);
        setFixedSize(280, 200);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);

        QLabel* thumb = new QLabel(this);
        thumb->setFixedHeight(140);
        thumb->setAlignment(Qt::AlignCenter);
        thumb->setStyleSheet("background-color: #0e0e0e; color: #444444; font-size: 20px; border: none;");

        QPixmap pixmap(thumbnailPath);
        if (!thumbnailPath.isEmpty() && !pixmap.isNull())
        {
            thumb->setPixmap(pixmap.scaled(280, 140, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            thumb->setScaledContents(false);
        }
        else
        {
            thumb->setText("\u25A4");
        }

        QWidget* infoBar = new QWidget(this);
        infoBar->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout* infoLayout = new QHBoxLayout(infoBar);
        infoBar->setContentsMargins(12,8,8,8);

        QLabel* nameLabel = new QLabel(displayName, this);
        nameLabel->setStyleSheet("color: #ffffff; font-size: 13px; border: none; background: transparent;");
        nameLabel->setWordWrap(false);

        QToolButton* menuButton = new QToolButton(this);
        menuButton->setIcon(QIcon(iconPath("more-vertical.svg")));
        menuButton->setIconSize(QSize(16, 16));
        menuButton->setFixedSize(28, 28);
        menuButton->setStyleSheet(
            "QToolButton { background: transparent; border: none; border-radius: 14px; }"
            "QToolButton:hover { background: rgba(255,255,255,20); }"
        );

        connect(menuButton , &QToolButton::clicked, this , &PlaylistItemWidget::showContextMenu);

        infoLayout->addWidget(nameLabel);
        infoLayout->addStretch();
        infoLayout->addWidget(menuButton);

        layout->addWidget(thumb);
        layout->addWidget(infoBar);
    }

    void PlaylistItemWidget::showContextMenu()
    {
        QMenu menu(this);
        menu.setStyleSheet(kMenuStyle);

        QAction* renameAction = menu.addAction(QIcon(iconPath("edit.svg")), "Rename");
        QAction* deleteAction = menu.addAction(QIcon(iconPath("trash.svg")), "Delete");
        QAction* chosen = menu.exec(QCursor::pos());

        if (chosen == renameAction) emit renameRequested();
        else if (chosen == deleteAction) emit deleteRequested();
    }

    void PlaylistItemWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        emit playRequested();
        QFrame::mouseDoubleClickEvent(event);
    }
}