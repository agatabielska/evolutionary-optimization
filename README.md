# evolutionary-optimization


## The problem

We are given 200 nodes on a plane, each with (x, y) coordinates and a node cost.
Distances are Euclidean, rounded to the nearest integer.

The goal is to select exactly **100 of the 200 nodes** and connect them into a
**closed cycle** so that the objective is minimal:

    objective = (sum of edge lengths in the cycle) + (sum of costs of the selected nodes)

Two instances are used: `TSPA` and `TSPB`.

## What is implemented (Assignment 1: greedy heuristics)

| Method | Description |
| --- | --- |
| Random | Pick 100 random nodes in random order. 200 runs. |
| Nearest neighbor, end of path | Start from a node, repeatedly append the node with the smallest (distance + node cost) to the end of the path. |
| Nearest neighbor, any position | Same idea, but a new node may be added at the beginning, at the end, or inside the path. The best change of the objective is chosen. |
| Greedy cycle | Start from a node, add its best neighbour to form a 2-node cycle, then keep inserting the node + position with the smallest increase of the objective. |

Every greedy method is run once from each of the 200 starting nodes,
so you get 200 solutions per method and instance. Random search also makes 200 solutions.

## Project layout

    .
    ├── main.cpp                  entry point, runs all methods on both instances
    ├── solutions/
    │   └── task1.h               the four algorithms
    ├── utils/
    │   ├── reader.h              CSV reader, distance matrix, node costs
    │   ├── euclidean.h           rounded Euclidean distance
    │   ├── result_manager.h      objective evaluation, best/worst/avg, saving results
    │   └── plotter.h             creates 2D SVG plots after all methods finish
    ├── data/
    │   ├── TSPA.csv              instance A
    │   └── TSPB.csv              instance B
    └── calculated_solutions/     output files end up here

Each data file has 200 lines in the form `x;y;cost`.

## Requirements

* A C++17 compiler (g++ is used below)
* The program must be started from the **repository root**, because the
  paths `./data/...` and `calculated_solutions/...` are relative.

## Running on Windows

### 1. Install g++

The easiest way is MSYS2 with the MinGW-w64 toolchain.

1. Install MSYS2 from https://www.msys2.org/
2. Open the *MSYS2 UCRT64* terminal and run:

       pacman -S mingw-w64-ucrt-x86_64-gcc

3. Add `C:\msys64\ucrt64\bin` to your `PATH`.
4. Open a new PowerShell and check:

       g++ --version

### 2. Build

In PowerShell, from the repository root:

    g++ -std=c++17 -O2 main.cpp -o evolutionary-optimization.exe

Adding `-O2` is recommended, the nearest neighbor (any position) and greedy
cycle methods are noticeably faster with optimizations on.

### 3. Run

    .\evolutionary-optimization.exe

Expected console output:

    --- Dataset TSPA ---

    --- Dataset TSPB ---

The program prints only the dataset headers.

After all result files have been saved, the program automatically creates
`calculated_solutions/TSPA_best_solutions.svg` and
`calculated_solutions/TSPB_best_solutions.svg`. Each image contains one
panel per method: all nodes are colored by node cost, while the selected
100-node cycle is drawn on top. Plot generation happens after the result
timers are stopped, so it does not contribute to the reported method times.

## Running on Linux

### 1. Install g++

Debian / Ubuntu:

    sudo apt update
    sudo apt install build-essential

### 2. Build

From the repository root:

    mkdir -p calculated_solutions
    g++ -std=c++17 -O2 main.cpp $(find utils solutions -name '*.cpp') -o evolutionary-optimization

(If there are no `.cpp` files in `utils` and `solutions`, the `find` part
expands to nothing and the build still works.)

### 3. Run

    ./evolutionary-optimization


## Troubleshooting

**`Error opening file: ./data/TSPA.csv`**
You are not in the repository root, or the `data` folder is missing.
`cd` into the project folder and run the executable from there.

**`Error opening file for writing: calculated_solutions/...`**
The `calculated_solutions` folder does not exist. Create it in project root (New-Item -ItemType Directory -Force calculated_solutions for Windows) and run again.

**`g++` is not recognized (Windows)**
The MinGW `bin` folder is not on your `PATH`. Add it and restart PowerShell.

**Linker error about multiple definitions**
Functions in the headers are not marked `inline`. This only shows up if a
`.cpp` file in `utils/` or `solutions/` includes them. Either mark them
`inline` or build with just `main.cpp`.
