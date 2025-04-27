#include "CommandLine.h"
#include "Simulator.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Parse command line arguments
    SimulationConfig config = CommandLine::parseArguments(argc, argv);
    
    // If help was requested or invalid arguments, print help and exit
    if (config.helpRequested || !CommandLine::validateConfig(config)) {
        CommandLine::printHelp(argv[0]);
        return config.helpRequested ? 0 : 1;  // Exit with error code if invalid args
    }
    
    // Create simulator with the configuration
    Simulator simulator(config);
    
    // Initialize simulator
    if (!simulator.initialize()) {
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
    std::cout << "Starting simulation...\n";
    simulator.run();
    std::cout << "Simulation completed.\n";
    
    // Print results
    simulator.printResults();
    
    return 0;
}