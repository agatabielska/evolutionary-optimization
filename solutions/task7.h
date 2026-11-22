#pragma once

#include "task2.h"
#include "task5.h"
#include "task6.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Task7Stats {
    std::vector<int> iterations;
};

namespace task7_detail {

using local_search_detail::solution_size;
using local_search_detail::total_nodes;
using Clock = std::chrono::high_resolution_clock;

// Weights of the weighted greedy cycle used as the repair operator:
//     score(v) = repair_regret_weight * (c2 - c1) - repair_cost_weight * c1
// where c1 / c2 are the best / second best insertion cost of node v.
// TODO: copy the exact default weights of task2_greedy_cycle_weighted from task2.h
constexpr double repair_regret_weight = 1.0;
constexpr double repair_cost_weight = 1.0;

enum class DestroyKind {
    random_nodes,
    single_subpath,
    multiple_subpaths,
    randomized_bad_nodes
};

inline double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

inline std::string destroy_name(DestroyKind kind) {
    if (kind == DestroyKind::random_nodes) return "random 30 nodes";
    if (kind == DestroyKind::single_subpath) return "single random subpath (30 nodes)";
    if (kind == DestroyKind::multiple_subpaths) return "3-5 random subpaths (30 nodes)";
    return "randomized bad nodes (30 nodes)";
}

inline std::vector<int> random_nodes(const std::vector<int>& solution,
                                     std::mt19937& generator,
                                     int removed_total = 30) {
    std::vector<int> positions(solution_size);
    std::iota(positions.begin(), positions.end(), 0);
    std::shuffle(positions.begin(), positions.end(), generator);
    positions.resize(removed_total);

    std::vector<bool> remove(total_nodes, false);
    for (int position : positions) remove[solution[position]] = true;
    std::vector<int> remaining;
    remaining.reserve(solution_size - removed_total);
    for (int node : solution) {
        if (!remove[node]) remaining.push_back(node);
    }
    return remaining;
}

inline std::vector<int> single_subpath(const std::vector<int>& solution,
                                       std::mt19937& generator,
                                       int removed_total = 30) {
    std::uniform_int_distribution<int> start_dist(0, solution_size - 1);
    const int start = start_dist(generator);
    std::vector<bool> remove(total_nodes, false);
    for (int i = 0; i < removed_total; ++i) remove[solution[(start + i) % solution_size]] = true;

    std::vector<int> remaining;
    remaining.reserve(solution_size - removed_total);
    for (int node : solution) {
        if (!remove[node]) remaining.push_back(node);
    }
    return remaining;
}

inline std::vector<int> random_composition(int total, int parts,
                                           std::mt19937& generator) {
    std::uniform_int_distribution<int> cut_dist(0, total);
    std::vector<int> cuts(parts - 1);
    for (int& cut : cuts) cut = cut_dist(generator);
    std::sort(cuts.begin(), cuts.end());

    std::vector<int> result(parts);
    int previous = 0;
    for (int i = 0; i < parts - 1; ++i) {
        result[i] = cuts[i] - previous;
        previous = cuts[i];
    }
    result[parts - 1] = total - previous;
    return result;
}

inline std::vector<int> multiple_subpaths(const std::vector<int>& solution,
                                          std::mt19937& generator,
                                          int removed_total = 30) {
    std::uniform_int_distribution<int> count_dist(3, 5);
    std::uniform_int_distribution<int> start_dist(0, solution_size - 1);
    int count = count_dist(generator);
    count = std::min(count, removed_total);

    std::vector<int> lengths =
        random_composition(removed_total - count, count, generator);
    for (int& length : lengths) ++length;
    std::shuffle(lengths.begin(), lengths.end(), generator);

    std::vector<int> gaps = random_composition(
        solution_size - removed_total - count, count, generator);
    for (int& gap : gaps) ++gap;

    std::vector<bool> remove(total_nodes, false);
    int position = start_dist(generator);
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < lengths[i]; ++j) {
            remove[solution[(position + j) % solution_size]] = true;
        }
        position += lengths[i] + gaps[i];
    }

    std::vector<int> remaining;
    remaining.reserve(solution_size - removed_total);
    for (int node : solution) {
        if (!remove[node]) remaining.push_back(node);
    }
    return remaining;
}

// D4 (Algorithm 5): precompute badness & weights once, then draw 30 without replacement.
inline std::vector<int> randomized_bad_nodes(const std::vector<int>& solution,
                                             std::mt19937& generator,
                                             int dataset,
                                             int removed_total = 30) {
    std::array<int, solution_size> badness{};
    for (int i = 0; i < solution_size; ++i) {
        const int node = solution[i];
        const int previous = solution[(i + solution_size - 1) % solution_size];
        const int next = solution[(i + 1) % solution_size];
        const int adjacent_max =
            std::max(DISTANCE_MATRIX[previous][node][dataset],
                     DISTANCE_MATRIX[node][next][dataset]);
        badness[i] = NODE_COST[node][dataset] + adjacent_max;
    }

    const auto [min_it, max_it] = std::minmax_element(badness.begin(), badness.end());
    const int minimum = *min_it;
    const int range = std::max(1, *max_it - minimum);

    std::vector<int> positions(solution_size);
    std::vector<double> weights(solution_size);
    for (int i = 0; i < solution_size; ++i) {
        positions[i] = i;
        const double normalized = static_cast<double>(badness[i] - minimum) / range;
        weights[i] = 1.0 + 4.0 * normalized;
    }

    std::vector<bool> remove(total_nodes, false);
    for (int removed_count = 0; removed_count < removed_total; ++removed_count) {
        std::discrete_distribution<int> weighted_position(weights.begin(), weights.end());
        const int weight_index = weighted_position(generator);
        remove[solution[positions[weight_index]]] = true;
        positions.erase(positions.begin() + weight_index);
        weights.erase(weights.begin() + weight_index);
    }

    std::vector<int> remaining;
    remaining.reserve(solution_size - removed_total);
    for (int node : solution) {
        if (!remove[node]) remaining.push_back(node);
    }
    return remaining;
}

