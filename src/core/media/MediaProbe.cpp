#include "core/media/MediaProbe.h"
#include <iostream>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
}

namespace SumPlayer
{

MediaProbe::MediaProbe()
    : m_duration(0.0)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_frameRate(0.0)
    , m_audioChannels(0)
    , m_audioSampleRate(0)
{
}

MediaProbe::~MediaProbe()
{
}

bool MediaProbe::probe(const std::string& filepath)
{
    // Quick metadata peek only: open, read stream info, close before leaving.
    AVFormatContext* formatCtx = nullptr;

    int result = avformat_open_input(
        &formatCtx,
        filepath.c_str(),
        nullptr,
        nullptr
    );

    if (result < 0)
    {
        char errBuf[256];
        av_strerror(result, errBuf, sizeof(errBuf));
        std::cout << "ERROR: Could not open file: " << errBuf << std::endl;
        return false;
    }

    result = avformat_find_stream_info(formatCtx, nullptr);

    if (result < 0)
    {
        std::cout << "ERROR: Could not find stream info." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    m_filename = filepath;

    if (formatCtx->duration != AV_NOPTS_VALUE)
        m_duration = (double)formatCtx->duration / AV_TIME_BASE;

    int videoStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1,nullptr, 0);

    int audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1,nullptr, 0);

    // Video and audio are checked separately because some files are weird on purpose.
    if(videoStreamIndex >= 0){
        AVStream* Stream = formatCtx->streams[videoStreamIndex];
        AVCodecParameters* codecParams = Stream->codecpar;

        m_videoWidth = codecParams->width;
        m_videoHeight = codecParams->height;

        if (Stream->avg_frame_rate.den != 0)
        {
            m_frameRate = (double)Stream->avg_frame_rate.num / Stream->avg_frame_rate.den;
        }

        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (codec) std::cout << "[PlaybackEngine] Video decoder selected: " << codec->name << std::endl;
        if (codec){
            m_videoCodec = codec->long_name;
        }

    }

    if(audioStreamIndex >=0){
        AVStream* Stream = formatCtx->streams[audioStreamIndex];
        AVCodecParameters* params = Stream->codecpar;

        m_audioChannels = params->ch_layout.nb_channels;
        m_audioSampleRate = params->sample_rate;

        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (codec) std::cout << "[PlaybackEngine] Video decoder selected: " << codec->name << std::endl;
        if(codec) {
            m_audioCodec = codec->long_name;
        }
    }

    avformat_close_input(&formatCtx);
    return true;
}

std::string MediaProbe::getFilename()           const { return m_filename;        }
double      MediaProbe::getDuration()           const { return m_duration;        }
int         MediaProbe::getVideoWidth()         const { return m_videoWidth;      }
int         MediaProbe::getVideoHeight()        const { return m_videoHeight;     }
double      MediaProbe::getFrameRate()          const { return m_frameRate;       }
int         MediaProbe::getAudioChannels()      const { return m_audioChannels;   }
int         MediaProbe::getAudioSampleRate()    const { return m_audioSampleRate; }
std::string MediaProbe::getVideoCodec()         const { return m_videoCodec;      }
std::string MediaProbe::getAudioCodec()         const { return m_audioCodec;      }

}
