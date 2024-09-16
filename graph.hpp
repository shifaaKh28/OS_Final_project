#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <iostream>

class Graph
{
private:
    std::vector<std::vector<int>> adjMat; // Adjacency matrix to represent the graph
    int numVertices;                      // Number of vertices
    int numEdges;                         // Number of edges    
public:
    // Constructor to initialize the graph with a specific number of vertices
    Graph(int vertices);

    // Function to add an edge from vertex u to vertex v with weight w
    void addEdge(int u, int v, int weight);

    // Function to remove an edge from vertex u to vertex v
    void removeEdge(int u, int v);

    // Function to get the number of vertices
    int getNumberOfVertices() const;

    // Function to get the number of edges
    int getNumberOfEdges() const;

    // Function to return the adjacency matrix
    std::vector<std::vector<int>> getAdjacencyMatrix() const;

    // Function to print the adjacency matrix (optional for debugging)
    void printAdjacencyMatrix() const;
};

#endif
