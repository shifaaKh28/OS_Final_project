#include <iostream>
#include <thread>
#include <netinet/in.h> // For socket programming (struct sockaddr_in, etc.)
#include <unistd.h>     // For close() function
#include <sstream>      // For string stream (to parse client requests)
#include <string>
#include <atomic>       // Include this for std::atomic
#include <queue>
#include <condition_variable>
#include <mutex>
#include "MST_algo.hpp"
#include "graph.hpp"
#include "Pipeline.hpp"
#include "Activeobject.hpp" // Custom header for active object (asynchronous task management)

#define PORT 8080        // The port on which the server will listen
#define BUFFER_SIZE 1024 // Buffer size for reading data from the client
#define THREAD_POOL_SIZE 4 // Number of threads in the Leader-Follower thread pool

// Thread pool management for Leader-Follower
std::mutex leaderMutex;
std::condition_variable leaderCV;
std::queue<int> clientQueue;

std::atomic<bool> serverRunning(true);  // Global flag for server running status
int serverFd;  // Declare serverFd globally so it can be accessed from multiple threads

// This function handles the requests coming from a single client
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};  
    std::unique_ptr<Graph> graph;  // Each client gets its own graph

    // Client handling loop
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

        // Handle CREATE, ADD, REMOVE, SOLVE commands
        if (command == "CREATE") {
            int size;
            ss >> size;
            graph = std::make_unique<Graph>(size);  // Create a new graph for this client
            std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        } else if (command == "ADD") {
            if (!graph) {
                std::string response = "Graph is not created. Use CREATE command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                continue;
            }

            int u, v, weight;
            ss >> u >> v >> weight;
            graph->addEdge(u, v, weight);  
            std::string response = "Edge added: (" + std::to_string(u) + ", " + std::to_string(v) + ") with weight " + std::to_string(weight) + "\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        } else if (command == "REMOVE") {
            if (!graph) {
                std::string response = "Graph is not created. Use CREATE command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                continue;
            }

            int u, v;
            ss >> u >> v;
            graph->removeEdge(u, v);
            std::string response = "Edge removed: (" + std::to_string(u) + ", " + std::to_string(v) + ")\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        } else if (command == "SOLVE") {
            if (!graph) {
                std::string response = "Graph is not created. Use CREATE command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                continue;
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
        } else if (command == "SHUTDOWN") {
            std::string response = "Server shutting down.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
            std::cout << "Client initiated shutdown command." << std::endl;
            serverRunning = false;  // Signal to stop the server
            close(serverFd);  // Close the server socket to unblock accept()
            break;
        } else {
            std::string response = "Unknown command.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    close(clientSocket);  // Close the client socket connection after processing
    std::cout << "Client socket closed.\n";  // Debug message to indicate client socket closed
}

// Function to run the server with Leader-Follower and Active Object Pattern
void runServerWithLeaderFollower() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int newSocket;  // Declare newSocket outside the while loop

    // Create an ActiveObject with a thread pool of 4 threads
    ActiveObject activeObject(THREAD_POOL_SIZE);

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

    // Thread pool for Leader-Follower pattern
    std::vector<std::thread> threadPool;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        threadPool.emplace_back([&]() {
            while (serverRunning) {
                int clientSocket;

                // Leader-Follower mechanism: One thread acts as the leader
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
                activeObject.enqueueTask([clientSocket]() {
                    handleClient(clientSocket);  // Process client requests through ActiveObject
                });
            }
        });
    }

    // Accept new connections and assign them to a thread
    while (serverRunning) {
        newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (newSocket >= 0) {
            std::cout << "New client connection accepted.\n";

            {
                std::lock_guard<std::mutex> lock(leaderMutex);
                clientQueue.push(newSocket);
            }
            leaderCV.notify_one();  // Notify a thread to become the leader and handle the connection
        }
    }

    for (auto& th : threadPool) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::cout << "Server has shut down gracefully.\n";
}

int main() {
    runServerWithLeaderFollower();  // Start the server with Leader-Follower
    return 0;
}
