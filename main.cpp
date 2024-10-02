#include <iostream>
#include <thread>
#include <netinet/in.h> // For socket programming
#include <unistd.h>     // For close() function
#include <sstream>
#include <string>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <set>          // To track active clients
#include "MST_algo.hpp"
#include "graph.hpp"
#include "Pipeline.hpp"
#include "Activeobject.hpp"

#define PORT 8080
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 4

std::mutex leaderMutex;
std::condition_variable leaderCV;
std::queue<int> clientQueue;

// A set to track all active client sockets
std::set<int> activeClientSockets;
std::mutex clientSocketMutex; // Mutex to protect access to activeClientSockets

std::atomic<bool> serverRunning(true);
std::atomic<int> activeClients(0);
int serverFd;

// Function to close all active client connections
void closeAllClients() {
    std::lock_guard<std::mutex> lock(clientSocketMutex);
    for (int clientSocket : activeClientSockets) {
        shutdown(clientSocket, SHUT_RDWR); // Disable read/write operations on the client socket
        close(clientSocket);               // Close the client socket
        std::cout << "Closed client socket: " << clientSocket << std::endl;
    }
    activeClientSockets.clear(); // Clear the set of active clients
}

// Function to handle client requests
void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE] = {0};
    std::unique_ptr<Graph> graph;

    // Add the client socket to the set of active clients
    {
        std::lock_guard<std::mutex> lock(clientSocketMutex);
        activeClientSockets.insert(clientSocket);
    }

    activeClients++; // Increment active clients counter

    while (serverRunning)
    {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);
        if (bytesRead <= 0)
        {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request(buffer, bytesRead);
        std::stringstream ss(request);
        std::string command;
        ss >> command;

        Pipeline pipeline;

        if (command == "CREATE")
        {
            pipeline.addStep([&]() {
                int size;
                ss >> size;
                graph = std::make_unique<Graph>(size);
                std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        }
        else if (command == "ADD")
        {
            pipeline.addStep([&]() {
                if (!graph)
                {
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
        }
        else if (command == "REMOVE")
        {
            pipeline.addStep([&]() {
                if (!graph)
                {
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
        }
    else if (command == "SOLVE")
{
    pipeline.addStep([&]() {
        if (!graph)
        {
            std::string response = "Graph is not created. Use CREATE command first.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
            return;
        }

        std::string algorithm;
        ss >> algorithm;
        MSTAlgo* algo = nullptr;

        if (algorithm == "PRIM")
        {
            algo = MSTFactory::createMSTAlgorithm(MSTFactory::PRIM);
        }
        else if (algorithm == "KRUSKAL")
        {
            algo = MSTFactory::createMSTAlgorithm(MSTFactory::KRUSKAL);
        }

        if (algo)
        {
            MSTTree mst = algo->computeMST(*graph);

            // Construct the response string to send to the client and only
            std::vector<std::pair<int, int>> mstEdges = mst.getEdges();
            std::string response = "Following are the edges in the constructed MST:\n";
            for (const auto& edge : mstEdges) {
                int u = edge.first;
                int v = edge.second;
                int weight = graph->getAdjacencyMatrix()[u][v];
                response += std::to_string(u) + " -- " + std::to_string(v) + " == " + std::to_string(weight) + "\n";
            }

            // Append the total weight to the response
            int totalWeight = mst.getTotalWeight();
            response += "Minimum Cost Spanning Tree: " + std::to_string(totalWeight) + "\n";

            // Send the response to the client
            send(clientSocket, response.c_str(), response.size(), 0);

            // Cleanup
            delete algo;
        }
        else
        {
            std::string response = "Unknown algorithm requested.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    });
}

  else if (command == "SHUTDOWN")
        {
            pipeline.addStep([&]() {
                std::string response = "Shutting down this client.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                std::cout << "Client initiated shutdown command." << std::endl;

                // Close only this client connection
                close(clientSocket);  // Close the client socket to end the connection
                {
                    std::lock_guard<std::mutex> lock(clientSocketMutex);
                    activeClientSockets.erase(clientSocket);  // Remove this client from the active clients set
                }

                activeClients--; // Decrement active clients counter
                return;  // Exit the loop for this client only
            });
        }
        else
        {
            pipeline.addStep([&]() {
                std::string response = "Unknown command.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        }

        // Execute the pipeline for the current command
        pipeline.execute();
    }

    // Ensure the client is removed and socket closed if not done earlier
    {
        std::lock_guard<std::mutex> lock(clientSocketMutex);
        activeClientSockets.erase(clientSocket);
    }

    close(clientSocket);  // Close the client socket connection after processing
    std::cout << "Client socket closed.\n";
}

void runServerWithLeaderFollower()
{
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int newSocket;

    // Create an ActiveObject with a thread pool of THREAD_POOL_SIZE
    ActiveObject activeObject(THREAD_POOL_SIZE);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running and listening on port " << PORT << std::endl;

    // Start a thread pool for handling client requests
    std::vector<std::thread> threadPool;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
    {
        threadPool.emplace_back([&]()
        {
            while (serverRunning)
            {
                int clientSocket;

                // Leader-Follower mechanism: One thread acts as the leader and processes a new connection.
                {
                    std::unique_lock<std::mutex> lock(leaderMutex);
                    leaderCV.wait(lock, [&]() { return !clientQueue.empty() || !serverRunning; });

                    if (!serverRunning)
                    {
                        return;  // Exit the thread immediately if the server is shutting down
                    }

                    if (!clientQueue.empty())
                    {
                        clientSocket = clientQueue.front();
                        clientQueue.pop();
                    }
                }

                // Process client requests asynchronously using ActiveObject
                activeObject.enqueueTask([clientSocket]() {
                    handleClient(clientSocket);
                });
            }
        });
    }

    // Separate thread to listen for shutdown command from the server console
    std::thread shutdownThread([&]()
    {
        std::string input;
        while (serverRunning)
        {
            std::cin >> input;
            if (input == "shutdown")
            {
                std::cout << "Server shutting down...\n";
                serverRunning = false;  // Signal to stop the server

                // Properly shutdown the server socket to unblock accept() and stop listening
                shutdown(serverFd, SHUT_RDWR);  // Disable read/write operations
                close(serverFd);  // Close server socket

                closeAllClients(); // Close all connected clients

                leaderCV.notify_all();  // Notify all waiting threads to stop
                break;
            }
        }
    });

    // Main loop to accept clients
    while (serverRunning)
    {
        newSocket = accept(serverFd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (newSocket >= 0)
        {
            std::cout << "New client connection accepted.\n";
            {
                std::lock_guard<std::mutex> lock(leaderMutex);
                clientQueue.push(newSocket);
            }
            leaderCV.notify_one(); // Notify a thread to handle this client
        }
        else if (!serverRunning)
        {
            break; // Exit the loop if the server is shutting down
        }
    }

    // Wait for the shutdown thread to finish
    shutdownThread.join();

    // Instead of detaching, we now join all threads to ensure they finish gracefully
    for (auto &th : threadPool)
    {
        if (th.joinable())
        {
            th.join();  // Wait for all threads to finish before exiting
        }
    }

    std::cout << "Server has shut down immediately.\n";
}



int main()
{
    runServerWithLeaderFollower(); // Start the server with Leader-Follower
    return 0;
}
