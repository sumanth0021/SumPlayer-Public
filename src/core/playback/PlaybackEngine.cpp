#include "core/playback/PlaybackEngine.h"

#include <ass/ass.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/hwcontext.h>
    typedef struct ass_library ASS_Library;
    typedef struct ass_renderer ASS_Renderer;
    typedef struct ass_track ASS_Track;
}

namespace SumPlayer
{

PlaybackEngine::PlaybackEngine()
    : m_formatCtx(nullptr)
    , m_videoCodecCtx(nullptr)
    , m_audioCodecCtx(nullptr)
    , m_videoFrame(nullptr)
    , m_audioFrame(nullptr)
    , m_videoFrameRGB(nullptr)
    , m_swsCtx(nullptr)
    , m_swrCtx(nullptr)
    , m_videoStreamIndex(-1)
    , m_audioStreamIndex(-1)
    , m_width(0)
    , m_height(0)
    , m_frameRate(0.0)
    , m_duration(0.0)
    , m_sampleRate(44100)
    , m_channels(2)
    , m_rgbBuffer(nullptr)
    , m_videoQueue(48)
    , m_audioQueue(120)
    , m_running(false)
    , m_isOpen(false)
    , m_startPts(-1.0)
    , m_paused(false)
    , m_seekRequested(false)
    , m_seekTarget(0.0)
    , m_swFrame(nullptr)
    , m_hwDeviceCtx(nullptr)
    , m_hwDecodeActive(false)
    , m_audioTrackChangeRequest(false)
    , m_audioTrackChangeTarget(-1)
    , m_audioSwitchPending(false)
    , m_pendingAudioStreamIndex(-1)
    , m_pendingAudioCodecCtx(nullptr)
    , m_pendingSwrCtx(nullptr)
    , m_pendingAudioFrame(nullptr)
    , m_subtitleStreamIndex(-1)
    , m_subtitleCodecCtx(nullptr)
    , m_assLibrary(nullptr)
    , m_assRenderer(nullptr)
    , m_assTrack(nullptr)
    , m_subtitleTrackChangeRequest(false)
    , m_subtitleTrackChangeTarget(-2) 
    , m_subtitleFileLoadRequested(false)
    , m_loopEnabled(false)

{
    initSubtitleSystem();
}

PlaybackEngine::~PlaybackEngine()
{
    stop();
    if (m_assRenderer) ass_renderer_done(m_assRenderer);
    if (m_assLibrary)  ass_library_done(m_assLibrary);
}

bool PlaybackEngine::open(const std::string& filepath)
{
    stop();

    int result = avformat_open_input(
        &m_formatCtx, filepath.c_str(), nullptr, nullptr);
        
    if (result < 0)
    {
        char errBuf[256];
        av_strerror(result, errBuf, sizeof(errBuf));
        reportError(ErrorSeverity::Error, std::string("Cannot open file: ") + errBuf);
        return false;
    }

    avformat_find_stream_info(m_formatCtx, nullptr);

    m_videoStreamIndex = av_find_best_stream(
        m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    m_audioStreamIndex = av_find_best_stream(
        m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    m_subtitleStreamIndex = av_find_best_stream(
    m_formatCtx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);

    if (m_videoStreamIndex < 0)
    {
        reportError(ErrorSeverity::Error, "This file has no video stream.");
        return false;
    }

    if (m_subtitleStreamIndex >= 0) openSubtitleCodec();

    if (m_formatCtx->duration != AV_NOPTS_VALUE)
        m_duration = (double)m_formatCtx->duration / AV_TIME_BASE;

    AVStream* vStream = m_formatCtx->streams[m_videoStreamIndex];
    if (vStream->avg_frame_rate.den != 0)
        m_frameRate = (double)vStream->avg_frame_rate.num
                    / (double)vStream->avg_frame_rate.den;

    if (!openVideoCodec()) { stop(); return false; }
    if (m_audioStreamIndex >= 0) openAudioCodec();

    m_videoFrame    = av_frame_alloc();
    m_swFrame = av_frame_alloc();
    m_audioFrame    = av_frame_alloc();
    m_videoFrameRGB = av_frame_alloc();

    int bufSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGB24, m_width, m_height, 1);
    m_rgbBuffer = (uint8_t*)av_malloc(bufSize);

    av_image_fill_arrays(
        m_videoFrameRGB->data,
        m_videoFrameRGB->linesize,
        m_rgbBuffer,
        AV_PIX_FMT_RGB24,
        m_width, m_height, 1
    );

    m_isOpen.store(true);
    std::cout << "[PlaybackEngine] Opened: " << filepath << std::endl;
    std::cout << "[PlaybackEngine] "
              << m_width << "x" << m_height
              << " @ " << m_frameRate << "fps" << std::endl;

    return true;
}

bool PlaybackEngine::initSubtitleSystem()
{
    m_assLibrary = ass_library_init();
    if (!m_assLibrary)
    {
        std::cout << "[PlaybackEngine] Failed to init ASS library." << std::endl;
        return false;
    }

    ass_set_message_cb(m_assLibrary,
        [](int level, const char* fmt, va_list args, void* data)
        {
            char buf[512];
            vsnprintf(buf, sizeof(buf), fmt, args);
            std::cout << "[libass level " << level << "] " << buf << std::endl;
        },
        nullptr);

    m_assRenderer = ass_renderer_init(m_assLibrary);
    if (!m_assRenderer)
    {
        std::cout << "[PlaybackEngine] Failed to init ASS renderer." << std::endl;
        return false;
    }

    ass_set_fonts(m_assRenderer, nullptr, "sans-serif",
                  ASS_FONTPROVIDER_AUTODETECT, nullptr, 1);

    std::cout << "[PlaybackEngine] Subtitle system initialized." << std::endl;
    return true;
}

bool PlaybackEngine::openSubtitleCodec()
{
    AVStream* stream = m_formatCtx->streams[m_subtitleStreamIndex];
    AVCodecParameters* params = stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (!codec) 
    {
        reportError(ErrorSeverity::Warning, "This subtitle format is not supported.");
        return false;
    }

    m_subtitleCodecCtx = avcodec_alloc_context3(codec);
    if (!m_subtitleCodecCtx) return false;

    avcodec_parameters_to_context(m_subtitleCodecCtx, params);

    if (avcodec_open2(m_subtitleCodecCtx, codec, nullptr) < 0)
    {
        reportError(ErrorSeverity::Warning, "Failed to open the subtitle track.");
        return false;
    }

    m_assTrack = ass_new_track(m_assLibrary);

    if (m_subtitleCodecCtx->subtitle_header && m_subtitleCodecCtx->subtitle_header_size > 0)
    {
        ass_process_codec_private(m_assTrack,
            (char*)m_subtitleCodecCtx->subtitle_header,
            m_subtitleCodecCtx->subtitle_header_size);
    }
    else if (params->extradata && params->extradata_size > 0)
    {
        
        ass_process_codec_private(m_assTrack,
            (char*)params->extradata, params->extradata_size);
    }

    std::cout << "[PlaybackEngine] Subtitle codec opened: " << codec->name << std::endl;
    return true;
}

void PlaybackEngine::decodeSubtitlePacket(AVPacket* packet)
{
    if (!m_subtitleCodecCtx || !packet) return;

    AVSubtitle subtitle;
    int gotSubtitle = 0;

    int result = avcodec_decode_subtitle2(
        m_subtitleCodecCtx, &subtitle, &gotSubtitle, packet);

    if (result < 0 || !gotSubtitle) return;

    AVStream* stream = m_formatCtx->streams[m_subtitleStreamIndex];
    double packetTime = 0.0;
    if (packet->pts != AV_NOPTS_VALUE)
        packetTime = (double)packet->pts * av_q2d(stream->time_base);

    if (subtitle.num_rects == 0)
    {
        std::lock_guard<std::mutex> lock(m_subtitleMutex);
        if (!m_bitmapSubtitles.empty())
        {
            m_bitmapSubtitles.back().endTime = packetTime;
        }
        avsubtitle_free(&subtitle);
        return;
    }

    for (unsigned int i = 0; i < subtitle.num_rects; i++)
    {
        AVSubtitleRect* rect = subtitle.rects[i];

        if (rect->type == SUBTITLE_ASS && rect->ass && m_assTrack)
        {
            long long startMs = (long long)((packetTime * 1000.0)
                + subtitle.start_display_time);

            long long durationMs = subtitle.end_display_time - subtitle.start_display_time;

            if (durationMs <= 0 && packet->duration > 0)
            {
                durationMs = (long long)(packet->duration * av_q2d(stream->time_base) * 1000.0);
            }

            if (durationMs <= 0) durationMs = 3000;

            std::lock_guard<std::mutex> lock(m_subtitleMutex);
            ass_process_chunk(m_assTrack, rect->ass,
                (int)strlen(rect->ass), startMs, durationMs);
        }
        else if (rect->type == SUBTITLE_BITMAP && rect->data[0] && rect->data[1])
        {
            BitmapSubtitle bs;
            bs.w = rect->w;
            bs.h = rect->h;
            bs.x = rect->x;
            bs.y = rect->y;
            bs.rgba.resize((size_t)rect->w * rect->h * 4);

            const uint8_t* indices = rect->data[0];
            const uint32_t* palette = (const uint32_t*)rect->data[1];

            for (int y = 0; y < rect->h; y++)
            {
                for (int x = 0; x < rect->w; x++)
                {
                    uint8_t idx = indices[y * rect->linesize[0] + x];
                    uint32_t argb = palette[idx];

                    uint8_t a = (argb >> 24) & 0xFF;
                    uint8_t r = (argb >> 16) & 0xFF;
                    uint8_t g = (argb >> 8)  & 0xFF;
                    uint8_t b = argb & 0xFF;

                    size_t off = ((size_t)y * rect->w + x) * 4;
                    bs.rgba[off + 0] = r;
                    bs.rgba[off + 1] = g;
                    bs.rgba[off + 2] = b;
                    bs.rgba[off + 3] = a;
                }
            }

            double durationSeconds = 2.0;
            if (packet->duration > 0)
            {
                durationSeconds = (double)packet->duration * av_q2d(stream->time_base);
            }

            bs.startTime = packetTime;
            bs.endTime   = packetTime + durationSeconds;
            bs.active    = true;

            std::lock_guard<std::mutex> lock(m_subtitleMutex);
            m_bitmapSubtitles.push_back(std::move(bs));

            while (m_bitmapSubtitles.size() > 8)
                m_bitmapSubtitles.pop_front();

            std::cout << "[PlaybackEngine] Bitmap subtitle queued: " << rect->w << "x" << rect->h
                       << " showing " << bs.startTime << "s - " << bs.endTime << "s"
                       << " (queue size " << m_bitmapSubtitles.size() << ")" << std::endl;
        }
    }

    avsubtitle_free(&subtitle);
}

void PlaybackEngine::compositeSubtitles(uint8_t* rgbData, int linesize, int width, int height, double videoPts)
{
    if (!m_assRenderer || !m_assTrack) return;

    std::lock_guard<std::mutex> lock(m_subtitleMutex);

    long long timestampMs = (long long)(videoPts * 1000.0);
    int changeDetected = 0;

    ASS_Image* img = ass_render_frame(
        m_assRenderer, m_assTrack, timestampMs, &changeDetected);

    static double lastPtsWithSub = -1.0;
    bool hasImage = (img != nullptr);
    if (hasImage && lastPtsWithSub < 0.0)
        std::cout << "[SubDebug] TEXT subtitle appeared at pts=" << videoPts << std::endl;
    if (!hasImage && lastPtsWithSub >= 0.0)
        std::cout << "[SubDebug] TEXT subtitle disappeared at pts=" << videoPts << std::endl;
    lastPtsWithSub = hasImage ? videoPts : -1.0;

    for (; img != nullptr; img = img->next)
    {
        uint8_t r = (img->color >> 24) & 0xFF;
        uint8_t g = (img->color >> 16) & 0xFF;
        uint8_t b = (img->color >> 8)  & 0xFF;
        uint8_t alphaBase = 255 - (img->color & 0xFF);

        const uint8_t* bitmap = img->bitmap;

        for (int y = 0; y < img->h; y++)
        {
            int destY = img->dst_y + y;
            if (destY < 0 || destY >= height) continue;

            for (int x = 0; x < img->w; x++)
            {
                int destX = img->dst_x + x;
                if (destX < 0 || destX >= width) continue;

                uint8_t glyphAlpha = bitmap[y * img->stride + x];
                if (glyphAlpha == 0) continue;

                float a = (glyphAlpha / 255.0f) * (alphaBase / 255.0f);

                int pixelOffset = destY * linesize + destX * 3;
                uint8_t* dst = rgbData + pixelOffset;

                dst[0] = (uint8_t)(r * a + dst[0] * (1.0f - a));
                dst[1] = (uint8_t)(g * a + dst[1] * (1.0f - a));
                dst[2] = (uint8_t)(b * a + dst[2] * (1.0f - a));
            }
        }
    }
}

void PlaybackEngine::compositeBitmapSubtitle(uint8_t* rgbData, int linesize, int width, int height, double videoPts)
{
    std::lock_guard<std::mutex> lock(m_subtitleMutex);

    while (!m_bitmapSubtitles.empty() && videoPts > m_bitmapSubtitles.front().endTime)
        m_bitmapSubtitles.pop_front();

    for (const BitmapSubtitle& bs : m_bitmapSubtitles)
    {
        if (videoPts < bs.startTime || videoPts > bs.endTime) continue;

        for (int y = 0; y < bs.h; y++)
        {
            int destY = bs.y + y;
            if (destY < 0 || destY >= height) continue;

            for (int x = 0; x < bs.w; x++)
            {
                int destX = bs.x + x;
                if (destX < 0 || destX >= width) continue;

                size_t off = ((size_t)y * bs.w + x) * 4;
                uint8_t r = bs.rgba[off + 0];
                uint8_t g = bs.rgba[off + 1];
                uint8_t b = bs.rgba[off + 2];
                uint8_t alpha = bs.rgba[off + 3];

                if (alpha == 0) continue;
                float a = alpha / 255.0f;

                int pixelOffset = destY * linesize + destX * 3;
                uint8_t* dst = rgbData + pixelOffset;

                dst[0] = (uint8_t)(r * a + dst[0] * (1.0f - a));
                dst[1] = (uint8_t)(g * a + dst[1] * (1.0f - a));
                dst[2] = (uint8_t)(b * a + dst[2] * (1.0f - a));
            }
        }
    }
}

bool PlaybackEngine::openVideoCodec()
{
    AVStream*          stream = m_formatCtx->streams[m_videoStreamIndex];
    AVCodecParameters* params = stream->codecpar;
    const AVCodec*     codec  = avcodec_find_decoder(params->codec_id);
    if (!codec) 
    {
        reportError(ErrorSeverity::Error, "No video decoder available for this file's codec.");
        return false;
    }

    m_videoCodecCtx = avcodec_alloc_context3(codec);
    if(!m_videoCodecCtx) return false;

    avcodec_parameters_to_context(m_videoCodecCtx, params);
    if(initHwDecoder())
    {
        m_videoCodecCtx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);
        m_videoCodecCtx->opaque = this;
        m_videoCodecCtx->get_format = getHwFormat;
    }
    avcodec_open2(m_videoCodecCtx, codec, nullptr);

    m_width  = m_videoCodecCtx->width;
    m_height = m_videoCodecCtx->height;

    if (m_assRenderer)
    {
        ass_set_frame_size(m_assRenderer, m_width, m_height);
        ass_set_storage_size(m_assRenderer, m_width, m_height);
    }

    m_swsCtx = nullptr; 
    return true;
}

void PlaybackEngine::setSubtitleFontScale(double scale)
{
    if (m_assRenderer)
    {
        ass_set_font_scale(m_assRenderer, scale);
    }
}


bool PlaybackEngine::initHwDecoder()
{
    int result = av_hwdevice_ctx_create(&m_hwDeviceCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0 );
    if(result < 0)
    {
        reportError(ErrorSeverity::Info, "Hardware acceleration unavailable — using software decoding.");
        return false;
    }
    std::cout<<"[PlaybackEngine] D3D11VA hardware decoding  created successfully."<<std::endl;
    return true;
}

AVPixelFormat PlaybackEngine::getHwFormat(AVCodecContext* ctx, const AVPixelFormat* formats)
{
    for (const AVPixelFormat* p = formats; *p != AV_PIX_FMT_NONE; ++p)
    {
        if (*p == AV_PIX_FMT_D3D11)
        {
            PlaybackEngine* self = static_cast<PlaybackEngine*>(ctx->opaque);
            self->m_hwDecodeActive = true;
            std::cout << "[PlaybackEngine] Hardware pixel format negotiated: D3D11 decode ACTIVE." << std::endl;
            return AV_PIX_FMT_D3D11;
        }
    }
    PlaybackEngine* self = static_cast<PlaybackEngine*>(ctx->opaque);
    if (self)
    {
        self->reportError(ErrorSeverity::Info, "This video's format isn't supported by hardware acceleration — using software decoding.");
    }
    return formats[0];
}

bool PlaybackEngine::openAudioCodec()
{
    AVStream*          stream = m_formatCtx->streams[m_audioStreamIndex];
    AVCodecParameters* params = stream->codecpar;
    const AVCodec*     codec  = avcodec_find_decoder(params->codec_id);
    if (!codec) 
    {
        reportError(ErrorSeverity::Warning, "This file's audio format is not supported. Video will play without sound.");
        return false;
    }

    m_audioCodecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_audioCodecCtx, params);
    avcodec_open2(m_audioCodecCtx, codec, nullptr);

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
    return true;
}

