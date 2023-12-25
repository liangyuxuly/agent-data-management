#ifndef DIRECTORYCOPY_THREAD_POOL_H
#define DIRECTORYCOPY_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(int threads);

    ~ThreadPool();

    template<class F>
    void enqueue(F &&f) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace(std::forward<F>(f));
        condition.notify_one();
    };

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

#endif //DIRECTORYCOPY_THREAD_POOL_H
