# smart-city

Smart City Public Transport Simulation (IS2202 Problem C - C++)

## Overview
A C++ simulation modeling an urban public transport network using graph data structures and pathfinding algorithms.

## Project Structure
- `SmartCity/`
  - `main.cpp` — Builds the city topology, bus and train routes, and executes simulation and profiling
  - `CityGraph.h` — Location nodes, travel edges, and adjacency list representations
  - `Route.h` — Bus and train route schedules and traversal logic
  - `PathFinder.h` — Dijkstra's shortest path algorithm implementation
  - `Passenger.h` — Passenger demand generation and journey simulation
  - `Profiler.h` — Performance and efficiency metrics tracking
  
## Prerequisites
- C++17 compatible compiler (e.g., `g++` via MinGW/MSYS2 on Windows, Clang, or GCC on Linux/macOS)

## Build & Run

### Compile
```bash
cd SmartCity
g++ -std=c++17 main.cpp -o transport
```

### Run
**Windows:**
```powershell
.\transport.exe
```

**macOS / Linux:**
```bash
./transport
```

## Generated Outputs
When executed, the simulation generates:
- `passenger_log.txt` — Detailed event log of passenger transit
- Output CSV reports detailing travel times and network efficiency metrics
