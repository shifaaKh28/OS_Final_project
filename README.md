# MST Solver Server

## Overview

This project is a multi-threaded server application that computes the **Minimum Spanning Tree (MST)** of a graph using **Prim’s** and **Kruskal’s** algorithms. Clients can connect to the server, create graphs, add or remove edges, compute the MST, and retrieve various data about the MST, such as:

- Total weight of the MST
- Longest distance between any two vertices
- Average distance between vertices
- Shortest distance between two distinct vertices \(X_i\) and \(X_j\) where the edge belongs to the MST

### Features
- **Multi-threaded server:** Implements the **Leader-Follower** pattern for handling client connections.
- **Pipeline processing:** Each client request is processed through a pipeline, allowing commands to be processed in steps.
- **MST Algorithms:** Supports **Prim’s** and **Kruskal’s** algorithms for computing the MST.
- **Query the MST:** Clients can query the MST for its total weight, longest and shortest distances, and average distance between vertices.

## Project Requirements

This server answers the following MST-related queries:
1. **Total weight of the MST**
2. **Longest distance between two vertices**
3. **Average distance between vertices** 
   - Assumes distance(x,x) = 0 for any vertex `x`
   - Considers all distances \(X_i, X_j\) where \(i = 1..n, j ≥ i\)
4. **Shortest distance between two vertices \(X_i, X_j\)** where \(i ≠ j\) and the edge belongs to the MST

The server processes requests in both **Leader-Follower** and **Pipeline** patterns.

## Setup

### Prerequisites
To build and run this project, you'll need the following installed:
- A **C++ compiler** that supports C++11 (e.g., `g++`)
- **Make** (for building the project)
- **Telnet** (or any other client to communicate with the server)

### Building the Project

1. Clone the repository to your local machine:

    ```bash
    git clone <repository-url>
    cd <project-directory>
    ```

2. Build the project using `make`:

    ```bash
    make
    ```

### Running the Server

Once the project is built, run the server:

```bash
./server
```

The server will listen on port 8080 by default.

### Running the Client

You can use `telnet` to connect to the server and send commands. In a separate terminal, run:

```bash
telnet localhost 8080
```

You can now send commands to interact with the server.

### Available Commands

#### 1. `CREATE <number_of_vertices>`
Create a graph with the specified number of vertices.

Example:
```
CREATE 5
```

#### 2. `ADD <u> <v> <weight>`
Add an edge between vertex `u` and vertex `v` with a specific weight.

Example:
```
ADD 0 1 10
```

#### 3. `REMOVE <u> <v>`
Remove the edge between vertex `u` and vertex `v`.

Example:
```
REMOVE
