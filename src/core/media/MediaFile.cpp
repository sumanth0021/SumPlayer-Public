#include "core/media/MediaFile.h"
#include <iostream>

namespace SumPlayer{
    MediaFile::MediaFile():
        m_duration(0.0),
        m_isvalid(false),
        m_errorMessage("")
        {

        }

        bool MediaFile::open(const std::string& filepath)
        {
            if(filepath.empty()){
                m_errorMessage = "File Path is Empty";
                m_isvalid = false;
                return false;
            }

            std::string ext = " ";
            size_t dotPos = filepath.find_last_of('.');

            if (dotPos!= std::string::npos){
                ext = filepath.substr(dotPos);
            }

            if (ext != ".mp4" && ext != ".mkv" && ext != ".avi" && ext != ".mp3")
            {
                m_errorMessage = "unsupported file format :" + ext;
                m_isvalid = false;
                return false; 
            }

            if (filepath == "corrupt.mp4")
            {
                m_errorMessage = "File is corrupt";
                m_isvalid = false;
                return false;
            }

            m_path = filepath;
            m_format = ext;
            m_duration = 7245.6;
            m_isvalid = true;
            m_errorMessage = "";
            return true;

        }

        bool MediaFile::getisvalid() const { return m_isvalid; }
        std::string MediaFile::getpath() const { return m_path;}
        std::string MediaFile::getFormat() const{ return m_format;}
        double MediaFile:: getDuration() const {return m_duration;}
        std:: string MediaFile:: getErrorMessage() const {return m_errorMessage;}

}