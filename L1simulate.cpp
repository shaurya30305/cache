#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <memory>

// MESI Protocol states
enum CacheLineState {
    INVALID,
    SHARED,
    EXCLUSIVE,
    MODIFIED
};

// Bus request types
enum BusRequestType {
    BUS_READ,
    BUS_READ_X,
    BUS_UPGRADE,
    BUS_INVALIDATE,
    BUS_WRITEBACK
};

// Forward declarations
class Core;
class Bus;
class Memory;

// Cache line structure
struct CacheLine {
    bool valid;
    uint32_t tag;
    CacheLineState state;
    std::vector<uint32_t> data;  // Block data
    int lruCounter;              // For LRU replacement
    
    // Default constructor
    CacheLine() : valid(false), tag(0), state(INVALID), lruCounter(0) {}
    
    // Constructor with block size
    CacheLine(int blockSize) : valid(false), tag(0), state(INVALID), lruCounter(0) {
        data.resize(blockSize / 4, 0);
    }
};

// Bus request structure
struct BusRequest {
    int sourceCoreId;
    BusRequestType type;
    uint32_t address;
    std::vector<uint32_t> data;
    bool serviced;
    
    // Default constructor
    BusRequest() : sourceCoreId(0), type(BUS_READ), address(0), serviced(false) {}
    
    // Full constructor
    BusRequest(int id, BusRequestType t, uint32_t addr, int blockSize) 
        : sourceCoreId(id), type(t), address(addr), serviced(false) {
        data.resize(blockSize / 4, 0);
    }
};

// Statistics structure for a single core
struct CoreStats {
    int readInstructions;
    int writeInstructions;
    int totalCycles;
    int idleCycles;
    double missRate;
    int evictions;
    int writebacks;
    
    CoreStats() : readInstructions(0), writeInstructions(0), totalCycles(0), 
                  idleCycles(0), missRate(0.0), evictions(0), writebacks(0) {}
};

// Global statistics
struct SimulationStats {
    std::vector<CoreStats> coreStats;
    int busInvalidations;
    int busDataTraffic;  // in bytes
    
    SimulationStats(int numCores) : busInvalidations(0), busDataTraffic(0) {
        coreStats.resize(numCores);
    }
};

// Main memory simulation
class Memory {
public:
    Memory() {} // Default constructor
    
    void readBlock(uint32_t address, std::vector<uint32_t>& data) {
        // Simplified memory model - just clear the block
        // In a real implementation, this would read from actual memory
        for (size_t i = 0; i < data.size(); i++) {
            // Simulate some data based on address for verification
            data[i] = address + (i * 4);
        }
    }
    
    void writeBlock(uint32_t address, const std::vector<uint32_t>& data) {
        // Simplified memory model - no actual writing needed for simulation
        (void)address;  // Unused
        (void)data;     // Unused
    }
};

// Forward declaration of the Bus class
class Bus;

// Cache class implementation
class Cache {
private:
    int coreId;
    int s;          // Number of set bits
    int E;          // Associativity
    int b;          // Block size bits
    int S;          // Number of sets (2^s)
    int B;          // Block size in bytes (2^b)
    
    std::vector<std::vector<CacheLine>> sets;
    
    // Statistics
    int cacheAccesses;
    int cacheHits;
    int cacheMisses;
    int evictions;
    int writebacks;
    
    // Helper functions for address manipulation
    uint32_t getTag(uint32_t address) const {
        return address >> (s + b);
    }
    
    int getSetIndex(uint32_t address) const {
        return (address >> b) & ((1 << s) - 1);
    }
    
    int getBlockOffset(uint32_t address) const {
        return address & ((1 << b) - 1);
    }
    
    uint32_t constructAddress(uint32_t tag, int setIndex) const {
        return (tag << (s + b)) | (setIndex << b);
    }
    
