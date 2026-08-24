#pragma once
#include <string>

namespace SumPlayer{

    class MediaProbe{
        public:
            MediaProbe();
            ~MediaProbe();

            bool probe(const std::string& filePath);

            std::string getFilename() const;
            double getDuration() const;
            int getVideoWidth()  const;
            int getVideoHeight() const;
            double getFrameRate() const;
            int getAudioChannels() const;
            int getAudioSampleRate() const;
            std::string getVideoCodec() const;
            std::string getAudioCodec() const;

        private:
            std::string m_filename;
            double m_duration;
            int m_videoWidth;
            int m_videoHeight;
            double m_frameRate;
            int m_audioChannels;
            int m_audioSampleRate;
            std::string m_videoCodec;
            std::string m_audioCodec;

    };
}