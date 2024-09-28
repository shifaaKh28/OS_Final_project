#include <iostream>
#include <thread>
#include <netinet/in.h> // For socket programming (struct sockaddr_in, etc.)
#include <unistd.h>     // For close() function
#include <sstream>      // For string stream (to parse client requests)
#include <string>
#include "MST_algo.hpp"
#include "graph.hpp"
#include "Pipeline.hpp"
#include "Activeobject.hpp" // Custom header for active object (asynchronous task management)

// Define constants for server port and buffer size
#define PORT 8080        // The port on which the server will listen
#define BUFFER_SIZE 1024 // Buffer size for reading data from the client

/**
 * This function handles the requests coming from a single client. It reads the client's commands,
 * modifies the graph (if applicable), and processes MST algorithms based on the command.
 * The function loops until the client disconnects.
 *
 * @param clientSocket - the socket file descriptor for the client connection
 * @param graph - a unique pointer to the graph data structure that will be manipulated
 */
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};  
    std::unique_ptr<Graph> graph;  // Each client gets its own graph

    // Client handling loop
    while (true) {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE);
        if (bytesRead <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request(buffer, bytesRead);
        std::stringstream ss(request);
        std::string command;
        ss >> command;

        // Handle CREATE, ADD, REMOVE, SOLVE commands...
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
        }         // Handle the "REMOVE" command to remove an edge between two vertices
        else if (command == "REMOVE")
        {
            if (!graph)
            {                                                                               // If the graph hasn't been created yet
                std::string response = "Graph is not created. Use CREATE command first.\n"; // Error response
                send(clientSocket, response.c_str(), response.size(), 0);                   // Send response to client
                continue;                                                                   // Go back to the loop to await a new command
            }

            int u, v;                                                                                        // Variables to store the vertices of the edge to be removed
            ss >> u >> v;                                                                                    // Extract the vertices from the request
            graph->removeEdge(u, v);                                                                         // Remove the edge from the graph
            std::string response = "Edge removed: (" + std::to_string(u) + ", " + std::to_string(v) + ")\n"; // Prepare response
            send(clientSocket, response.c_str(), response.size(), 0);                                        // Send response to client
        }
        // Handle the "SOLVE" command to solve the MST using either Prim's or Kruskal's algorithm
        else if (command == "SOLVE")
        {
            if (!graph)
            {                                                                               // If the graph hasn't been created yet
                std::string response = "Graph is not created. Use CREATE command first.\n"; // Error response
                send(clientSocket, response.c_str(), response.size(), 0);                   // Send response to client
                continue;                                                                   // Go back to the loop to await a new command
            }

            std::string algorithm; // Variable to store the algorithm type
            ss >> algorithm;       // Extract the algorithm type from the request (e.g., PRIM or KRUSKAL)

            MSTAlgo *algo = nullptr; // Pointer to the algorithm object
            if (algorithm == "PRIM")
            {
                algo = MSTFactory::createMSTAlgorithm(MSTFactory::PRIM); // Use Prim's algorithm
            }
            else if (algorithm == "KRUSKAL")
            {
                algo = MSTFactory::createMSTAlgorithm(MSTFactory::KRUSKAL); // Use Kruskal's algorithm
            }

            if (algo)
            {
                MSTTree mst = algo->computeMST(*graph);                                           // Compute the MST of the current graph
                int totalWeight = mst.getTotalWeight();                                           // Get the total weight of the MST
                std::string response = "MST total weight: " + std::to_string(totalWeight) + "\n"; // Prepare response
                send(clientSocket, response.c_str(), response.size(), 0);                         // Send response to client
                delete algo;                                                                      // Clean up the algorithm object
            }
            else
            {
                std::string response = "Unknown algorithm requested.\n";  // Error response for unknown algorithm
                send(clientSocket, response.c_str(), response.size(), 0); // Send response to client
            }
        }
        // Add this within the handleClient function to handle the "SHUTDOWN" command
        else if (command == "SHUTDOWN")
        {
            std::string response = "Server shutting down.\n";
            send(clientSocket, response.c_str(), response.size(), 0);
            close(clientSocket); // Close the client socket
            exit(0);             // Exit the server process gracefully
        }
        // Handle any unknown commands
        else
        {
            std::string response = "Unknown command.\n";              // Error response for unknown command
            send(clientSocket, response.c_str(), response.size(), 0); // Send response to client
        }
    }

    close(clientSocket); // Close the client socket connection after processing
}

/**
 * This function sets up the server socket, listens for incoming client connections,
 * and processes their requests asynchronously using the Active Object pattern.
 */
void runServer() {
    int serverFd, newSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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

    while (true) {
        if ((newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection established with client" << std::endl;

        // Create a new thread for each client, passing the client socket
        std::thread clientThread(handleClient, newSocket);
        clientThread.detach();  // Detach the thread to let it run independently
    }

    close(serverFd);
}


/**
 * Main function to start the server.
 * It calls runServer(), which handles the entire process of accepting and processing client requests.
 */
int main()
{
    runServer(); // Start the server by calling runServer()

    return 0; // Return 0 to indicate successful execution (this line will never be reached due to the infinite loop)
}
