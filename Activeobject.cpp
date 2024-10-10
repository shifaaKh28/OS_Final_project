#include "Activeobject.hpp"
#include <iostream>

ActiveObject::ActiveObject(int numThreads) : running(true), cancelingTasks(false)
{
    // Launch the specified number of worker threads
    for (int i = 0; i < numThreads; ++i)
    {
        workers.emplace_back(&ActiveObject::workerThread, this);
    }
}

ActiveObject::~ActiveObject()
{
    shutdown(); // Ensure that the threads are properly shut down
}

// Enqueue tasks to the task queue
void ActiveObject::enqueueTask(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(mtx);
    if (running && !cancelingTasks)
    { // Only enqueue tasks if running and not in shutdown
        tasks.push(std::move(task));
        cv.notify_one(); // Notify one waiting thread that a new task is available
    }
}

// Worker thread function that processes tasks
void ActiveObject::workerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            // Wait for a task to be available or for shutdown
            cv.wait(lock, [this]()
                    { return !tasks.empty() || !running || cancelingTasks; });

            if (!running || cancelingTasks)
            {
                break; // Exit if the object is shutting down or tasks are being cancelled
            }

            if (!tasks.empty())
            {
                task = std::move(tasks.front()); // Get the next task
                tasks.pop();                     // Remove the task from the queue
            }
        }

        // Execute the task outside of the locked section
        if (task)
        {
            task();
        }
    }
}


// Shut down all worker threads gracefully
void ActiveObject::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        running = false; // Signal all threads to stop
    }
    cv.notify_all(); // Wake up all waiting threads
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join(); // Ensure all threads are joined properly
        }
    }
}
