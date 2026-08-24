#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <vector>
#include <atomic>

namespace SumPlayer
{

struct VideoFrame
{
    std::vector<uint8_t> data;
    int                  width;
    int                  height;
    int                  linesize;
    double               pts;
};

struct AudioFrame
{
    std::vector<uint8_t> data;
    int                  sampleRate;
    int                  channels;
    double               pts;
};

template<typename T>
class FrameQueue
{
public:
    explicit FrameQueue(int maxSize = 64)
        : m_maxSize(maxSize)
        , m_stopped(false)
        , m_interrupted(false)
    {}

    bool push(T frame)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condNotFull.wait(lock, [this]{
            return (int)m_queue.size() < m_maxSize || m_stopped || m_interrupted.load();
        });
        if (m_stopped || m_interrupted.load()) return false;
        m_queue.push(std::move(frame));
        m_condNotEmpty.notify_one();
        return true;
    }

    bool pop(T& frame)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condNotEmpty.wait(lock, [this]{
            return !m_queue.empty() || m_stopped;
        });
        if (m_stopped && m_queue.empty()) return false;
        frame = std::move(m_queue.front());
        m_queue.pop();
        m_condNotFull.notify_one();
        return true;
    }

    bool tryPop(T& frame)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        frame = std::move(m_queue.front());
        m_queue.pop();
        m_condNotFull.notify_one();
        return true;
    }

    bool peek(T& frame) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        frame = m_queue.front();
        return true;
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
        m_condNotEmpty.notify_all();
        m_condNotFull.notify_all();
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) m_queue.pop();
        m_stopped = false;
    }

    void interrupt()
    {
        // Used by seek/track switching to wake a decoder stuck waiting for space.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_interrupted.store(true);
        m_condNotFull.notify_all();
    }

    void clearInterrupt()
    {
        m_interrupted.store(false);
    }

    int size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (int)m_queue.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T>           m_queue;
    mutable std::mutex      m_mutex;
    std::condition_variable m_condNotEmpty;
    std::condition_variable m_condNotFull;
    int                     m_maxSize;
    bool                    m_stopped;
    std::atomic<bool>       m_interrupted;
};

}
