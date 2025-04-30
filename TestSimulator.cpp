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
#include <vector>
#include <string>
#include <getopt.h>
#include <algorithm>

// Parse command line arguments and return configuration
SimulationConfig parseCommandLineArguments(int argc, char* argv[]) {
    SimulationConfig config;
    int opt;
    const char* optString = "ht:s:E:b:o:";
    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch (opt) {
            case 'h': config.helpRequested = true; break;
            case 't': config.appName = optarg; break;
            case 's': config.setBits = std::stoi(optarg); break;
            case 'E': config.associativity = std::stoi(optarg); break;
            case 'b': config.blockBits = std::stoi(optarg); break;
            case 'o': config.outputFile = optarg; break;
            default:
                std::cerr << "Unknown option: " << static_cast<char>(optopt) << std::endl;
                config.helpRequested = true;
                break;
        }
    }
    return config;
}

// Validate the configuration
bool validateConfig(const SimulationConfig& config) {
    if (config.helpRequested) return true;
    bool valid = true;
    if (config.appName.empty()) {
        std::cerr << "Error: Application name (-t) is required" << std::endl;
        valid = false;
    }
    if (config.setBits <= 0) {
        std::cerr << "Error: Number of set bits (-s) must be positive" << std::endl;
        valid = false;
    }
    if (config.associativity <= 0) {
        std::cerr << "Error: Associativity (-E) must be positive" << std::endl;
        valid = false;
    }
    if (config.blockBits <= 0) {
        std::cerr << "Error: Number of block bits (-b) must be positive" << std::endl;
        valid = false;
    }
    return valid;
}

// Print help message
void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n";
    std::cout << "Simulate L1 cache with MESI coherence protocol.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t <n>    : Name of parallel application (e.g. app1) whose 4 traces are to be used in simulation\n";
    std::cout << "  -s <bits> : Number of set index bits (S = 2^s)\n";
    std::cout << "  -E <ways> : Associativity (number of lines per set)\n";
    std::cout << "  -b <bits> : Number of block bits (B = 2^b)\n";
    std::cout << "  -o <file> : Logs output in file for plotting etc.\n";
    std::cout << "  -h        : Prints this help\n";
}

// Debugging print functions
void printCacheLine(const CacheLine& line, int setIdx, int wayIdx) {
    std::cout << "  [S" << setIdx << ",W" << wayIdx << "] ";
    if (!line.isValid()) {
        std::cout << "INVALID\n";
        return;
    }
    std::cout << "Tag=0x" << std::hex << std::setw(8) << std::setfill('0')
              << line.getTag() << std::dec
              << ", State=" << line.getMESIStateString()
              << ", LRU=" << line.getLRUCounter()
              << (line.isDirty() ? ", Dirty" : ", Clean")
              << "\n";
}

void printCacheSet(const CacheSet& set, int setIdx) {
    for (unsigned int w = 0; w < set.getAssociativity(); ++w) {
        printCacheLine(set.getLines()[w], setIdx, w);
    }
}

void printCache(const Cache& c) {
    int sets = 1 << c.getSetBits();
    int ways = c.getAssociativity();
    int bsz  = 1 << c.getBlockBits();

    std::cout << "-- Cache Core" << c.getCoreId()
              << " (" << sets << " sets, " << ways << " ways, "
              << bsz << "B block) --\n";
    std::cout << "   Acc=" << c.getAccessCount()
              << " Ht="  << c.getHitCount()
              << " Ms="  << c.getMissCount() << "\n";
    for (int s = 0; s < sets; ++s) printCacheSet(c.getSets()[s], s);
}

void printProc(const Processor& p) {
    std::cout << "Proc" << p.getCoreId()
              << (p.isBlocked() ? "[Blk]" : "[Run]")
              << " Exec="   << p.getInstructionsExecuted()
              << " StallC=" << p.getCyclesBlocked()
              << " More="   << (p.hasMoreInstructions() ? "Y" : "N")
              << "\n";
}

//------------------------------------------------------------------------------
// Test‐simulator subclass
//------------------------------------------------------------------------------
class TestSimulator : public Simulator {
private:
    unsigned int finishCycle[4] = {0,0,0,0};
public:
    using Simulator::Simulator;
    using Simulator::isSimulationComplete;
    using Simulator::getCurrentCycle;
    using Simulator::getCaches;
    using Simulator::getProcessors;
    using Simulator::getMainMemory;

