# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -O2
TARGET = L1simulate
SRC = L1simulate.cpp

# Default rule to build the simulator
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Run the simulator with example parameters
run: $(TARGET)
	./$(TARGET) -t app4 -s 7 -E 2 -b 5 -o results.txt

# Clean up generated files
clean:
	rm -f $(TARGET) results.txt
