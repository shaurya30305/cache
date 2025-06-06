# Makefile for L1 Cache Simulator

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -w # -w suppresses all warnings

# Target executable
TARGET = L1simulate

# Source files
SRCS = TestSimulator.cpp \
       CommandLine.cpp \
       Simulator.cpp \
       Cache.cpp \
       CacheSet.cpp \
       CacheLine.cpp \
       Address.cpp \
       Processor.cpp \
       TraceReader.cpp \
       MainMemory.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files
DEPS = $(wildcard *.h)

# Default target
all: $(TARGET)

# Link the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild everything
rebuild: clean all

# PHONY targets
.PHONY: all clean rebuild