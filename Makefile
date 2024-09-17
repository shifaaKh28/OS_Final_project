# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -pthread

# Executable name
TARGET = server

# Source files
SRC = main.cpp MST_algo.cpp graph.cpp MST_tree.cpp Activeobject.cpp Pipeline.cpp

# Object files (generated from .cpp files)
OBJ = $(SRC:.cpp=.o)

# Build the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Compile each source file into an object file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean the build (remove object files and executable)
clean:
	rm -f $(OBJ) $(TARGET)

# Run the server
run: $(TARGET)
	./$(TARGET)