void PlaybackEngine::play()
{
    if (!m_isOpen.load()) return;

    m_videoQueue.reset();
    m_audioQueue.reset();
    m_avSync.start();
    m_running.store(true);
    m_startPts = -1.0;

    m_decodeThread = std::thread(&PlaybackEngine::decodeLoop, this);
    std::cout << "[PlaybackEngine] Playing." << std::endl;
}

void PlaybackEngine::pause()
{
    m_paused.store(true);
}

void PlaybackEngine::resume()
{
    m_paused.store(false);
}

bool PlaybackEngine::isPaused() const
{
    return m_paused.load();
}

void PlaybackEngine::requestSeek(double targetSeconds)
{
    if(targetSeconds < 0.0 ) targetSeconds = 0.0;
    if(targetSeconds > m_duration) targetSeconds = m_duration;
    
    m_seekTarget.store(targetSeconds);
    m_seekRequested.store(true);
}

void PlaybackEngine::performSeek(double targetSeconds)
{
    std::cout << "[PlaybackEngine] Seeking to " << targetSeconds << "s" <<std::endl;

    int64_t Target =  (int64_t)(targetSeconds * AV_TIME_BASE);

    int result = av_seek_frame(m_formatCtx, -1, Target, AVSEEK_FLAG_BACKWARD);

    if(result < 0){
        std::cout << "[PlaybackEngine] Seek failed." << std::endl;
        return;
    }

    if (m_assTrack)
        {
            std::lock_guard<std::mutex> lock(m_subtitleMutex);
            ass_free_track(m_assTrack);
            m_assTrack = ass_new_track(m_assLibrary);
            if (m_subtitleCodecCtx && m_subtitleCodecCtx->subtitle_header
                && m_subtitleCodecCtx->subtitle_header_size > 0)
            {
                ass_process_codec_private(m_assTrack,
                    (char*)m_subtitleCodecCtx->subtitle_header,
                    m_subtitleCodecCtx->subtitle_header_size);
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_subtitleMutex);
            m_bitmapSubtitles.clear();
        }

    if(m_videoCodecCtx) avcodec_flush_buffers(m_videoCodecCtx);
    if(m_audioCodecCtx) avcodec_flush_buffers(m_audioCodecCtx);

    m_videoQueue.reset();
    m_audioQueue.reset();

    m_startPts = -1.0;
    m_avSync.stop();
    m_avSync.start();

    m_avSync.requestClockReset();

    std::cout << "[PlaybackEngine] Seek completed."<<std::endl;
}

