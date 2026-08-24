#include "core/playlist/Playlist.h"
#include "core/media/ThumbnailGenerator.h"

#include <QStandardPaths>
#include <QDir>

#include <ctime>
#include <functional>

namespace SumPlayer
{

    Playlist::Playlist(const std::string& name):
        m_name(name),
        m_currentIndex(-1)
        {
            std::time_t t = std::time(nullptr);
            std::tm tm;
            localtime_s(&tm, &t);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%b %d, %Y", &tm);
            m_dateCreated = buf;
        }

        std::string Playlist::getDateCreated() const
        {
            return m_dateCreated;
        }


        void Playlist::addRestoredItem(const std::string& filepath, const std::string& displayName, const std::string& thumbnailPath)
        {
            PlaylistItem item;
            item.filepath = filepath;
            item.displayName = displayName;
            item.thumbnailPath = thumbnailPath;

            m_items.push_back(item);

            if (m_currentIndex == -1)
            {
                m_currentIndex = 0;
            }
        }

        void Playlist::addItem(const std::string& filepath)
            {
                PlaylistItem item;
            item.filepath = filepath;

            size_t slashPos = filepath.find_last_of("/\\");
            item.displayName = (slashPos != std::string::npos)
                ? filepath.substr(slashPos + 1)
                : filepath;

            QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QString thumbDir = appDataDir + "/thumbnails";
            QDir().mkpath(thumbDir);

            size_t hash = std::hash<std::string>{}(filepath);
            QString thumbPath = QString("%1/%2.jpg").arg(thumbDir).arg(hash, 0, 16);

            if (ThumbnailGenerator::generate(filepath, thumbPath.toStdString()))
            {
                item.thumbnailPath = thumbPath.toStdString();
            }

            m_items.push_back(item);

            if (m_currentIndex == -1)
            {
                m_currentIndex = 0;
            }
        }



        void Playlist::removeItem(int index)
        {
            if(index < 0 || index >= int(m_items.size()))
            {
                return;
            } 
            m_items.erase(m_items.begin() + index);
            if(m_items.empty())
            {
                m_currentIndex = -1;
            }
            else if(m_currentIndex >= int(m_items.size()))
            {
                m_currentIndex = int(m_items.size()) -1;
            }

        }

        void Playlist::renameItem(int index, const std::string& newName)
        {
            if(index <0 || index >= int(m_items.size()))
            {
                return;
            }
            m_items[index].displayName = newName;
        }

        bool Playlist::next()
        {
            if(m_currentIndex + 1 < int(m_items.size()))
            {
                return false;
            }
            m_currentIndex++;
            return true;
        }

        bool Playlist::prevoius()
        {
            if(m_currentIndex < 0) return false;

            m_currentIndex--;
            return true;
        }

        const PlaylistItem* Playlist::getCurrentItem() const
        {
            if(m_currentIndex == -1 || m_items.empty())
            {
                return nullptr;
            }

            return &m_items[m_currentIndex];
        }

        const PlaylistItem* Playlist::getItem(int index) const
        {
            if(index < 0 || index >= int(m_items.size()))
            {
                return nullptr;
            }
            return &m_items[index];
        }

        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QString thumbDir = cacheDir + "/thumbnails";

        int Playlist::getCurrentIndex() const { return m_currentIndex;}
        int Playlist::getSize() const { return int(m_items.size());}
        bool Playlist::isEmpty() const { return m_items.empty();}

        std::string Playlist::getName() const { return m_name;}
        void Playlist::setName(const std::string& name) { m_name = name;}   

}


