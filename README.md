Here’s the updated `README.md` file, incorporating the use of **ActiveObject** and **Pipeline** patterns, along with explanations of how these design patterns are utilized in the project, and details on how to run various profiling and coverage commands.

---

# OS Final Project - Minimum Spanning Tree (MST) Server

This project implements a multithreaded server using the **Leader-Follower**, **ActiveObject**, and **Pipeline** patterns. The server can handle multiple client connections, allowing them to create graphs, add/remove edges, compute MST (Minimum Spanning Tree) using Prim's or Kruskal's algorithm, and query various properties of the MST.

## Design Patterns

### 1. **Leader-Follower Pattern**
The **Leader-Follower** pattern is used to manage multiple client connections. One thread acts as the leader and processes incoming client requests. When the leader finishes handling a request, it becomes a follower, and another thread takes over as the leader. This ensures efficient client request handling.

### 2. **ActiveObject Pattern**
The **ActiveObject** pattern is used to handle client requests asynchronously. Instead of processing requests in the main server loop, each request is enqueued as a task, which is processed by a pool of worker threads. This helps in improving the scalability and responsiveness of the server.

### 3. **Pipeline Pattern**
The **Pipeline** pattern is used to process each command from the client as a series of steps. This allows for flexible execution of different stages of the command processing, making it easy to extend the server functionality without changing the core logic.

## Features
- Create graphs and manage edges through client commands.
- Compute the MST (Minimum Spanning Tree) using Prim's or Kruskal's algorithms.
- Query the MST for:
  - Longest distance
  - Shortest distance between vertices
  - Average distance between edges
- Multithreaded server capable of handling multiple clients concurrently.
- Memory checks, code coverage, and performance profiling using Valgrind and related tools.

## Prerequisites

Ensure you have the following installed:
- `g++` with C++14 or higher
- `make`
- `telnet` (for testing client connections)
- `Valgrind` (optional, for memory profiling and code coverage)

## Build the Project

To compile the project, use the following command:

```bash
make
```

This will generate the `server` executable.

## Running the Server

After building the project, start the server with:

```bash
./server
```

The server will listen for client connections on port `8080`.

## Connecting to the Server

To connect to the server and issue commands, use `telnet`:

```bash
telnet localhost 8080
```

You can then enter various commands to interact with the server.

## Commands

Here’s a list of available commands:

- **CREATE**: Create a graph with a specified number of vertices.
    - Example: `create 3`
    
- **ADD**: Add an edge between two vertices with a specified weight.
    - Example: `add 0 1 5`
    
- **REMOVE**: Remove an edge between two vertices.
    - Example: `remove 0 1`
    
- **SOLVE**: Solve the MST using either Prim's or Kruskal's algorithm.
    - Example: `solve prim`
    - Example: `solve kruskal`
    
- **LONGEST DISTANCE**: Query the longest distance in the MST.
    - Example: `longest distance`
    
- **AVG DISTANCE**: Query the average distance in the MST.
    - Example: `avg distance`
    
- **SHORTEST DISTANCE**: Query the shortest distance between two vertices in the MST.
    - Example: `shortest distance 0 1`
    
- **SHUTDOWN**: Disconnect the client from the server.
    - Example: `shutdown`

## Examples

1. **Create a Graph with 4 Vertices**
    ```
    create 4
    ```
    Output:
    ```
    Graph created with 4 vertices.
    ```

2. **Add Edges Between Vertices**
    ```
    add 0 1 5
    add 1 2 8
    ```
    Output:
    ```
    Edge added: (0, 1) with weight 5
    Edge added: (1, 2) with weight 8
    ```

3. **Solve the MST using Prim's Algorithm**
    ```
    solve prim
    ```
    Output:
    ```
    Following are the edges in the constructed MST:
    0 -- 1 == 5
    1 -- 2 == 8
    Minimum Cost Spanning Tree: 13
    ```

4. **Query the Longest Distance**
    ```
    longest distance
    ```
    Output:
    ```
    Longest distance in MST: 8
    ```

5. **Query the Shortest Distance between Vertices 0 and 1**
    ```
    shortest distance 0 1
    ```
    Output:
    ```
    Shortest distance between 0 and 1 in MST: 5
    ```

## Profiling and Coverage Commands

### 1. Code Coverage
To check code coverage, run the following command:

```bash
make coverage
```

This will generate a code coverage report for the project.

### 2. Memory Checking with Valgrind
To check for memory leaks and errors, use Valgrind:

```bash
make valgrind
```

This will execute Valgrind with the server to check for memory issues.

### 3. Performance Profiling
To profile the performance of the server:

```bash
make profile
```

This will generate a performance profiling report using Valgrind's Callgrind tool.

## Clean the Project

To clean the project and remove compiled files:

make clean
```


