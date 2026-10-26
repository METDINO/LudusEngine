#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace fire {
class ThreadPool {
public:
    explicit ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency()) {
        if (thread_count == 0) {
            thread_count = 1;
        }

        try {
            for (std::size_t i = 0; i < thread_count; ++i) {
                m_workers.emplace_back(&ThreadPool::WorkerLoop, this);
            }
        } catch (...) {
            stop();
            throw;
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using result_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<result_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<result_type> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock{m_mtx};
            if (m_stop) {
                throw std::runtime_error("thread pool is stopping");
            }
            m_tasks.emplace([task]() {
                try {
                    (*task)();
                } catch (...) {
                }
            });
        }
        m_cv.notify_one();
        return result;
    }

    ~ThreadPool() {
        stop();
        for (auto& w : m_workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock{m_mtx};
                m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

                if (m_stop && m_tasks.empty()) {
                    return;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            task();
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock{m_mtx};
            m_stop = true;
        }
        m_cv.notify_all();
    }

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_stop{false};
};
} // namespace fire