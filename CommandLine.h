#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <string>

// Structure to hold simulation configuration parameters
struct SimulationConfig {
    std::string appName;  // Application name for trace files
    int setBits;          // Number of set index bits (s)
    int associativity;    // Associativity (E)
    int blockBits;        // Number of block bits (b)
    std::string outputFile; // Output file for logging
    bool helpRequested;   // Whether help was requested
    
    // Constructor with default values
    SimulationConfig() 
        : appName(""), setBits(0), associativity(0), blockBits(0), 
          outputFile(""), helpRequested(false) {}
};

class CommandLine {
public:
    // Parse command line arguments and return configuration
    static SimulationConfig parseArguments(int argc, char* argv[]);
    
    // Print help message
    static void printHelp(const char* programName);
    
    // Validate configuration
    static bool validateConfig(const SimulationConfig& config);
};

#endif // COMMAND_LINE_H