void PlaybackEngine::requestSubtitleTrackChange(int streamIndex)
{
    m_subtitleTrackChangeTarget.store(streamIndex);
    m_subtitleTrackChangeRequest.store(true);
}

int PlaybackEngine::getCurrentSubtitleStreamIndex() const
{
    return m_subtitleStreamIndex;
}

void PlaybackEngine::performSubtitleTrackChange(int newStreamIndex)
{
    if (newStreamIndex == m_subtitleStreamIndex) return;

    std::cout << "[Timing] performAudioTrackChange STARTED at "
           << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;

    if (m_subtitleCodecCtx)
    {
        avcodec_free_context(&m_subtitleCodecCtx);
        m_subtitleCodecCtx = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(m_subtitleMutex);
        if (m_assTrack)
        {
            ass_free_track(m_assTrack);
            m_assTrack = nullptr;
        }
        m_bitmapSubtitles.clear();
    }

    m_subtitleStreamIndex = newStreamIndex;

    if (m_subtitleStreamIndex >= 0)
    {
        openSubtitleCodec();
    }

    std::cout << "[PlaybackEngine] Switched subtitle track to stream index "
              << m_subtitleStreamIndex << std::endl;

    std::cout << "[Timing] performAudioTrackChange FINISHED at "
           << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
}

