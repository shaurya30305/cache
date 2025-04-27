#ifndef TRACE_READER_H
#define TRACE_READER_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

// Simple struct to represent an instruction from the trace
struct Instruction {
    enum class Type { READ, WRITE, INVALID };
    
    Type type;
    uint32_t address;
    
    Instruction() : type(Type::INVALID), address(0) {}
    Instruction(Type t, uint32_t addr) : type(t), address(addr) {}
    
    bool isValid() const { return type != Type::INVALID; }
};

class TraceReader {
private:
    std::string applicationName;            // Base name of the application
    int numCores;                           // Number of cores/trace files
    std::vector<std::ifstream> traceFiles;  // One file per core
    std::vector<bool> fileEnded;            // Tracks EOF status for each file
    
public:
    // Constructor
    TraceReader(const std::string& appName, int numCores = 4);
    
    // Destructor
    ~TraceReader();
    
    // Open trace files
    bool openTraceFiles();
    
    // Check if there are more instructions for a core
    bool hasMoreInstructions(int coreId) const;
    
    // Get next instruction for a core
    Instruction getNextInstruction(int coreId);
    
    // Check if all trace files are at EOF
    bool allTracesCompleted() const;
    
    // Reset all trace files to beginning
    void resetTraces();
};

#endif // TRACE_READER_H