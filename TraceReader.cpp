#include "TraceReader.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

// Constructor
TraceReader::TraceReader(const std::string& appName, int numCores)
    : applicationName(appName), numCores(numCores) {
    
    // Initialize vectors to the correct size
    traceFiles.resize(numCores);
    fileEnded.resize(numCores, false);
}

// Destructor
TraceReader::~TraceReader() {
    // Close all open files
    for (auto& file : traceFiles) {
        if (file.is_open()) {
            file.close();
        }
    }
}

// Open trace files
bool TraceReader::openTraceFiles() {
    bool allFilesOpened = true;
    
    for (int i = 0; i < numCores; i++) {
        // Construct filename based on application name and core ID
        std::string filename = applicationName + "_proc" + std::to_string(i) + ".trace";
        
        // Open the file
        traceFiles[i].open(filename);
        
        // Check if file opened successfully
        if (!traceFiles[i].is_open()) {
            std::cerr << "Error: Could not open trace file: " << filename << std::endl;
            allFilesOpened = false;
        }
        
        // Initialize EOF status
        fileEnded[i] = false;
    }
    
    return allFilesOpened;
}

// Check if there are more instructions for a core
bool TraceReader::hasMoreInstructions(int coreId) const {
    if (coreId < 0 || coreId >= numCores) {
        std::cerr << "Error: Invalid core ID: " << coreId << std::endl;
        return false;
    }
    
    return !fileEnded[coreId];
}

// Get next instruction for a core
Instruction TraceReader::getNextInstruction(int coreId) {
    if (coreId < 0 || coreId >= numCores) {
        std::cerr << "Error: Invalid core ID: " << coreId << std::endl;
        return Instruction(); // Return invalid instruction
    }
    
    if (fileEnded[coreId]) {
        return Instruction(); // Return invalid instruction if at EOF
    }
    
    // Read a line from the file
    std::string line;
    if (std::getline(traceFiles[coreId], line)) {
        // Parse the line
        std::istringstream iss(line);
        char opType;
        std::string addressStr;
        
        // Extract operation type and address
        if (iss >> opType >> addressStr) {
            Instruction::Type type;
            
            // Convert operation type
            if (opType == 'R' || opType == 'r') {
                type = Instruction::Type::READ;
            } else if (opType == 'W' || opType == 'w') {
                type = Instruction::Type::WRITE;
            } else {
                std::cerr << "Error: Unknown operation type in trace: " << opType << std::endl;
                return Instruction(); // Return invalid instruction
            }
            
            // Convert address string to uint32_t
            uint32_t address = 0;
            
            // Remove "0x" prefix if present
            if (addressStr.substr(0, 2) == "0x") {
                addressStr = addressStr.substr(2);
            }
            
            // Convert hex string to uint32_t
            address = static_cast<uint32_t>(std::strtoul(addressStr.c_str(), nullptr, 16));
            
            // Return valid instruction
            return Instruction(type, address);
        }
    }
    
    // If we reached here, we're at EOF or line parsing failed
    fileEnded[coreId] = true;
    return Instruction(); // Return invalid instruction
}

// Check if all trace files are at EOF
bool TraceReader::allTracesCompleted() const {
    for (bool ended : fileEnded) {
        if (!ended) {
            return false; // At least one trace still has instructions
        }
    }
    return true; // All traces are complete
}

// Reset all trace files to beginning
void TraceReader::resetTraces() {
    for (int i = 0; i < numCores; i++) {
        if (traceFiles[i].is_open()) {
            traceFiles[i].clear();  // Clear EOF and error flags
            traceFiles[i].seekg(0); // Go to beginning of file
            fileEnded[i] = false;   // Reset EOF status
        }
    }
}