#pragma once

#include "task3.h"
#include "task4.h"
#include "task5.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Task 6: multiple start local search (MSLS) and iterated local search (ILS).
//
// Basic local search used by both methods: the best version from Task 5, i.e. steepest local
// search (intra-route edge exchange + inter-route node exchange) with the list of improving
// moves LM and the full neighborhood ("Steepest LS + improving move list").
//
// MSLS: `iterations` runs of the basic local search from random solutions, the result is the
//       best local optimum found.
// ILS : random start -> local search -> repeat { perturbation of the current solution,
//       local search, accept if the new solution is not worse } until the time limit
//       (the average running time of MSLS) is reached.

struct Task6Stats {
    std::vector<int> ls_runs;     // number of runs of the basic local search, one value per run
    double time_limit_ms = 0.0;   // ILS only: stopping condition (average time of MSLS)
};

namespace task6_detail {

using local_search_detail::solution_size;
using local_search_detail::total_nodes;
using task5_detail::BestFirst;
using task5_detail::Kind;
using task5_detail::Move;
using task5_detail::Status;
using Clock = std::chrono::high_resolution_clock;

inline double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

// ---------------------------------------------------------------------------------------------
// Perturbation. Two independent components, each can be switched off:
//   1. `bridges` local double-bridge moves: every one draws three cut points inside a window of
//      `window` consecutive positions (random start in the cycle) and swaps the two segments
//      between them: A B C R -> A C B R;
//   2. `exchanges` random selected nodes are replaced by random non-selected nodes.
// bridges > 0, exchanges == 0 : only component 1
// bridges == 0, exchanges > 0 : only component 2
// bridges > 0, exchanges > 0  : both
// Defaults = the best settings of the parameter study: 2 bridges, window 30, 10 exchanges.
// ---------------------------------------------------------------------------------------------
struct PerturbConfig {
    int bridges = 2;
    int window = 30;
    int exchanges = 10;
};

inline std::string perturb_name(const PerturbConfig& c) {
    if (c.bridges > 0 && c.exchanges == 0) {
        return "ILS only 1 (double bridge): W=" + std::to_string(c.window) +
               " b=" + std::to_string(c.bridges);
    }
    if (c.bridges == 0) {
        return "ILS only 2 (node replacement): m=" + std::to_string(c.exchanges);
    }
    return "ILS both: W=" + std::to_string(c.window) + " b=" + std::to_string(c.bridges) +
           " m=" + std::to_string(c.exchanges);
}

inline void perturb(std::vector<int>& solution, std::mt19937& generator,
                    const PerturbConfig& config = PerturbConfig()) {
    std::uniform_int_distribution<int> any_position(0, solution_size - 1);

    // --- 1. local double bridges ---
    const int window = std::max(3, std::min(config.window, solution_size - 1));
    std::uniform_int_distribution<int> cut_distribution(1, window);
    for (int b = 0; b < config.bridges; ++b) {
        std::rotate(solution.begin(), solution.begin() + any_position(generator),
                    solution.end());
        int cuts[3];
        do {
            for (int& c : cuts) c = cut_distribution(generator);
            std::sort(cuts, cuts + 3);
        } while (cuts[0] == cuts[1] || cuts[1] == cuts[2]);
        // segments: [0,c0) [c0,c1) [c1,c2) [c2,k)  ->  swap the two middle ones
        std::rotate(solution.begin() + cuts[0], solution.begin() + cuts[1],
                    solution.begin() + cuts[2]);
    }

    // --- 2. random replacements of selected nodes by non-selected ones ---
    if (config.exchanges > 0) {
        std::array<bool, total_nodes> selected{};
        for (int node : solution) selected[node] = true;
        std::vector<int> outside;
        outside.reserve(total_nodes - solution_size);
        for (int node = 0; node < total_nodes; ++node) {
            if (!selected[node]) outside.push_back(node);
        }
        for (int e = 0; e < config.exchanges && !outside.empty(); ++e) {
            const int position = any_position(generator);
            const int index = std::uniform_int_distribution<int>(
                0, static_cast<int>(outside.size()) - 1)(generator);
            std::swap(solution[position], outside[index]);  // old node becomes "outside"
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Steepest local search with the list of improving moves LM (full neighborhood). Same algorithm
// as task5_detail::Search, but one object can be run many times on different solutions, so the
// memory of LM is reused and no statistics are collected (the time is what matters here).
// ---------------------------------------------------------------------------------------------
class ListLocalSearch {
public:
    explicit ListLocalSearch(int dataset) : dataset_(dataset) {
        lm_.reserve(20000);
        deferred_.reserve(20000);
    }

    // improves `solution` in place until it is a local optimum
    void run(std::vector<int>& solution) {
        sol_ = solution;
        pos_.fill(-1);
        for (int i = 0; i < solution_size; ++i) pos_[sol_[i]] = i;

        evaluate_all();
        bool fresh = true;
        while (true) {
            if (apply_best_applicable()) {
                fresh = false;
                continue;
            }
            if (fresh) break;  // LM is up to date and has no applicable move: local optimum
            evaluate_all();
            fresh = true;
        }
        solution = sol_;
    }

private:
    int dataset_;
    std::vector<int> sol_;
    std::array<int, total_nodes> pos_{};
    std::vector<Move> lm_;
    std::vector<Move> deferred_;

    int D(int a, int b) const { return DISTANCE_MATRIX[a][b][dataset_]; }
    bool selected(int node) const { return pos_[node] >= 0; }
    int succ(int node) const {
        const int p = pos_[node] + 1;
        return sol_[p == solution_size ? 0 : p];
    }
    int pred(int node) const {
        const int p = pos_[node];
        return sol_[p == 0 ? solution_size - 1 : p - 1];
    }
    bool has_edge(int from, int to) const {
        return selected(from) && selected(to) && succ(from) == to;
    }

    void offer_edge(int a, int a2, int b, int b2) {
        const int delta = D(a, b) + D(a2, b2) - D(a, a2) - D(b, b2);
        if (delta < 0) lm_.push_back(Move{Kind::edge_exchange, a, a2, b, b2, delta});
    }

    void offer_exchange(int prev, int node, int next, int outside) {
        const int delta = NODE_COST[outside][dataset_] - NODE_COST[node][dataset_] +
                          D(prev, outside) + D(outside, next) - D(prev, node) - D(node, next);
        if (delta < 0) {
            lm_.push_back(Move{Kind::inter_route_exchange, prev, node, next, outside, delta});
        }
    }

    void evaluate_all() {
        lm_.clear();
        for (int i = 0; i < solution_size; ++i) {
            const int u = sol_[i];
            const int u2 = sol_[(i + 1) % solution_size];
            for (int j = i + 2; j < solution_size; ++j) {
                if (i == 0 && j == solution_size - 1) continue;
                const int v = sol_[j];
                const int v2 = sol_[(j + 1) % solution_size];
                offer_edge(u, u2, v, v2);  // applicable now
                offer_edge(u, u2, v2, v);  // relatively inverted edges
            }
        }
        for (int i = 0; i < solution_size; ++i) {
            const int prev = sol_[(i + solution_size - 1) % solution_size];
            const int node = sol_[i];
            const int next = sol_[(i + 1) % solution_size];
            for (int o = 0; o < total_nodes; ++o) {
                if (!selected(o)) offer_exchange(prev, node, next, o);
            }
        }
        std::make_heap(lm_.begin(), lm_.end(), BestFirst());
    }

    Status check(const Move& m) const {
        if (m.kind == Kind::inter_route_exchange) {
            if (!selected(m.n2) || selected(m.n4)) return Status::remove;
            const int p = pred(m.n2), n = succ(m.n2);
            if ((p == m.n1 && n == m.n3) || (p == m.n3 && n == m.n1)) return Status::ready;
            return Status::remove;
        }
        const bool f1 = has_edge(m.n1, m.n2), r1 = has_edge(m.n2, m.n1);
        const bool f2 = has_edge(m.n3, m.n4), r2 = has_edge(m.n4, m.n3);
        if (!(f1 || r1) || !(f2 || r2)) return Status::remove;
        return (f1 == f2) ? Status::ready : Status::deferred;
    }

    bool apply_best_applicable() {
        deferred_.clear();
        bool applied = false;
        while (!lm_.empty()) {
            std::pop_heap(lm_.begin(), lm_.end(), BestFirst());
            const Move m = lm_.back();
            lm_.pop_back();
            const Status status = check(m);
            if (status == Status::remove) continue;
            if (status == Status::deferred) {
                deferred_.push_back(m);
                continue;
            }
            apply(m);
            applied = true;
            break;
        }
        for (const Move& m : deferred_) {
            lm_.push_back(m);
            std::push_heap(lm_.begin(), lm_.end(), BestFirst());
        }
        return applied;
    }

    void apply(const Move& m) {
        if (m.kind == Kind::inter_route_exchange) {
            const int p = pos_[m.n2];
            sol_[p] = m.n4;
            pos_[m.n4] = p;
            pos_[m.n2] = -1;
            return;
        }
        if (has_edge(m.n1, m.n2)) reverse_path(m.n2, m.n3);
        else reverse_path(m.n1, m.n4);
    }

    void reverse_path(int s, int t) {
        int i = pos_[s], j = pos_[t];
        int len = (j - i + solution_size) % solution_size + 1;
        if (2 * len > solution_size) {
            const int ni = (j + 1) % solution_size;
            const int nj = (i + solution_size - 1) % solution_size;
            i = ni;
            j = nj;
            len = solution_size - len;
        }
        for (int k = 0; k < len / 2; ++k) {
            const int a = sol_[i], b = sol_[j];
            sol_[i] = b;
            pos_[b] = i;
            sol_[j] = a;
            pos_[a] = j;
            if (++i == solution_size) i = 0;
            if (--j < 0) j = solution_size - 1;
        }
    }
};

}  // namespace task6_detail

// MSLS: `runs` independent runs of MSLS, each = best of `iterations` local searches from random
// solutions. Run times (one per MSLS run) are recorded in the result manager.
inline Task6Stats task6_msls(ResultManager& result_manager, int runs, int iterations,
                             unsigned seed) {
    using namespace task6_detail;
    const int dataset = result_manager.dataset;
    std::mt19937 generator(seed);
    ListLocalSearch search(dataset);
    Task6Stats stats;

    for (int r = 0; r < runs; ++r) {
        const auto begin = Clock::now();
        std::vector<int> best;
        int best_cost = std::numeric_limits<int>::max();
        for (int it = 0; it < iterations; ++it) {
            std::vector<int> solution = local_search_detail::random_solution(generator);
            search.run(solution);
            const int cost = task4_detail::objective(solution, dataset);
            if (cost < best_cost) {
                best_cost = cost;
                best = solution;
            }
        }
        const auto end = Clock::now();
        result_manager.check_solution(best.data(), solution_size);
        result_manager.record_run_time(elapsed_ms(begin, end));
        stats.ls_runs.push_back(iterations);
    }
    return stats;
}

// ILS: `runs` independent runs, each started from a random solution and stopped after
// `time_limit_ms` milliseconds (checked after every local search).
inline Task6Stats task6_ils(ResultManager& result_manager, int runs, double time_limit_ms,
                            unsigned seed, const task6_detail::PerturbConfig& config = {}) {
    using namespace task6_detail;
    const int dataset = result_manager.dataset;
    std::mt19937 generator(seed);
    ListLocalSearch search(dataset);
    Task6Stats stats;
    stats.time_limit_ms = time_limit_ms;

    for (int r = 0; r < runs; ++r) {
        const auto begin = Clock::now();
        std::vector<int> current = local_search_detail::random_solution(generator);
        search.run(current);
        int current_cost = task4_detail::objective(current, dataset);
        int ls_runs = 1;

        std::vector<int> candidate;
        while (elapsed_ms(begin, Clock::now()) < time_limit_ms) {
            candidate = current;
            perturb(candidate, generator, config);
            search.run(candidate);
            ++ls_runs;
            const int cost = task4_detail::objective(candidate, dataset);
            if (cost <= current_cost) {  // accept equal or better; current is also the best
                current_cost = cost;
                current.swap(candidate);
            }
        }
        const auto end = Clock::now();
        result_manager.check_solution(current.data(), solution_size);
        result_manager.record_run_time(elapsed_ms(begin, end));
        stats.ls_runs.push_back(ls_runs);
    }
    return stats;
}

// Table with the number of runs of the basic local search (average (min - max)).
inline void write_task6_stats(const std::string& filename,
                              const std::vector<std::pair<std::string, Task6Stats>>& methods) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "Number of runs of the basic local search per run of the method "
            "(average (minimum - maximum))\n";
    file << std::left << std::setw(50) << "Method" << std::right << std::setw(30)
         << "LS runs: av (min - max)" << std::setw(20) << "Time limit [ms]" << '\n';
    for (const auto& [name, stats] : methods) {
        file << std::left << std::setw(50) << name << std::right;
        if (stats.ls_runs.empty()) {
            file << std::setw(30) << "-" << std::setw(20) << "-" << '\n';
            continue;
        }
        const double average =
            std::accumulate(stats.ls_runs.begin(), stats.ls_runs.end(), 0.0) / stats.ls_runs.size();
        const auto [min_it, max_it] = std::minmax_element(stats.ls_runs.begin(), stats.ls_runs.end());
        std::ostringstream cell;
        cell << std::fixed << std::setprecision(1) << average << " (" << *min_it << " - "
             << *max_it << ")";
        file << std::setw(30) << cell.str();
        if (stats.time_limit_ms > 0.0) {
            file << std::setw(20) << std::fixed << std::setprecision(3) << stats.time_limit_ms;
        } else {
            file << std::setw(20) << "-";
        }
        file << '\n';
    }
}