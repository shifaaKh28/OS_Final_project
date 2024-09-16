#ifndef MST_TREE_HPP
#define MST_TREE_HPP

#include "graph.hpp"
#include <vector>
#include <algorithm>
#include <limits>

class MSTTree
{
private:
    Graph mstGraph;                         // The graph that represents the MST
    int totalWeight;                        // The total weight of the MST
    std::vector<std::pair<int, int>> edges; // Edges in the MST

    // Helper function to calculate all pairs shortest path (Floyd-Warshall)
    std::vector<std::vector<int>> floydWarshall() const;

public:
    // Constructor to build the MST tree from a given graph and edges
    MSTTree(const Graph &graph, const std::vector<std::pair<int, int>> &mstEdges);

    // Function to calculate the total weight of the MST
    int getTotalWeight() const;

    // Function to find the longest distance between two vertices in the MST
    int getLongestDistance() const;

    // Function to calculate the average distance between any two vertices in the graph
    double getAverageDistance() const;

    // Function to find the shortest distance between two vertices in the MST
    int getShortestDistance(int u, int v) const;

    // Function to print the MST tree for debugging
    void printMST() const;
};

#endif
