#include "core/playlist/PlaylistManager.h"

namespace SumPlayer
{
    Playlist& PlaylistManager::createPlaylist(const std::string& name)
    {
        m_playlists.push_back(std::make_unique<Playlist>(name));
        return *m_playlists.back();
    }

    void PlaylistManager::removePlaylist(int index)
    {
        if(index < 0 || index >= int(m_playlists.size()))
        {
            return;
        }
        m_playlists.erase(m_playlists.begin() + index);
    }

    Playlist* PlaylistManager::getPlaylist(int index)
    {
        if (index < 0 || index >= (int)m_playlists.size())
        {
            return nullptr;
        }

        return m_playlists[index].get();
    }
    
    const Playlist* PlaylistManager::getPlaylist(int index) const
    {
        if (index < 0 || index >= (int)m_playlists.size())
        {
            return nullptr;
        }

        return m_playlists[index].get();
    }
    

    int PlaylistManager::getCount() const
    {
        return (int)(m_playlists.size());
    }
}