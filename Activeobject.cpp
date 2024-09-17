#include "Activeobject.hpp"

ActiveObject::ActiveObject(int numThreads) : running(true) {
    // Launch the specified number of worker threads
    for (int i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ActiveObject::workerThread, this);  // Start a worker thread
    }
}

ActiveObject::~ActiveObject() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        running = false;  // Signal all threads to stop
    }
    cv.notify_all();  // Notify all threads waiting on the condition variable
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();  // Wait for all threads to finish
        }
    }
}

void ActiveObject::enqueueTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push(std::move(task));  // Add the task to the queue
    }
    cv.notify_one();  // Notify one worker thread that a new task is available
}

void ActiveObject::workerThread() {
    while (running) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return !tasks.empty() || !running; });  // Wait until there is a task or we are stopping
            if (!running && tasks.empty()) {
                return;  // Stop the thread if we are not running and no tasks are left
            }
            task = std::move(tasks.front());  // Get the next task
            tasks.pop();  // Remove the task from the queue
        }
        task();  // Execute the task outside the lock
    }
}
