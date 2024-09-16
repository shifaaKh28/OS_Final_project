CXX = g++
CXXFLAGS = -g
LDFLAGS = -pthread  # Add this line

# Define the executable file 
EXEC = OS_Final_Project

# List all the object files
OBJS = graph.o MST_algo.o MST_tree.o server.o main.o

# Default target
all: $(EXEC)

# Link the program
$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile the source files into object files
graph.o: graph.cpp graph.hpp
	$(CXX) $(CXXFLAGS) -c $<

MST_algo.o: MST_algo.cpp MST_algo.hpp
	$(CXX) $(CXXFLAGS) -c $<

MST_tree.o: MST_tree.cpp MST_tree.hpp
	$(CXX) $(CXXFLAGS) -c $<

server.o: server.cpp server.hpp
	$(CXX) $(CXXFLAGS) -c $<

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean up
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
