#!/bin/bash

# Create the file with 100 lines, each with 11 values
for i in {1..100}; do
    # Generate the first value (random number between 0 and 3)
    first_value=$(shuf -i 0-3 -n 1)

    # Generate 10 random numbers less than 1 million
    random_values=""
    for j in {1..10}; do
        rand=$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')
        # Limit the value to below 1,000,000 using modulo
        rand=$((rand % 1000000))
        random_values+="$rand "
    done

    # Print the line: first value + 10 random values < 1,000,000
    echo "$first_value $random_values"
done > input.txt
