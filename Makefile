# Contributors: Wasim Shebalny, Shifaa Khatib.
# Define the C++ compiler and the flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++14 -g -pthread -fprofile-arcs -ftest-coverage
LDFLAGS = -lgcov -fprofile-arcs -ftest-coverage -lpthread

# Define the target executable
TARGET = server

# Define the source files and object files
SRCS = main.cpp MST_algo.cpp graph.cpp MST_tree.cpp Activeobject.cpp Pipeline.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run Valgrind memory check
valgrind: $(TARGET)
	valgrind --leak-check=full --track-origins=yes --log-file=valgrind_report.txt ./$(TARGET)

# Code Coverage target
coverage: all
	./$(TARGET)
	gcov -o . $(SRCS) > tst.txt 2>&1
	lcov --capture --directory . --output-file coverage.info >> tst.txt 2>&1
	genhtml coverage.info --output-directory out >> tst.txt 2>&1

# Clean intermediate coverage files
coverage_clean:
	rm -f *.gcno *.gcda *.gcov

# Clean up all build files, intermediate files, and coverage files
clean: coverage_clean
	rm -f $(OBJS) $(TARGET) gmon.out callgrind.out coverage.info
	rm -rf out valgrind_report.txt gprof_report.txt tst.txt

# Phony targets
.PHONY: all clean coverage profile valgrind coverage_clean
