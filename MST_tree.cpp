#include "MST_tree.hpp"
#include <iostream>
#include <cmath>
#include <limits>

// Constructor to initialize the MST tree from a graph and the MST edges
MSTTree::MSTTree(const Graph& graph, const std::vector<std::pair<int, int>>& mstEdges)
    : mstGraph(graph.getNumberOfVertices()), totalWeight(0), edges(mstEdges) {
    
    // Copy only the edges in the MST into the MST graph
    for (const auto& edge : mstEdges) {
        int u = edge.first;
        int v = edge.second;
        int weight = graph.getAdjacencyMatrix()[u][v];
        mstGraph.addEdge(u, v, weight);
        totalWeight += weight;
    }
}

// Function to calculate the total weight of the MST
int MSTTree::getTotalWeight() const {
    return totalWeight;
}

// Function to calculate the longest distance between two vertices in the MST
int MSTTree::getLongestDistance() const {
    auto dist = floydWarshall();
    int longest = 0;
    for (int i = 0; i < mstGraph.getNumberOfVertices(); ++i) {
        for (int j = 0; j < mstGraph.getNumberOfVertices(); ++j) {
            if (dist[i][j] != std::numeric_limits<int>::max()) {
                longest = std::max(longest, dist[i][j]);
            }
        }
    }
    return longest;
}

// Function to calculate the average distance between any two vertices in the MST
double MSTTree::getAverageDistance() const {
    auto dist = floydWarshall();
    double totalDistance = 0;
    int count = 0;

    for (int i = 0; i < mstGraph.getNumberOfVertices(); ++i) {
        for (int j = i; j < mstGraph.getNumberOfVertices(); ++j) {
            if (dist[i][j] != std::numeric_limits<int>::max()) {
                totalDistance += dist[i][j];
                count++;
            }
        }
    }

    return (count == 0) ? 0 : totalDistance / count;
}

// Function to find the shortest distance between two vertices in the MST (i ≠ j)
int MSTTree::getShortestDistance(int u, int v) const {
    // Check that u and v are valid indices
    int n = mstGraph.getNumberOfVertices();
    if (u < 0 || v < 0 || u >= n || v >= n) {
        std::cerr << "Error: Invalid vertex index. u: " << u << ", v: " << v << "\n";
        return -1; // Return error if invalid vertices
    }

    if (u == v) {
        std::cout << "Error: i == j, please provide distinct vertices.\n";
        return -1; // Return -1 to indicate an error
    }

    // Run Floyd-Warshall algorithm on the MST adjacency matrix
    auto dist = floydWarshall();

    // Check if there is no path between u and v in the MST (i.e., distance is infinity)
    if (dist[u][v] == std::numeric_limits<int>::max()) {
        std::cout << "No path exists between vertices " << u << " and " << v << " in the MST." << std::endl;
        return -1; // Return -1 or another value to indicate no path exists
    }

    return dist[u][v]; // Return the shortest distance between u and v
}


// Function to print the MST tree (for debugging)
void MSTTree::printMST() const {
    mstGraph.printAdjacencyMatrix();
}

// Helper function to calculate all-pairs shortest path using Floyd-Warshall algorithm on the MST
std::vector<std::vector<int>> MSTTree::floydWarshall() const {
    int n = mstGraph.getNumberOfVertices();
    
    // Ensure we have a valid graph
    if (n == 0) {
        std::cerr << "Error: Graph has no vertices.\n";
        return std::vector<std::vector<int>>(); // Return empty result
    }

    auto adjMat = mstGraph.getAdjacencyMatrix();
    std::vector<std::vector<int>> dist(n, std::vector<int>(n, std::numeric_limits<int>::max()));

    // Initialize distances: If there's an edge in the MST, set the distance to the weight of that edge
    for (const auto& edge : edges) {
        int u = edge.first;
        int v = edge.second;

        // Check that u and v are valid indices
        if (u < 0 || v < 0 || u >= n || v >= n) {
            std::cerr << "Error: Invalid edge between vertices " << u << " and " << v << ".\n";
            continue;
        }

        int weight = adjMat[u][v];
        dist[u][v] = weight;
        dist[v][u] = weight; // Undirected, so make it symmetric
    }

    // The distance from any vertex to itself is 0
    for (int i = 0; i < n; ++i) {
        dist[i][i] = 0;
    }

    // Floyd-Warshall algorithm to compute shortest paths in the MST
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (dist[i][k] != std::numeric_limits<int>::max() && dist[k][j] != std::numeric_limits<int>::max()) {
                    dist[i][j] = std::min(dist[i][j], dist[i][k] + dist[k][j]);
                }
            }
        }
    }

    return dist;
}


// Function to return the edges in the MST
std::vector<std::pair<int, int>> MSTTree::getEdges() const {
    return edges;
}
