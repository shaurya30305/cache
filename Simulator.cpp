#include "Simulator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// A single global bus‐reservation timestamp (file‐scope)
// ensures we serialize all bus transactions.
static unsigned int busBusyUntil = 0;

// Constructor
Simulator::Simulator(const SimulationConfig& config)
  : config(config),
    traceReader(config.appName, 4),  // 4 cores
    mainMemory(1 << config.blockBits),
    currentCycle(0),
    totalInstructions(0),
    totalCycles(0),
    totalMemoryAccesses(0),
    totalCacheHits(0),
    totalCacheMisses(0),
    cacheToCache(0)
{ }

// Destructor
Simulator::~Simulator() {
  if (logFile.is_open())
    logFile.close();
}

// Initialize simulation
bool Simulator::initialize() {
  if (!traceReader.openTraceFiles()) {
    std::cerr << "Error: Failed to open trace files.\n";
    return false;
  }

  if (!config.outputFile.empty()) {
    logFile.open(config.outputFile);
    if (!logFile.is_open())
      std::cerr << "Warning: Could not open output file: "
                << config.outputFile << "\n";
  }

  initializeComponents();
  return true;
}

// Initialize caches, processors, and coherence callbacks
void Simulator::initializeComponents() {
  int numSets   = 1 << config.setBits;
  int blockSize = 1 << config.blockBits;

  // 1) Create one Cache + Processor per core
  for (int i = 0; i < 4; ++i) {
    caches.emplace_back(std::make_unique<Cache>(
      i, numSets, config.associativity,
      blockSize, config.setBits,
      config.blockBits, mainMemory));

    processors.emplace_back(std::make_unique<Processor>(
      i, traceReader, *caches.back()));
  }

  // 2) Hook up coherence: every cache’s issueCoherenceRequest →
  //    we arbitrate and snoop here.
  for (auto& cachePtr : caches) {
    cachePtr->setCoherenceCallback(
      [this, blockSize](BusTransaction t,
                        const Address& addr,
                        int requestingCore,
                        bool& dataProvided,
                        int& sourceCore)
      {
        // --- 1) Reserve the bus ---
        // Determine bus‐transfer length (in cycles)
        unsigned int numWords = blockSize / 4;
        unsigned int length = 0;

        switch (t) {
          case BusTransaction::BUS_RD:
          case BusTransaction::BUS_RDX:
            // Transfer entire block: 2 cycles per word
            length = 2 * numWords;
            break;

          case BusTransaction::BUS_UPGR:
          case BusTransaction::INVALIDATE:
            // Just an invalidate or upgrade packet
            length = 2;
            break;

          case BusTransaction::FLUSH:
            // Write back dirty block to memory
            length = 100;
            break;

          default:
            length = 0;
        }

        // Bus arbitration: start when free
        unsigned int startCycle = std::max(currentCycle, busBusyUntil);
        busBusyUntil = startCycle + length;

        // --- 2) Snooping: let every *other* cache react ---
        for (int core = 0; core < (int)caches.size(); ++core) {
          if (core == requestingCore) continue;

          bool providedByPeer = false;
          // ask that cache to handle the bus event
          caches[core]->handleBusTransaction(
            t, addr, requestingCore, providedByPeer);

          // if it forwarded data & nobody else has yet, record it
          if (providedByPeer && !dataProvided) {
            dataProvided = true;
            sourceCore   = core;
            cacheToCache++;
          }
        }
      });
  }
}

// Main simulation loop
void Simulator::run() {
  if (logFile.is_open())
    logFile << "Cycle,P0,P1,P2,P3,MemAccesses,Hits,Misses\n";

  // Continue until all traces done & no one is blocked
  while (!traceReader.allTracesCompleted() ||
         std::any_of(processors.begin(), processors.end(),
                     [](auto& p){ return p->isBlocked(); }))
  {
    processNextCycle();
    if (currentCycle % 1000 == 0)
      logStatistics();
  }

  totalCycles = currentCycle;
  totalInstructions = 0;
  for (auto& p : processors)
    totalInstructions += p->getInstructionsExecuted();

  logStatistics();
}

// Advance one cycle
bool Simulator::processNextCycle() {
  ++currentCycle;
  for (auto& c : caches)
    c->setCycle(currentCycle);

  // Let each core try its next instruction if not blocked
  for (auto& p : processors) {
    if (!p->isBlocked() && p->hasMoreInstructions())
      p->executeNextInstruction();
  }

  // Unblock any cores whose miss has now resolved
  for (size_t i = 0; i < caches.size(); ++i) {
    if (caches[i]->checkMissResolved())
      processors[i]->setBlocked(false);
  }

  // Update global stats
  for (auto& c : caches) {
    totalCacheHits   += c->getHitCount();
    totalCacheMisses += c->getMissCount();
    totalMemoryAccesses += c->getAccessCount();
  }

  return true;
}

void Simulator::logStatistics() {
  if (!logFile.is_open()) return;
  logFile << currentCycle << ",";
  for (auto& p : processors) {
    if (p->isBlocked())      logFile << "B,";
    else if (p->hasMoreInstructions()) logFile << "A,";
    else                     logFile << "C,";
  }
  logFile << totalMemoryAccesses << ","
          << totalCacheHits       << ","
          << totalCacheMisses     << "\n";
}

unsigned int   Simulator::getTotalInstructions()      const { return totalInstructions; }
unsigned int   Simulator::getTotalCycles()            const { return totalCycles; }
double         Simulator::getAverageMemoryAccessTime()const {
  return totalMemoryAccesses==0 ? 0.0
       : double(totalCycles)/totalMemoryAccesses;
}

void Simulator::printResults() const {
  std::cout << "\n===== Simulation Results =====\n";
  std::cout << "Total Instructions: " << totalInstructions << "\n";
  std::cout << "Total Cycles:       " << totalCycles << "\n";
  std::cout << "IPC:                "
            << std::fixed << std::setprecision(3)
            << double(totalInstructions)/totalCycles << "\n";

  std::cout << "\n===== Memory System =====\n";
  std::cout << "Mem Accesses:       " << totalMemoryAccesses << "\n";
  std::cout << "Cache Hits:         " << totalCacheHits
            << " (" << std::fixed<<std::setprecision(2)
            << (100.0*totalCacheHits/totalMemoryAccesses)<<"%)\n";
  std::cout << "Cache Misses:       " << totalCacheMisses
            << " (" << std::fixed<<std::setprecision(2)
            << (100.0*totalCacheMisses/totalMemoryAccesses)<<"%)\n";

  std::cout << "\n===== Cache-to-Cache Transfers =====\n";
  std::cout << "Transfers:          " << cacheToCache << "\n";

  std::cout << "\n===== Per-Processor =====\n";
  for (size_t i = 0; i < processors.size(); ++i) {
    std::cout << "Core " << i
              << "  Instr: " << processors[i]->getInstructionsExecuted()
              << "  Stalls: " << processors[i]->getCyclesBlocked() << "\n";
  }

  if (!config.outputFile.empty())
    std::cout << "\nLog written to: " << config.outputFile << "\n";
}
