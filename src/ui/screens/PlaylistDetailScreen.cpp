#include "ui/screens/PlaylistDetailScreen.h"
#include "ui/widgets/PlaylistItemWidget.h"
#include "core/playlist/PlaylistManager.h"
#include "ui/dialogs/CreatePlaylistDialog.h"
#include "core/media/ThumbnailGenerator.h"
#include "core/app/IconPath.h"

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>

namespace SumPlayer
{
    PlaylistDetailScreen::PlaylistDetailScreen(PlaylistManager* playlistManager, QWidget* parent)
        : QWidget(parent)
        , m_playlistManager(playlistManager)
        , m_currentPlaylistIndex(-1)
    {
        setStyleSheet("background-color: #111111;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(40, 25, 40, 25);

        QPushButton* backButton = new QPushButton("< Back to Home", this);
        backButton->setFlat(true);
        backButton->setCursor(Qt::PointingHandCursor);
        backButton->setStyleSheet("color: #999999; background: transparent; border: none; font-size: 13px; text-align: left;");
        connect(backButton, &QPushButton::clicked, this, &PlaylistDetailScreen::backRequested);

        m_nameLabel = new QLabel(this);
        m_nameLabel->setStyleSheet("color: #ffffff; font-size: 26px; font-weight: bold; border: none; background: transparent;");

        m_metaLabel = new QLabel(this);
        m_metaLabel->setStyleSheet("color: #888888; font-size: 12px; border: none; background: transparent;");

        QHBoxLayout* headerRow = new QHBoxLayout();
        QVBoxLayout* titleColumn = new QVBoxLayout();
        titleColumn->addWidget(m_nameLabel);
        titleColumn->addWidget(m_metaLabel);

        QPushButton* addFileButton = new QPushButton("  Add File to Playlist", this);
        addFileButton->setIcon(QIcon(iconPath("plus.svg")));
        addFileButton->setIconSize(QSize(16, 16));
        addFileButton->setCursor(Qt::PointingHandCursor);
        addFileButton->setFixedHeight(38);
        addFileButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #838282;"
            "  color: #111111;"
            "  border: none;"
            "  border-radius: 8px;"
            "  padding: 0 18px;"
            "  font-size: 13px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover { background-color: #e6e6e6; }"
            "QPushButton:pressed { background-color: #cccccc; }"
        );
        connect(addFileButton, &QPushButton::clicked, this, &PlaylistDetailScreen::onAddFileClicked);

        headerRow->addLayout(titleColumn);
        headerRow->addStretch();
        headerRow->addWidget(addFileButton, 0, Qt::AlignTop);

        m_emptyLabel = new QLabel("No video in this playlist", this);
        m_emptyLabel->setStyleSheet("color: #555555; font-size: 14px; border: none; background: transparent;");
        m_emptyLabel->setAlignment(Qt::AlignCenter);

        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("background: transparent; border: none;");

        QWidget* gridContainer = new QWidget();
        gridContainer->setStyleSheet("background: transparent; border: none;");
        m_itemsGrid = new QGridLayout(gridContainer);
        m_itemsGrid->setSpacing(20);
        m_itemsGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        scrollArea->setWidget(gridContainer);

        mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(headerRow);
        mainLayout->addSpacing(15);
        mainLayout->addWidget(m_emptyLabel);
        mainLayout->addWidget(scrollArea);
    }

    void PlaylistDetailScreen::showPlaylist(int playlistIndex)
    {
        m_currentPlaylistIndex = playlistIndex;
        refreshItems();
    }

    void PlaylistDetailScreen::refreshItems()
    {
        Playlist* pl = m_playlistManager->getPlaylist(m_currentPlaylistIndex);
        if (!pl) return;

        m_nameLabel->setText(QString::fromStdString(pl->getName()));
        m_metaLabel->setText(QString("%1 videos \u00B7 Created %2")
            .arg(pl->getSize())
            .arg(QString::fromStdString(pl->getDateCreated())));

        QLayoutItem* child;
        while ((child = m_itemsGrid->takeAt(0)) != nullptr)
        {
            delete child->widget();
            delete child;
        }

        m_emptyLabel->setVisible(pl->getSize() == 0);

        const int columns = 4;
        for (int i = 0; i < pl->getSize(); i++)
        {
            const PlaylistItem* item = pl->getItem(i);
            if (!item) continue;

            PlaylistItemWidget* widget = new PlaylistItemWidget(
                QString::fromStdString(item->displayName),
                QString::fromStdString(item->thumbnailPath)
            );

            connect(widget, &PlaylistItemWidget::playRequested, this, [this, i]()
            {
                Playlist* current = m_playlistManager->getPlaylist(m_currentPlaylistIndex);
                if (!current) return;
                const PlaylistItem* it = current->getItem(i);
                if (it) emit playRequested(i, QString::fromStdString(it->filepath));
            });
            
            connect(widget, &PlaylistItemWidget::renameRequested, this, [this, i]()
            {
                onItemRenameRequested(i);
            });

            connect(widget, &PlaylistItemWidget::deleteRequested, this, [this, i]()
            {
                onItemDeleteRequested(i);
            });

            m_itemsGrid->addWidget(widget, i / columns, i % columns);
        }
    }

    void PlaylistDetailScreen::onAddFileClicked()
    {
        Playlist* pl = m_playlistManager->getPlaylist(m_currentPlaylistIndex);
        if (!pl) return;

        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "Add Files to Playlist",
            QString(),
            "Media Files (*.mp4 *.mkv *.avi *.mov *.mp3 *.flac *.wav);;All Files (*.*)"
        );

        if (files.isEmpty()) return;

        for (const QString& f : files)
        {
            pl->addItem(f.toStdString());
        }

        refreshItems();
    }

    void PlaylistDetailScreen::onItemRenameRequested(int itemIndex)
        {
            Playlist* pl = m_playlistManager->getPlaylist(m_currentPlaylistIndex);
            if (!pl) return;

            const PlaylistItem* item = pl->getItem(itemIndex);
            if (!item) return;

            CreatePlaylistDialog dialog("Rename Video",
                                        QString::fromStdString(item->displayName), this);
            if (dialog.exec() == QDialog::Accepted)
            {
                QString name = dialog.playlistName();
                if (!name.isEmpty())
                {
                    pl->renameItem(itemIndex, name.toStdString());
                    QTimer::singleShot(0, this, [this]() { refreshItems(); });
                }
            }
        }

    void PlaylistDetailScreen::onItemDeleteRequested(int itemIndex)
    {
        Playlist* pl = m_playlistManager->getPlaylist(m_currentPlaylistIndex);
        if (!pl) return;
        pl->removeItem(itemIndex);
        QTimer::singleShot(0, this, [this]() { refreshItems(); });
    }
}