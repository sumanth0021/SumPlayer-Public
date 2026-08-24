#include "ui/screens/HomeScreen.h"
#include "ui/widgets/ActionCard.h"
#include "ui/widgets/PlaylistCardWidget.h"
#include "ui/dialogs/CreatePlaylistDialog.h"
#include "core/app/IconPath.h"
#include "core/playlist/PlaylistManager.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>


namespace SumPlayer
{
    HomeScreen::HomeScreen(PlaylistManager* playlistManager, QWidget* parent)
        : QWidget(parent)
        , m_playlistManager(playlistManager)
        , m_playlistGrid(nullptr)
    {
        setStyleSheet("background-color: #111111;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(40, 30, 40, 30);

        QLabel* homeTitle = new QLabel("Home", this);
        homeTitle->setStyleSheet(
            "color: #ffffff; font-size: 32px; font-weight: bold; border: none; background: transparent;"
        );

        QPushButton* settingsButton = new QPushButton(this);
        settingsButton->setIcon(QIcon(iconPath("settings.svg")));
        settingsButton->setIconSize(QSize(18, 18));
        settingsButton->setFixedSize(40, 40);
        settingsButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #2a2a2a;"
            "  border: none;"
            "  border-radius: 20px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3a3a3a;"
            "}"
        );

        connect(settingsButton, &QPushButton::clicked, this, &HomeScreen::settingsRequested);

        QHBoxLayout* topRow = new QHBoxLayout();
        ActionCard* openCard = new ActionCard(
        "folder.svg", "Open Video", "Play a local video file", this);
        ActionCard* createCard = new ActionCard(
        "plus.svg", "Create Playlist", "Organize your videos", this);

        topRow->addWidget(openCard);
        topRow->addWidget(createCard);

        connect(openCard, &ActionCard::clicked,
                this, &HomeScreen::openFileRequested);
        connect(createCard, &ActionCard::clicked,
                this, &HomeScreen::onCreatePlaylistClicked);

        QLabel* playlistsHeader = new QLabel("YOUR PLAYLISTS", this);
        playlistsHeader->setStyleSheet(
            "color: #666666; font-size: 11px; font-weight: bold; border: none; background: transparent;"
        );

        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("background: transparent; border: none;");

        // The cards live in a plain grid inside the scroll area, easy to rebuild when things change.
        QWidget* gridContainer = new QWidget();
        gridContainer->setStyleSheet("background: transparent; border: none;");
        m_playlistGrid = new QGridLayout(gridContainer);
        m_playlistGrid->setSpacing(20);
        m_playlistGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        scrollArea->setWidget(gridContainer);

        QHBoxLayout* titleRow = new QHBoxLayout();
        titleRow->addWidget(homeTitle);
        titleRow->addStretch();
        titleRow->addWidget(settingsButton);

        mainLayout->addLayout(titleRow);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(topRow);
        mainLayout->addSpacing(25);
        mainLayout->addWidget(playlistsHeader);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(scrollArea);

        refreshPlaylists();
    }

    void HomeScreen::refreshPlaylists()
    {
        // Full rebuild is simple here and keeps rename/delete from leaving old widgets behind.
        QLayoutItem* child;
        while ((child = m_playlistGrid->takeAt(0)) != nullptr)
        {
            delete child->widget();
            delete child;
        }

        const int columns = 4;

        for (int i = 0; i < m_playlistManager->getCount(); i++)
        {
            Playlist* pl = m_playlistManager->getPlaylist(i);
            if (!pl) continue;

            QString cardThumb = "";
            if (!pl->isEmpty())
            {
                // For now the first item gets to be the playlist cover.
                const PlaylistItem* first = pl->getItem(0);
                if (first) cardThumb = QString::fromStdString(first->thumbnailPath);
            }

            PlaylistCardWidget* card = new PlaylistCardWidget(
                QString::fromStdString(pl->getName()),
                pl->getSize(),
                QString::fromStdString(pl->getDateCreated()),
                cardThumb
            );

            connect(card, &PlaylistCardWidget::opened, this, [this, i]()
            {
                emit playlistOpenRequested(i);
            });

            connect(card, &PlaylistCardWidget::renameRequested, this, [this, i]()
            {
                onPlaylistRenameRequested(i);
            });

            connect(card, &PlaylistCardWidget::deleteRequested, this, [this, i]()
            {
                onPlaylistDeleteRequested(i);
            });

            m_playlistGrid->addWidget(card, i / columns, i % columns);

            emit playlistsChanged();
        }
    }

    void HomeScreen::onCreatePlaylistClicked()
    {
        CreatePlaylistDialog dialog("New Playlist", "", this);
        if (dialog.exec() == QDialog::Accepted)
        {
            QString name = dialog.playlistName();
            if (!name.isEmpty())
            {
                m_playlistManager->createPlaylist(name.toStdString());
                refreshPlaylists();
            }
        }
    }

    void HomeScreen::onPlaylistRenameRequested(int index)
    {
        Playlist* pl = m_playlistManager->getPlaylist(index);
        if (!pl) return;

        CreatePlaylistDialog dialog("Rename Playlist",
                                     QString::fromStdString(pl->getName()), this);
        if (dialog.exec() == QDialog::Accepted)
        {
            QString name = dialog.playlistName();
            if (!name.isEmpty())
            {
                pl->setName(name.toStdString());
                // Let the menu/dialog finish its moment before we rebuild the whole grid.
                QTimer::singleShot(0, this, [this]() { refreshPlaylists();});
            }
        }
    }

    void HomeScreen::onPlaylistDeleteRequested(int index)
    {
        m_playlistManager->removePlaylist(index);
        // Same tiny delay as rename, so Qt is not holding a widget while we delete it.
        QTimer::singleShot(0, this, [this]() { refreshPlaylists();});
    }
}