    // Find the LRU line in a set
    int findLRULine(int setIndex) const {
        int lruIndex = 0;
        int minCounter = sets[setIndex][0].lruCounter;
        
        for (int i = 1; i < E; i++) {
            if (sets[setIndex][i].lruCounter < minCounter) {
                minCounter = sets[setIndex][i].lruCounter;
                lruIndex = i;
            }
        }
        
        return lruIndex;
    }
    
    // Update LRU counters
    void updateLRU(int setIndex, int lineIndex) {
        // Increment the accessed line's counter to be the highest
        int maxCounter = -1;
        for (int i = 0; i < E; i++) {
            if (sets[setIndex][i].valid && sets[setIndex][i].lruCounter > maxCounter) {
                maxCounter = sets[setIndex][i].lruCounter;
            }
        }
        
        sets[setIndex][lineIndex].lruCounter = maxCounter + 1;
    }
    
public:
    // Default constructor
    Cache() : coreId(0), s(0), E(0), b(0), S(0), B(0), 
              cacheAccesses(0), cacheHits(0), cacheMisses(0), evictions(0), writebacks(0) {}
    
    // Full constructor
    Cache(int id, int setBits, int associativity, int blockBits) 
        : coreId(id), s(setBits), E(associativity), b(blockBits),
          S(1 << setBits), B(1 << blockBits),
          cacheAccesses(0), cacheHits(0), cacheMisses(0), evictions(0), writebacks(0) {
        
        // Initialize cache sets and lines
        sets.resize(S);
        for (int i = 0; i < S; i++) {
            sets[i].resize(E);
            for (int j = 0; j < E; j++) {
                sets[i][j] = CacheLine(B);
            }
        }
    }
    
    // Copy constructor
    Cache(const Cache& other) 
        : coreId(other.coreId), s(other.s), E(other.E), b(other.b),
          S(other.S), B(other.B), sets(other.sets),
          cacheAccesses(other.cacheAccesses), cacheHits(other.cacheHits),
          cacheMisses(other.cacheMisses), evictions(other.evictions),
          writebacks(other.writebacks) {}
    
    // Move constructor
    Cache(Cache&& other) noexcept 
        : coreId(other.coreId), s(other.s), E(other.E), b(other.b),
          S(other.S), B(other.B), sets(std::move(other.sets)),
          cacheAccesses(other.cacheAccesses), cacheHits(other.cacheHits),
          cacheMisses(other.cacheMisses), evictions(other.evictions),
          writebacks(other.writebacks) {}
    
    // Assignment operator
    Cache& operator=(const Cache& other) {
        if (this != &other) {
            coreId = other.coreId;
            s = other.s;
            E = other.E;
            b = other.b;
            S = other.S;
            B = other.B;
            sets = other.sets;
            cacheAccesses = other.cacheAccesses;
            cacheHits = other.cacheHits;
            cacheMisses = other.cacheMisses;
            evictions = other.evictions;
            writebacks = other.writebacks;
        }
        return *this;
    }
    
    // Move assignment operator
    Cache& operator=(Cache&& other) noexcept {
        if (this != &other) {
            coreId = other.coreId;
            s = other.s;
            E = other.E;
            b = other.b;
            S = other.S;
            B = other.B;
            sets = std::move(other.sets);
            cacheAccesses = other.cacheAccesses;
            cacheHits = other.cacheHits;
            cacheMisses = other.cacheMisses;
            evictions = other.evictions;
            writebacks = other.writebacks;
        }
        return *this;
    }
    
    // Read operation
    bool read(uint32_t address, uint32_t& data, Bus& bus, Memory& memory);
    
    // Write operation
    bool write(uint32_t address, uint32_t data, Bus& bus, Memory& memory);
    
    // Handle bus requests (snooping)
    void handleBusRequest(BusRequest& request, Bus& bus);
    
    // Get cache statistics
    void getStats(CoreStats& stats) const {
        stats.missRate = (cacheAccesses > 0) ? static_cast<double>(cacheMisses) / cacheAccesses : 0.0;
        stats.evictions = evictions;
        stats.writebacks = writebacks;
    }
};

