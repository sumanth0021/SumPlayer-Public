#pragma once
#include <string>

namespace SumPlayer
{
    class PlayerState{
        public:
        PlayerState();
        ~PlayerState();


        void setFilename(const std::string& newFilename);
        void setResolution(int newwidth, int newHeight);
        void setDuration(double newDuration);
        void setVolume(int newVolume);
        void setSpeed(double newSpeed);
        void Seek(double newPosition);

        std::string getFilename() const;
        int getWidth() const;
        int getHeight() const;
        double getDuration() const;
        bool getIsPlaying() const;
        double getPosition() const;
        int getVolume() const;
        double getSpeed() const;

        void play();
        void pause();

        void printState();

    private:
    
        std::string m_filename;
        int  m_width;
        int m_height;
        double m_duration;
        bool m_IsPlaying;
        double m_position;
        int m_volume;
        double m_speed;



 
    };
}
