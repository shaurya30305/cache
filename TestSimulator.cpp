#include "CommandLine.h"    // For SimulationConfig
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

// Function to create per-core trace files that exercise MESI coherence
// Writes directly to working directory so TraceReader can find them
void createTestTraceFiles(const std::string& appName) {
    // CORE 0: Write then read same block (M → hit)
    {
        std::ofstream f(appName + "_proc0.trace");
        f << "W 0x00001000" << std::endl;
        f << "R 0x00001004" << std::endl;
    }
    std::cout << "Created '" << appName << "_proc0.trace' [W 0x1000, R 0x1004]" << std::endl;

    // CORE 1: Dummy read then read the block Core0 wrote (tests M→S)
    {
        std::ofstream f(appName + "_proc1.trace");
        f << "R 0x00003000" << std::endl;
        f << "R 0x00001000" << std::endl;
    }
    std::cout << "Created '" << appName << "_proc1.trace' [R 0x3000, R 0x1000]" << std::endl;

    // CORE 2: Two dummy reads then write same block (tests invalidation)
    {
        std::ofstream f(appName + "_proc2.trace");
        f << "R 0x00004000" << std::endl;
        f << "R 0x00004004" << std::endl;
        f << "W 0x00001000" << std::endl;
    }
    std::cout << "Created '" << appName << "_proc2.trace' [R 0x4000, R 0x4004, W 0x1000]" << std::endl;

    // CORE 3: Three dummy reads then read same block (tests shared supply)
    {
        std::ofstream f(appName + "_proc3.trace");
        f << "R 0x00005000" << std::endl;
        f << "R 0x00005004" << std::endl;
        f << "R 0x00005008" << std::endl;
        f << "R 0x00001000" << std::endl;
    }
    std::cout << "Created '" << appName << "_proc3.trace' [R 0x5000×3, R 0x1000]" << std::endl;
}

// Print a single cache line
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

// Print one cache set
void printCacheSet(const CacheSet& set, int setIdx) {
    int ways = set.getAssociativity();
    for (int w = 0; w < ways; ++w) {
        printCacheLine(set.getLines()[w], setIdx, w);
    }
}

// Print full cache contents and stats
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

    for (int s = 0; s < sets; ++s) {
        printCacheSet(c.getSets()[s], s);
    }
}

// Print processor status
void printProc(const Processor& p) {
    std::cout << "Proc" << p.getCoreId()
              << (p.isBlocked() ? "[Blk]" : "[Run]")
              << " Exec="    << p.getInstructionsExecuted()
              << " StallC="  << p.getCyclesBlocked()
              << " More="    << (p.hasMoreInstructions() ? "Y" : "N")
              << "\n";
}

// Test simulator subclass exposing internals
class TestSimulator : public Simulator {
public:
    using Simulator::Simulator;
    using Simulator::isSimulationComplete;
    using Simulator::getCurrentCycle;
    using Simulator::getCaches;
    using Simulator::getProcessors;
    using Simulator::getMainMemory;

    void stepAndPrint() {
        processNextCycle();
        std::cout << "\n===== Cycle " << getCurrentCycle() << " =====\n";
        for (auto& cp : getCaches())     printCache(*cp);
        for (auto& pp : getProcessors()) printProc(*pp);
        auto& mem = getMainMemory();
        std::cout << "Mem[R=" << mem.getReadCount()
                  << ",W="     << mem.getWriteCount() << "]\n";
    }
};

int main() {
    std::string appName = "test_app";
    createTestTraceFiles(appName);

    SimulationConfig cfg;
    cfg.appName       = appName;
    cfg.setBits       = 2;  // 4 sets
    cfg.associativity = 2;  // 2-way
    cfg.blockBits     = 4;  // 16 bytes
    cfg.outputFile    = "test_output.log";

    TestSimulator sim(cfg);
    if (!sim.initialize()) {
        std::cerr << "Init failed\n";
        return 1;
    }

    std::cout << "Starting MESI coherence test...\n";
    while (!sim.isSimulationComplete()) {
        sim.stepAndPrint();
        std::cout << "Press Enter...";
        std::cin.get();
    }
    std::cout << "Simulation done.\n";
    sim.printResults();
    return 0;
}
