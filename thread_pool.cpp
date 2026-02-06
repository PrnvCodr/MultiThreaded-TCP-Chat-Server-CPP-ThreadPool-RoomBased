#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) {
    // Ensure at least 1 thread
    if (num_threads == 0) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        num_threads = sysinfo.dwNumberOfProcessors;
        if (num_threads == 0) num_threads = 1;
    }
    
    // Create worker threads (w32::Thread automatically starts)
    for (size_t i = 0; i < num_threads; ++i) {
        workers.push_back(w32::Thread([this, i] {
            while (true) {
                std::function<void()> task;
                
                {
                    w32::LockGuard lock(queue_mutex);
                    
                    // Wait until there's a task or we're stopping
                    condition.wait(lock, [this] {
                        return stop.load() || !tasks.empty();
                    });
                    
                    // Exit if stopping and no tasks left
                    if (stop.load() && tasks.empty()) {
                        return;
                    }
                    
                    // Get the next task
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                
                // Execute the task
                active_tasks++;
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "[ThreadPool] Task exception: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "[ThreadPool] Unknown task exception" << std::endl;
                }
                active_tasks--;
            }
        }));
    }
    
    std::cout << "[ThreadPool] Created with " << num_threads << " worker threads" << std::endl;
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        w32::LockGuard lock(queue_mutex);
        if (stop.load()) {
            return; // Already stopped
        }
        stop.store(true);
    }
    
    // Wake up all threads
    condition.notify_all();
    
    // Wait for all threads to finish
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    std::cout << "[ThreadPool] Shutdown complete" << std::endl;
}

size_t ThreadPool::pending_tasks() const {
    w32::LockGuard lock(queue_mutex);
    return tasks.size();
}
