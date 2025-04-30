#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "TraceReader.h"
#include "Cache.h"
#include <string>

class Processor {
private:
    int coreId;                 // ID of this processor core
    TraceReader& traceReader;   // Reference to the shared trace reader
    Cache& l1Cache;             // Reference to this processor's L1 cache
    
    bool blocked;               // Whether this processor is blocked (on cache miss)
    unsigned int cyclesBlocked; // Count of cycles spent blocked
    unsigned int instructionsExecuted; // Count of instructions executed
    
public:
    // Constructor
    Processor(int id, TraceReader& reader, Cache& cache);
    
    // Execute the next instruction if possible (returns true if executed)
    bool executeNextInstruction();
    
    // Check if this processor is blocked
    bool isBlocked() const;
    
    // Set blocked state (when cache miss occurs)
    void setBlocked(bool state);
    
    // Get core ID
    int getCoreId() const;
    
    // Check if processor has more instructions
    bool hasMoreInstructions() const;
    void incrementCyclesBlocked() { cyclesBlocked++; }
    // Get statistics
    unsigned int getCyclesBlocked() const;
    unsigned int getInstructionsExecuted() const;
    
    // Reset statistics
    void resetStats();
};

#endif // PROCESSOR_H