#include "CommandLine.h" // For SimulationConfig
#include "Simulator.h"
#include "Cache.h"
#include "CacheSet.h"
#include "CacheLine.h"
#include "Address.h"
#include "TraceReader.h"
#include "MainMemory.h"
#include "Processor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>

// Function to create test trace files
void createTestTraceFiles(const std::string& appName) {
    // Create directory if it doesn't exist
    if (!std::filesystem::exists(appName)) {
        std::filesystem::create_directory(appName);
    }
    
    // Sample instructions for processor 0
    std::vector<std::string> instructions = {
        "R 0x00000010", // Read from address 0x10
        "W 0x00000020", // Write to address 0x20
        "R 0x00000010", // Read hit to address 0x10
        "R 0x00001000", // Read miss to new address
        "W 0x00001004", // Write to address in same block as previous read
        "R 0x00002000", // Read to a new address
        "W 0x00003000", // Write to a new address
        "R 0x00003004", // Read hit (same block as previous write)
        "W 0x00000010", // Write hit to previously read address
        "R 0x00004000"  // Read miss to new address
    };
    
    // Create the main trace file with instructions
    std::ofstream file0(appName + "_proc0.trace");
    if (file0.is_open()) {
        for (const auto& inst : instructions) {
            file0 << inst << std::endl;
        }
        file0.close();
        std::cout << "Created trace file: " << appName << "_proc0.trace with " << instructions.size() << " instructions" << std::endl;
    } else {
        std::cerr << "Error: Could not create trace file: " << appName << "_proc0.trace" << std::endl;
    }
    
    // Create three empty trace files
    for (int i = 1; i < 4; i++) {
        std::ofstream file(appName + "_proc" + std::to_string(i) + ".trace");
        if (file.is_open()) {
            file.close();
            std::cout << "Created empty trace file: " << appName << "_proc" << i << ".trace" << std::endl;
        } else {
            std::cerr << "Error: Could not create trace file: " << appName << "_proc" << i << ".trace" << std::endl;
        }
    }
}

// Function to print cache line details
void printCacheLine(const CacheLine& line, int setIndex, int lineIndex) {
    std::cout << "  Line " << lineIndex << ": ";
    if (line.isValid()) {
        std::cout << "Valid, Tag: 0x" << std::hex << std::setw(8) << std::setfill('0') 
                  << line.getTag() << std::dec;
        std::cout << ", " << (line.isDirty() ? "Dirty" : "Clean");
        std::cout << ", LRU: " << line.getLRUCounter();
    } else {
        std::cout << "Invalid";
    }
    std::cout << std::endl;
}

// Function to print cache set details
void printCacheSet(const CacheSet& set, int setIndex) {
    std::cout << "Set " << setIndex << ":" << std::endl;
    const auto& lines = set.getLines();
    for (size_t i = 0; i < lines.size(); i++) {
        printCacheLine(lines[i], setIndex, i);
    }
}

// Function to print cache details
void printCache(const Cache& cache) {
    std::cout << "Cache (Core " << cache.getCoreId() << "):" << std::endl;
    std::cout << "  Sets: " << (1 << cache.getSetBits()) << ", Block size: " << (1 << cache.getBlockBits()) << " bytes" << std::endl;
    std::cout << "  Statistics: Accesses=" << cache.getAccessCount() 
              << ", Hits=" << cache.getHitCount() 
              << ", Misses=" << cache.getMissCount() << std::endl;
    
    // Print detailed cache contents
    const auto& sets = cache.getSets();
    for (size_t i = 0; i < sets.size(); i++) {
        printCacheSet(sets[i], i);
    }
    std::cout << std::endl;
}

// Function to print processor details
void printProcessor(const Processor& proc) {
    std::cout << "Processor " << proc.getCoreId() << ":" << std::endl;
    std::cout << "  Status: " << (proc.isBlocked() ? "Blocked" : "Ready") << std::endl;
    std::cout << "  Instructions Executed: " << proc.getInstructionsExecuted() << std::endl;
    std::cout << "  Cycles Blocked: " << proc.getCyclesBlocked() << std::endl;
    std::cout << "  Has More Instructions: " << (proc.hasMoreInstructions() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

// Custom simulator class for testing that exposes internals
class TestSimulator : public Simulator {
public:
    // Constructor - reuse the base class constructor
    TestSimulator(const SimulationConfig& config) : Simulator(config) {}
    
    // Run a single cycle and print detailed state
    bool runSingleCycle() {
        bool result = processNextCycle();
        printDetailedState();
        return result;
    }
    
    // Make isSimulationComplete public in this derived class
    using Simulator::isSimulationComplete;
    
    // Print detailed state of the simulator
    void printDetailedState() {
        std::cout << "\n============= CYCLE " << getCurrentCycle() << " =============\n";
        
        // Print cache states
        for (const auto& cache : getCaches()) {
            printCache(*cache);
        }
        
        // Print processor states
        for (const auto& proc : getProcessors()) {
            printProcessor(*proc);
        }
        
        // Print memory statistics
        std::cout << "Main Memory:" << std::endl;
        std::cout << "  Reads: " << getMainMemory().getReadCount() << std::endl;
        std::cout << "  Writes: " << getMainMemory().getWriteCount() << std::endl;
        std::cout << std::endl;
        
        std::cout << "========================================\n";
    }
    
    // Expose protected members for testing
    unsigned int getCurrentCycle() const { return currentCycle; }
    const std::vector<std::unique_ptr<Cache>>& getCaches() const { return caches; }
    const std::vector<std::unique_ptr<Processor>>& getProcessors() const { return processors; }
    const MainMemory& getMainMemory() const { return mainMemory; }
};

int main(int argc, char* argv[]) {
    // Create test trace files
    std::string appName = "test_app";
    createTestTraceFiles(appName);
    
    // Set up simulation configuration
    SimulationConfig config;
    config.appName = appName;
    config.setBits = 2;       // 4 sets
    config.associativity = 2; // 2-way associative
    config.blockBits = 4;     // 16-byte blocks
    config.outputFile = "test_output.log";
    
    // Create and initialize test simulator
    TestSimulator simulator(config);
    
    if (!simulator.initialize()) {
        std::cerr << "Failed to initialize simulator. Exiting." << std::endl;
        return 1;
    }
    
    // Display simulation parameters
    std::cout << "===== Test Simulation Configuration =====\n";
    std::cout << "Application: " << config.appName << std::endl;
    std::cout << "Cache Configuration:\n";
    std::cout << "  Sets: " << (1 << config.setBits) << " (2^" << config.setBits << ")" << std::endl;
    std::cout << "  Associativity: " << config.associativity << std::endl;
    std::cout << "  Block Size: " << (1 << config.blockBits) << " bytes (2^" << config.blockBits << ")" << std::endl;
    std::cout << "=====================================\n\n";
    
    // Run simulation one cycle at a time, printing state after each
    std::cout << "Starting simulation...\n";
    
    // Continue until all trace files are done and no processors are blocked
    while (!simulator.isSimulationComplete()) {
        simulator.runSingleCycle();
        
        // Wait for user to press Enter to continue to next cycle
        std::cout << "Press Enter to continue to next cycle...";
        std::cin.get();
    }
    
    std::cout << "Simulation completed.\n";
    
    // Print final results
    simulator.printResults();
    
    return 0;
}