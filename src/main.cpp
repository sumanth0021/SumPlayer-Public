// SUM PLAYER - A native media player built with Qt, FFmpeg, and libass
// Copyright (C) 2026 Sumanth

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#include "core/media/MediaFile.h"
#include "core/media/MediaProbe.h"
#include "core/playback/PlayerState.h"
#include "core/playlist/Playlist.h"
#include "core/playlist/PlaylistManager.h"
#include "ui/windows/MainWindow.h"

#include <QApplication>
#include <QStyleFactory>

#include <ass/ass.h>

#include <iostream>
#include <memory>


int main(int argc, char *argv[])
{
{
    SumPlayer::PlaylistManager manager;

    SumPlayer::Playlist& favorites = manager.createPlaylist("Favorites");
    favorites.addItem("C:/Users/nonu0/Downloads/movie.mp4");
    favorites.addItem("C:/Users/nonu0/Downloads/documentary.mkv");

    SumPlayer::Playlist& music = manager.createPlaylist("Music");
    music.addItem("C:/Users/nonu0/Downloads/song.mp3");

    std::cout << "Playlist count: " << manager.getCount() << std::endl;

    for (int i = 0; i < manager.getCount(); i++)
    {
        SumPlayer::Playlist* pl = manager.getPlaylist(i);
        std::cout << "Playlist [" << i << "]: " << pl->getName()
                  << " (" << pl->getSize() << " items)" << std::endl;

        for (int j = 0; j < pl->getSize(); j++)
        {
            const SumPlayer::PlaylistItem* item = pl->getItem(j);
            std::cout << "  - " << item->displayName
                      << "  (" << item->filepath << ")" << std::endl;
        }
    }

    favorites.renameItem(0, "My Favorite Movie");
    std::cout << "After rename: "
              << favorites.getItem(0)->displayName << std::endl;
}

{
    ASS_Library* assLib = ass_library_init();
    if (assLib)
    {
        std::cout << "[libass test] ass_library_init() succeeded." << std::endl;

        ASS_Renderer* assRenderer = ass_renderer_init(assLib);
        if (assRenderer)
        {
            std::cout << "[libass test] ass_renderer_init() succeeded." << std::endl;
            ass_renderer_done(assRenderer);
        }
        else
        {
            std::cout << "[libass test] ass_renderer_init() FAILED." << std::endl;
        }

        ass_library_done(assLib);
    }
    else
    {
        std::cout << "[libass test] ass_library_init() FAILED." << std::endl;
    }
}


    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    QApplication::setApplicationName("SumPlayer");
    QApplication::setApplicationVersion("1.0.o");
    QApplication::setOrganizationName("SumPlayer");

 
    SumPlayer::MainWindow window;
    window.show();
    return app.exec();
    
}
