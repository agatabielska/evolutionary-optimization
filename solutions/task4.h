#pragma once
#include "task3.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

struct Task4Stats {
    long long evaluated_moves = 0;         // number of computed deltas, summed over all runs
    long long applied_moves = 0;           // number of applied moves, summed over all runs
    int runs = 0;
    double candidate_precompute_ms = 0.0;  // one-off time for building candidate lists
};

namespace task4_detail {

using local_search_detail::solution_size;
using local_search_detail::total_nodes;

enum class Kind { inter_route_exchange, edge_exchange };

struct CandidateMove {
    Kind kind;
    int first;   // inter_route_exchange: position in the cycle
                 // edge_exchange:        position of the first removed edge
    int second;  // inter_route_exchange: id of the outside node
                 // edge_exchange:        position of the second removed edge (first < second)
};

using Clock = std::chrono::high_resolution_clock;

inline double elapsed_ms(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

// Objective computed from scratch (used only for debugging checks).
inline int objective(const std::vector<int>& solution, int dataset) {
    int value = 0;
    for (int i = 0; i < solution_size; ++i) {
        value += DISTANCE_MATRIX[solution[i]][solution[(i + 1) % solution_size]][dataset];
        value += NODE_COST[solution[i]][dataset];
    }
    return value;
}

// Edge exchange (first, second) removes edges (first, first+1) and (second, second+1).
inline bool valid_edge_pair(int first, int second) {
    return second > first + 1 && !(first == 0 && second == solution_size - 1);
}

inline int evaluate(const std::vector<int>& solution, const CandidateMove& move, int dataset) {
    if (move.kind == Kind::inter_route_exchange) {
        return local_search_detail::inter_route_exchange_delta(solution, move.first, move.second, dataset);
    }
    return local_search_detail::edge_exchange_delta(solution, move.first, move.second, dataset);
}

// Applies a move and keeps position[] (node -> position in the cycle, -1 if not selected) valid.
inline void apply_candidate_move(std::vector<int>& solution, std::vector<int>& position,
                                 const CandidateMove& move) {
    if (move.kind == Kind::inter_route_exchange) {
        const int old_node = solution[move.first];
        solution[move.first] = move.second;
        position[move.second] = move.first;
        position[old_node] = -1;
    } else {
        std::reverse(solution.begin() + move.first + 1, solution.begin() + move.second + 1);
        for (int p = move.first + 1; p <= move.second; ++p) {
            position[solution[p]] = p;
        }
    }
}

// For every node the `candidate_count` best other nodes u by d(v,u) + cost(u).
inline std::vector<std::vector<int>> build_candidate_lists(int dataset, int candidate_count) {
    candidate_count = std::min(candidate_count, total_nodes - 1);
    std::vector<std::vector<int>> lists(total_nodes);
    for (int v = 0; v < total_nodes; ++v) {
        std::vector<std::pair<int, int>> keyed;  // (d(v,u) + cost(u), u)
        keyed.reserve(total_nodes - 1);
        for (int u = 0; u < total_nodes; ++u) {
            if (u != v) {
                keyed.emplace_back(DISTANCE_MATRIX[v][u][dataset] + NODE_COST[u][dataset], u);
            }
        }
        std::partial_sort(keyed.begin(), keyed.begin() + candidate_count, keyed.end());
        for (int i = 0; i < candidate_count; ++i) {
            lists[v].push_back(keyed[i].second);
        }
    }
    return lists;
}

// Calls visit(move) for every candidate move: v is a selected node, u is in cand(v) and the move introduces edge (v,u).
template <class Visitor>
inline void for_each_candidate_move(const std::vector<int>& solution,
                                    const std::vector<int>& position,
                                    const std::vector<std::vector<int>>& candidates,
                                    Visitor&& visit) {
    for (int i = 0; i < solution_size; ++i) {
        const int v = solution[i];
        for (int u : candidates[v]) {
            const int j = position[u];
            if (j < 0) {
                visit(CandidateMove{Kind::inter_route_exchange, (i + 1) % solution_size, u});
                visit(CandidateMove{Kind::inter_route_exchange,
                                    (i + solution_size - 1) % solution_size, u});
            } else {
                int first = std::min(i, j);
                int second = std::max(i, j);
                if (valid_edge_pair(first, second)) {
                    visit(CandidateMove{Kind::edge_exchange, first, second});
                }
                const int pred_i = (i + solution_size - 1) % solution_size;
                const int pred_j = (j + solution_size - 1) % solution_size;
                first = std::min(pred_i, pred_j);
                second = std::max(pred_i, pred_j);
                if (valid_edge_pair(first, second)) {
                    visit(CandidateMove{Kind::edge_exchange, first, second});
                }
            }
        }
    }
}

// Steepest local search restricted to candidate moves.
inline void improve_with_candidates(std::vector<int>& solution, std::vector<int>& position,
                                    const std::vector<std::vector<int>>& candidates,
                                    int dataset, Task4Stats& stats) {
    while (true) {
        int best_delta = 0;
        CandidateMove best_move{Kind::edge_exchange, 0, 0};
        bool found = false;

        for_each_candidate_move(solution, position, candidates, [&](const CandidateMove& move) {
            const int move_delta = evaluate(solution, move, dataset);
            ++stats.evaluated_moves;
            if (move_delta < best_delta) {
                best_delta = move_delta;
                best_move = move;
                found = true;
            }
        });
        if (!found) {
            return;
        }

#ifdef TASK4_DEBUG_CHECK
        const int before = objective(solution, dataset);
#endif
        apply_candidate_move(solution, position, best_move);
        ++stats.applied_moves;
#ifdef TASK4_DEBUG_CHECK
        if (objective(solution, dataset) != before + best_delta) {
            std::cerr << "TASK4 delta mismatch\n";
            std::abort();
        }
#endif
    }
}

// Counts the evaluated / applied moves of the Task 3 steepest search.
inline void count_task3_steepest_moves(std::vector<int>& solution, int dataset,
                                       Task4Stats& stats) {
    while (true) {
        const std::vector<local_search_detail::Move> moves =
            local_search_detail::neighborhood(solution, /*use_edges=*/true);
        int best_delta = 0;
        const local_search_detail::Move* best_move = nullptr;
        for (const local_search_detail::Move& move : moves) {
            const int move_delta = local_search_detail::delta(solution, move, dataset);
            if (move_delta < best_delta) {
                best_delta = move_delta;
                best_move = &move;
            }
        }
        stats.evaluated_moves += static_cast<long long>(moves.size());
        if (best_move == nullptr) {
            return;
        }
        local_search_detail::apply_move(solution, *best_move);
        ++stats.applied_moves;
    }
}

// Runs `improve(solution, stats)` on a copy of every starting solution, checks the result and records the running time of each run.
template <class Improve>
inline Task4Stats run_on_starts(ResultManager& result_manager,
                                const std::vector<std::vector<int>>& starts, Improve&& improve) {
    Task4Stats stats;
    for (const std::vector<int>& start_solution : starts) {
        std::vector<int> solution = start_solution;
        const auto begin = Clock::now();
        improve(solution, stats);
        const auto end = Clock::now();
        result_manager.check_solution(solution.data(), solution_size);
        result_manager.record_run_time(elapsed_ms(begin, end));
        ++stats.runs;
    }
    return stats;
}

}  
// The same random starting solutions are reused by all compared methods (paired comparison).
inline std::vector<std::vector<int>> task4_make_random_starts(int count, unsigned seed) {
    std::mt19937 generator(seed);
    std::vector<std::vector<int>> starts;
    starts.reserve(count);
    for (int i = 0; i < count; ++i) {
        starts.push_back(local_search_detail::random_solution(generator));
    }
    return starts;
}

// Baseline: exactly the Task 3 steepest local search
inline Task4Stats task4_steepest_baseline(ResultManager& result_manager,
                                          const std::vector<std::vector<int>>& starts) {
    std::mt19937 unused_generator(0);  // required by the interface, steepest never uses it
    const int dataset = result_manager.dataset;
    Task4Stats stats = task4_detail::run_on_starts(
        result_manager, starts, [&](std::vector<int>& solution, Task4Stats&) {
            local_search_detail::improve(solution, dataset, /*steepest=*/true,
                                         /*use_edges=*/true, unused_generator);
        });
    for (const std::vector<int>& start_solution : starts) {
        std::vector<int> solution = start_solution;
        task4_detail::count_task3_steepest_moves(solution, dataset, stats);
    }
    return stats;
}

// Steepest local search restricted to candidate moves.
inline Task4Stats task4_steepest_candidates(ResultManager& result_manager,
                                            const std::vector<std::vector<int>>& starts,
                                            int candidate_count = 10) {
    using namespace task4_detail;
    const int dataset = result_manager.dataset;

    const auto precompute_begin = Clock::now();
    const std::vector<std::vector<int>> candidates = build_candidate_lists(dataset, candidate_count);
    const auto precompute_end = Clock::now();

    Task4Stats stats = run_on_starts(
        result_manager, starts, [&](std::vector<int>& solution, Task4Stats& run_stats) {
            std::vector<int> position(total_nodes, -1);
            for (int p = 0; p < solution_size; ++p) {
                position[solution[p]] = p;
            }
            improve_with_candidates(solution, position, candidates, dataset, run_stats);
        });
    stats.candidate_precompute_ms = elapsed_ms(precompute_begin, precompute_end);
    return stats;
}

// Writes the table with the average number of evaluated / applied moves per run.
inline void write_task4_stats(const std::string& filename,
                              const std::vector<std::pair<std::string, Task4Stats>>& methods) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening task 4 stats file: " << filename << '\n';
        return;
    }

    file << "Average number of evaluated and applied moves per run\n";
    file << std::left << std::setw(62) << "Method" << std::right << std::setw(16) << "Evaluated"
         << std::setw(12) << "Applied" << std::setw(22) << "Candidate lists [ms]" << '\n';

    for (const auto& [name, stats] : methods) {
        file << std::left << std::setw(62) << name << std::right;
        if (stats.runs == 0 || stats.evaluated_moves == 0) {
            file << std::setw(16) << "-" << std::setw(12) << "-" << std::setw(22) << "-" << '\n';
            continue;
        }
        file << std::fixed << std::setprecision(1)
             << std::setw(16) << static_cast<double>(stats.evaluated_moves) / stats.runs
             << std::setw(12) << static_cast<double>(stats.applied_moves) / stats.runs;
        if (stats.candidate_precompute_ms > 0.0) {  // only the candidate methods build lists
            file << std::setprecision(3) << std::setw(22) << stats.candidate_precompute_ms << '\n';
        } else {
            file << std::setw(22) << "-" << '\n';
        }
    }
}