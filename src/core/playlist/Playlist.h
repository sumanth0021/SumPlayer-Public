#pragma once

#include <string>
#include <vector>
#include "core/playlist/PlaylistItem.h"
#include "core/media/ThumbnailGenerator.h"

namespace SumPlayer
{
    class Playlist
    {
        public:

         explicit Playlist(const std::string& name);

         void addItem(const std::string& filepath);
         void removeItem(int index);
         void renameItem(int index, const std::string& newName);

         void addRestoredItem(const std::string& filepath, const std::string& displayName, const std::string& thumbnailPath);

         bool next();
         bool prevoius();

         const PlaylistItem* getCurrentItem() const;
         const PlaylistItem* getItem(int index) const;
         int getCurrentIndex() const;
         int getSize() const;
         bool isEmpty() const;

         std::string getDateCreated() const;
         std::string getName() const;
         void setName(const std::string& name);

         private:
         std::vector<PlaylistItem> m_items;
         std::string m_dateCreated;
         std::string m_name;
         int m_currentIndex;
    };
}

