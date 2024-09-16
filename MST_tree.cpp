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

// Function to find the shortest distance between two vertices in the MST
int MSTTree::getShortestDistance(int u, int v) const {
    auto dist = floydWarshall();
    return dist[u][v];
}

// Function to print the MST tree (for debugging)
void MSTTree::printMST() const {
    mstGraph.printAdjacencyMatrix();
}

// Helper function to calculate all-pairs shortest path using Floyd-Warshall algorithm
std::vector<std::vector<int>> MSTTree::floydWarshall() const {
    int n = mstGraph.getNumberOfVertices();
    auto adjMat = mstGraph.getAdjacencyMatrix();
    std::vector<std::vector<int>> dist = adjMat;

    // Initialize distances: if there's no edge, set to infinity
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && dist[i][j] == 0) {
                dist[i][j] = std::numeric_limits<int>::max();
            }
        }
    }

    // Floyd-Warshall algorithm
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
