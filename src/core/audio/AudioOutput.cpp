#include "core/audio/AudioOutput.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QTimer>
#include <iostream>
#include <algorithm>

namespace SumPlayer
{

AudioOutput::AudioOutput(QObject* parent)
    : QObject(parent)
    , m_audioDevice(nullptr)
    , m_audioQueue(nullptr)
    , m_avSync(nullptr)
    , m_feedTimer(nullptr)
    , m_sampleRate(44100)
    , m_channels(2)
    , m_pendingOffset(0)
    , m_hasPending(false)
    , m_firstFrameFed(false)
    , m_firstPts(0.0)
    , m_paused(false)
    , m_volume(1.0f)
{
    m_feedTimer = new QTimer(this);
    connect(m_feedTimer, &QTimer::timeout, this, &AudioOutput::onTick);
}

AudioOutput::~AudioOutput()
{
    stop();
}

bool AudioOutput::start(int sampleRate, int channels,
                        FrameQueue<AudioFrame>& audioQueue,
                        AVSync& avSync)
{
    stop();

    m_audioQueue = &audioQueue;
    m_avSync     = &avSync;
    m_sampleRate = sampleRate;
    m_channels   = channels;

    m_pendingOffset  = 0;
    m_hasPending     = false;
    m_firstFrameFed  = false;
    m_firstPts       = 0.0;

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format))
    {
        std::cout << "[AudioOutput] Format not supported." << std::endl;
        return false;
    }

    m_audioSink = std::make_unique<QAudioSink>(device, format);
    m_audioSink->setBufferSize(16384);
    m_audioDevice = m_audioSink->start();

    if (!m_audioDevice)
    {
        std::cout << "[AudioOutput] Failed to start." << std::endl;
        return false;
    }

    m_feedTimer->start(10);

    std::cout << "[AudioOutput] Started. "
              << sampleRate << "Hz " << channels << "ch" << std::endl;
    return true;
}

void AudioOutput::pause()
{
    if(m_paused || !m_audioSink) return;


    if(m_avSync){
        double frozenAt = m_avSync->getAudioClock();
        m_firstPts = frozenAt;
    }

    m_audioSink->suspend();
    m_feedTimer->stop();
    m_paused = true;

    std::cout<< "[AudioOutput] paused." << std::endl;
}

void AudioOutput::resume()
{
    if(!m_paused || !m_audioSink) return;

    m_firstFeedTime = std::chrono::steady_clock::now();
    m_firstFrameFed = true;

    m_audioSink->resume();
    m_feedTimer->start(10);

    m_paused = false;
    std::cout<< "[AudioOutput] Resumed. "<<std::endl;
}

bool AudioOutput::isPaused() const
{
    return m_paused;
}

void AudioOutput::onTick()
{
    if(m_paused) return;

    // Seek/track changes ask the clock to forget its old anchor.
    if(m_avSync && m_avSync->consumePendingReset())
    {
        bool flush = m_avSync->consumePendingFlush();
        resetAnchor(flush);
    }

    feedDevice();
    updateClock();
}

void AudioOutput::resetAnchor(bool flushSink)
{
    m_firstFrameFed = false;
    m_hasPending = false;
}


void AudioOutput::setVolume(float volume)
{
    if(volume < 0.0f) volume = 0.0f;
    if(volume > 4.0f) volume = 4.0f;
    m_volume.store(volume);
}

float AudioOutput::getvolume() const
{
    return m_volume.load();
}

void AudioOutput::applyVolume(uint8_t* data, int byteCount)
{
    float volume = m_volume.load();

    int16_t* samples = reinterpret_cast<int16_t*>(data);
    int cout = byteCount / 2;

    for(int i = 0; i < cout; i++)
    {
        float scaled = static_cast<float>(samples[i] * volume);

        if(scaled > 32767.0f) scaled = 32767.0f ;
        if(scaled < -32768.0f) scaled = -32768.0f ; 

        samples[i] = static_cast<int16_t>(scaled);
    }


}

void AudioOutput::feedDevice()
{
    if (!m_audioDevice || !m_audioQueue) return;

    qint64 bytesFree = m_audioSink->bytesFree();

    while (bytesFree > 0)
    {
        if (m_hasPending)
        {
            // Qt may accept only part of a frame, so we keep the leftover for the next tick.
            uint8_t* ptr = m_pendingFrame.data.data() + m_pendingOffset;
            qint64 remaining   = (qint64)m_pendingFrame.data.size() - (qint64)m_pendingOffset;
            qint64 toWrite     = std::min(bytesFree, remaining);

            applyVolume(ptr, (int)toWrite);

            qint64 written = m_audioDevice->write(
                reinterpret_cast<const char*>(ptr), toWrite);

            if (written <= 0) break;

            m_pendingOffset += written;
            bytesFree        -= written;

            if ((qint64)m_pendingOffset >= (qint64)m_pendingFrame.data.size())
                m_hasPending = false;
            else
                break;

            continue;
        }

        AudioFrame frame;
        if (!m_audioQueue->tryPop(frame))
            break;

        if (!m_firstFrameFed)
        {
            // First fed frame becomes the clock anchor; video follows this.
            m_firstPts      = frame.pts;
            m_firstFeedTime = std::chrono::steady_clock::now();
            m_firstFrameFed = true;
        }

        qint64 toWrite = std::min<qint64>(bytesFree, (qint64)frame.data.size());

        applyVolume(frame.data.data(), (int)toWrite);

        qint64 written = m_audioDevice->write(
            reinterpret_cast<const char*>(frame.data.data()), toWrite);

        if (written <= 0) break;

        bytesFree -= written;

        if (written < (qint64)frame.data.size())
        {
            m_pendingFrame  = std::move(frame);
            m_pendingOffset = (size_t)written;
            m_hasPending    = true;
            break;
        }
    }
}

void AudioOutput::updateClock()
{
    if (!m_avSync || !m_firstFrameFed) return;

    std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - m_firstFeedTime;

    double clock = m_firstPts + elapsed.count();
    m_avSync->setAudioClock(clock);
}

void AudioOutput::stop()
{
    m_feedTimer->stop();

    if (m_audioSink)
    {
        m_audioSink->stop();
        m_audioSink.reset();
    }
    m_audioDevice   = nullptr;
    m_hasPending    = false;
    m_firstFrameFed = false;
    m_paused = false;
}

}
