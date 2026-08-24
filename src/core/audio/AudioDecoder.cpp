#include "core/audio/AudioDecoder.h"
#include <iostream>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
}

namespace SumPlayer
{

AudioDecoder::AudioDecoder()
    : m_formatCtx(nullptr)
    , m_codecCtx(nullptr)
    , m_packet(nullptr)
    , m_frame(nullptr)
    , m_swrCtx(nullptr)
    , m_audioStreamIndex(-1)
    , m_sampleRate(44100)
    , m_channels(2)
    , m_isOpen(false)
{}

AudioDecoder::~AudioDecoder()
{
    stop();
}

bool AudioDecoder::open(const std::string& filepath)
{
    int result = avformat_open_input(
        &m_formatCtx, filepath.c_str(), nullptr, nullptr);
    if (result < 0) return false;

    result = avformat_find_stream_info(m_formatCtx, nullptr);
    if (result < 0)
    {
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_audioStreamIndex = av_find_best_stream(
        m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (m_audioStreamIndex < 0)
    {
        avformat_close_input(&m_formatCtx);
        return false;
    }

    AVStream*          stream = m_formatCtx->streams[m_audioStreamIndex];
    AVCodecParameters* params = stream->codecpar;
    const AVCodec*     codec  = avcodec_find_decoder(params->codec_id);

    if (!codec) return false;

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, params);
    avcodec_open2(m_codecCtx, codec, nullptr);

    m_sampleRate = params->sample_rate;
    m_channels   = params->ch_layout.nb_channels;

    m_swrCtx = swr_alloc();

    AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;

    av_opt_set_chlayout  (m_swrCtx, "in_chlayout",
                          &params->ch_layout, 0);
    av_opt_set_int       (m_swrCtx, "in_sample_rate",
                           m_sampleRate, 0);
    av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt",
                          (AVSampleFormat)params->format, 0);
    av_opt_set_chlayout  (m_swrCtx, "out_chlayout",
                          &outLayout, 0);
    av_opt_set_int       (m_swrCtx, "out_sample_rate",
                           44100, 0);
    av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt",
                           AV_SAMPLE_FMT_S16, 0);

    swr_init(m_swrCtx);

    m_packet = av_packet_alloc();
    m_frame  = av_frame_alloc();
    m_isOpen = true;

    std::cout << "[AudioDecoder] Opened. "
              << m_sampleRate << "Hz "
              << m_channels << "ch" << std::endl;
    return true;
}

void AudioDecoder::setAudioCallback(AudioCallback callback)
{
    m_audioCallback = callback;
}

void AudioDecoder::start() {}

bool AudioDecoder::decodeNextChunk()
{
    if (!m_isOpen) return false;

    while (av_read_frame(m_formatCtx, m_packet) >= 0)
    {
        if (m_packet->stream_index != m_audioStreamIndex)
        {
            av_packet_unref(m_packet);
            continue;
        }

        int result = avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);
        if (result < 0) continue;

        result = avcodec_receive_frame(m_codecCtx, m_frame);
        if (result == AVERROR(EAGAIN)) continue;
        if (result < 0) return false;

        int outSamples = swr_get_out_samples(
            m_swrCtx, m_frame->nb_samples);

        int bufSize = outSamples * 2 * sizeof(int16_t);
        std::vector<uint8_t> buffer(bufSize);
        uint8_t* outPtr = buffer.data();

        int converted = swr_convert(
            m_swrCtx,
            &outPtr,
            outSamples,
            (const uint8_t**)m_frame->data,
            m_frame->nb_samples
        );

        if (converted > 0 && m_audioCallback)
        {
            int actualSize = converted * 2 * sizeof(int16_t);
            std::cout << "[Audio] Writing " << actualSize << " bytes" << std::endl;
            m_audioCallback(buffer.data(), actualSize);
        }

        av_frame_unref(m_frame);
        return true;
    }

    return false;
}

void AudioDecoder::stop()
{
    if (m_swrCtx)    { swr_free(&m_swrCtx);                m_swrCtx    = nullptr; }
    if (m_frame)     { av_frame_free(&m_frame);             m_frame     = nullptr; }
    if (m_packet)    { av_packet_free(&m_packet);           m_packet    = nullptr; }
    if (m_codecCtx)  { avcodec_free_context(&m_codecCtx);  m_codecCtx  = nullptr; }
    if (m_formatCtx) { avformat_close_input(&m_formatCtx); m_formatCtx = nullptr; }
    m_isOpen = false;
}

bool AudioDecoder::isOpen()        const { return m_isOpen;  }
int  AudioDecoder::getSampleRate() const { return 44100;     }
int  AudioDecoder::getChannels()   const { return 2;         }

}
