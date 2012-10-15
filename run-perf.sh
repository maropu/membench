#!/bin/bash

# Compile
g++ cachebench.cpp -o cachebench || exit 1
g++ tlbbench.cpp -o tlbbench || exit 1

# Run benchmarks
./cachebench; echo -e "\n"; ./tlbbench

# Clean up
rm cachebench tlbbench
