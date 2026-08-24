#include "core/playlist/PlaylistStore.h"
#include "core/playlist/PlaylistManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

namespace SumPlayer
{
    std::string PlaylistStore::getStoreFilePath()
    {
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(cacheDir);
        return (cacheDir + "/playlists.json").toStdString();
    }

    void PlaylistStore::save(const PlaylistManager& manager)
    {
        // Tiny JSON cache: enough to remember playlists without inventing a database yet.
        QJsonArray playlistsArray;

        for (int i = 0; i < manager.getCount(); i++)
        {
            const Playlist* pl = manager.getPlaylist(i);
            if (!pl) continue;

            QJsonObject plObj;
            plObj["name"] = QString::fromStdString(pl->getName());
            plObj["dateCreated"] = QString::fromStdString(pl->getDateCreated());

            QJsonArray itemsArray;
            for (int j = 0; j < pl->getSize(); j++)
            {
                const PlaylistItem* item = pl->getItem(j);
                if (!item) continue;

                QJsonObject itemObj;
                itemObj["filepath"] = QString::fromStdString(item->filepath);
                itemObj["displayName"] = QString::fromStdString(item->displayName);
                itemObj["thumbnailPath"] = QString::fromStdString(item->thumbnailPath);
                itemsArray.append(itemObj);
            }
            plObj["items"] = itemsArray;

            playlistsArray.append(plObj);
        }

        QJsonObject root;
        root["playlists"] = playlistsArray;
        QJsonDocument doc(root);

        QFile file(QString::fromStdString(getStoreFilePath()));
        if (!file.open(QIODevice::WriteOnly))
        {
            std::cout << "[PlaylistStore] Failed to save playlists." << std::endl;
            return;
        }
        file.write(doc.toJson());
        file.close();

        std::cout << "[PlaylistStore] Saved " << manager.getCount() << " playlist(s)." << std::endl;
    }

    void PlaylistStore::load(PlaylistManager& manager)
    {
        QFile file(QString::fromStdString(getStoreFilePath()));
        if (!file.open(QIODevice::ReadOnly))
        {
            std::cout << "[PlaylistStore] No saved playlists found." << std::endl;
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject())
        {
            std::cout << "[PlaylistStore] Playlist cache file is corrupted." << std::endl;
            return;
        }

        QJsonArray playlistsArray = doc.object().value("playlists").toArray();

        for (const QJsonValue& plVal : playlistsArray)
        {
            QJsonObject plObj = plVal.toObject();
            std::string name = plObj.value("name").toString().toStdString();

            Playlist& pl = manager.createPlaylist(name);

            QJsonArray itemsArray = plObj.value("items").toArray();
            for (const QJsonValue& itemVal : itemsArray)
            {
                QJsonObject itemObj = itemVal.toObject();
                pl.addRestoredItem(
                    itemObj.value("filepath").toString().toStdString(),
                    itemObj.value("displayName").toString().toStdString(),
                    itemObj.value("thumbnailPath").toString().toStdString()
                );
            }
        }

        std::cout << "[PlaylistStore] Loaded " << manager.getCount() << " playlist(s)." << std::endl;
    }
}
