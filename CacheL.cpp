#include "CacheLine.h"
#include <cstring>
#include <iostream>

// Constructor - initialize an empty cache line with specified block size
CacheLine::CacheLine(int blockSize) : valid(false), dirty(false), tag(0), lruCounter(0) {
    // Initialize data vector with specified block size (in bytes)
    data.resize(blockSize, 0);
}

// Reset the cache line (invalidate)
void CacheLine::reset() {
    valid = false;
    dirty = false;
    tag = 0;
    lruCounter = 0;
    std::fill(data.begin(), data.end(), 0);
}

// Check if the line is valid
bool CacheLine::isValid() const {
    return valid;
}

// Check if the line is dirty
bool CacheLine::isDirty() const {
    return dirty;
}

// Get the tag
uint32_t CacheLine::getTag() const {
    return tag;
}

// Match tag - returns true if this line contains the specified tag
bool CacheLine::matchTag(uint32_t tagToMatch) const {
    return valid && (tag == tagToMatch);
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
void CacheLine::loadData(const std::vector<uint8_t>& newData, uint32_t newTag) {
    if (newData.size() != data.size()) {
        std::cerr << "Error: Data size mismatch in cache line load" << std::endl;
        return;
    }
    
    // Copy data
    std::copy(newData.begin(), newData.end(), data.begin());
    
    // Set tag and valid bit
    tag = newTag;
    valid = true;
    dirty = false; // Initial load is clean
}

// Read a word (4 bytes) from the cache line
uint32_t CacheLine::readWord(uint32_t offset) const {
    if (!valid) {
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
    if (!valid) {
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
    
    // Mark as dirty
    dirty = true;
}

// Mark line as dirty (after a write)
void CacheLine::setDirty() {
    if (valid) {
        dirty = true;
    }
}

// Clear dirty bit (after write-back)
void CacheLine::clearDirty() {
    dirty = false;
}

// Get entire data block
const std::vector<uint8_t>& CacheLine::getData() const {
    return data;
}

// Get block size
size_t CacheLine::getBlockSize() const {
    return data.size();
}