void PlaybackEngine::requestLoadSubtitleFile(const std::string& filepath)
{
    {
        std::lock_guard<std::mutex> lock(m_pendingSubtitleFileMutex);
        m_pendingSubtitleFilePath = filepath;
    }
    m_subtitleFileLoadRequested.store(true);
}

void PlaybackEngine::performLoadSubtitleFile(const std::string& filepath)
{
    if (!m_assLibrary) return;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "[PlaybackEngine] Could not open subtitle file: " << filepath << std::endl;
        return;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();
    if (content.size() >= 3 &&
    (unsigned char)content[0] == 0xEF &&
    (unsigned char)content[1] == 0xBB &&
    (unsigned char)content[2] == 0xBF)
    {
        content.erase(0, 3);
        std::cout << "[PlaybackEngine] Stripped UTF-8 BOM from subtitle file." << std::endl;
    }


    if (content.empty())
    {
        std::cout << "[PlaybackEngine] Subtitle file is empty: " << filepath << std::endl;
        return;
    }

    ASS_Track* newTrack = ass_read_memory(
        m_assLibrary,
        const_cast<char*>(content.data()),
        content.size(),
        (char*)"UTF-8"
    );

    if (!newTrack)
    {
        reportError(ErrorSeverity::Warning, "Could not load this subtitle file — it may be corrupted or in an unsupported format.");
        return;
    }

    if (m_subtitleCodecCtx)
    {
        avcodec_free_context(&m_subtitleCodecCtx);
        m_subtitleCodecCtx = nullptr;
    }
    m_subtitleStreamIndex = -1;

    {
        std::lock_guard<std::mutex> lock(m_subtitleMutex);
        if (m_assTrack)
        {
            ass_free_track(m_assTrack);
        }
        m_assTrack = newTrack;
    }

    m_videoQueue.reset();

    std::cout << "[PlaybackEngine] Loaded external subtitle file: " << filepath << std::endl;
}

