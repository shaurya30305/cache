#include "CacheLine.h"
#include <cstring>
#include <iostream>

// Constructor - initialize an empty cache line with specified block size
CacheLine::CacheLine(int blockSize) 
    : mesiState(MESIState::INVALID), dirty(false), tag(0), lruCounter(0) {
    // Initialize data vector with specified block size (in bytes)
    data.resize(blockSize, 0);
}

// Reset the cache line (invalidate)
void CacheLine::reset() {
    mesiState = MESIState::INVALID;
    dirty = false;
    tag = 0;
    lruCounter = 0;
    std::fill(data.begin(), data.end(), 0);
}

// Get MESI state
MESIState CacheLine::getMESIState() const {
    return mesiState;
}

// Set MESI state
void CacheLine::setMESIState(MESIState state) {
    // If transitioning from M to another state, data should be written back
    if (mesiState == MESIState::MODIFIED && state != MESIState::MODIFIED) {
        dirty = true;  // Ensure dirty bit is set for write-back
    }
    
    // If transitioning to INVALID, no need to keep dirty information
    if (state == MESIState::INVALID) {
        dirty = false;
    }
    
    // If transitioning to SHARED or EXCLUSIVE from MODIFIED, data is now clean
    if ((state == MESIState::SHARED || state == MESIState::EXCLUSIVE) && 
        mesiState == MESIState::MODIFIED) {
        dirty = false;
    }
    
    mesiState = state;
}

// Helper methods for checking MESI states
bool CacheLine::isModified() const {
    return mesiState == MESIState::MODIFIED;
}

bool CacheLine::isExclusive() const {
    return mesiState == MESIState::EXCLUSIVE;
}

bool CacheLine::isShared() const {
    return mesiState == MESIState::SHARED;
}

bool CacheLine::isInvalid() const {
    return mesiState == MESIState::INVALID;
}

// Check if the line is valid (any state except INVALID)
bool CacheLine::isValid() const {
    return mesiState != MESIState::INVALID;
}

// Check if the line is dirty
bool CacheLine::isDirty() const {
    // In MESI, MODIFIED state implies dirty
    return mesiState == MESIState::MODIFIED || dirty;
}

// Get the tag
uint32_t CacheLine::getTag() const {
    return tag;
}

// Match tag - returns true if this line contains the specified tag
bool CacheLine::matchTag(uint32_t tagToMatch) const {
    return isValid() && (tag == tagToMatch);
}

// Get LRU counter value
unsigned int CacheLine::getLRUCounter() const {
    return lruCounter;
}

// Set LRU counter value
void CacheLine::setLRUCounter(unsigned int counter) {
    lruCounter = counter;
}

// Update LRU counter (mark as most recently used)
void CacheLine::updateLRU(unsigned int newValue) {
    lruCounter = newValue;
}

// Load data into the cache line
void CacheLine::loadData(const std::vector<uint8_t>& newData, uint32_t newTag, MESIState state) {
    if (newData.size() != data.size()) {
        std::cerr << "Error: Data size mismatch in cache line load" << std::endl;
        return;
    }
    
    // Copy data
    std::copy(newData.begin(), newData.end(), data.begin());
    
    // Set tag and state
    tag = newTag;
    mesiState = state;
    
    // Initial load is clean unless in MODIFIED state
    dirty = (state == MESIState::MODIFIED);
}

// Read a word (4 bytes) from the cache line
uint32_t CacheLine::readWord(uint32_t offset) const {
    if (!isValid()) {
        std::cerr << "Error: Attempting to read from invalid cache line" << std::endl;
        return 0;
    }
    
    if (offset + 3 >= data.size()) {
        std::cerr << "Error: Word offset out of range in cache line read" << std::endl;
        return 0;
    }
    
    // Combine 4 bytes into a word (assuming little-endian)
    uint32_t word = 0;
    word |= static_cast<uint32_t>(data[offset]) << 0;
    word |= static_cast<uint32_t>(data[offset + 1]) << 8;
    word |= static_cast<uint32_t>(data[offset + 2]) << 16;
    word |= static_cast<uint32_t>(data[offset + 3]) << 24;
    
    return word;
}

// Write a word (4 bytes) to the cache line
void CacheLine::writeWord(uint32_t offset, uint32_t value) {
    if (!isValid()) {
        std::cerr << "Error: Attempting to write to invalid cache line" << std::endl;
        return;
    }
    
    if (offset + 3 >= data.size()) {
        std::cerr << "Error: Word offset out of range in cache line write" << std::endl;
        return;
    }
    
    // Split word into 4 bytes (assuming little-endian)
    data[offset] = (value >> 0) & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
    data[offset + 2] = (value >> 16) & 0xFF;
    data[offset + 3] = (value >> 24) & 0xFF;
    
    // A write always makes the line MODIFIED in MESI
    mesiState = MESIState::MODIFIED;
    dirty = true;
}

// Mark line as dirty (after a write)
void CacheLine::setDirty() {
    if (isValid()) {
        dirty = true;
        
        // In MESI, a write to a valid line makes it MODIFIED
        mesiState = MESIState::MODIFIED;
    }
}

// Clear dirty bit (after write-back)
void CacheLine::clearDirty() {
    dirty = false;
    
    // If the line was in MODIFIED state, it becomes EXCLUSIVE after write-back
    if (mesiState == MESIState::MODIFIED) {
        mesiState = MESIState::EXCLUSIVE;
    }
}

// Get entire data block
const std::vector<uint8_t>& CacheLine::getData() const {
    return data;
}

// Get block size
size_t CacheLine::getBlockSize() const {
    return data.size();
}

// String representation of MESI state
std::string CacheLine::getMESIStateString() const {
    switch (mesiState) {
        case MESIState::MODIFIED:  return "MODIFIED";
        case MESIState::EXCLUSIVE: return "EXCLUSIVE";
        case MESIState::SHARED:    return "SHARED";
        case MESIState::INVALID:   return "INVALID";
        default:                   return "UNKNOWN";
    }
}