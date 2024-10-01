#ifndef ACTIVEOBJECT_HPP
#define ACTIVEOBJECT_HPP

#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class ActiveObject {
public:
    // Constructor to initialize the thread pool with a given number of threads
    ActiveObject(int numThreads);

    // Destructor to ensure proper shutdown
    ~ActiveObject();

    // Method to enqueue tasks into the task queue
    void enqueueTask(std::function<void()> task);

    // Method to cancel all pending tasks (before shutdown)
    void cancelAllTasks();

    // Method to gracefully shut down all worker threads
    void shutdown();

private:
    // Internal worker thread function that processes tasks
    void workerThread();

    // Method to clear all pending tasks from the task queue
    void clearPendingTasks();

    // Thread pool
    std::vector<std::thread> workers;

    // Task queue   
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex mtx;
    std::condition_variable cv;

    // Flags to control the running state and task cancelation
    bool running;            // Indicates whether the ActiveObject is still running
    bool cancelingTasks;      // Indicates whether tasks are being canceled
};

#endif  // ACTIVEOBJECT_HPP
