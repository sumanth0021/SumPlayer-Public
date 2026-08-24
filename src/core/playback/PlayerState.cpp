#include "core/playback/PlayerState.h"
#include <iostream>

namespace SumPlayer
{
    PlayerState::PlayerState():
        m_filename(""),
        m_width(0),
        m_height(0),
        m_duration(0.0),
        m_IsPlaying(false),
        m_position(0.0),
        m_volume(100),
        m_speed(1.0)

    {
        std::cout<< "[PlayerState] Created "<<std::endl;
    }

    PlayerState::~PlayerState()
    {
        std::cout<< "[PlayerState] Destroyed . File : " << m_filename <<std::endl;
    }

    void PlayerState::setFilename(const std::string& newFilename)
    {
        m_filename = newFilename;

    }

    void PlayerState:: setResolution(int newWidth, int newHeight)
    {
        m_width = newWidth;
        m_height = newHeight;
    }

    void PlayerState::setDuration(double newDuration)
    {
        m_duration = newDuration;
    }

    void PlayerState:: setVolume(int newVolume)
    {
        if (newVolume < 0 ) m_volume = 0;
        else if (newVolume > 100) m_volume = 100;
        else m_volume = newVolume;
    }

    void PlayerState:: setSpeed(double newSpeed)
    {
        if (newSpeed <= 0.0){
            std::cout << "Error : speed must be positive. "<<std::endl;
            return;
        }
        m_speed = newSpeed;
    }

    void PlayerState::Seek(double newPosition)
    {
        if (newPosition < 0.0)  m_position = 0.0;
        if (newPosition > m_duration) m_position = m_duration;
        m_position = newPosition;
        std::cout << "Seek to postion : " << m_position << std::endl;
    }

    void PlayerState::play()
    {
        m_IsPlaying = true;
        std::cout << "playing : " << m_filename << std::endl;
    }
           
    void PlayerState::pause()
    {
        m_IsPlaying = false;
        std::cout << "paused at : " << m_position << std::endl;
    }

    std::string PlayerState :: getFilename() const { return m_filename;}
    int PlayerState :: getWidth() const { return m_width;}
    int PlayerState :: getHeight() const { return m_height;}
    double PlayerState :: getDuration() const { return m_duration;}
    bool PlayerState :: getIsPlaying() const { return m_IsPlaying;}
    double PlayerState :: getPosition() const { return m_position;}
    int PlayerState ::getVolume() const { return m_volume;}
    double PlayerState :: getSpeed() const { return m_speed;}

    void PlayerState::printState()
    {
        std::cout << "---------------------------------------------"<<std::endl;
        std::cout << "Filename : " << m_filename << std::endl;
        std::cout << "size :" << m_width << "x" << m_height << std::endl;
        std::cout << "duration :" << m_duration << "s" << std::endl;
        std::cout << "Position : " << m_position << "s" << std::endl;
        std::cout << "volume :"  << m_volume <<  std::endl;
        std::cout << "Speed : " << m_speed << "x" << std::endl;
        std::cout << "IsPlaying : " << m_IsPlaying << std::endl;
        std::cout << "---------------------------------------------"<<std::endl;

    }

}