// Bus implementation for inter-cache communication
class Bus {
private:
    std::vector<Core*> cores;
    int dataTraffic;  // in bytes
    int invalidations;
    
public:
    Bus() : dataTraffic(0), invalidations(0) {}
    
    void connectCore(Core* core) {
        cores.push_back(core);
    }
    
    // Send a request on the bus
    int sendRequest(BusRequest& request);
    
    // Get bus statistics
    void getStats(SimulationStats& stats) const {
        stats.busInvalidations = invalidations;
        stats.busDataTraffic = dataTraffic;
    }
    
    void incrementInvalidations() {
        invalidations++;
    }
    
    void addDataTraffic(int bytes) {
        dataTraffic += bytes;
    }
};

// Core processor implementation
class Core {
private:
    int coreId;
    Cache cache;
    std::ifstream traceFile;
    
    bool isStalled;
    int stallCycles;
    int totalCycles;
    int idleCycles;
    int readInstructions;
    int writeInstructions;
    bool finished;
    
public:
    // Default constructor
    Core() : coreId(0), isStalled(false), stallCycles(0), totalCycles(0), 
             idleCycles(0), readInstructions(0), writeInstructions(0), finished(false) {}
    
    // Full constructor
    Core(int id, const std::string& traceFilename, int s, int E, int b)
        : coreId(id), cache(id, s, E, b), 
          isStalled(false), stallCycles(0), totalCycles(0), 
          idleCycles(0), readInstructions(0), writeInstructions(0), finished(false) {
        
        traceFile.open(traceFilename);
        if (!traceFile.is_open()) {
            std::cerr << "Error: Could not open trace file: " << traceFilename << std::endl;
            finished = true;
        }
    }
    
    // Copy constructor
    Core(const Core& other) = delete; // Prevent copying because of file stream
    
    // Move constructor
    Core(Core&& other) noexcept 
        : coreId(other.coreId), cache(std::move(other.cache)), 
          isStalled(other.isStalled), stallCycles(other.stallCycles),
          totalCycles(other.totalCycles), idleCycles(other.idleCycles),
          readInstructions(other.readInstructions), writeInstructions(other.writeInstructions),
          finished(other.finished) {
        
        // Handle file stream
        traceFile = std::move(other.traceFile);
    }
    
    // Assignment operator - deleted because of file stream
    Core& operator=(const Core& other) = delete;
    
    // Move assignment operator
    Core& operator=(Core&& other) noexcept {
        if (this != &other) {
            // Close existing file if open
            if (traceFile.is_open()) {
                traceFile.close();
            }
            
            coreId = other.coreId;
            cache = std::move(other.cache);
            isStalled = other.isStalled;
            stallCycles = other.stallCycles;
            totalCycles = other.totalCycles;
            idleCycles = other.idleCycles;
            readInstructions = other.readInstructions;
            writeInstructions = other.writeInstructions;
            finished = other.finished;
            
            // Move file stream
            traceFile = std::move(other.traceFile);
        }
        return *this;
    }
    
    ~Core() {
        if (traceFile.is_open()) {
            traceFile.close();
        }
    }
    
    // Execute next instruction from trace
    bool executeNextInstruction(Bus& bus, Memory& memory) {
        if (finished) return true;
        
        // If core is stalled, decrease stall cycles
        if (isStalled) {
            stallCycles--;
            idleCycles++;
            
            if (stallCycles <= 0) {
                isStalled = false;
            }
            
            totalCycles++;
            return false;
        }
        
        // Read next instruction from trace
        std::string line;
        if (!std::getline(traceFile, line)) {
            finished = true;
            return true;
        }
        
        std::istringstream iss(line);
        char operation;
        std::string addressStr;
        
        iss >> operation >> addressStr;
        
        // Parse address (remove 0x prefix if present)
        if (addressStr.substr(0, 2) == "0x") {
            addressStr = addressStr.substr(2);
        }
        
        uint32_t address = std::stoul(addressStr, nullptr, 16);
        uint32_t data = 0;  // Dummy data for simulation
        
        bool cacheHit = false;
        
        if (operation == 'R') {
            readInstructions++;
            cacheHit = cache.read(address, data, bus, memory);
        } else if (operation == 'W') {
            writeInstructions++;
            cacheHit = cache.write(address, data, bus, memory);
        } else {
            std::cerr << "Error: Unknown operation: " << operation << std::endl;
        }
        
        // Handle stalling if cache miss
        if (!cacheHit) {
            isStalled = true;
            stallCycles = 100;  // Memory access time
            idleCycles++;
        }
        
        totalCycles++;
        return false;
    }
    
