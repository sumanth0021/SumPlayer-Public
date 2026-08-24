#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include <chrono>
#include <vector>
#include <mutex>
#include <deque>

#include "core/playback/FrameQueue.h"
#include "core/playback/AVSync.h"
#include <libavutil/pixfmt.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct SwsContext;
struct SwrContext;
struct AVBufferRef;

struct AudioTrackInfo
    {
        int streamIndex;
        std::string language;
        std::string codecName;
    };

struct SubtitleTrackInfo{
    int streamIndex;
    std::string language;
    std::string codecName;
};

struct BitmapSubtitle
{
    std::vector<uint8_t> rgba;
    int x = 0, y = 0, w = 0, h = 0;
    double startTime = 0.0;
    double endTime = 0.0;
    bool active = false;
};

extern "C" {
    typedef struct ass_library ASS_Library;
    typedef struct ass_renderer ASS_Renderer;
    typedef struct ass_track ASS_Track;
}


namespace SumPlayer
{

   
class PlaybackEngine
{
public:
    using VideoFrameCallback = std::function<void(
        const uint8_t*, int, int, int)>;

    PlaybackEngine();
    ~PlaybackEngine();

    bool open(const std::string& filepath);
    void play();
    void stop();
    bool isPlaying() const;
    void pause();
    void resume();
    bool isPaused() const;
    bool isOpen() const;
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled()const;

    void setVideoFrameCallback(VideoFrameCallback callback);
    void renderNextFrame();
    void requestSeek(double targetSeconds);
    void setSubtitleFontScale(double scale);

    std::vector<AudioTrackInfo> getAudioTracks() const;
    std::vector<SubtitleTrackInfo> getSubtitleTracks() const;
    void requestAudioTrackChange(int streamIndex);
    int getCurrentAudioStreamIndex() const;

    void requestSubtitleTrackChange(int streamIndex);
    int getCurrentSubtitleStreamIndex() const;
    void requestLoadSubtitleFile(const std::string& filepath);

    enum class ErrorSeverity { Info, Warning, Error };
    using ErrorCallback = std::function<void(ErrorSeverity severity, const std::string& message)>;
    

    void setErrorCallback(ErrorCallback callback);
    

    FrameQueue<AudioFrame>& getAudioQueue() { return m_audioQueue; }
    AVSync&                 getAVSync()     { return m_avSync;     }

    double getDuration()  const;
    double getPosition()  const;
    int    getWidth()     const;
    int    getHeight()    const;
    double getFrameRate() const;

private:
    void decodeLoop();
    bool openVideoCodec();
    bool openAudioCodec();
    void decodeVideoPacket(AVPacket* packet);
    void decodeAudioPacket(AVPacket* packet);
    void performSeek(double targetSeconds);
    bool initHwDecoder();
    void performAudioTrackChange(int newStreamIndex);
    void compositeSubtitles(uint8_t* rgbData, int linesize, int width, int height, double videoPts);
    void compositeBitmapSubtitle(uint8_t* rgbData, int linesize, int width, int height, double videoPts);
    static AVPixelFormat getHwFormat(AVCodecContext* ctx, const AVPixelFormat* formats);

    void cancelPendingAudioSwitch();
    void decodePendingAudioPacket(AVPacket* packet);
    bool initSubtitleSystem();   
    bool openSubtitleCodec(); 
    void decodeSubtitlePacket(AVPacket* packet);
    void performSubtitleTrackChange(int newStreamIndex);

    void performLoadSubtitleFile(const std::string& filepath);

    ErrorCallback m_errorCallback;
    void reportError(ErrorSeverity severity, const std::string& message);

    std::atomic<bool> m_subtitleFileLoadRequested;
    std::string       m_pendingSubtitleFilePath;
    std::mutex        m_pendingSubtitleFileMutex;


    AVFormatContext* m_formatCtx;
    AVCodecContext*  m_videoCodecCtx;
    AVCodecContext*  m_audioCodecCtx;
    AVFrame*         m_videoFrame;
    AVFrame*         m_audioFrame;
    AVFrame*         m_videoFrameRGB;
    SwsContext*      m_swsCtx;
    SwrContext*      m_swrCtx;
    AVFrame*  m_swFrame;
    AVBufferRef* m_hwDeviceCtx;

    bool             m_audioSwitchPending;
    int              m_pendingAudioStreamIndex;
    AVCodecContext*  m_pendingAudioCodecCtx;
    SwrContext*      m_pendingSwrCtx;
    AVFrame*         m_pendingAudioFrame;

    ASS_Library*  m_assLibrary;
    ASS_Renderer* m_assRenderer;
    ASS_Track*    m_assTrack;

    int              m_subtitleStreamIndex;
    AVCodecContext*  m_subtitleCodecCtx;
    std::deque<BitmapSubtitle> m_bitmapSubtitles;

    int    m_videoStreamIndex;
    int    m_audioStreamIndex;
    int    m_width;
    int    m_height;
    double m_frameRate;
    double m_duration;
    int    m_sampleRate;
    int    m_channels;
    bool m_hwDecodeActive;

    uint8_t* m_rgbBuffer;

    FrameQueue<VideoFrame> m_videoQueue;
    FrameQueue<AudioFrame> m_audioQueue;
    AVSync                 m_avSync;

    std::thread       m_decodeThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_isOpen;
    std::atomic<bool> m_paused;
    std::mutex m_subtitleMutex;
    std::atomic<bool> m_loopEnabled;
    
    
    std::atomic<bool> m_seekRequested;
    std::atomic<double> m_seekTarget;
    std::atomic<bool> m_audioTrackChangeRequest;
    std::atomic<int> m_audioTrackChangeTarget;
    std::atomic<bool> m_subtitleTrackChangeRequest;
    std::atomic<int>  m_subtitleTrackChangeTarget;


    VideoFrameCallback m_videoFrameCallback;

    double m_startPts;
    std::chrono::time_point<std::chrono::steady_clock> m_startWallTime;
};

}
