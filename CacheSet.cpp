#include "CacheSet.h"
#include <algorithm>
#include <limits>
#include <iostream>

// Constructor - initialize an empty set with specified associativity and block size
CacheSet::CacheSet(unsigned int associativity, int blockSize) 
    : lruCounter(0), associativity(associativity) {
    
    // Create specified number of cache lines
    lines.reserve(associativity);
    for (unsigned int i = 0; i < associativity; i++) {
        lines.emplace_back(blockSize);
    }
}

// Find a cache line matching the specified tag
// Returns pointer to the line if found, nullptr if not found
CacheLine* CacheSet::findLine(uint32_t tag) {
    for (auto& line : lines) {
        if (line.isValid() && line.getTag() == tag) {
            return &line;
        }
    }
    return nullptr;
}

// Find a cache line matching the specified tag (const version)
// Returns pointer to the line if found, nullptr if not found
const CacheLine* CacheSet::findLine(uint32_t tag) const {
    for (const auto& line : lines) {
        if (line.isValid() && line.getTag() == tag) {
            return &line;
        }
    }
    return nullptr;
}

// Find a victim line for replacement using LRU policy
// Returns pointer to the LRU line
CacheLine* CacheSet::findVictim() {
    // First, try to find an invalid line
    CacheLine* invalidLine = findInvalidLine();
    if (invalidLine != nullptr) {
        return invalidLine;
    }
    
    // If all lines are valid, find the one with the lowest LRU counter
    CacheLine* victim = &lines[0];
    unsigned int lowestCounter = lines[0].getLRUCounter();
    
    for (auto& line : lines) {
        if (line.getLRUCounter() < lowestCounter) {
            lowestCounter = line.getLRUCounter();
            victim = &line;
        }
    }
    
    return victim;
}

// Update LRU status of a line that was just accessed
void CacheSet::updateLRU(CacheLine* line) {
    // Increment global counter
    lruCounter++;
    
    // Prevent potential overflow
    if (lruCounter == std::numeric_limits<unsigned int>::max()) {
        // Reset all counters while maintaining relative ordering
        std::vector<std::pair<unsigned int, CacheLine*>> lineCounters;
        lineCounters.reserve(lines.size());
        
        for (auto& cacheLine : lines) {
            if (cacheLine.isValid()) {
                lineCounters.emplace_back(cacheLine.getLRUCounter(), &cacheLine);
            }
        }
        
        // Sort by counter value (ascending)
        std::sort(lineCounters.begin(), lineCounters.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        // Reassign counters starting from 0 with increments of 1
        for (size_t i = 0; i < lineCounters.size(); i++) {
            lineCounters[i].second->setLRUCounter(i);
        }
        
        // Reset global counter
        lruCounter = lineCounters.size();
    }
    
    // Update the accessed line's counter to the current global counter
    line->setLRUCounter(lruCounter);
}

// Check if set is full (no invalid lines)
bool CacheSet::isFull() const {
    for (const auto& line : lines) {
        if (!line.isValid()) {
            return false;
        }
    }
    return true;
}

// Find an invalid line if one exists
// Returns pointer to an invalid line, or nullptr if all lines are valid
CacheLine* CacheSet::findInvalidLine() {
    for (auto& line : lines) {
        if (!line.isValid()) {
            return &line;
        }
    }
    return nullptr;
}

// Get vector of all cache lines in this set
const std::vector<CacheLine>& CacheSet::getLines() const {
    return lines;
}

// Get number of lines in this set (associativity)
unsigned int CacheSet::getAssociativity() const {
    return associativity;
}

// Get the current LRU counter value
unsigned int CacheSet::getLRUCounter() const {
    return lruCounter;
}