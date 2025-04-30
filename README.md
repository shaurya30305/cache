COMPILATION INSTRUCTIONS

1.) pull all .cpp and .h files into a single directory, as well as the make file
2.) run $make, which will create L1simulate
3.) run the out file in the following format ./L1simulate
  -t <n>    : Name of parallel application (e.g. app1) whose 4 traces are to be used in simulation
  -s <bits> : Number of set index bits (S = 2^s)
  -E <ways> : Associativity (number of lines per set)
  -b <bits> : Number of block bits (B = 2^b)
  -o <file> : Logs output in file for plotting etc.
  -h        : Prints this help
