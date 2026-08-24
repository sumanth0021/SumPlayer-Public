#pragma once

#include <QWidget>
#include <QPixmap>

class QLabel;
class QGridLayout;

namespace SumPlayer{

    class PlaylistManager;

    class PlaylistDetailScreen: public QWidget
    {
        Q_OBJECT

        public:
            explicit PlaylistDetailScreen(PlaylistManager* playlistManager, QWidget* parent = nullptr);
            void showPlaylist(int playlistIndex);
            int getCurrentPlaylistIndex() const { return m_currentPlaylistIndex; }

        signals:
            void backRequested();
            void playRequested(int itemIndex, const QString& filepath);
            void playlistsChanged();

        private:
            void refreshItems();
            void onAddFileClicked();
            void onItemRenameRequested(int itemIndex);
            void onItemDeleteRequested(int itemIndex);

            PlaylistManager* m_playlistManager;
            int m_currentPlaylistIndex;

            QLabel* m_nameLabel;
            QLabel* m_metaLabel;
            QGridLayout* m_itemsGrid;
            QLabel* m_emptyLabel;
    };
}