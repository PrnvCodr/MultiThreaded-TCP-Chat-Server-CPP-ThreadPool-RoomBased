#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <type_traits>
#include "win32_compat.h"

/**
 * @brief High-performance thread pool for Windows
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    /**
     * @brief Enqueue a task for execution.
     * simplified to accept std::function<void()> directly.
     * Return value removed as it is unused in this server.
     */
    void enqueue(std::function<void()> task) {
        {
            w32::LockGuard lock(queue_mutex);
            if (stop.load()) {
                // throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
                return; 
            }
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }
    
    size_t pending_tasks() const;
    size_t thread_count() const { return workers.size(); }
    bool is_running() const { return !stop.load(); }
    void shutdown();

private:
    std::vector<w32::Thread> workers;
    std::queue<std::function<void()>> tasks;
    
    mutable w32::Mutex queue_mutex;
    w32::ConditionVariable condition;
    std::atomic<bool> stop{false};
    std::atomic<size_t> active_tasks{0};
};

// Template implementation removed
#endif // THREAD_POOL_H