void PlaybackEngine::performAudioTrackChange(int newStreamIndex)
{
    if (newStreamIndex == m_audioStreamIndex || newStreamIndex < 0) return;

    std::cout << "[Timing] performAudioTrackChange STARTED at "
           << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;

    m_audioQueue.interrupt();

    if (m_audioCodecCtx)
    {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    m_audioStreamIndex = newStreamIndex;

    if (m_audioStreamIndex >= 0)
    {
        openAudioCodec();
    }

    m_audioQueue.clearInterrupt();

    std::cout << "[PlaybackEngine] Switched audio track to stream index "
              << m_audioStreamIndex << std::endl;
    std::cout << "[Timing] performAudioTrackChange FINISHED at "
           << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
}

void PlaybackEngine::cancelPendingAudioSwitch()
{
    if (m_pendingAudioCodecCtx) { avcodec_free_context(&m_pendingAudioCodecCtx); m_pendingAudioCodecCtx = nullptr; }
    if (m_pendingSwrCtx)        { swr_free(&m_pendingSwrCtx); m_pendingSwrCtx = nullptr; }
    if (m_pendingAudioFrame)    { av_frame_free(&m_pendingAudioFrame); m_pendingAudioFrame = nullptr; }
    m_audioSwitchPending = false;
    m_pendingAudioStreamIndex = -1;
}

void PlaybackEngine::decodePendingAudioPacket(AVPacket* packet)
{
     if (!m_pendingAudioCodecCtx || !m_pendingSwrCtx) return;

    if (avcodec_send_packet(m_pendingAudioCodecCtx, packet) < 0) return;
    if (avcodec_receive_frame(m_pendingAudioCodecCtx, m_pendingAudioFrame) < 0) return;

    double pts = 0.0;
    if (m_pendingAudioFrame->pts != AV_NOPTS_VALUE)
    {
        AVStream* stream = m_formatCtx->streams[m_pendingAudioStreamIndex];
        pts = (double)m_pendingAudioFrame->pts * av_q2d(stream->time_base);
    }

    int outSamples = swr_get_out_samples(m_pendingSwrCtx, m_pendingAudioFrame->nb_samples);
    int bufSize = outSamples * 2 * sizeof(int16_t);
    std::vector<uint8_t> buffer(bufSize);
    uint8_t* outPtr = buffer.data();

    int converted = swr_convert(
        m_pendingSwrCtx, &outPtr, outSamples,
        (const uint8_t**)m_pendingAudioFrame->data, m_pendingAudioFrame->nb_samples);

    av_frame_unref(m_pendingAudioFrame);
    if (converted <= 0) return;


    if (m_audioCodecCtx) { avcodec_free_context(&m_audioCodecCtx); m_audioCodecCtx = nullptr; }
    if (m_swrCtx)        { swr_free(&m_swrCtx); m_swrCtx = nullptr; }

    m_audioCodecCtx   = m_pendingAudioCodecCtx;
    m_swrCtx          = m_pendingSwrCtx;
    m_audioStreamIndex = m_pendingAudioStreamIndex;

    m_pendingAudioCodecCtx = nullptr;
    m_pendingSwrCtx        = nullptr;
    m_audioSwitchPending   = false;

    m_audioQueue.reset(); 
    AudioFrame af;
    af.pts        = pts;
    af.sampleRate = 44100;
    af.channels   = 2;
    int actualSize = converted * 2 * sizeof(int16_t);
    af.data.resize(actualSize);
    std::memcpy(af.data.data(), buffer.data(), actualSize);
    m_audioQueue.push(std::move(af));
    m_avSync.requestClockReset(); 

    std::cout << "[PlaybackEngine] Audio track switch complete -> stream "
              << m_audioStreamIndex << std::endl;
}


void PlaybackEngine::requestAudioTrackChange(int streamIndex)
{
    m_audioTrackChangeTarget.store(streamIndex);
    m_audioTrackChangeRequest.store(true);
}

int PlaybackEngine::getCurrentAudioStreamIndex() const
{
    return m_audioStreamIndex;
}

void PlaybackEngine::setLoopEnabled(bool enabled)
{
    m_loopEnabled.store(enabled);
}

bool PlaybackEngine::isLoopEnabled() const
{
    return m_loopEnabled.load();
}


void PlaybackEngine::stop()
{
    m_running.store(false);
    m_avSync.stop();

    m_videoQueue.stop();
    m_audioQueue.stop();

    if (m_decodeThread.joinable())
        m_decodeThread.join();

    if (m_rgbBuffer)
    {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }
    if (m_videoFrameRGB)
    {
        av_frame_free(&m_videoFrameRGB);
        m_videoFrameRGB = nullptr;
    }
    if (m_videoFrame)
    {
        av_frame_free(&m_videoFrame);
        m_videoFrame = nullptr;
    }
    if (m_audioFrame)
    {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
    if (m_videoCodecCtx)
    {
        avcodec_free_context(&m_videoCodecCtx);
        m_videoCodecCtx = nullptr;
    }
    if (m_audioCodecCtx)
    {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }
    if (m_formatCtx)
    {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
    if(m_swFrame)
    {
        av_frame_free(&m_swFrame);
        m_swFrame = nullptr;
    }
    if(m_hwDeviceCtx)
    {
        av_buffer_unref(&m_hwDeviceCtx);
        m_hwDeviceCtx = nullptr;
    }

    if (m_assTrack)
    {
        ass_free_track(m_assTrack);
        m_assTrack = nullptr;
    }
    if (m_subtitleCodecCtx)
    {
        avcodec_free_context(&m_subtitleCodecCtx);
        m_subtitleCodecCtx = nullptr;
    }
    m_subtitleStreamIndex = -1;

    m_hwDecodeActive = false;

    cancelPendingAudioSwitch();
    std::lock_guard<std::mutex> lock(m_subtitleMutex);
    m_bitmapSubtitles.clear();

    m_isOpen.store(false);
    std::cout << "[PlaybackEngine] Stopped." << std::endl;
}

bool PlaybackEngine::isOpen() const
{
    return m_isOpen.load();
}

void PlaybackEngine::decodeLoop()
{
    AVPacket* packet = av_packet_alloc();

    while (m_running.load())
{
    if (m_seekRequested.load())
    {
        m_videoQueue.interrupt();
        m_audioQueue.interrupt();
        double target = m_seekTarget.load();
        performSeek(target);
        m_videoQueue.clearInterrupt();
        m_audioQueue.clearInterrupt();
        m_seekRequested.store(false);
        continue;
    }

    if (m_audioTrackChangeRequest.load())
    {
        m_videoQueue.interrupt();
        m_audioQueue.interrupt();
        int target = m_audioTrackChangeTarget.load();
        performAudioTrackChange(target);
        m_videoQueue.clearInterrupt();
        m_audioQueue.clearInterrupt();
        m_audioTrackChangeRequest.store(false);
        continue;
    }

    if (m_subtitleTrackChangeRequest.load())
    {
        m_videoQueue.interrupt();
        m_audioQueue.interrupt();
        int target = m_subtitleTrackChangeTarget.load();
        performSubtitleTrackChange(target);
        m_videoQueue.clearInterrupt();
        m_audioQueue.clearInterrupt();
        m_subtitleTrackChangeRequest.store(false);
        continue;
    }

    if (m_subtitleFileLoadRequested.load())
    {
        m_videoQueue.interrupt();
        m_audioQueue.interrupt();

        std::string path;
        {
            std::lock_guard<std::mutex> lock(m_pendingSubtitleFileMutex);
            path = m_pendingSubtitleFilePath;
        }
        performLoadSubtitleFile(path);

        m_videoQueue.clearInterrupt();
        m_audioQueue.clearInterrupt();
        m_subtitleFileLoadRequested.store(false);
        continue;
    }

    int result = av_read_frame(m_formatCtx, packet);
    if (result < 0)
    {
        if (m_loopEnabled.load())
            {
                std::cout << "[DecodeLoop] EOF reached — looping back to start." << std::endl;
                performSeek(0.0);
                continue;
            }

            std::cout << "[DecodeLoop] EOF reached — flushing decoders." << std::endl;
            break;
    }

    if (packet->stream_index == m_videoStreamIndex)
        decodeVideoPacket(packet);
    else if (packet->stream_index == m_audioStreamIndex)
        decodeAudioPacket(packet);
    else if (m_audioSwitchPending && packet->stream_index == m_pendingAudioStreamIndex)
        decodePendingAudioPacket(packet);
    else if (packet->stream_index == m_subtitleStreamIndex)
    decodeSubtitlePacket(packet);

    av_packet_unref(packet);
}

    decodeVideoPacket(nullptr);
    if (m_audioCodecCtx)
        decodeAudioPacket(nullptr);

    av_packet_free(&packet);

    while (m_running.load() &&
           (!m_videoQueue.empty() || !m_audioQueue.empty()))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_running.store(false);
}

void PlaybackEngine::setErrorCallback(ErrorCallback callback)
{
    m_errorCallback = callback;
}

void PlaybackEngine::reportError(ErrorSeverity severity, const std::string& message)
{
    std::cout << "[PlaybackEngine] " << message << std::endl;
    if (m_errorCallback)
    {
        m_errorCallback(severity, message);
    }
}

std::vector<AudioTrackInfo> PlaybackEngine::getAudioTracks() const
{
    std::vector<AudioTrackInfo> tracks;
    if (!m_formatCtx) return tracks;

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        AVStream* stream = m_formatCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            AudioTrackInfo info;
            info.streamIndex = (int)i;

            AVDictionaryEntry* lang = av_dict_get(stream->metadata, "language", nullptr, 0);
            info.language = lang ? lang->value : "und";

            const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
            info.codecName = codec ? codec->name : "unknown";

            tracks.push_back(info);
        }
    }
    return tracks;
}

std::vector<SubtitleTrackInfo> PlaybackEngine::getSubtitleTracks() const
{
    std::vector<SubtitleTrackInfo> tracks;
    if (!m_formatCtx) return tracks;

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        AVStream* stream = m_formatCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            SubtitleTrackInfo info;
            info.streamIndex = (int)i;

            AVDictionaryEntry* lang = av_dict_get(stream->metadata, "language", nullptr, 0);
            info.language = lang ? lang->value : "und";

            const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
            info.codecName = codec ? codec->name : "unknown";

            tracks.push_back(info);
        }
    }
    return tracks;
}



