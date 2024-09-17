#include <iostream>        
#include <thread>         
#include <unistd.h>       
#include <sstream>         
#include <string>          
#include "graph.hpp"       
#include "Pipeline.hpp"    
#include "Activeobject.hpp"

// Define constants for server port and buffer size
#define PORT 8080              // The port on which the server will listen
#define BUFFER_SIZE 1024       // Buffer size for reading data from the client

/**
 * This function handles the requests coming from a single client. It reads the client's commands,
 * modifies the graph (if applicable), and processes MST algorithms based on the command.
 * The function loops until the client disconnects.
 *
 * @param clientSocket - the socket file descriptor for the client connection
 * @param graph - a unique pointer to the graph data structure that will be manipulated
 */
void handleClient(int clientSocket, std::unique_ptr<Graph>& graph) {
    char buffer[BUFFER_SIZE] = {0};  // Create a buffer to store client requests
    
    // Infinite loop to keep reading and processing commands from the client until disconnection
    while (true) {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);  // Read from the client

        // If no data is received (client disconnected or error), exit the loop
        if (bytesRead <= 0) {
            std::cout << "Client disconnected.\n";  // Notify that client has disconnected
            break;  // Exit the loop to stop processing this client
        }

        // Convert the received buffer data into a string for easier processing
        std::string request(buffer, bytesRead);
        std::stringstream ss(request);  // Create a stringstream object for parsing the request
        std::string command;            // Variable to hold the command part of the request
        ss >> command;                  // Extract the command (e.g., CREATE, ADD, REMOVE, SOLVE)

        // Handle the "CREATE" command to create a new graph with the specified number of vertices
        if (command == "CREATE") {
            int size;                    // Variable to store the size of the graph
            ss >> size;                  // Read the size from the client request
            graph = std::unique_ptr<Graph>(new Graph(size));  // Create a new graph object
            std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";  // Prepare response
            send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
        }
        // Handle the "ADD" command to add an edge between two vertices with a specific weight
        else if (command == "ADD") {
            if (!graph) {  // If the graph hasn't been created yet
                std::string response = "Graph is not created. Use CREATE command first.\n";  // Error response
                send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
                continue;  // Go back to the loop to await a new command
            }

            int u, v, weight;  // Variables to store vertices and weight
            ss >> u >> v >> weight;  // Extract the vertices and weight from the request
            graph->addEdge(u, v, weight);  // Add the edge to the graph
            std::string response = "Edge added: (" + std::to_string(u) + ", " + std::to_string(v) + ") with weight " + std::to_string(weight) + "\n";  // Prepare response
            send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
        } 
        // Handle the "REMOVE" command to remove an edge between two vertices
        else if (command == "REMOVE") {
            if (!graph) {  // If the graph hasn't been created yet
                std::string response = "Graph is not created. Use CREATE command first.\n";  // Error response
                send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
                continue;  // Go back to the loop to await a new command
            }

            int u, v;  // Variables to store the vertices of the edge to be removed
            ss >> u >> v;  // Extract the vertices from the request
            graph->removeEdge(u, v);  // Remove the edge from the graph
            std::string response = "Edge removed: (" + std::to_string(u) + ", " + std::to_string(v) + ")\n";  // Prepare response
            send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
        } 
        // Handle the "SOLVE" command to solve the MST using either Prim's or Kruskal's algorithm
        else if (command == "SOLVE") {
            if (!graph) {  // If the graph hasn't been created yet
                std::string response = "Graph is not created. Use CREATE command first.\n";  // Error response
                send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
                continue;  // Go back to the loop to await a new command
            }

            std::string algorithm;  // Variable to store the algorithm type
            ss >> algorithm;        // Extract the algorithm type from the request (e.g., PRIM or KRUSKAL)

            MSTAlgo* algo = nullptr;  // Pointer to the algorithm object
            if (algorithm == "PRIM") {
                algo = MSTFactory::createMSTAlgorithm(MSTFactory::PRIM);  // Use Prim's algorithm
            } else if (algorithm == "KRUSKAL") {
                algo = MSTFactory::createMSTAlgorithm(MSTFactory::KRUSKAL);  // Use Kruskal's algorithm
            }

            if (algo) {
                MSTTree mst = algo->computeMST(*graph);  // Compute the MST of the current graph
                int totalWeight = mst.getTotalWeight();  // Get the total weight of the MST
                std::string response = "MST total weight: " + std::to_string(totalWeight) + "\n";  // Prepare response
                send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
                delete algo;  // Clean up the algorithm object
            } else {
                std::string response = "Unknown algorithm requested.\n";  // Error response for unknown algorithm
                send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
            }
        } 
        // Handle any unknown commands
        else {
            std::string response = "Unknown command.\n";  // Error response for unknown command
            send(clientSocket, response.c_str(), response.size(), 0);  // Send response to client
        }
    }

    close(clientSocket);  // Close the client socket connection after processing
}

/**
 * This function sets up the server socket, listens for incoming client connections,
 * and processes their requests asynchronously using the Active Object pattern.
 */
void runServer() {
    int serverFd, newSocket;  // File descriptors for the server and client sockets
    struct sockaddr_in address;  // Server address structure
    int opt = 1;  // Option value for setsockopt()
    int addrlen = sizeof(address);  // Size of the address structure

    // Creating a socket for the server
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");  // If socket creation fails, print an error and exit
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of address and port
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");  // If setting options fails, print an error and exit
        exit(EXIT_FAILURE);
    }

    // Setting up the server address and binding it to the specified port
    address.sin_family = AF_INET;  // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Bind to any available network interface
    address.sin_port = htons(PORT);  // Convert port number to network byte order

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");  // If binding fails, print an error and exit
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming client connections (maximum 3 queued connections)
    if (listen(serverFd, 3) < 0) {
        perror("Listen failed");  // If listening fails, print an error and exit
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running and listening on port " << PORT << std::endl;  // Notify that the server is ready

    // Active Object for processing client requests asynchronously
    ActiveObject activeObject;

    // Infinite loop to continuously accept and handle new client connections
    while (true) {
        // Accept a new client connection
        if ((newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");  // If accepting a connection fails, print an error and continue
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection established with client" << std::endl;  // Notify that a new client has connected

        // Create a unique_ptr to the graph object for each client session
        std::unique_ptr<Graph> graph;

        // Enqueue the task to handle the client request asynchronously
        activeObject.enqueueTask([newSocket, &graph]() {
            handleClient(newSocket, graph);  // Pass the client socket and graph to the handler function
        });
    }

    close(serverFd);  // Close the server socket when done (this won't happen in this loop)
}

/**
 * Main function to start the server.
 * It calls runServer(), which handles the entire process of accepting and processing client requests.
 */
int main() {
    runServer();  // Start the server by calling runServer()

    return 0;  // Return 0 to indicate successful execution (this line will never be reached due to the infinite loop)
}
