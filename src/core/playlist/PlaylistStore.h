#pragma once
#include <string>

namespace SumPlayer
{
    class PlaylistManager;

    class PlaylistStore
    {
    public:
        static void save(const PlaylistManager& manager);
        static void load(PlaylistManager& manager);

    private:
        static std::string getStoreFilePath();
    };
}