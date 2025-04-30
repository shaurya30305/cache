This program simulates a 4-core processor system with private L1 caches, supporting the MESI protocol for cache coherence. Each core processes its own memory trace file and interacts through a shared bus and main memory.

Compilation Instructions:
    To compile the simulator, use a C++ compiler that supports C++11 or later. For example, using g++:
        g++ -std=c++11 -o L1simulate L1simulate.cpp
    This command creates an executable named L1simulate.

Input Requirements
    The simulator requires 4 memory trace files for the application, named as:
        app4_proc0.trace
        app4_proc1.trace
        app4_proc2.trace
        app4_proc3.trace
    Each trace file should contain lines with either R <address> or W <address> in hexadecimal format (e.g., R 0x0040).

How to Run
    Use the following command format:
        ./L1simulate -t <trace_prefix> -s <s> -E <E> -b <b> -o <output_file>
    Where:
        -t <trace_prefix> specifies the prefix for the 4 trace files
        -s <s> sets the number of set index bits (cache sets = 2^s)
        -E <E> sets the associativity (lines per set)
        -b <b> sets the block offset bits (block size = 2^b bytes)
        -o <output_file> writes the simulation results to the specified file
    Example:
        ./L1simulate -t app4 -s 7 -E 2 -b 5 -o results.txt
    This simulates the app4 traces with:
        128 sets (2^7)
        2-way set associative cache
        32-byte blocks (2^5)
        Results output to results.txt