    // Handle bus requests (called by the bus)
    void handleBusRequest(BusRequest& request, Bus& bus) {
        if (request.sourceCoreId != coreId) {
            cache.handleBusRequest(request, bus);
        }
    }
    
    // Get core statistics
    void getStats(CoreStats& stats) const {
        stats.readInstructions = readInstructions;
        stats.writeInstructions = writeInstructions;
        stats.totalCycles = totalCycles;
        stats.idleCycles = idleCycles;
        
        cache.getStats(stats);
    }
    
    int getId() const { return coreId; }
    bool isFinished() const { return finished; }
};

// Implementation of Bus::sendRequest now that Core is defined
int Bus::sendRequest(BusRequest& request) {
    // Process the request by all cores
    for (Core* core : cores) {
        if (core->getId() != request.sourceCoreId) {
            core->handleBusRequest(request, *this);
        }
    }
    
    // Update statistics based on request type
    int cycles = 0;
    
    switch (request.type) {
        case BUS_READ:
        case BUS_READ_X:
            if (request.serviced) {
                // Data transferred from another cache
                dataTraffic += request.data.size() * 4;  // 4 bytes per word
                cycles = 2 * request.data.size();        // 2 cycles per word
            } else {
                // Data fetched from memory
                dataTraffic += request.data.size() * 4;
                cycles = 100;  // Memory access time
            }
            break;
            
        case BUS_INVALIDATE:
            invalidations++;
            cycles = 2;  // Bus invalidate takes 2 cycles
            break;
            
        case BUS_UPGRADE:
            cycles = 2;  // Bus upgrade takes 2 cycles
            break;
            
        case BUS_WRITEBACK:
            dataTraffic += request.data.size() * 4;
            cycles = 100;  // Memory writeback time
            break;
    }
    
    return cycles;
}

// Cache read implementation
bool Cache::read(uint32_t address, uint32_t& data, Bus& bus, Memory& memory) {
    cacheAccesses++;
    
    uint32_t tag = getTag(address);
    int setIndex = getSetIndex(address);
    int wordOffset = (getBlockOffset(address) / 4);  // Convert byte offset to word offset
    
    // Check if the address is in cache
    for (int i = 0; i < E; i++) {
        CacheLine& line = sets[setIndex][i];
        
        if (line.valid && line.tag == tag && line.state != INVALID) {
            // Cache hit
            data = line.data[wordOffset];
            updateLRU(setIndex, i);
            cacheHits++;
            return true;
        }
    }
    
    // Cache miss
    cacheMisses++;
    
    // Send bus read request
    BusRequest request(coreId, BUS_READ, address & ~((1 << b) - 1), B);
    bus.sendRequest(request);
    
    if (!request.serviced) {
        // No other cache has this data, get from memory
        memory.readBlock(address & ~((1 << b) - 1), request.data);
    }
    
    // Find a place to put this block
    int lineIndex = -1;
    for (int i = 0; i < E; i++) {
        if (!sets[setIndex][i].valid) {
            lineIndex = i;
            break;
        }
    }
    

    // If no empty line, evict LRU
    if (lineIndex == -1) {
        lineIndex = findLRULine(setIndex);
        
        // Check if dirty, need writeback
        if (sets[setIndex][lineIndex].valid && sets[setIndex][lineIndex].state == MODIFIED) {
            writebacks++;
            
            // Construct address from tag and set index
            uint32_t evictAddress = constructAddress(sets[setIndex][lineIndex].tag, setIndex);
            
            // Send writeback request
            BusRequest writebackReq(coreId, BUS_WRITEBACK, evictAddress, B);
            writebackReq.data = sets[setIndex][lineIndex].data;
            
            bus.sendRequest(writebackReq);
            
            // Writeback to memory
            memory.writeBlock(evictAddress, sets[setIndex][lineIndex].data);
        }
        
        evictions++;
    }
    
    // Update cache line
    sets[setIndex][lineIndex].valid = true;
    sets[setIndex][lineIndex].tag = tag;
    sets[setIndex][lineIndex].data = request.data;
    
    // Set state based on other caches
    if (request.serviced) {
        sets[setIndex][lineIndex].state = SHARED;
    } else {
        sets[setIndex][lineIndex].state = EXCLUSIVE;
    }
    
    updateLRU(setIndex, lineIndex);
    
    // Return requested data
    data = sets[setIndex][lineIndex].data[wordOffset];
    
    return false;  // Return false to indicate cache miss
}

