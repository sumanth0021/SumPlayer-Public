#pragma once
#include <string>

namespace SumPlayer
{
struct PlaylistItem
{
    std::string filepath;
    std::string displayName;
    std::string thumbnailPath;
};

struct AudioTrackInfo
{
    int streamIndex;
    std::string language;
    std::string codecName;
};

}