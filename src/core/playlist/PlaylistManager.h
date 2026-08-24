#pragma once
#include <memory>
#include "core/playlist/Playlist.h"

namespace SumPlayer
{
    class PlaylistManager
    {
        public:
            Playlist& createPlaylist(const std::string& name);
            void removePlaylist(int index);
            Playlist* getPlaylist(int index);
            const Playlist* getPlaylist(int index) const;
            int getCount() const;
        
        private:
            std::vector<std::unique_ptr<Playlist>> m_playlists;
    };
}