inline std::vector<int> destroy(const std::vector<int>& solution, DestroyKind kind,
                                std::mt19937& generator, int dataset,
                                int removed_total = 30) {
    if (kind == DestroyKind::random_nodes)
        return random_nodes(solution, generator, removed_total);
    if (kind == DestroyKind::single_subpath)
        return single_subpath(solution, generator, removed_total);
    if (kind == DestroyKind::multiple_subpaths)
        return multiple_subpaths(solution, generator, removed_total);
    return randomized_bad_nodes(solution, generator, dataset, removed_total);
}

inline void repair(std::vector<int>& cycle, int dataset) {
    std::array<bool, total_nodes> selected{};
    for (int node : cycle) selected[node] = true;

    while (static_cast<int>(cycle.size()) < solution_size) {
        int best_node = -1;
        int best_position = -1;
        int best_cost = std::numeric_limits<int>::max();
        double best_score = -std::numeric_limits<double>::infinity();

        for (int node = 0; node < total_nodes; ++node) {
            if (selected[node]) continue;

            int c1 = std::numeric_limits<int>::max();
            int c2 = std::numeric_limits<int>::max();
            int position = -1;
            const int len = static_cast<int>(cycle.size());
            for (int i = 0; i < len; ++i) {
                const int u = cycle[i];
                const int v = cycle[(i + 1) % len];
                const int delta =
                    DISTANCE_MATRIX[u][node][dataset] +
                    DISTANCE_MATRIX[node][v][dataset] -
                    DISTANCE_MATRIX[u][v][dataset] + NODE_COST[node][dataset];
                if (delta < c1) {
                    c2 = c1;
                    c1 = delta;
                    position = i + 1;
                } else if (delta < c2) {
                    c2 = delta;
                }
            }

            const double score =
                repair_regret_weight * static_cast<double>(c2 - c1) -
                repair_cost_weight * static_cast<double>(c1);
            if (score > best_score ||
                (score == best_score &&
                 (c1 < best_cost ||
                  (c1 == best_cost && node < best_node)))) {
                best_score = score;
                best_cost = c1;
                best_node = node;
                best_position = position;
            }
        }

        cycle.insert(cycle.begin() + best_position, best_node);
        selected[best_node] = true;
    }
}

}  // namespace task7_detail

inline Task7Stats task7_lns(ResultManager& result_manager, int runs,
                            double time_limit_ms, unsigned seed,
                            task7_detail::DestroyKind destroy_kind,
                            bool use_local_search,
                            int removed_total = 30) {
    using namespace task7_detail;
    Task7Stats stats;
    std::mt19937 generator(seed);
    task6_detail::ListLocalSearch local_search(result_manager.dataset);

    for (int run = 0; run < runs; ++run) {
        const auto begin = Clock::now();
        std::vector<int> current = local_search_detail::random_solution(generator);
        local_search.run(current);
        int current_cost = task4_detail::objective(current, result_manager.dataset);
        int iterations = 0;

        while (elapsed_ms(begin, Clock::now()) < time_limit_ms) {
            std::vector<int> candidate =
                destroy(current, destroy_kind, generator, result_manager.dataset,
                        removed_total);
            repair(candidate, result_manager.dataset);
            if (use_local_search) local_search.run(candidate);
            ++iterations;

            const int candidate_cost =
                task4_detail::objective(candidate, result_manager.dataset);
            if (candidate_cost < current_cost) {
                current = candidate;
                current_cost = candidate_cost;
            }
        }

        const auto end = Clock::now();
        result_manager.check_solution(current.data(), solution_size);
        result_manager.record_run_time(elapsed_ms(begin, end));
        stats.iterations.push_back(iterations);
    }
    return stats;
}

inline void write_task7_stats(
    const std::string& filename,
    const std::vector<std::pair<std::string, Task7Stats>>& methods) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening task 7 iterations file: " << filename << '\n';
        return;
    }

    file << "Number of LNS main-loop iterations per run (average (minimum - maximum))\n";
    file << std::left << std::setw(70) << "Method" << std::right << std::setw(30)
         << "Iterations: av (min - max)" << '\n';
    for (const auto& [name, stats] : methods) {
        file << std::left << std::setw(70) << name << std::right;
        if (stats.iterations.empty()) {
            file << std::setw(30) << "-" << '\n';
            continue;
        }
        const double average = std::accumulate(stats.iterations.begin(),
                                               stats.iterations.end(), 0.0) /
                               stats.iterations.size();
        const auto [min_it, max_it] =
            std::minmax_element(stats.iterations.begin(), stats.iterations.end());
        std::ostringstream cell;
        cell << std::fixed << std::setprecision(1) << average << " ("
             << *min_it << " - " << *max_it << ")";
        file << std::setw(30) << cell.str() << '\n';
    }
}