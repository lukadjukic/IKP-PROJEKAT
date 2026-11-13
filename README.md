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
build/ — Generated build artifacts and executables.  
profiling/ — Profiling results and screenshots.  
src/ — C source and header files.  
tests/ — Tests.  

## Requirements:
- GCC,
- GNU Make,
- POSIX-compliant operating system
- Clang Tidy    (optional)
- Clang Format  (optional)
- Valgrind      (optional)

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
make tidy           # To run the static analyzer
make format         # To format all the source code in /src
make format-check   # To run the format checker, without changing the files in src/

make debug          # To compile  all   executables in debug mode
make debug-server   # To compile server executable  in debug mode
make debug-admin    # To compile admin  executable  in debug mode
make debug-client   # To compile client executable  in debug mode
make grind          # To compile server executable  in debug mode, and run it with valgrind
make sanitize       # To compile all executables with sanitizer enabled
```
## Usage

Start the server first, then run the client or administrator program as needed.
