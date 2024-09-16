#include "MST_algo.hpp"
#include <queue>
#include <vector>
#include <algorithm>
#include <iostream>
#include <limits>
#include <tuple>


MSTTree Prim::computeMST(const Graph& graph) {
    int n = graph.getNumberOfVertices();
    std::vector<bool> inMST(n, false);  // Tracks which vertices are included in the MST
    std::vector<int> key(n, std::numeric_limits<int>::max()); // Used to pick the minimum weight edge
    std::vector<int> parent(n, -1); // Array to store the constructed MST
    std::vector<std::pair<int, int>> mstEdges;

    // Start with the first vertex (0)
    key[0] = 0;

    // Priority queue to pick the smallest edge based on the key
    auto compare = [](std::pair<int, int> p1, std::pair<int, int> p2) {
        return p1.second > p2.second;
    };

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, decltype(compare)> pq(compare);
    pq.push({0, 0});  // Insert the first vertex in the priority queue

    while (!pq.empty()) {
        int u = pq.top().first;  // Extract the vertex with the smallest key value
        pq.pop();

        // Check if this vertex is already included in the MST
        if (inMST[u]) continue;
        
        // Mark this vertex as included in the MST
        inMST[u] = true;

        // Update keys and parent for all adjacent vertices of the extracted vertex
        for (int v = 0; v < n; ++v) {
            int weight = graph.getAdjacencyMatrix()[u][v];

            // Only consider edges that are not yet in the MST and have smaller weights
            if (weight != 0 && !inMST[v] && key[v] > weight) {
                key[v] = weight;  // Update the key
                pq.push({v, key[v]});  // Push the vertex with the updated key into the priority queue
                parent[v] = u;  // Track the parent of vertex v
            }
        }
    }

    // Collect the MST edges based on the parent array
    for (int v = 1; v < n; ++v) {
        if (parent[v] != -1) {
            mstEdges.push_back({parent[v], v});
        }
    }

    // Return the constructed MST tree
    return MSTTree(graph, mstEdges);
}


// Kruskal's Algorithm
MSTTree Kruskal::computeMST(const Graph& graph) {
    int n = graph.getNumberOfVertices();
    std::vector<std::tuple<int, int, int>> edges; // {weight, u, v}
    std::vector<std::pair<int, int>> mstEdges;
    std::vector<int> parent(n);
    std::vector<int> rank(n, 0);

    // Initialize the Union-Find structure
    for (int i = 0; i < n; ++i) {
        parent[i] = i;
    }

    // Collect all edges
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            int weight = graph.getAdjacencyMatrix()[u][v];
            if (weight != 0) {
                edges.push_back({weight, u, v});
            }
        }
    }

    // Sort edges by weight
    std::sort(edges.begin(), edges.end());

    // Process each edge in increasing order of weight
    for (const auto& edge : edges) {
        int weight, u, v;
        std::tie(weight, u, v) = edge;

        if (findParent(u, parent) != findParent(v, parent)) {
            mstEdges.push_back({u, v});
            unionFind(u, v, parent, rank);
        }
    }

    return MSTTree(graph, mstEdges);
}

// Helper function for Union-Find (find with path compression)
int Kruskal::findParent(int vertex, std::vector<int>& parent) {
    if (parent[vertex] != vertex) {
        parent[vertex] = findParent(parent[vertex], parent);
    }
    return parent[vertex];
}

// Helper function for Union-Find (union by rank)
void Kruskal::unionFind(int u, int v, std::vector<int>& parent, std::vector<int>& rank) {
    int rootU = findParent(u, parent);
    int rootV = findParent(v, parent);

    if (rank[rootU] > rank[rootV]) {
        parent[rootV] = rootU;
    } else if (rank[rootU] < rank[rootV]) {
        parent[rootU] = rootV;
    } else {
        parent[rootV] = rootU;
        rank[rootU]++;
    }
}

// MSTFactory: Factory method to create MST algorithm
MSTAlgo* MSTFactory::createMSTAlgorithm(MSTFactory::AlgorithmType type) {
    if (type == PRIM) {
        return new Prim();
    } else if (type == KRUSKAL) {
        return new Kruskal();
    }
    return nullptr;
}
