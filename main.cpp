#include <iostream>
#include <thread>
#include <netinet/in.h> // For socket programming (struct sockaddr_in, etc.)
#include <unistd.h>     // For close() function
#include <sstream>      // For string stream (to parse client requests)
#include <string>
#include <atomic>       // For std::atomic to manage shared state
#include <queue>
#include <condition_variable>
#include <mutex>
#include "MST_algo.hpp"
#include "graph.hpp"
#include "Pipeline.hpp"
#include <csignal>
#include "Activeobject.hpp" // Custom header for active object (asynchronous task management)

#define PORT 8080        // The port on which the server will listen
#define BUFFER_SIZE 1024 // Buffer size for reading data from the client
#define THREAD_POOL_SIZE 4 // Number of threads in the Leader-Follower thread pool

// Thread pool management for Leader-Follower pattern
std::mutex leaderMutex;
std::condition_variable leaderCV;
std::queue<int> clientQueue;

std::atomic<bool> serverRunning(true);  // Global flag for server running status
int serverFd;  // Declare serverFd globally so it can be accessed from multiple threads


// Signal handler to catch Ctrl + C (SIGINT)
void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Shutting down server gracefully..." << std::endl;

    // Set the flag to stop the server
    serverRunning = false;

    // Close the server socket to unblock the `accept()` call
    close(serverFd);

    // Perform any other cleanup you need here
}

/**
 * This function handles the requests coming from a single client.
 * It reads the client's commands (CREATE, ADD, REMOVE, SOLVE, etc.),
 * modifies the graph, and processes MST algorithms based on the command.
 * 
 * @param clientSocket - socket file descriptor for the client connection.
 */
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};  
    std::unique_ptr<Graph> graph;  // Each client gets its own graph

    while (serverRunning) {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);
        if (bytesRead <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request(buffer, bytesRead);
        std::stringstream ss(request);
        std::string command;
        ss >> command;

        // Create a pipeline for processing this request
        Pipeline pipeline;

        if (command == "CREATE") {
            pipeline.addStep([&]() {
                int size;
                ss >> size;
                graph = std::make_unique<Graph>(size);  // Create a new graph for this client
                std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        } else if (command == "ADD") {
            pipeline.addStep([&]() {
                if (!graph) {
                    std::string response = "Graph is not created. Use CREATE command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }

                int u, v, weight;
                ss >> u >> v >> weight;
                graph->addEdge(u, v, weight);  
                std::string response = "Edge added: (" + std::to_string(u) + ", " + std::to_string(v) + ") with weight " + std::to_string(weight) + "\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        } else if (command == "REMOVE") {
            pipeline.addStep([&]() {
                if (!graph) {
                    std::string response = "Graph is not created. Use CREATE command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }

                int u, v;
                ss >> u >> v;
                graph->removeEdge(u, v);
                std::string response = "Edge removed: (" + std::to_string(u) + ", " + std::to_string(v) + ")\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        } else if (command == "SOLVE") {
            pipeline.addStep([&]() {
                if (!graph) {
                    std::string response = "Graph is not created. Use CREATE command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }

                std::string algorithm;
                ss >> algorithm;

                MSTAlgo* algo = nullptr;
                if (algorithm == "PRIM") {
                    algo = MSTFactory::createMSTAlgorithm(MSTFactory::PRIM);
                } else if (algorithm == "KRUSKAL") {
                    algo = MSTFactory::createMSTAlgorithm(MSTFactory::KRUSKAL);
                }

                if (algo) {
                    MSTTree mst = algo->computeMST(*graph);
                    int totalWeight = mst.getTotalWeight();
                    std::string response = "MST total weight: " + std::to_string(totalWeight) + "\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    delete algo;
                } else {
                    std::string response = "Unknown algorithm requested.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                }
            });
        } else if (command == "SHUTDOWN") {
            pipeline.addStep([&]() {
                std::string response = "Server shutting down.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                std::cout << "Client initiated shutdown command." << std::endl;
                serverRunning = false;
                close(serverFd);  // Close the server socket to unblock accept()
            });
        } else {
            pipeline.addStep([&]() {
                std::string response = "Unknown command.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        }

        // Execute the pipeline for this request
        pipeline.execute();
    }

    close(clientSocket);
    std::cout << "Client socket closed.\n";
}
/**
 * This function sets up the server and uses the Leader-Follower pattern to accept client connections.
 * Once a client connection is accepted, it is processed asynchronously using the ActiveObject pattern.
 * The ActiveObject manages the task queue and worker threads for processing client requests.
 */
void runServerWithLeaderFollower() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int newSocket;  // Declare newSocket outside the while loop

    // Create an ActiveObject with a thread pool of 4 threads.
    // This is the ActiveObject pattern, which manages a pool of threads and tasks.
    ActiveObject activeObject(THREAD_POOL_SIZE);

    // Set up the server socket
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running and listening on port " << PORT << std::endl;

    // Leader-Follower pattern is used here:
    // Multiple threads wait to become the "leader" and accept client connections.
    // Once a connection is accepted, the leader thread passes the client to the worker threads for processing.
    std::vector<std::thread> threadPool;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        threadPool.emplace_back([&]() {
            while (serverRunning) {
                int clientSocket;

                // Leader-Follower mechanism: One thread acts as the leader and processes a new connection.
                {
                    std::unique_lock<std::mutex> lock(leaderMutex);
                    leaderCV.wait(lock, [&]() { return !clientQueue.empty() || !serverRunning; });

                    if (!serverRunning && clientQueue.empty()) {
                        return;  // Exit the thread if the server is shutting down
                    }

                    clientSocket = clientQueue.front();  // Get client socket
                    clientQueue.pop();
                }

                // Enqueue the client-handling task using ActiveObject
                // ActiveObject asynchronously processes the client's requests.
                activeObject.enqueueTask([clientSocket]() {
                    handleClient(clientSocket);  // Process client requests through ActiveObject
                });
            }
        });
    }

    // Accept new client connections and assign them to the worker threads
    while (serverRunning) {
        newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (newSocket >= 0) {
            std::cout << "New client connection accepted.\n";

            // Add the client socket to the queue and notify a worker thread
            {
                std::lock_guard<std::mutex> lock(leaderMutex);
                clientQueue.push(newSocket);
            }
            leaderCV.notify_one();  // Notify a thread to become the leader and handle the connection
        }
    }

    // Wait for all threads in the thread pool to finish
    for (auto& th : threadPool) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::cout << "Server has shut down gracefully.\n";
}

int main() {
    // Register signal handler for Ctrl + C (SIGINT)
    signal(SIGINT, signalHandler);

    // Start the server
    runServerWithLeaderFollower();

    // Ensure server stops cleanly and coverage data is generated
    std::cout << "Server stopped. Now generating coverage data..." << std::endl;
    return 0;
}

