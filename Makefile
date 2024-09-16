# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall

# Server source files (include server_main.cpp for the entry point)
SERVER_SRCS = server.cpp graph.cpp MST_algo.cpp MST_tree.cpp server_main.cpp
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)

# Client source files (main.cpp for the client)
CLIENT_SRCS = main.cpp
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Executable names
SERVER_EXEC = mst_server
CLIENT_EXEC = mst_client

# Default target to build both server and client
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Rule to build the server executable (with server_main.cpp)
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXEC) $(SERVER_OBJS)

# Rule to build the client executable (with main.cpp)
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXEC) $(CLIENT_OBJS)

# Rule to build object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up the build
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC)