    // override to capture finish cycle (without printing details)
    bool processNextCycle() override {
        bool cont = Simulator::processNextCycle();
        for (auto& pp : getProcessors()) {
            const auto& p = *pp;
            int cid = p.getCoreId();
            if (!finishCycle[cid] && !p.hasMoreInstructions()) {
                finishCycle[cid] = getCurrentCycle();
            }
        }
        return cont;
    }

    // after completion, print all eight metrics
    void printAllStats() const {
        std::cout << "\n==== Final Statistics ====" << std::endl;
        for (int c = 0; c < 4; ++c) {
            const auto& cache = *getCaches()[c];
            const auto& proc  = *getProcessors()[c];

            unsigned reads     = cache.getReadCount();
            unsigned writes    = cache.getWriteCount();
            unsigned accesses  = cache.getAccessCount();
            unsigned misses    = cache.getMissCount();
            unsigned evicts    = cache.getEvictionCount();
            unsigned wbacks    = cache.getWritebackCount();
            unsigned idleCycles= proc.getCyclesBlocked();
            unsigned execCycles = getCurrentCycle() - idleCycles;
            double   missRate  = accesses ? double(misses)/accesses : 0.0;
            unsigned maxexectime = std::max({finishCycle[0], finishCycle[1], finishCycle[2], finishCycle[3]});

            std::cout << "Core " << c << ":\n";
            std::cout << "  1) #reads         = " << reads << "\n";
            std::cout << "     #writes        = " << writes << "\n";
            std::cout << "  2) exec cycles    = " << execCycles << "\n";
            std::cout << "  3) idle cycles    = " << idleCycles << "\n";
            std::cout << "  4) miss rate      = " << std::fixed << std::setprecision(2)
                      << (missRate * 100) << "%\n";
            std::cout << "  5) evictions      = " << evicts << "\n";
            std::cout << "  6) writebacks     = " << wbacks << "\n\n";
            std::cout << " Maximum execution time = " << maxexectime << "\n";
        }
        std::cout << "  7) bus invalidations = " << getInvalidationCount() << "\n";
        std::cout << "  8) bus traffic bytes = " << getBusTrafficBytes() << "\n";
    }
};

//------------------------------------------------------------------------------
// main()
//------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Parse command line arguments
    SimulationConfig config = parseCommandLineArguments(argc, argv);
    
    // If help was requested or invalid arguments, print help and exit
    if (config.helpRequested || !validateConfig(config)) {
        printHelp(argv[0]);
        return config.helpRequested ? 0 : 1;
    }
    
    // Create simulator with the configuration
    TestSimulator sim(config);
    
    // Initialize simulator
    if (!sim.initialize()) {
        std::cerr << "Failed to initialize simulator. Exiting." << std::endl;
        return 1;
    }
    
    // Display simulation parameters
    std::cout << "===== Simulation Configuration =====\n";
    std::cout << "Application: " << config.appName << std::endl;
    std::cout << "Cache Configuration:\n";
    std::cout << "  Sets: " << (1 << config.setBits) << " (2^" << config.setBits << ")" << std::endl;
    std::cout << "  Associativity: " << config.associativity << std::endl;
    std::cout << "  Block Size: " << (1 << config.blockBits) << " bytes (2^" << config.blockBits << ")" << std::endl;
    std::cout << "Output File: " << (config.outputFile.empty() ? "None" : config.outputFile) << std::endl;
    std::cout << "=====================================\n\n";
    
    // Run simulation
    std::cout << "Running simulation...\n";
    while (!sim.isSimulationComplete()) {
        sim.processNextCycle();
    }
    std::cout << "Simulation completed.\n";
    
    // Write statistics either to the specified output file or to stdout
    if (!config.outputFile.empty()) {
        std::ofstream ofs(config.outputFile);
        if (!ofs) {
            std::cerr << "Error: Could not open output file " << config.outputFile << std::endl;
            return 1;
        }
        auto* origBuf = std::cout.rdbuf();
        std::cout.rdbuf(ofs.rdbuf());
        sim.printAllStats();
        std::cout.rdbuf(origBuf);
    } else {
        sim.printAllStats();
    }
    
    return 0;
}
