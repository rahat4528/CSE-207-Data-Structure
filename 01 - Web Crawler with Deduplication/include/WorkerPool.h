#ifndef CRAWLER_WORKER_POOL_H
#define CRAWLER_WORKER_POOL_H

#include "Common.h"
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace crawler {

class WorkerPool {
public:
    explicit WorkerPool(size_t numThreads);
    ~WorkerPool();

    void enqueue(std::function<void()> task);

    // Blocks until all currently enqueued tasks and active workers are idle
    void waitIdle();

    void stop();

    size_t threadCount() const noexcept { return m_threads.size(); }

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_idleCv;

    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_activeWorkers{0};
};

} // namespace crawler

#endif // CRAWLER_WORKER_POOL_H