void PlaybackEngine::decodeVideoPacket(AVPacket* packet)
{
    int result = avcodec_send_packet(m_videoCodecCtx, packet);
    if (result < 0 && packet != nullptr) return;

    while (true)
    {
        result = avcodec_receive_frame(m_videoCodecCtx, m_videoFrame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) break;
        if (result < 0) break;

        double pts = 0.0;
        if (m_videoFrame->pts != AV_NOPTS_VALUE)
        {
            AVStream* stream = m_formatCtx->streams[m_videoStreamIndex];
            pts = (double)m_videoFrame->pts * av_q2d(stream->time_base);
        }

        AVFrame* scaleSource = m_videoFrame;

        if (m_videoFrame->format == AV_PIX_FMT_D3D11)
        {
            int transferResult = av_hwframe_transfer_data(m_swFrame, m_videoFrame, 0);
            if (transferResult < 0)
            {
                std::cout << "[PlaybackEngine] Failed to transfer hardware frame." << std::endl;
                av_frame_unref(m_videoFrame);
                continue;
            }
            scaleSource = m_swFrame;
        }

        if (!m_swsCtx)
        {
            m_swsCtx = sws_getContext(
                m_width, m_height, (AVPixelFormat)scaleSource->format,
                m_width, m_height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr
            );
        }

        sws_scale(
            m_swsCtx,
            scaleSource->data,
            scaleSource->linesize,
            0, m_height,
            m_videoFrameRGB->data,
            m_videoFrameRGB->linesize
        );

    

        VideoFrame vf;
        vf.width    = m_width;
        vf.height   = m_height;
        vf.linesize = m_videoFrameRGB->linesize[0];
        vf.pts      = pts;

        int dataSize = vf.linesize * m_height;
        vf.data.resize(dataSize);
        std::memcpy(vf.data.data(),
                    m_videoFrameRGB->data[0], dataSize);

        m_videoQueue.push(std::move(vf));
        if (scaleSource == m_swFrame)
        {
            av_frame_unref(m_swFrame);
        }
        av_frame_unref(m_videoFrame);
    }
}