// Cache write implementation
bool Cache::write(uint32_t address, uint32_t data, Bus& bus, Memory& memory) {
    cacheAccesses++;
    
    uint32_t tag = getTag(address);
    int setIndex = getSetIndex(address);
    int wordOffset = (getBlockOffset(address) / 4);  // Convert byte offset to word offset
    
    // Check if the address is in cache
    for (int i = 0; i < E; i++) {
        CacheLine& line = sets[setIndex][i];
        
        if (line.valid && line.tag == tag && line.state != INVALID) {
            // Cache hit
            
            if (line.state == MODIFIED) {
                // Already modified, just update data
                line.data[wordOffset] = data;
            }
            else if (line.state == EXCLUSIVE) {
                // Exclusive, can modify without bus transaction
                line.data[wordOffset] = data;
                line.state = MODIFIED;
            }
            else if (line.state == SHARED) {
                // Shared, need to invalidate in other caches
                BusRequest request(coreId, BUS_INVALIDATE, address & ~((1 << b) - 1), B);
                bus.sendRequest(request);
                
                line.data[wordOffset] = data;
                line.state = MODIFIED;
            }
            
            updateLRU(setIndex, i);
            cacheHits++;
            return true;
        }
    }
    
    // Cache miss - write-allocate policy
    cacheMisses++;
    
    // Send bus read exclusive request
    BusRequest request(coreId, BUS_READ_X, address & ~((1 << b) - 1), B);
    bus.sendRequest(request);
    
    if (!request.serviced) {
        // No other cache has this data, get from memory
        memory.readBlock(address & ~((1 << b) - 1), request.data);
    }
    
    // Find a place to put this block
    int lineIndex = -1;
    for (int i = 0; i < E; i++) {
        if (!sets[setIndex][i].valid) {
            lineIndex = i;
            break;
        }
    }
    
    // If no empty line, evict LRU
    if (lineIndex == -1) {
        lineIndex = findLRULine(setIndex);
        
        // Check if dirty, need writeback
        if (sets[setIndex][lineIndex].valid && sets[setIndex][lineIndex].state == MODIFIED) {
            writebacks++;
            
            // Construct address from tag and set index
            uint32_t evictAddress = constructAddress(sets[setIndex][lineIndex].tag, setIndex);
            
            // Send writeback request
            BusRequest writebackReq(coreId, BUS_WRITEBACK, evictAddress, B);
            writebackReq.data = sets[setIndex][lineIndex].data;
            
            bus.sendRequest(writebackReq);
            
            // Writeback to memory
            memory.writeBlock(evictAddress, sets[setIndex][lineIndex].data);
        }
        
        evictions++;
    }
    
    // Update cache line
    sets[setIndex][lineIndex].valid = true;
    sets[setIndex][lineIndex].tag = tag;
    sets[setIndex][lineIndex].data = request.data;
    
    // Modify the data
    sets[setIndex][lineIndex].data[wordOffset] = data;
    sets[setIndex][lineIndex].state = MODIFIED;
    
    updateLRU(setIndex, lineIndex);
    
    return false;  // Return false to indicate cache miss
}

