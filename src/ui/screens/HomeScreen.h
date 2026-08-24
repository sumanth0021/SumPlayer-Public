#pragma once

#include <QWidget>

class QGridLayout;

namespace SumPlayer
{
    class PlaylistManager;

    class HomeScreen : public QWidget
    {
        Q_OBJECT
    public:
        explicit HomeScreen(PlaylistManager* playlistManager, QWidget* parent = nullptr);
        void refreshPlaylists();

    signals:
        void openFileRequested();
        void playlistOpenRequested(int playlistIndex);
        void settingsRequested();
        void playlistsChanged();

    private:
        void onCreatePlaylistClicked();
        void onPlaylistRenameRequested(int index);
        void onPlaylistDeleteRequested(int index);

        PlaylistManager* m_playlistManager;
        QGridLayout* m_playlistGrid;
    };
}
