#pragma once

#include <string>
#include <functional>
#include <atomic>

struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct SwrContext;

namespace SumPlayer
{

class AudioDecoder
{
public:
    using AudioCallback = std::function<void(const uint8_t*, int)>;

    AudioDecoder();
    ~AudioDecoder();

    bool open(const std::string& filepath);
    void setAudioCallback(AudioCallback callback);
    void start();
    void stop();
    bool isOpen()        const;
    int  getSampleRate() const;
    int  getChannels()   const;

    bool decodeNextChunk();

private:
    AVFormatContext* m_formatCtx;
    AVCodecContext*  m_codecCtx;
    AVPacket*        m_packet;
    AVFrame*         m_frame;
    SwrContext*      m_swrCtx;

    int  m_audioStreamIndex;
    int  m_sampleRate;
    int  m_channels;
    bool m_isOpen;

    AudioCallback m_audioCallback;
};

}