// Handle bus requests (snooping)
void Cache::handleBusRequest(BusRequest& request, Bus& bus) {
    uint32_t tag = getTag(request.address);
    int setIndex = getSetIndex(request.address);
    
    // Check if the address is in cache
    for (int i = 0; i < E; i++) {
        CacheLine& line = sets[setIndex][i];
        
        if (line.valid && line.tag == tag && line.state != INVALID) {
            // Found the line in this cache
            
            switch (request.type) {
                case BUS_READ:
                    if (line.state == MODIFIED) {
                        // Need to provide the data and change to SHARED
                        request.data = line.data;
                        request.serviced = true;
                        line.state = SHARED;
                    }
                    else if (line.state == EXCLUSIVE) {
                        // Other cache can have a copy, change to SHARED
                        request.data = line.data;
                        request.serviced = true;
                        line.state = SHARED;
                    }
                    // For SHARED state, other cache will get data from memory
                    break;
                    
                case BUS_READ_X:
                    if (line.state == MODIFIED) {
                        // Other cache wants exclusive access - provide data
                        request.data = line.data;
                        request.serviced = true;
                        
                        // Invalidate our copy
                        line.state = INVALID;
                        line.valid = false;
                    }
                    else if (line.state == EXCLUSIVE || line.state == SHARED) {
                        // Invalidate our copy
                        line.state = INVALID;
                        line.valid = false;
                    }
                    break;
                    
                case BUS_INVALIDATE:
                    if (line.state == SHARED || line.state == EXCLUSIVE) {
                        // Other cache wants to modify - invalidate our copy
                        line.state = INVALID;
                        line.valid = false;
                        bus.incrementInvalidations();
                    }
                    break;
                    
                case BUS_UPGRADE:
                    if (line.state == SHARED) {
                        // Other cache upgrading from shared to modified
                        line.state = INVALID;
                        line.valid = false;
                        bus.incrementInvalidations();
                    }
                    break;
                    
                case BUS_WRITEBACK:
                    // Nothing to do - writebacks don't affect other caches
                    break;
            }
            
            return;
        }
    }
}

// Cache simulator class to manage the entire simulation
class CacheSimulator {
private:
    std::vector<std::unique_ptr<Core>> cores;
    Bus bus;
    Memory memory;
    SimulationStats stats;
    
    int s;  // Set bits
    int E;  // Associativity
    int b;  // Block bits
    
public:
    CacheSimulator(const std::string& tracePrefix, int setBits, int associativity, int blockBits)
        : stats(4), s(setBits), E(associativity), b(blockBits) {
        
        // Create cores with individual trace files
        for (int i = 0; i < 4; i++) {
            std::string traceFile = tracePrefix + "_proc" + std::to_string(i) + ".trace";
            // Use std::unique_ptr with explicit constructor in C++11
            cores.push_back(std::unique_ptr<Core>(new Core(i, traceFile, setBits, associativity, blockBits)));
            bus.connectCore(cores[i].get());
        }
    }
    
    void runSimulation() {
        bool allCoresFinished = false;
        int maxCycles = 10000000;  // Safeguard against infinite loops
        int currentCycle = 0;
        
        while (!allCoresFinished && currentCycle < maxCycles) {
            allCoresFinished = true;
            
            // Try to execute one instruction from each core
            for (size_t i = 0; i < cores.size(); i++) {
                bool coreFinished = cores[i]->executeNextInstruction(bus, memory);
                if (!coreFinished) {
                    allCoresFinished = false;
                }
            }
            
            currentCycle++;
        }
        
        if (currentCycle >= maxCycles) {
            std::cerr << "Warning: Simulation reached maximum cycle limit of " << maxCycles << std::endl;
        }
        
        // Collect statistics
        for (int i = 0; i < 4; i++) {
            cores[i]->getStats(stats.coreStats[i]);
        }
        
        bus.getStats(stats);
    }
    
