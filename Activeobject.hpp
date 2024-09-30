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
    ActiveObject(int numThreads);
    ~ActiveObject();

    void enqueueTask(std::function<void()> task);

private:
    void workerThread();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool running;
};

#endif  // ACTIVEOBJECT_HPP
