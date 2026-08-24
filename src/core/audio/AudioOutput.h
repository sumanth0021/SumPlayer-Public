#pragma once

#include "core/playback/AVSync.h"
#include "core/playback/FrameQueue.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QObject>

#include <atomic>
#include <chrono>
#include <memory>

namespace SumPlayer
{

    class AudioOutput : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioOutput(QObject *parent = nullptr);
        ~AudioOutput();

        bool start(int sampleRate, int channels,
                   FrameQueue<AudioFrame> &audioQueue,
                   AVSync &avSync);
        void stop();

        void pause();
        void resume();
        bool isPaused() const;

        void setVolume(float volume);
        float getvolume() const;
        void resetAnchor(bool flushSink = false);

    private slots:
        void onTick();

    private:
        void feedDevice();
        void updateClock();
        void applyVolume(uint8_t *data, int byteCount);

        std::unique_ptr<QAudioSink> m_audioSink;
        QIODevice *m_audioDevice;
        FrameQueue<AudioFrame> *m_audioQueue;
        AVSync *m_avSync;
        class QTimer *m_feedTimer;

        int m_sampleRate;
        int m_channels;

        AudioFrame m_pendingFrame;
        size_t m_pendingOffset;
        bool m_hasPending;

        bool m_firstFrameFed;
        double m_firstPts;
        std::chrono::steady_clock::time_point m_firstFeedTime;

        bool m_paused;
        std::atomic<float> m_volume;
    };

}