    void printStats(std::ostream& out) {
        out << "===== Cache Simulation Results =====" << std::endl;
        out << "Cache parameters: " << (1 << s) << " sets, " << E << "-way, " 
            << (1 << b) << " bytes per block" << std::endl;
        out << std::endl;
        
        for (int i = 0; i < 4; i++) {
            out << "Core " << i << " Statistics:" << std::endl;
            out << "  Read Instructions: " << stats.coreStats[i].readInstructions << std::endl;
            out << "  Write Instructions: " << stats.coreStats[i].writeInstructions << std::endl;
            out << "  Total Instructions: " 
                << (stats.coreStats[i].readInstructions + stats.coreStats[i].writeInstructions) << std::endl;
            out << "  Total Execution Cycles: " << stats.coreStats[i].totalCycles << std::endl;
            out << "  Idle Cycles: " << stats.coreStats[i].idleCycles << std::endl;
            out << "  Cache Miss Rate: " << std::fixed << std::setprecision(4)
                << (stats.coreStats[i].missRate * 100) << "%" << std::endl;
            out << "  Cache Evictions: " << stats.coreStats[i].evictions << std::endl;
            out << "  Cache Writebacks: " << stats.coreStats[i].writebacks << std::endl;
            out << std::endl;
        }
        
        out << "Bus Statistics:" << std::endl;
        out << "  Number of Invalidations: " << stats.busInvalidations << std::endl;
        out << "  Data Traffic on Bus: " << stats.busDataTraffic << " bytes" << std::endl;
    }
    
    int getMaxExecutionTime() const {
        int maxTime = 0;
        for (int i = 0; i < 4; i++) {
            maxTime = std::max(maxTime, stats.coreStats[i].totalCycles);
        }
        return maxTime;
    }
};

// Print command line help
void printHelp() {
    std::cout << "Usage: ./L1simulate [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -t <tracefile>: name of parallel application (e.g. app1) whose 4 traces are to be used" << std::endl;
    std::cout << "  -s <s>: number of set index bits (number of sets in the cache = S = 2^s)" << std::endl;
    std::cout << "  -E <E>: associativity (number of cache lines per set)" << std::endl;
    std::cout << "  -b <b>: number of block bits (block size = B = 2^b)" << std::endl;
    std::cout << "  -o <outfilename>: logs output in file for plotting etc." << std::endl;
    std::cout << "  -h: prints this help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string tracePrefix;
    int s = 0, E = 0, b = 0;
    std::string outFileName;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            tracePrefix = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            s = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-E") == 0 && i + 1 < argc) {
            E = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            b = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outFileName = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        }
    }
    
    // Validate parameters
    if (tracePrefix.empty()) {
        std::cerr << "Error: Trace prefix is required (-t)" << std::endl;
        printHelp();
        return 1;
    }
    
    // Set default parameters if not specified
    if (s <= 0) {
        s = 6;  // 64 sets
        std::cout << "Using default value for s: " << s << std::endl;
    }
    if (E <= 0) {
        E = 2;  // 2-way set associative
        std::cout << "Using default value for E: " << E << std::endl;
    }
    if (b <= 0) {
        b = 5;  // 32-byte blocks
        std::cout << "Using default value for b: " << b << std::endl;
    }
    
    try {
        // Initialize and run simulation
        CacheSimulator simulator(tracePrefix, s, E, b);
        
        std::cout << "Starting cache simulation..." << std::endl;
        simulator.runSimulation();
        std::cout << "Simulation complete!" << std::endl;
        
        // Output results
        if (outFileName.empty()) {
            simulator.printStats(std::cout);
        } else {
            std::ofstream outFile(outFileName);
            if (outFile.is_open()) {
                simulator.printStats(outFile);
                std::cout << "Results written to " << outFileName << std::endl;
                outFile.close();
            } else {
                std::cerr << "Error: Could not open output file " << outFileName << std::endl;
                simulator.printStats(std::cout);
            }
        }
        
        // Output max execution time for experiments
        std::cout << "Maximum Execution Time: " << simulator.getMaxExecutionTime() << " cycles" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}