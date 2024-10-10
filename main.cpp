#include <iostream> 
#include <thread>    // Thread library to create parallel threads
#include <netinet/in.h> // Socket programming library, specifically for TCP/IP protocol
#include <unistd.h>  // Unix standard library for system functions like close()
#include <sstream>   // String stream library to process strings
#include <string>    
#include <atomic>    // Provides atomic variables that can be safely used in multi-threaded programs
#include <queue>     // Queue library for storing client requests in order
#include <condition_variable> // Used for thread synchronization and signaling
#include <mutex>     // Provides mutexes to protect shared resources between threads
#include <set>       
#include "MST_algo.hpp" 
#include "graph.hpp"    
#include "Pipeline.hpp" 
#include "Activeobject.hpp"

#define PORT 8080 // The port number the server listens on
#define BUFFER_SIZE 1024 // Size of the buffer used to read data from clients
#define THREAD_POOL_SIZE 4 // Number of threads in the server's thread pool

// Global variables for thread synchronization
std::mutex leaderMutex; // Mutex for the Leader-Follower pattern
std::condition_variable leaderCV; // Condition variable for managing leader threads
std::queue<int> clientQueue; // Queue to hold the client sockets waiting to be processed

// Set to store all active client sockets
std::set<int> activeClientSockets;
std::mutex clientSocketMutex; // Mutex to protect access to activeClientSockets

std::atomic<bool> serverRunning(true); // Atomic flag to indicate if the server is running
std::atomic<int> activeClients(0); // Counter for the number of currently active clients
int serverFd; // File descriptor for the server socket

// Function to close all active client connections
void closeAllClients()
{
    std::lock_guard<std::mutex> lock(clientSocketMutex); // Lock the mutex to ensure thread safety
    for (int clientSocket : activeClientSockets) // Iterate through all active client sockets
    {
        shutdown(clientSocket, SHUT_RDWR); // Disable both reading and writing on the client socket
        close(clientSocket); // Close the socket
        std::cout << "Closed client socket: " << clientSocket << std::endl; // Output a message about the closed client socket
    }
    activeClientSockets.clear(); // Clear the set of active clients
}

