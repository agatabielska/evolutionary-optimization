<div align="center">

# Evolutionary Optimization

**Heuristics and local search for a node-selection cycle problem**

<a href="https://isocpp.org/"><img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17"></a>
<a href="https://gcc.gnu.org/"><img src="https://img.shields.io/badge/GCC-C%2B%2B%20compiler-4A8B3B?logo=gnu&logoColor=white" alt="GCC"></a>
<a href="https://www.python.org/"><img src="https://img.shields.io/badge/Python-3-3776AB?logo=python&logoColor=white" alt="Python 3"></a>

</div>

## Problem

For each instance, select **100 of 200 planar nodes** and connect them into a
closed cycle. Minimize the sum of rounded Euclidean edge lengths and selected
node costs. The project includes instances **TSPA** and **TSPB**.

## Experiments

| Task | Methods |
|---|---|
| 1 | Random, nearest-neighbor and greedy-cycle construction |
| 2 | Regret and weighted-regret construction |
| 3 | Greedy/steepest local search with node or edge exchange |
| 4 | Steepest local search with candidate lists |
| 5 | Improving-move-list local search, with combined variants |
| 6 | Multi-start and iterated local search (MSLS/ILS) |
| 7 | Large-neighborhood search, heuristics and parameter sweeps |
| 8 | Global-convexity analysis of 1,000 greedy-search local optima |

Tasks run for both instances. The standard run executes **Tasks 1–8 in order**.
Task 8 compares common undirected edges and selected nodes, creates scatter
plots, and reports Pearson and Spearman correlations.

## Build and run

Requires a C++17 compiler. From the repository root:

```powershell
g++ -std=c++17 -O2 main.cpp -o evolutionary-optimization.exe
.\evolutionary-optimization.exe
```

Run one task only with `--task1` through `--task8`, for example:

```powershell
.\evolutionary-optimization.exe --task4
.\evolutionary-optimization.exe --task8
```

Task 8 uses the `ILS both: W=30 b=2 m=10` reference saved by Task 6. Run Task 6
first when invoking `--task8` on its own if no Task 6 results are present.
Task 7 also supports `--task7-heuristic` and `--task7-sweep`. Task 8 accepts
`--global-convexity-n`, `--global-convexity-seed`, and
`--global-convexity-output` to customize its run count, seed, and output path.

## Results and layout

Generated reports and plots are written under `calculated_solutions/task<N>/`.
Task 8 writes its local-optima JSON, six SVG charts per instance, and combined
correlation CSV to `calculated_solutions/task8/`.

```text
data/                     TSPA and TSPB instance data (x;y;cost)
solutions/                Task 1–8 implementations
utils/                    Instance reader, objective/result utilities, SVG plots
scripts/                  Supporting Python utilities
calculated_solutions/     Per-task reports, solutions and charts
main.cpp                  Program entry point and task dispatcher
```
