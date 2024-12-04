#!/bin/bash

# Create the file with 100 lines, each with 11 values
for i in {1..100}; do
    # Generate the first value (random number between 0 and 4)
    first_value=$(shuf -i 0-3 -n 1)
    
    # Generate 10 random 64-bit numbers (from /dev/urandom)
    random_values=$(od -An -N80 -t u8 /dev/urandom | tr -s ' ' '\n' | head -n 11)
    
    # Combine the first value with the 10 random 64-bit numbers
    # Echo the first value, followed by the 10 random values on the same line
    echo -n "$first_value "  # First value
    echo "$random_values" | tr '\n' ' '  # 10 random 64-bit values in one line
    echo  # New line after each line of numbers
done > input.txt  # Redirect the result to output.txt