// Function to handle a single client connection
void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE] = {0}; // Buffer to store incoming client data
    std::unique_ptr<Graph> graph;   // Pointer to store the client's graph
    std::unique_ptr<MSTTree> mst;   // Pointer to store the client's computed MST

    // Add the client socket to the set of active clients
    {
        std::lock_guard<std::mutex> lock(clientSocketMutex);
        activeClientSockets.insert(clientSocket);
    }

    activeClients++; // Increment the counter of active clients

    while (serverRunning)
    {
        int bytesRead = read(clientSocket, buffer, BUFFER_SIZE); // Read data from the client
        if (bytesRead <= 0) // If no data is read, or there's an error, assume the client disconnected
        {
            std::cout << "Client disconnected.\n";
            break;
        }

        std::string request(buffer, bytesRead); // Convert the raw buffer data into a string
        std::stringstream ss(request); // Use stringstream to process the incoming request
        std::string command;
        ss >> command; // Extract the first word (command) from the request

        // Handle the "longest distance" command
        if (request.find("longest distance") != std::string::npos) {
            if (mst) { // Check if an MST is already computed
                int longestDistance = mst->getLongestDistance(); // Get the longest distance in the MST
                std::string response = "Longest distance in MST: " + std::to_string(longestDistance) + "\n";
                send(clientSocket, response.c_str(), response.size(), 0); // Send the response to the client
            } else {
                std::string response = "MST not computed yet. Use solve command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0); // Inform the client that the MST isn't computed yet
            }
            continue; // Move to the next iteration
        }

        // Handle the "avg distance" command
        if (request.find("avg distance") != std::string::npos) {
            if (mst) {
                double averageDistance = mst->getAverageDistance(); // Compute the average distance of edges in the MST
                std::string response = "Average distance in MST: " + std::to_string(averageDistance) + "\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            } else {
                std::string response = "MST not computed yet. Use solve command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
            continue;
        }

        // Handle the "shortest distance" command
        if (request.find("shortest distance") != std::string::npos) {
            int u, v;
            ss >> u >> v; // Read the two vertices for which the shortest distance is requested
            if (mst && u >= 0 && v >= 0 && u < graph->getNumberOfVertices() && v < graph->getNumberOfVertices()) {
                int shortestDistance = mst->getShortestDistance(u, v); // Calculate the shortest distance between two vertices in the MST
                std::string response;
                if (shortestDistance == -1) {
                    response = "No path exists between vertices " + std::to_string(u) + " and " + std::to_string(v) + ".\n";
                } else {
                    response = "Shortest distance between " + std::to_string(u) + " and " + std::to_string(v) + " in MST: " + std::to_string(shortestDistance) + "\n";
                }
                send(clientSocket, response.c_str(), response.size(), 0);
            } else {
                std::string response = "Invalid vertex indices or MST not computed yet. Use solve command first.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
            continue;
        }

        // Create a pipeline to handle multiple steps in sequence
        Pipeline pipeline;

        // Handle the "create" command to create a new graph
        if (command == "create")
        {
            pipeline.addStep([&](){
                int size;
                ss >> size; // Read the size of the graph (number of vertices)
                graph = std::make_unique<Graph>(size); // Create a new graph
                std::string response = "Graph created with " + std::to_string(size) + " vertices.\n";
                send(clientSocket, response.c_str(), response.size(), 0); // Send response back to the client
            });
        }
        // Handle the "add" command to add an edge to the graph
        else if (command == "add")
        {
            pipeline.addStep([&](){
                if (!graph)
                {
                    std::string response = "Graph is not created. Use create command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }
                int u, v, weight;
                ss >> u >> v >> weight; // Read the vertices and the weight of the edge
                graph->addEdge(u, v, weight); // Add the edge to the graph
                std::string response = "Edge added: (" + std::to_string(u) + ", " + std::to_string(v) + ") with weight " + std::to_string(weight) + "\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        }
        // Handle the "remove" command to remove an edge from the graph
        else if (command == "remove")
        {
            pipeline.addStep([&](){
                if (!graph)
                {
                    std::string response = "Graph is not created. Use create command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }
                int u, v;
                ss >> u >> v; // Read the vertices of the edge to be removed
                graph->removeEdge(u, v); // Remove the edge from the graph
                std::string response = "Edge removed: (" + std::to_string(u) + ", " + std::to_string(v) + ")\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            });
        }
        // Handle the "solve" command to compute the MST
        else if (command == "solve")
        {
            pipeline.addStep([&](){
                if (!graph)
                {
                    std::string response = "Graph is not created. Use create command first.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                    return;
                }

                std::string algorithm;
                ss >> algorithm; // Read which algorithm to use (Prim or Kruskal)
                MSTAlgo* algo = nullptr;

                if (algorithm == "prim")
                {
                    algo = MSTFactory::createMSTAlgorithm(MSTFactory::PRIM); // Use Prim's algorithm
                }
                else if (algorithm == "kruskal")
                {
                    algo = MSTFactory::createMSTAlgorithm(MSTFactory::KRUSKAL); // Use Kruskal's algorithm
                }

                if (algo)
                {
                    mst = std::make_unique<MSTTree>(algo->computeMST(*graph)); // Compute the MST

                    // Construct the response string to send to the client
                    std::vector<std::pair<int, int>> mstEdges = mst->getEdges();
                    std::string response = "Following are the edges in the constructed MST:\n";
                    for (const auto& edge : mstEdges) {
                        int u = edge.first;
                        int v = edge.second;
                        int weight = graph->getAdjacencyMatrix()[u][v]; // Get the weight of each edge
                        response += std::to_string(u) + " -- " + std::to_string(v) + " == " + std::to_string(weight) + "\n";
                    }

                    // Append the total weight to the response
                    int totalWeight = mst->getTotalWeight();
                    response += "Minimum Cost Spanning Tree: " + std::to_string(totalWeight) + "\n";

                    // Send the response to the client
                    send(clientSocket, response.c_str(), response.size(), 0);

                    // Clean up the algorithm object
                    delete algo;
                }
                else
                {
                    std::string response = "Unknown algorithm requested.\n";
                    send(clientSocket, response.c_str(), response.size(), 0);
                }
            });
        }
        // Handle the "shutdown" command to shut down a client
        else if (command == "shutdown")
        {
            pipeline.addStep([&](){
                std::string response = "Shutting down this client.\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                std::cout << "Client initiated shutdown command.\n";

                // Close only this client's connection
                close(clientSocket); // Close the client's socket
                {
                    std::lock_guard<std::mutex> lock(clientSocketMutex);
                    activeClientSockets.erase(clientSocket); // Remove this client from the active set
                }

                activeClients--; // Decrease the active client count
                return; // Exit the loop for this client
            });
        }
        else
        {
            pipeline.addStep([&](){
                std::string response = "Unknown command.\n";
                send(clientSocket, response.c_str(), response.size(), 0); // Inform the client that the command is unknown
            });
        }

        // Execute all steps in the pipeline for this command
        pipeline.execute();
    }

    // Ensure that the client is removed from the active set and its socket is closed if not done earlier
    {
        std::lock_guard<std::mutex> lock(clientSocketMutex);
        activeClientSockets.erase(clientSocket); // Remove the client from the active set
    }

    close(clientSocket); // Close the client's socket
    std::cout << "Client socket closed.\n"; // Log the closure
}

// Main server function using the Leader-Follower pattern
void runServer()
{
    struct sockaddr_in address; // Structure for socket address
    int opt = 1; // Option for setting socket options
    int addrlen = sizeof(address); // Length of the address
    int newSocket; // Socket for new client connections

    // Create an ActiveObject with a thread pool of THREAD_POOL_SIZE
    ActiveObject activeObject(THREAD_POOL_SIZE);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // Create the server socket
    {
        perror("Socket failed"); // Print error if socket creation fails
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // Set socket options
    {
        perror("setsockopt"); // Print error if setting options fails
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP address
    address.sin_port = htons(PORT); // Set the port for the server

    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) // Bind the socket to the port
    {
        perror("Bind failed"); // Print error if binding fails
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0) // Start listening for client connections
    {
        perror("Listen failed"); // Print error if listening fails
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running and listening on port " << PORT << std::endl; // Print server start message

    // Create a thread pool for handling client requests
    std::vector<std::thread> threadPool;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
    {
        threadPool.emplace_back([&](){
            while (serverRunning) // While the server is running
            {
                int clientSocket;

                // Leader-Follower mechanism: One thread acts as the leader and processes new connections
                {
                    std::unique_lock<std::mutex> lock(leaderMutex);
                    leaderCV.wait(lock, [&]() { return !clientQueue.empty() || !serverRunning; }); // Wait for new connections

                    if (!serverRunning) // If the server is shutting down, exit the loop
                    {
                        return;
                    }

                    if (!clientQueue.empty()) // If there are client connections waiting
                    {
                        clientSocket = clientQueue.front(); // Get the next client socket
                        clientQueue.pop(); // Remove it from the queue
                    }
                }

                // Process client requests asynchronously using the ActiveObject pattern
                activeObject.enqueueTask([clientSocket]() {
                    handleClient(clientSocket); // Handle the client in a separate task
                });
            }
        });
    }

    // Thread to listen for a shutdown command from the server console
    std::thread shutdownThread([&](){
        std::string input;
        while (serverRunning) // While the server is running
        {
            std::cin >> input; // Wait for user input in the console
            if (input == "shutdown") // If the input is "shutdown"
            {
                std::cout << "Server shutting down...\n"; // Print shutdown message
                serverRunning = false;  // Set the server running flag to false

                // Shut down the server socket to stop accepting new connections
                shutdown(serverFd, SHUT_RDWR); // Disable read/write operations on the server socket
                close(serverFd);  // Close the server socket

                closeAllClients(); // Close all active client connections

                leaderCV.notify_all();  // Wake up all waiting threads to stop
                break;
            }
        }
    });

    // Main loop to accept new client connections
    while (serverRunning)
    {
        newSocket = accept(serverFd, (struct sockaddr *)&address, (socklen_t *)&addrlen); // Accept a new client connection

        if (newSocket >= 0) // If a new client is connected
        {
            std::cout << "New client connection accepted.\n"; // Print message about new client
            {
                std::lock_guard<std::mutex> lock(leaderMutex);
                clientQueue.push(newSocket); // Add the client socket to the queue
            }
            leaderCV.notify_one(); // Notify a thread to handle this client
        }
        else if (!serverRunning) // If the server is shutting down
        {
            break; // Exit the loop
        }
    }

    // Wait for the shutdown thread to finish
    shutdownThread.join();

    // Join all threads to ensure they finish gracefully
    for (auto &th : threadPool)
    {
        if (th.joinable()) // If the thread can be joined
        {
            th.join(); // Wait for it to finish
        }
    }

    std::cout << "Server has shut down immediately.\n"; // Print shutdown message
}

// Main function to start the server
int main()
{
    runServer(); // Start the server using the Leader-Follower pattern
    return 0; // Return 0 to indicate successful execution
}
