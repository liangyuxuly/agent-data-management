//
// Created by liangyuxu on 2023/12/21.
//

#include "thread_pool.h"

ThreadPool::ThreadPool(int threads) {
    stop = false;
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();
    for (std::thread &worker: workers)
        worker.join();

    // std::cout << "~ThreadPool()" << std::endl;
}
