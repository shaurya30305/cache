#include "Simulator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Constructor
Simulator::Simulator(const SimulationConfig& config)
    : config(config), 
      traceReader(config.appName, 4),  // 4 cores/processors
      mainMemory(1 << config.blockBits),
      currentCycle(0),
      totalInstructions(0),
      totalCycles(0),
      totalMemoryAccesses(0),
      totalCacheHits(0),
      totalCacheMisses(0),
      cacheToCache(0) {
}

// Destructor
Simulator::~Simulator() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// Initialize simulation
bool Simulator::initialize() {
    // Open trace files
    if (!traceReader.openTraceFiles()) {
        std::cerr << "Error: Failed to open one or more trace files." << std::endl;
        return false;
    }
    
    // Open output file if specified
    if (!config.outputFile.empty()) {
        logFile.open(config.outputFile);
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open output file: " << config.outputFile << std::endl;
        }
    }
    
    // Initialize cache and processor components
    initializeComponents();
    
    return true;
}

// Initialize caches and processors
void Simulator::initializeComponents() {
    // Calculate cache parameters
    int numSets = 1 << config.setBits;
    int blockSize = 1 << config.blockBits;
    
    // Create caches and processors
    for (int i = 0; i < 4; i++) {  // 4 cores/processors
        // Create cache for this processor
        caches.push_back(std::make_unique<Cache>(
            i,                      // Core ID
            numSets,                // Number of sets
            config.associativity,   // Associativity
            blockSize,              // Block size
            config.setBits,         // Set bits
            config.blockBits,       // Block bits
            mainMemory              // Reference to main memory
        ));
        
        // Create processor connected to this cache
        processors.push_back(std::make_unique<Processor>(
            i,                      // Core ID
            traceReader,            // Reference to trace reader
            *caches.back()          // Reference to this processor's cache
        ));
    }
    
    // Connect caches to each other for coherence (will be implemented later)
}

// Run the simulation
void Simulator::run() {
    // Log header
    if (logFile.is_open()) {
        logFile << "Cycle,P0_Status,P1_Status,P2_Status,P3_Status,MemAccesses,CacheHits,CacheMisses" << std::endl;
    }
    
    // Run cycles until all processors have completed their traces
    while (!traceReader.allTracesCompleted() || 
           std::any_of(processors.begin(), processors.end(), 
                     [](const auto& p) { return p->isBlocked(); })) {
        
        // Process the next cycle
        processNextCycle();
        
        // Log cycle information
        if (currentCycle % 1000 == 0) {  // Log every 1000 cycles to reduce file size
            logStatistics();
        }
    }
    
    // Record final statistics
    totalCycles = currentCycle;
    
    // Collect total instructions from all processors
    totalInstructions = 0;
    for (const auto& proc : processors) {
        totalInstructions += proc->getInstructionsExecuted();
    }
    
    // Log final statistics
    logStatistics();
}

// Process the next simulation cycle
bool Simulator::processNextCycle() {
    currentCycle++;
    
    // Try to execute an instruction on each processor
    for (auto& proc : processors) {
        if (!proc->isBlocked() && proc->hasMoreInstructions()) {
            proc->executeNextInstruction();
        }
    }
    
    // Handle cache miss resolutions and coherence
    handleCacheMissResolution();
    
    // For now, we'll just check caches directly
    // In the full implementation, you would implement coherence here
    
    // Update statistics
    for (const auto& cache : caches) {
        totalCacheHits += cache->getHitCount();
        totalCacheMisses += cache->getMissCount();
        totalMemoryAccesses += cache->getAccessCount();
    }
    
    return true;
}

// Handle resolution of cache misses and unblock processors
void Simulator::handleCacheMissResolution() {
    // This is a simplified version
    // In the full implementation, you would handle timing for miss resolution,
    // including memory access times and cache-to-cache transfers
    
    // For now, we'll just unblock processors after a fixed delay
    
    // Check each cache for completed miss handling
    for (size_t i = 0; i < caches.size(); i++) {
        // If cache indicates miss is resolved, unblock the processor
        if (caches[i]->checkMissResolved()) {
            processors[i]->setBlocked(false);
        }
    }
}

// Log statistics for the current cycle
void Simulator::logStatistics() {
    if (!logFile.is_open()) {
        return;
    }
    
    logFile << currentCycle << ",";
    
    // Log processor status (B for blocked, A for active, C for completed)
    for (const auto& proc : processors) {
        if (proc->isBlocked()) {
            logFile << "B,";
        } else if (proc->hasMoreInstructions()) {
            logFile << "A,";
        } else {
            logFile << "C,";
        }
    }
    
    // Log memory statistics
    logFile << totalMemoryAccesses << ","
            << totalCacheHits << ","
            << totalCacheMisses << std::endl;
}

// Get total instructions executed
unsigned int Simulator::getTotalInstructions() const {
    return totalInstructions;
}

// Get total simulation cycles
unsigned int Simulator::getTotalCycles() const {
    return totalCycles;
}

// Calculate average memory access time
double Simulator::getAverageMemoryAccessTime() const {
    if (totalMemoryAccesses == 0) {
        return 0.0;
    }
    return static_cast<double>(totalCycles) / totalMemoryAccesses;
}

// Print results to console
void Simulator::printResults() const {
    std::cout << "\n===== Simulation Results =====\n";
    std::cout << "Total Instructions: " << totalInstructions << std::endl;
    std::cout << "Total Cycles: " << totalCycles << std::endl;
    std::cout << "Instructions per Cycle (IPC): " 
              << std::fixed << std::setprecision(3) 
              << static_cast<double>(totalInstructions) / totalCycles << std::endl;
    
    std::cout << "\n===== Memory System =====\n";
    std::cout << "Total Memory Accesses: " << totalMemoryAccesses << std::endl;
    std::cout << "Cache Hits: " << totalCacheHits 
              << " (" << std::fixed << std::setprecision(2) 
              << (totalMemoryAccesses > 0 ? 100.0 * totalCacheHits / totalMemoryAccesses : 0.0) 
              << "%)" << std::endl;
    std::cout << "Cache Misses: " << totalCacheMisses
              << " (" << std::fixed << std::setprecision(2) 
              << (totalMemoryAccesses > 0 ? 100.0 * totalCacheMisses / totalMemoryAccesses : 0.0) 
              << "%)" << std::endl;
    
    std::cout << "\n===== Cache-to-Cache Transfers =====\n";
    std::cout << "Total Transfers: " << cacheToCache << std::endl;
    
    // Include per-processor statistics
    std::cout << "\n===== Per-Processor Statistics =====\n";
    for (size_t i = 0; i < processors.size(); i++) {
        std::cout << "Processor " << i << ":\n";
        std::cout << "  Instructions Executed: " << processors[i]->getInstructionsExecuted() << std::endl;
        std::cout << "  Cycles Blocked: " << processors[i]->getCyclesBlocked()
                  << " (" << std::fixed << std::setprecision(2)
                  << (totalCycles > 0 ? 100.0 * processors[i]->getCyclesBlocked() / totalCycles : 0.0)
                  << "%)" << std::endl;
    }
    
    // Output file information
    if (!config.outputFile.empty()) {
        std::cout << "\nDetailed statistics written to: " << config.outputFile << std::endl;
    }
}