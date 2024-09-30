# Define the C++ compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++14 -g -pthread -fprofile-arcs -ftest-coverage
LDFLAGS = -lgcov -pthread

# Define the target executable
TARGET = server

# Define the source files and object files
SRCS = main.cpp MST_algo.cpp graph.cpp MST_tree.cpp Activeobject.cpp Pipeline.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target to compile the code
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile each .cpp file into an object file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the server
run: $(TARGET)
	./$(TARGET)

# Code Coverage target
coverage: all
	./$(TARGET) &
	sleep 3
	pkill $(TARGET) # This stops the server after 3 seconds to simulate running
	gcov -o . $(SRCS) > coverage.txt 2>&1
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report

# Clean the coverage and object files
coverage_clean:
	rm -f *.gcno *.gcda *.gcov coverage.txt coverage.info
	rm -rf coverage_report

# Clean the project build and intermediate files
clean: coverage_clean
	rm -f $(OBJS) $(TARGET)

# Phony targets to avoid conflicts
.PHONY: all run clean coverage coverage_clean
