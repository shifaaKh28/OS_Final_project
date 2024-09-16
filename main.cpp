#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "server.hpp"
#include "MST_algo.hpp"
#include <string.h>
#include "Pipeline.hpp"
#include "Activeobject.hpp"

using namespace std;

Graph graph;
ActiveObject activeObject;

#define PORT 8080  // The port number to connect to the server

// Function to handle the "Newgraph" command
void handleNewGraph(int client_fd) {
    string response = "Enter edges in format: <from> <to> <weight>, -1 to finish.\n";
    send(client_fd, response.c_str(), response.size(), 0);
    cout << "Client " << client_fd << " used init command." << endl;
    
    while (true) {
        char buf[256];
        int nbytes = recv(client_fd, buf, sizeof buf, 0);
        if (nbytes <= 0) {
            cerr << "Error receiving data from client" << endl;
            break;
        }
        buf[nbytes] = '\0';
        
        int from, to, weight;
        sscanf(buf, "%d %d %d", &from, &to, &weight);
        if (from == -1) {
            response = "Successfully inputed all edges.\n";
            send(client_fd, response.c_str(), response.size(), 0);
            break;
        }
        graph.addEdge(from, to, weight);
    }
    cout << "Graph initialized." << endl;
}

// Function to handle the "Newedge" command
void handleNewEdge(Graph*& graph, int u, int v, int weight,int client_fd) {
    graph->addEdge(u, v, weight);
    send(client_fd, "Edge added.\n", 12, 0);
}

// Function to handle the "Removeedge" command
void handleRemoveEdge(Graph*& graph, int u, int v, int client_fd) {
    graph->removeEdge(u, v);
    send(client_fd, "Edge removed.\n", 14, 0);
}

// Function to handle the "Kosaraju" command
void handlePrim(int client_fd) {
    cout << "Client #" << client_fd << " requested Prim algorithm.\n";
    string response = "Searching MST with Prim algorithm...\n";
    send(client_fd, response.c_str(), response.size(), 0);
 // Create an instance of Prim
    Prim primInstance;
    
    // Create pipeline and add steps
    Pipeline pipeline;
    pipeline.addStep([client_fd, &primInstance] { // Capture the Prim instance by reference
        auto mstTree = primInstance.computeMST(graph);
    });
    pipeline.addStep([client_fd] {
        string response = "MST found.\n";
        send(client_fd, response.c_str(), response.size(), 0);
    });

    // Enqueue the pipeline task
    activeObject.enqueueTask([pipeline] {
        pipeline.execute();
    });
}


int main() {
    std::cout << "Welcome to the MST Client!\n"
              << "Available commands:\n"
              << "ADD u v weight    -> Adds an edge between vertices u and v with the given weight.\n"
              << "REMOVE u v        -> Removes the edge between vertices u and v.\n"
              << "UPDATE u v weight -> Updates the weight of the edge between vertices u and v.\n"
              << "SOLVE PRIM        -> Solves the MST using Prim's algorithm.\n"
              << "SOLVE KRUSKAL     -> Solves the MST using Kruskal's algorithm.\n"
              << "Type 'exit' to quit.\n";

    std::string command;
    while (true) {
        std::cout << "\nEnter command: ";
        std::getline(std::cin, command);

        if (command == "exit") {
            std::cout << "Exiting client..." << std::endl;
            break;
        }
    }

    return 0;
}
