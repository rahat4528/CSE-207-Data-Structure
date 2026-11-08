#include "WorkerPool.h"

namespace crawler {

WorkerPool::WorkerPool(size_t numThreads) {
    size_t count = (numThreads == 0) ? 1 : numThreads;
    m_threads.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        m_threads.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [this]() {
                        return m_stop.load() || !m_tasks.empty();
                    });

                    if (m_stop.load() && m_tasks.empty()) {
                        return;
                    }

                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                    m_activeWorkers++;
                }

                try {
                    task();
                } catch (...) {}

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_activeWorkers--;
                    if (m_tasks.empty() && m_activeWorkers == 0) {
                        m_idleCv.notify_all();
                    }
                }
            }
        });
    }
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks.push(std::move(task));
    }
    m_cv.notify_one();
}

void WorkerPool::waitIdle() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idleCv.wait(lock, [this]() {
        return m_tasks.empty() && m_activeWorkers == 0;
    });
}

void WorkerPool::stop() {
    m_stop.store(true);
    m_cv.notify_all();
    for (auto& th : m_threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    m_threads.clear();
}

} // namespace crawler

