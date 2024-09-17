#include <iostream>
#include <thread>
#include <netinet/in.h>  // For socket programming
#include <unistd.h>      // For close()
#include <sstream>
#include <string>
#include "MST_algo.hpp"
#include "graph.hpp"
#include "pipeline.hpp"
#include "active_object.hpp"

// Define server port and buffer size
#define PORT 8080
#define BUFFER_SIZE 1024

void handleClient(int clientSocket, std::unique_ptr<Graph>& graph) {
    char buffer[BUFFER_SIZE] = {0};
    
    // Continue reading commands from the client until they close the connection
    while (true) {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);
        
        // If no data is received or the connection is closed, break the loop
        if (bytesRead <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request(buffer, bytesRead);
        std::stringstream ss(request);
        std::string command;
        ss >> command;

        if (command == "CREATE") {
            int size;
            ss >> size;
            graph = std::unique_ptr<Graph>(new Graph(size));  // Create a new graph with the specified size
            std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        } 
        else if (command == "ADD") {
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
        } 
        else if (command == "REMOVE") {
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
        } 
        else if (command == "SOLVE") {
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
                delete algo;  // Clean up the algorithm object
            } else {
                std::string response = "Unknown algorithm requested.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
        } 
        else {
            std::string response = "Unknown command.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    close(clientSocket);  // Close the connection after processing
}
// Server function to handle incoming requests
void runServer() {
    int serverFd, newSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating server socket
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set server address and bind to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(serverFd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running and listening on port " << PORT << std::endl;

    // Active Object for processing tasks asynchronously
    ActiveObject activeObject;

    // Main server loop to accept and process client requests
    while (true) {
        if ((newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection established with client" << std::endl;

        // Create a unique_ptr to the graph object
        std::unique_ptr<Graph> graph;

        // Enqueue the task to handle the client request using the Active Object
        activeObject.enqueueTask([newSocket, &graph]() {
            handleClient(newSocket, graph);
        });
    }

    close(serverFd);  // Close server when finished
}

int main() {
    // Start the server in the main function
    runServer();

    return 0;
}
