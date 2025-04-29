#include "Processor.h"
#include <iostream>
#include "Address.h"

// Constructor
Processor::Processor(int id, TraceReader& reader, Cache& cache)
    : coreId(id), traceReader(reader), l1Cache(cache), 
      blocked(false), cyclesBlocked(0), instructionsExecuted(0) {
}

// Execute the next instruction if possible
bool Processor::executeNextInstruction() {
    // If processor is blocked, can't execute instructions
    if (blocked) {
        cyclesBlocked++;
        return false;
    }
    
    // Check if there are more instructions to execute
    if (!traceReader.hasMoreInstructions(coreId)) {
        return false;
    }
    
    // Get next instruction from trace
    Instruction inst = traceReader.getNextInstruction(coreId);
    
    // Validate instruction
    if (!inst.isValid()) {
        return false;
    }
    
    // Create Address object from the instruction's address
    // Cache configuration parameters taken from l1Cache
    Address addr(inst.address, l1Cache.getSetBits(), l1Cache.getBlockBits());
    
    // Process based on instruction type
    bool success = false;
    if (inst.type == Instruction::Type::READ) {
        // Attempt to read from cache
        success = l1Cache.read(addr);
    } else if (inst.type == Instruction::Type::WRITE) {
        // Attempt to write to cache
        success = l1Cache.write(addr);
    }
    
    // If operation was not immediately successful (cache miss), block the processor
    if (!success) {
        blocked = true;
    } else {
        // If operation succeeded, increment executed instruction count
        instructionsExecuted++;
    }
    
    return success;
}

// Check if this processor is blocked
bool Processor::isBlocked() const {
    return blocked;
}

// Set blocked state
void Processor::setBlocked(bool state) {
    std::cout << "Processor " << coreId << " setBlocked(" << state << ")" << std::endl;
    if (blocked && !state) {
        // If processor is being unblocked, count the instruction that caused blocking
        instructionsExecuted++;
    }
    blocked = state;
}

// Get core ID
int Processor::getCoreId() const {
    return coreId;
}

// Check if processor has more instructions
bool Processor::hasMoreInstructions() const {
    return traceReader.hasMoreInstructions(coreId);
}

// Get cycles blocked
unsigned int Processor::getCyclesBlocked() const {
    return cyclesBlocked;
}

// Get instructions executed
unsigned int Processor::getInstructionsExecuted() const {
    return instructionsExecuted;
}

// Reset statistics
void Processor::resetStats() {
    cyclesBlocked = 0;
    instructionsExecuted = 0;
    blocked = false;
}