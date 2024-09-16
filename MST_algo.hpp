#ifndef MST_ALGO_HPP
#define MST_ALGO_HPP

#include "graph.hpp"
#include "MST_tree.hpp"
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>

// Abstract base class for MST Algorithm
class MSTAlgo{
public:
    virtual MSTTree computeMST(const Graph& graph) = 0; // Pure virtual function to compute the MST
    virtual ~MSTAlgo() {}
};

// Prim's Algorithm implementation
class Prim : public MSTAlgo{
public:
    MSTTree computeMST(const Graph& graph) override;
};

// Kruskal's Algorithm implementation
class Kruskal : public MSTAlgo{
private:
    // Helper function for Kruskal's algorithm (Union-Find)
    int findParent(int vertex, std::vector<int>& parent);
    void unionFind(int u, int v, std::vector<int>& parent, std::vector<int>& rank);    

public:
    MSTTree computeMST(const Graph& graph) override;


};

// Factory class to select the algorithm dynamically
class MSTFactory {
public:
    enum AlgorithmType {
        PRIM,
        KRUSKAL
    };

    // Static method to create an MST algorithm based on the request
    static MSTAlgo* createMSTAlgorithm(AlgorithmType type);
};

#endif
