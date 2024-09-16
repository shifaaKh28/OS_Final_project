#ifndef SERVER_HPP
#define SERVER_HPP

#include "graph.hpp"
#include "MST_algo.hpp"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>

// Server class that handles incoming requests for MST computations
class MSTServer {
public:
    explicit MSTServer(int port);
    ~MSTServer();

    // Start the server
    void start();

private:
    int port;  // Server port
    int serverSocket;
    std::atomic<bool> stop;

    // Queue for clients to be processed by threads (Leader-Follower Pattern)
    std::queue<int> clientQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondVar;

    // Task queue for Pipeline pattern
    std::queue<std::function<void()>> taskQueue;
    std::mutex taskMutex;
    std::condition_variable taskCondVar;

    // Thread pool for Leader-Follower pattern
    std::vector<std::thread> threadPool;

    // Thread for processing tasks in the pipeline
    std::thread taskThread;

    // Start the background task processor
    void startTaskProcessing();

    // Process a client request
    void handleClient(int clientSocket);

    // Submit a task to the pipeline
    void submitTask(std::function<void()> task);

    // Task processor function (Pipeline pattern)
    void taskProcessor();

    // Leader-Follower thread function
    void leaderFollower();

    // Graph operation methods
    void processCommand(const std::string& command, int clientSocket);
    void addEdgeToGraph(Graph& graph, int u, int v, int weight);
    void removeEdgeFromGraph(Graph& graph, int u, int v);
    void updateEdgeWeight(Graph& graph, int u, int v, int weight);

    // Solve the MST problem and send the result back to the client
    void solveMST(Graph& graph, MSTFactory::AlgorithmType algoType, int clientSocket);
};

#endif


/**
 * void sendCommand(const std::string& command) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return;
    }

    send(sock, command.c_str(), command.length(), 0);
    read(sock, buffer, 1024);
    std::cout << "Server response: " << buffer << std::endl;
    close(sock);
}
 */