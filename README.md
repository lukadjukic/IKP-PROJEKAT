IKP Projekat

A C project implementing a client-server system with separate client, server, and administrator programs.
The emphasys is on the load balancer aspect on the server side as well as using networking functionality and concurency.

Components
Server — Handles client requests and manages the system.
Client — Connects to the server and sends requests.
Admin — Provides administrative functionality.
Project Structure
src/ — C source and header files.
build/ — Generated build artifacts.
build/artefacts/ — Compiled object files.
profiling/ — Profiling results and screenshots.
tests/ — Tests.
Build

From the repository root directory:

bash
make


Build a specific executable:

bash
make server
make admin
make client


Remove generated files:

bash
make clean

Usage

Start the server first, then run the client or admin program as needed.

Requirements
GCC
GNU Make
Posix compliant OS