#include "Activeobject.hpp"
#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>

ActiveObject::ActiveObject(int numThreads) : running(true) {
    for (int i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ActiveObject::workerThread, this);
    }
}

// Destructor of ActiveObject to stop all threads immediately
ActiveObject::~ActiveObject() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        running = false;  // Signal all threads to stop
        while (!tasks.empty()) {
            tasks.pop();  // Clear all pending tasks
        }
    }
    cv.notify_all();  // Notify all threads waiting on the condition variable
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.detach();  // Detach threads to avoid waiting
        }
    }
}

void ActiveObject::enqueueTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ActiveObject::workerThread() {
    while (running) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return !tasks.empty() || !running; });
            if (!running && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
        std::cout << "Task executed by thread." << std::endl;
    }
}
