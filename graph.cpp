#include "graph.hpp"

// Constructor: Initializes the graph with the given number of vertices
Graph::Graph(int vertices) : numVertices(vertices), numEdges(0)
{
    // Initialize the adjacency matrix with 0 (no edge) for all vertex pairs
    adjMat.resize(numVertices, std::vector<int>(numVertices, 0));
}

// Function to add an edge from vertex u to vertex v with weight w
void Graph::addEdge(int u, int v, int weight)
{
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices)
    {
        std::cerr << "Error: Invalid vertex number.\n";
        return;
    }

    if (adjMat[u][v] == 0)
    {
        numEdges++; // Increment edge count if a new edge is added
    }

    adjMat[u][v] = weight; // Add edge from u to v
    adjMat[v][u] = weight; // Add edge from v to u (undirected)
}

// Function to remove an edge from vertex u to vertex v
void Graph::removeEdge(int u, int v)
{
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices)
    {
        std::cerr << "Error: Invalid vertex number.\n";
        return;
    }

    if (adjMat[u][v] != 0)
    {
        adjMat[u][v] = 0; // Set the edge weight to 0 (indicating no edge)
        numEdges--;       // Decrement the edge count
    }
}

// Function to get the number of vertices
int Graph::getNumberOfVertices() const
{
    return numVertices;
}

// Function to get the number of edges
int Graph::getNumberOfEdges() const
{
    return numEdges;
}

// Function to return the adjacency matrix
std::vector<std::vector<int>> Graph::getAdjacencyMatrix() const
{
    return adjMat;
}

// Function to print the adjacency matrix (optional for debugging)
void Graph::printAdjacencyMatrix() const
{
    for (int i = 0; i < numVertices; ++i)
    {
        for (int j = 0; j < numVertices; ++j)
        {
            std::cout << adjMat[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
