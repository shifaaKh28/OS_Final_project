#include <iostream>
#include <thread>
#include <netinet/in.h> // For socket programming (struct sockaddr_in, etc.)
#include <unistd.h>     // For close() function
#include <sstream>      // For string stream (to parse client requests)
#include <string>
#include <atomic>       // Include this for std::atomic
#include "MST_algo.hpp"
#include "graph.hpp"
#include "Pipeline.hpp"
#include "Activeobject.hpp" // Custom header for active object (asynchronous task management)

#define PORT 8080        // The port on which the server will listen
#define BUFFER_SIZE 1024 // Buffer size for reading data from the client

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
            graph = std::unique_ptr<Graph>(new Graph(size));  // Create a new graph for this client
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

// Function to run the server
void runServer() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int newSocket;  // Declare newSocket outside the while loop

    // Create an ActiveObject with a thread pool of 4 threads
    ActiveObject activeObject(4);

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

    // Start a separate thread to listen for console input for shutdown
    std::thread shutdownThread([]() {
        std::string input;
        while (serverRunning) {
            std::cin >> input;  // Read input from the server's console
            if (input == "shutdown") {
                serverRunning = false;  // Set the flag to stop the server
                std::cout << "Shutting down the server..." << std::endl;
                close(serverFd);  // Close the listening socket to unblock accept()
                break;  // Exit the loop, allowing shutdown to proceed
            }
        }
    });

    // Server main loop, runs while serverRunning is true
    while (serverRunning) {
        newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (newSocket < 0) {
            if (serverRunning) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            break;  // Exit the loop if accept() fails after serverRunning is set to false
        }

        std::cout << "Connection established with client" << std::endl;

        // Enqueue the client handling task using ActiveObject
        activeObject.enqueueTask([newSocket]() {
            handleClient(newSocket);  // Handle the client in a separate task
        });
    }

    shutdownThread.join();  // Wait for the shutdown thread to finish
    std::cout << "Server has shut down gracefully.\n";
}

int main() {
    runServer();  // Start the server
    return 0;
}