void PlaybackEngine::decodeAudioPacket(AVPacket* packet)
{
    if (!m_audioCodecCtx || !m_swrCtx) return;

    int result = avcodec_send_packet(m_audioCodecCtx, packet);
    if (result < 0 && packet != nullptr) return;

    while (true)
    {
        result = avcodec_receive_frame(m_audioCodecCtx, m_audioFrame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) break;
        if (result < 0) break;

        double pts = 0.0;
        if (m_audioFrame->pts != AV_NOPTS_VALUE)
        {
            AVStream* stream = m_formatCtx->streams[m_audioStreamIndex];
            pts = (double)m_audioFrame->pts * av_q2d(stream->time_base);
        }

        int outSamples = swr_get_out_samples(
            m_swrCtx, m_audioFrame->nb_samples);
        int bufSize = outSamples * 2 * sizeof(int16_t);

        std::vector<uint8_t> buffer(bufSize);
        uint8_t* outPtr = buffer.data();

        int converted = swr_convert(
            m_swrCtx,
            &outPtr,
            outSamples,
            (const uint8_t**)m_audioFrame->data,
            m_audioFrame->nb_samples
        );

        if (converted > 0)
        {
            AudioFrame af;
            af.pts        = pts;
            af.sampleRate = 44100;
            af.channels   = 2;
            int actualSize = converted * 2 * sizeof(int16_t);
            af.data.resize(actualSize);
            std::memcpy(af.data.data(), buffer.data(), actualSize);
            m_audioQueue.push(std::move(af));
        }

        av_frame_unref(m_audioFrame);
    }
}

