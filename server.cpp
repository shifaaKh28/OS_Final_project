#include "server.hpp"
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define MAX_BUFFER 1024

MSTServer::MSTServer(int port) : port(port), serverSocket(-1), stop(false) {
    // Start background task processing
    taskThread = std::thread(&MSTServer::taskProcessor, this);
}

// Destructor to stop the server and threads cleanly
MSTServer::~MSTServer() {
    stop = true;
    taskCondVar.notify_all();
    taskThread.join();
    for (auto &t : threadPool) {
        if (t.joinable()) {
            t.join();
        }
    }
}

// Start the server and initialize thread pool for Leader-Follower pattern
void MSTServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: Unable to create socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Unable to bind socket" << std::endl;
        return;
    }

    listen(serverSocket, 5);
    std::cout << "Server listening on port " << port << std::endl;

    // Initialize the thread pool for Leader-Follower pattern
    for (int i = 0; i < 4; ++i) {
        threadPool.emplace_back(&MSTServer::leaderFollower, this);
    }

    while (!stop) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error: Unable to accept client" << std::endl;
            continue;
        }

        std::unique_lock<std::mutex> lock(queueMutex);
        clientQueue.push(clientSocket);
        queueCondVar.notify_one();  // Notify a thread to handle the request
    }
}

// Leader-Follower pattern: handle client requests using multiple threads
void MSTServer::leaderFollower() {
    while (!stop) {
        int clientSocket;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondVar.wait(lock, [this] { return !clientQueue.empty() || stop; });
            if (stop) return; // Exit the loop if the server is stopping
            clientSocket = clientQueue.front();
            clientQueue.pop();
        }

        handleClient(clientSocket);
    }
}

// Handle a client request and submit tasks to the pipeline
void MSTServer::handleClient(int clientSocket) {
    char buffer[MAX_BUFFER];
    std::memset(buffer, 0, MAX_BUFFER);

    int bytesRead = read(clientSocket, buffer, MAX_BUFFER);
    if (bytesRead < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        close(clientSocket);
        return;
    }

    std::string command(buffer);
    processCommand(command, clientSocket);

    close(clientSocket);
}

// Process the client command and route it through the pipeline
void MSTServer::processCommand(const std::string& command, int clientSocket) {
    Graph graph(5);  // Example: graph with 5 vertices

    submitTask([=, &graph]() {
        if (command.find("ADD") == 0) {
            int u, v, weight;
            sscanf(command.c_str(), "ADD %d %d %d", &u, &v, &weight);
            addEdgeToGraph(graph, u, v, weight);
        } else if (command.find("REMOVE") == 0) {
            int u, v;
            sscanf(command.c_str(), "REMOVE %d %d", &u, &v);
            removeEdgeFromGraph(graph, u, v);
        } else if (command.find("UPDATE") == 0) {
            int u, v, weight;
            sscanf(command.c_str(), "UPDATE %d %d %d", &u, &v, &weight);
            updateEdgeWeight(graph, u, v, weight);
        }

        submitTask([=, &graph]() {
            solveMST(graph, MSTFactory::PRIM, clientSocket);  // Solve using Prim for now
        });
    });
}

// Add an edge to the graph
void MSTServer::addEdgeToGraph(Graph& graph, int u, int v, int weight) {
    graph.addEdge(u, v, weight);
}

// Remove an edge from the graph
void MSTServer::removeEdgeFromGraph(Graph& graph, int u, int v) {
    graph.removeEdge(u, v);
}

// Update the weight of an edge in the graph
void MSTServer::updateEdgeWeight(Graph& graph, int u, int v, int weight) {
    graph.removeEdge(u, v);
    graph.addEdge(u, v, weight);
}

// Solve the MST using the specified algorithm and send back the result
void MSTServer::solveMST(Graph& graph, MSTFactory::AlgorithmType algoType, int clientSocket) {
    MSTAlgo* mstAlgo = MSTFactory::createMSTAlgorithm(algoType);
    MSTTree mst = mstAlgo->computeMST(graph);

    int totalWeight = mst.getTotalWeight();

    std::string response = "Total weight of MST: " + std::to_string(totalWeight) + "\n";
    write(clientSocket, response.c_str(), response.size());

    delete mstAlgo;
}

// Start the background task processing
void MSTServer::startTaskProcessing() {
    taskThread = std::thread(&MSTServer::taskProcessor, this);
}

// Submit a task to be processed asynchronously
void MSTServer::submitTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(taskMutex);
        taskQueue.push(task);
    }
    taskCondVar.notify_one();
}

// Task processor function (runs in the background)
void MSTServer::taskProcessor() {
    while (!stop) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(taskMutex);
            taskCondVar.wait(lock, [this] { return !taskQueue.empty() || stop; });

            if (stop && taskQueue.empty()) {
                return;
            }

            task = taskQueue.front();
            taskQueue.pop();
        }

        // Execute the task
        task();
    }
}
