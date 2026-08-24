#pragma once

#include <string> 
#include <optional>

namespace SumPlayer {

    class MediaFile
    {
        public:
        MediaFile();

        bool open(const std::string& filepath);

        bool getisvalid() const;
        std::string getpath() const;
        std::string getFormat() const;
        double getDuration() const;
        std::string getErrorMessage() const;

        private:
        std::string m_path;
        std::string m_format;
        double m_duration;
        bool m_isvalid;
        std::string m_errorMessage;
    };
    
}

