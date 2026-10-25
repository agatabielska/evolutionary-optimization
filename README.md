# evolutionary-optimization


## The problem

We are given 200 nodes on a plane, each with (x, y) coordinates and a node cost.
Distances are Euclidean, rounded to the nearest integer.

The goal is to select exactly **100 of the 200 nodes** and connect them into a
**closed cycle** so that the objective is minimal:

    objective = (sum of edge lengths in the cycle) + (sum of costs of the selected nodes)

Two instances are used: `TSPA` and `TSPB`.

## What is implemented

### Assignment 1: greedy heuristics

| Method | Description |
| --- | --- |
| Random | Pick 100 random nodes in random order. 200 runs. |
| Nearest neighbor, end of path | Start from a node, repeatedly append the node with the smallest (distance + node cost) to the end of the path. |
| Nearest neighbor, any position | Same idea, but a new node may be added at the beginning, at the end, or inside the path. The best change of the objective is chosen. |
| Greedy cycle | Start from a node, add its best neighbour to form a 2-node cycle, then keep inserting the node + position with the smallest increase of the objective. |

### Assignment 2: greedy regret heuristics

Two construction frameworks (the extended nearest neighbor, i.e. a path where
a node may be inserted at the beginning, at the end or inside, and the greedy
cycle) are combined with two selection criteria, which gives four methods.

| Method | Framework | Selection criterion |
| --- | --- | --- |
| Nearest neighbor (any position) + 2-regret | path | `c2 - c1` |
| Nearest neighbor (any position) + weighted sum | path | `w1 * (c2 - c1) - w2 * c1` |
| Greedy cycle + 2-regret | cycle | `c2 - c1` |
| Greedy cycle + weighted sum | cycle | `w1 * (c2 - c1) - w2 * c1` |

How the regret methods work:

1. In every iteration, for **each unvisited node** the increase of the objective
   (edge length change + node cost) is computed for **every possible insertion
   position** of the current partial solution.
2. For each node, `c1` is the best (smallest) insertion cost, `c2` is the
   second best one, and `pos1` is the position where `c1` occurs.
3. Each node gets a score. The node with the **largest score** is chosen and
   inserted at **its own best position** `pos1`.
4. Ties are broken by the smaller `c1`, then by the smaller node index.

The criteria:

* **2-regret:** `score = c2 - c1`. A node that would lose a lot if it did not
  get its best position now is inserted first.
* **Weighted sum:** `score = w1 * (c2 - c1) - w2 * c1`. It combines the regret
  with the greedy criterion (the best change of the objective, which should
  be small, hence the minus). By default `w1 = w2 = 0.5`. Other values can be
  passed as arguments of the functions, e.g.
  `task2_greedy_cycle_weighted(result_manager, 0.25, 0.75)`.

### Assignment 3: local search

Local search uses the objective-function delta for every move; it never builds
a neighbor and reevaluates the complete objective while exploring a
neighborhood. The implemented variants combine:

* steepest descent or greedy descent;
* intra-route exchange of two nodes or two edges (2-opt);
* random or weighted nearest-neighbor starting solutions.

The inter-route neighborhood always exchanges one selected node with one
unselected node. The steepest version evaluates the complete combined
neighborhood and applies its best improving move. The greedy version creates
the complete list of move descriptors, shuffles that list with a
`std::mt19937` generator seeded from `std::random_device`, and evaluates moves
in that order until the first improving move is found. Thus inter- and
intra-route moves are mixed, and the greedy version does not evaluate the
whole neighborhood before choosing a move.

Each of the eight combinations is run 200 times. Random starts are generated
by shuffling all 200 nodes and keeping the first 100. Greedy starts use the
weighted nearest-neighbor construction from Assignment 2, with starting nodes
0 through 199. The objective and running-time tables are written to
`calculated_solutions/local_search_report_dataset_0.txt` and
`calculated_solutions/local_search_report_dataset_1.txt`. Individual local
search results and best-solution plots are written to the same directory.

Sanity checks for the weighted methods: with `w1 = 0` the criterion reduces to
pure greedy (it should reproduce the corresponding Assignment 1 results), with
`w2 = 0` it reduces to pure 2-regret.

Every method (in both assignments) is run once from each of the 200 starting
nodes, so you get 200 solutions per method and instance. Random search also
makes 200 solutions.

## Project layout

    .
    ├── main.cpp                  entry point, runs all methods on both instances
    ├── solutions/
    │   ├── task1.h               Assignment 1: random, nearest neighbor (end of path / any position), greedy cycle
    │   └── task2.h               Assignment 2: 2-regret and weighted-sum variants of nearest neighbor (any position) and greedy cycle
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

## Output

For every method and instance a text file is written to `calculated_solutions/`.
The file name contains the method and the dataset index (`dataset_0` = TSPA,
`dataset_1` = TSPB), e.g. `greedy_cycle_regret_dataset_0.txt` or
`nn_anywhere_weighted_dataset_1.txt`. Each file has the following content:

    Best Cost: ...
    Worst Cost: ...
    Average Cost: ...
    Time: ... ms
    Best Solution: n1,n2,...,n100

`Best Solution` is the list of the 100 selected nodes in the order of the cycle
(the last node is connected back to the first one).

In addition, an SVG summary is generated for each dataset after all methods are
processed. The task-1 plot is saved as `TSPA_best_solutions.svg` or
`TSPB_best_solutions.svg`, and the task-2 plot is saved as
`TSPA_task2_best_solutions.svg` or `TSPB_task2_best_solutions.svg`.
Each image contains one panel per method; the chosen 100-node cycle is drawn on
top of the nodes, which are colored by node cost.

## Requirements

* A C++17-compatible compiler (g++ is used below)
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

Adding `-O2` is recommended, the nearest neighbor (any position), greedy
cycle and regret methods are noticeably faster with optimizations on.

### 3. Run

    .\evolutionary-optimization.exe

Expected console output:

    --- Dataset TSPA ---
    Saved plot: calculated_solutions/TSPA_best_solutions.svg
    Saved plot: calculated_solutions/TSPA_task2_best_solutions.svg

    --- Dataset TSPB ---
    Saved plot: calculated_solutions/TSPB_best_solutions.svg
    Saved plot: calculated_solutions/TSPB_task2_best_solutions.svg

The program prints the dataset headers and the two generated SVG file names for
each dataset.

Plot generation happens after the result timers are stopped, so it does not
contribute to the reported method times.

## Running on Linux

### 1. Install g++

Debian / Ubuntu:

    sudo apt update
    sudo apt install build-essential

### 2. Build

From the repository root:

    mkdir -p calculated_solutions
    g++ -std=c++17 -O2 main.cpp -o evolutionary-optimization

This project does not require any `.cpp` files from `utils` or `solutions`; all
logic is included directly from the headers.

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

**Structured binding / `auto [x, y]` errors**
The installed compiler is too old for C++17. This project uses structured
bindings in `task1.h`, so install a newer GCC (for example from MSYS2's
`mingw-w64-ucrt-x86_64-gcc` toolchain) and rebuild with `-std=c++17`.

**Linker error about multiple definitions**
Functions in the headers are not marked `inline`. This only shows up if a
`.cpp` file in `utils/` or `solutions/` includes them. Either mark them
`inline` or build with just `main.cpp`.