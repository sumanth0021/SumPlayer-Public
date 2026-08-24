#pragma once
#include <string>
#include <QImage>

namespace SumPlayer
{
    class ThumbnailGenerator
    {
    public:
        static bool generate(const std::string& videoPath,
                              const std::string& outputPath);
    };
}
