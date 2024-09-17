#ifndef ACTIVEOBJECT_HPP
#define ACTIVEOBJECT_HPP

#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class ActiveObject {
public:
    ActiveObject(int numThreads = 4);  // Constructor with number of worker threads
    ~ActiveObject();
    void enqueueTask(std::function<void()> task);  // Enqueue a task for processing

private:
    std::queue<std::function<void()>> tasks;  // Queue of tasks to be executed
    std::mutex mtx;  // Mutex to protect task queue
    std::condition_variable cv;  // Condition variable to notify worker threads
    std::vector<std::thread> workers;  // Vector to hold worker threads
    bool running;  // Flag to indicate whether the ActiveObject is running
    void workerThread();  // Function executed by each worker thread
};

#endif // ACTIVEOBJECT_HPP
