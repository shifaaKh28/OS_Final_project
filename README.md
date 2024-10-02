# Minimum Spanning Tree (MST) Server

## Overview

This project implements a server that allows clients to create and manipulate a graph, compute the Minimum Spanning Tree (MST) using Prim's or Kruskal's algorithm, and retrieve various properties of the MST. The server processes commands from the client to add/remove edges, compute the MST, and query the following properties:

- Total weight of the MST
- Longest distance between two vertices in the MST
- Average distance between any two vertices
- Shortest distance between two vertices in the MST

Additionally, the server supports a `shutdown` command for closing individual client connections or shutting down the server gracefully.

## Features

- **Graph Manipulation**: Create a graph, add edges, remove edges.
- **MST Algorithms**: Compute MST using Prim's or Kruskal's algorithm.
- **MST Properties**: Query properties like the total weight, longest distance, average distance, and shortest distance in the MST.
- **Pipeline Processing**: Commands are processed asynchronously in a pipeline.
- **Leader-Follower Pattern**: The server is designed using the Leader-Follower pattern for concurrency.
- **Client Shutdown**: Clients can shut down their own connections.
- **Server Shutdown**: The server can be shut down gracefully by typing `shutdown` in the server's console.

## Installation

1. **Clone the Repository**:
   ```bash
   git clone <repository-url>
   cd <project-directory>
   ```

2. **Build the Project**:
   Use `make` to compile the project:
   ```bash
   make
   ```

3. **Run the Server**:
   Start the server with the following command:
   ```bash
   ./server
   ```

4. **Connect with Clients**:
   Clients can connect to the server using `telnet`:
   ```bash
   telnet localhost 8080
   ```

## Commands

All commands are case-sensitive and should be written in lowercase.

### 1. `create <size>`
Creates a graph with the specified number of vertices.
- **Example**:
  ```bash
  create 5
  ```
  Response: 
  ```
  Graph created with 5 vertices.
  ```

### 2. `add <u> <v> <weight>`
Adds an edge between vertex `u` and vertex `v` with the specified weight.
- **Example**:
  ```bash
  add 0 1 10
  ```
  Response: 
  ```
  Edge added: (0, 1) with weight 10
  ```

### 3. `remove <u> <v>`
Removes the edge between vertex `u` and vertex `v`.
- **Example**:
  ```bash
  remove 0 1
  ```
  Response: 
  ```
  Edge removed: (0, 1)
  ```

### 4. `solve <algorithm>`
Computes the MST using the specified algorithm (`prim` or `kruskal`).
- **Example**:
  ```bash
  solve prim
  ```
  Response:
  ```
  Following are the edges in the constructed MST:
  0 -- 1 == 10
  0 -- 2 == 5
  Minimum Cost Spanning Tree: 15
  ```

### 5. `longest distance`
Returns the longest distance between any two vertices in the MST.
- **Example**:
  ```bash
  longest distance
  ```
  Response:
  ```
  Longest distance in MST: 25
  ```

### 6. `avg distance`
Returns the average distance between any two vertices in the MST.
- **Example**:
  ```bash
  avg distance
  ```
  Response:
  ```
  Average distance in MST: 12.5
  ```

### 7. `shortest distance <u> <v>`
Returns the shortest distance between vertex `u` and vertex `v` in the MST.
- **Example**:
  ```bash
  shortest distance 0 2
  ```
  Response:
  ```
  Shortest distance between 0 and 2 in MST: 10
  ```

### 8. `shutdown`
Shuts down the current client connection. For shutting down the entire server, type `shutdown` in the server terminal.
- **Example**:
  ```bash
  shutdown
  ```
  Response:
  ```
  Shutting down this client.
  ```

## Server Shutdown

To gracefully shut down the server, type `shutdown` in the server's terminal. This will terminate all client connections and stop the server.
- **Example**:
  ```bash
  shutdown
  ```
  Response in the server terminal:
  ```
  Server shutting down...
  ```

## Architecture

- **Leader-Follower Pattern**: Multiple threads act as leaders and process client connections.
- **ActiveObject Pattern**: Tasks are enqueued and processed asynchronously to avoid blocking.
- **Pipeline**: Client commands are handled in a pipeline, allowing easy extension of features.

## Future Enhancements

- **Command History**: Support for clients to view previously executed commands.
- **Error Logging**: A more robust logging system for error tracking and debugging.

## License

This project is open-source and available under the [MIT License](LICENSE).

---

Let me know if you need further adjustments or clarifications!