void PlaybackEngine::renderNextFrame()
{
    if (!m_isOpen.load()) return;
    if (m_paused.load()) return;

    VideoFrame frame;
    if (!m_videoQueue.peek(frame))
        return;

    if (m_startPts < 0.0)
    {
        m_startPts      = frame.pts;
        m_startWallTime = std::chrono::steady_clock::now();
    }

    double frameDue = frame.pts - m_startPts;

    double syncTime;
    if (m_avSync.isClockSet())
    {
        syncTime = m_avSync.getAudioClock() - m_startPts;
    }
    else
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - m_startWallTime;
        syncTime = elapsed.count();
    }

    if (syncTime < frameDue - 0.002)
        return;

    m_videoQueue.tryPop(frame);

    if (syncTime > frameDue + 0.100)
        return;

    compositeSubtitles(frame.data.data(), frame.linesize, frame.width, frame.height, frame.pts);
    compositeBitmapSubtitle(frame.data.data(), frame.linesize, frame.width, frame.height, frame.pts);

    if (m_videoFrameCallback)
        m_videoFrameCallback(
            frame.data.data(),
            frame.width,
            frame.height,
            frame.linesize
        );
}

void PlaybackEngine::setVideoFrameCallback(VideoFrameCallback cb)
{
    m_videoFrameCallback = cb;
}

bool   PlaybackEngine::isPlaying()   const { return m_running.load(); }
double PlaybackEngine::getDuration()  const { return m_duration;       }
double PlaybackEngine::getPosition()  const { return m_avSync.getAudioClock(); }
int    PlaybackEngine::getWidth()     const { return m_width;          }
int    PlaybackEngine::getHeight()    const { return m_height;         }
double PlaybackEngine::getFrameRate() const { return m_frameRate;      }

}
