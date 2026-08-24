#pragma once

#include <atomic>

namespace SumPlayer
{

class AVSync
{
public:
    AVSync()
        : m_audioClock(0.0)
        , m_running(false)
        , m_clockSet(false)
        , m_seekPending(false)
        , m_flushSinkPending(false)
    {}

    void setAudioClock(double pts)
    {
        m_audioClock.store(pts);
        m_clockSet.store(true);
    }

    double getAudioClock() const { return m_audioClock.load(); }
    bool   isClockSet()    const { return m_clockSet.load();   }

    void start()
    {
        m_running.store(true);
        m_clockSet.store(false);
        m_audioClock.store(0.0);
    }

    void stop()
    {
        m_running.store(false);
        m_clockSet.store(false);
    }

    bool isRunning() const { return m_running.load(); }

    void requestClockReset(bool flushSink = false)
    {
        m_seekPending.store(true);
        if (flushSink)
        {
            m_flushSinkPending.store(true);
        }
    }

    
    bool consumePendingReset()
    {
        return m_seekPending.exchange(false);
    }

    bool consumePendingFlush()
    {
        return m_flushSinkPending.exchange(false);
    }

private:
    std::atomic<double> m_audioClock;
    std::atomic<bool>   m_running;
    std::atomic<bool>   m_clockSet;
    std::atomic<bool>   m_seekPending;
    std::atomic<bool>   m_flushSinkPending;
};

}
