# IKP Projekat  
![Language](https://img.shields.io/badge/language-C-blue)
![Standard](https://img.shields.io/badge/C-C11-blue)
![License](https://img.shields.io/badge/license-MIT-green)
  
A C project implementing a client-server system consisting of separate client, server, and administrator programs.  

The main focus is on server-side load balancing, network communication, and concurrency.  

## Components  
Server — Handles client requests and manages the system.  
Client — Connects to the server and sends requests.  
Admin — Provides administrative functionality.  

## Project Structure  
src/ — C source and header files.  
build/ — Generated build artifacts and executables.  
build/artefacts/ — Compiled object files.  
profiling/ — Profiling results and screenshots.  
tests/ — Tests.  

## Requirements:
- GCC,
- GNU Make,
- POSIX-compliant operating system
- Clang Tidy    (optional)
- Clang Format  (optional)

## Building

Building is done from the repository root directory.

To build a specific executable, run:

```bash
make server
make admin
make client
```
To be all three, run:
```bash
make all
```

To remove generated files, run:

```bash
make clean
```

There are several different build targets:
```bash
make test           # To run tests
make debug          # To compile  all   executables in debug mode
make debug-server   # To compile server executable  in debug mode
make debug-admin    # To compile admin  executable  in debug mode
make debug-client   # To compile client executable  in debug mode
make tidy           # To run the static analyzer
make format         # To format all the source code in /src
make format-check   # To run the format checker, without changing the files in src/
```
## Usage

Start the server first, then run the client or administrator program as needed.
