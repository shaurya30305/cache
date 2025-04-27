#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "CommandLine.h" // For SimulationConfig
#include "TraceReader.h"
#include "Cache.h"
#include "Processor.h"
#include "MainMemory.h"
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm> // For std::none_of

class Simulator {
private:
    // Statistics
    unsigned int totalInstructions;
    unsigned int totalCycles;
    unsigned int totalMemoryAccesses;
    unsigned int totalCacheHits;
    unsigned int totalCacheMisses;
    
    // Cache-to-cache transfers (for coherence)
    unsigned int cacheToCache;
    
    // Private methods
    void logStatistics();
    void handleCacheMissResolution();
    void initializeComponents(); // Added this declaration
    
protected: // Changed from private to protected for TestSimulator access
    // Configuration
    SimulationConfig config;
    
    // Main components
    TraceReader traceReader;
    MainMemory mainMemory;
    std::vector<std::unique_ptr<Cache>> caches;
    std::vector<std::unique_ptr<Processor>> processors;
    
    // Simulation state
    unsigned int currentCycle;
    std::ofstream logFile;
    
    // Protected methods for derived classes
    bool processNextCycle();
    
    // Add this method for TestSimulator
    bool isSimulationComplete() {
        return traceReader.allTracesCompleted() && 
               std::none_of(processors.begin(), processors.end(), 
                          [](const auto& p) { return p->isBlocked(); });
    }
    
public:
    // Constructor
    Simulator(const SimulationConfig& config);
    
    // Destructor
    ~Simulator();
    
    // Run the simulation
    void run();
    
    // Initialize simulation
    bool initialize();
    
    // Get statistics
    unsigned int getTotalInstructions() const;
    unsigned int getTotalCycles() const;
    double getAverageMemoryAccessTime() const;
    
    // Print results to console
    void printResults() const;
};

#endif // SIMULATOR_H