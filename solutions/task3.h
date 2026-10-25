#pragma once

#include "../utils/result_manager.h"
#include "task2.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace local_search_detail {

constexpr int total_nodes = 200;
constexpr int solution_size = 100;

enum class MoveType { node_exchange, edge_exchange, inter_route_exchange };

struct Move {
    MoveType type;
    int first;
    int second;
    int outside;
};

inline int edge_cost(int from, int to, int dataset) {
    return DISTANCE_MATRIX[from][to][dataset];
}

inline int node_exchange_delta(const std::vector<int>& solution, int first,
                               int second, int dataset) {
    std::vector<int> affected_edges = {(first + solution_size - 1) % solution_size,
                                        first, (second + solution_size - 1) % solution_size,
                                        second};
    std::sort(affected_edges.begin(), affected_edges.end());
    affected_edges.erase(std::unique(affected_edges.begin(), affected_edges.end()),
                         affected_edges.end());

    int delta = 0;
    for (int edge : affected_edges) {
        int next = (edge + 1) % solution_size;
        int old_from = solution[edge];
        int old_to = solution[next];
        int new_from = old_from;
        if (edge == first) {
            new_from = solution[second];
        } else if (edge == second) {
            new_from = solution[first];
        }

        int new_to = old_to;
        if (next == first) {
            new_to = solution[second];
        } else if (next == second) {
            new_to = solution[first];
        }
        delta += edge_cost(new_from, new_to, dataset) -
                 edge_cost(old_from, old_to, dataset);
    }
    return delta;
}

inline int edge_exchange_delta(const std::vector<int>& solution, int first,
                               int second, int dataset) {
    int first_next = (first + 1) % solution_size;
    int second_next = (second + 1) % solution_size;
    return edge_cost(solution[first], solution[second], dataset) +
           edge_cost(solution[first_next], solution[second_next], dataset) -
           edge_cost(solution[first], solution[first_next], dataset) -
           edge_cost(solution[second], solution[second_next], dataset);
}

inline int inter_route_exchange_delta(const std::vector<int>& solution, int position,
                                      int outside, int dataset) {
    int previous = (position + solution_size - 1) % solution_size;
    int next = (position + 1) % solution_size;
    return NODE_COST[outside][dataset] - NODE_COST[solution[position]][dataset] +
           edge_cost(solution[previous], outside, dataset) +
           edge_cost(outside, solution[next], dataset) -
           edge_cost(solution[previous], solution[position], dataset) -
           edge_cost(solution[position], solution[next], dataset);
}

inline int delta(const std::vector<int>& solution, const Move& move, int dataset) {
    if (move.type == MoveType::node_exchange) {
        return node_exchange_delta(solution, move.first, move.second, dataset);
    }
    if (move.type == MoveType::edge_exchange) {
        return edge_exchange_delta(solution, move.first, move.second, dataset);
    }
    return inter_route_exchange_delta(solution, move.first, move.outside, dataset);
}

inline void apply_move(std::vector<int>& solution, const Move& move) {
    if (move.type == MoveType::node_exchange) {
        std::swap(solution[move.first], solution[move.second]);
    } else if (move.type == MoveType::edge_exchange) {
        // it does not include the last node in the range, so we need to add 1 to move.second
        std::reverse(solution.begin() + move.first + 1,
                     solution.begin() + move.second + 1);
    } else {
        solution[move.first] = move.outside;
    }
}

inline std::vector<Move> neighborhood(const std::vector<int>& solution,
                                      bool use_edges) {
    std::vector<Move> moves;
    for (int first = 0; first < solution_size; ++first) {
        for (int second = first + 1; second < solution_size; ++second) {
            if (use_edges) {
                if (second > first + 1 &&
                    !(first == 0 && second == solution_size - 1)) {
                    moves.push_back({MoveType::edge_exchange, first, second, -1});
                }
            } else {
                moves.push_back({MoveType::node_exchange, first, second, -1});
            }
        }
    }

    std::vector<bool> selected(total_nodes, false);
    for (int node : solution) {
        selected[node] = true;
    }
    for (int position = 0; position < solution_size; ++position) {
        for (int outside = 0; outside < total_nodes; ++outside) {
            if (!selected[outside]) {
                moves.push_back(
                    {MoveType::inter_route_exchange, position, -1, outside});
            }
        }
    }
    return moves;
}

inline std::vector<int> random_solution(std::mt19937& generator) {
    std::vector<int> nodes(total_nodes);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::shuffle(nodes.begin(), nodes.end(), generator);
    nodes.resize(solution_size);
    return nodes;
}

inline std::vector<int> weighted_nn_start_solution(int start_node, int dataset) {
    std::vector<int> path{start_node};
    std::vector<bool> visited(total_nodes, false);
    visited[start_node] = true;

    while (static_cast<int>(path.size()) < solution_size) {
        int best_node = -1;
        int best_position = -1;
        int best_c1 = std::numeric_limits<int>::max();
        double best_score = -std::numeric_limits<double>::infinity();

        for (int node = 0; node < total_nodes; ++node) {
            if (visited[node]) {
                continue;
            }
            int c1 = std::numeric_limits<int>::max();
            int c2 = std::numeric_limits<int>::max();
            int position = -1;
            auto consider = [&](int insertion_delta, int insertion_position) {
                if (insertion_delta < c1) {
                    c2 = c1;
                    c1 = insertion_delta;
                    position = insertion_position;
                } else if (insertion_delta < c2) {
                    c2 = insertion_delta;
                }
            };

            consider(DISTANCE_MATRIX[node][path.front()][dataset] +
                         NODE_COST[node][dataset],
                     0);
            consider(DISTANCE_MATRIX[path.back()][node][dataset] +
                         NODE_COST[node][dataset],
                     static_cast<int>(path.size()));
            for (size_t i = 0; i + 1 < path.size(); ++i) {
                int left = path[i];
                int right = path[i + 1];
                consider(DISTANCE_MATRIX[left][node][dataset] +
                             DISTANCE_MATRIX[node][right][dataset] -
                             DISTANCE_MATRIX[left][right][dataset] +
                             NODE_COST[node][dataset],
                         static_cast<int>(i + 1));
            }

            double score = 0.5 * static_cast<double>(c2 - c1) -
                           0.5 * static_cast<double>(c1);
            if (score > best_score ||
                (score == best_score &&
                 (c1 < best_c1 ||
                  (c1 == best_c1 && node < best_node)))) {
                best_score = score;
                best_c1 = c1;
                best_node = node;
                best_position = position;
            }
        }
        path.insert(path.begin() + best_position, best_node);
        visited[best_node] = true;
    }
    return path;
}

inline void improve(std::vector<int>& solution, int dataset, bool steepest,
                    bool use_edges, std::mt19937& generator) {
    while (true) {
        std::vector<Move> moves = neighborhood(solution, use_edges);
        if (!steepest) {
            std::shuffle(moves.begin(), moves.end(), generator);
            bool improved = false;
            for (const Move& move : moves) {
                if (delta(solution, move, dataset) < 0) {
                    apply_move(solution, move);
                    improved = true;
                    break;
                }
            }
            if (!improved) {
                return;
            }
            continue;
        }

        int best_delta = 0;
        const Move* best_move = nullptr;
        for (const Move& move : moves) {
            int move_delta = delta(solution, move, dataset);
            if (move_delta < best_delta) {
                best_delta = move_delta;
                best_move = &move;
            }
        }
        if (best_move == nullptr) {
            return;
        }
        apply_move(solution, *best_move);
    }
}

}  // namespace local_search_detail

inline void run_local_search(ResultManager& result_manager, bool steepest,
                             bool use_edges, bool greedy_start) {
    using namespace local_search_detail;
    std::random_device random_device;
    std::mt19937 generator(random_device());

    for (int repetition = 0; repetition < 200; ++repetition) {
        std::vector<int> solution =
            greedy_start ? weighted_nn_start_solution(repetition, result_manager.dataset)
                         : random_solution(generator);
        auto start = std::chrono::high_resolution_clock::now();
        improve(solution, result_manager.dataset, steepest, use_edges, generator);
        result_manager.check_solution(solution.data(), solution_size);
        auto end = std::chrono::high_resolution_clock::now();
        result_manager.record_run_time(
            std::chrono::duration<double, std::milli>(end - start).count());
    }
}

inline std::string local_search_method_name(bool steepest, bool use_edges,
                                            bool greedy_start) {
    return std::string(steepest ? "Steepest" : "Greedy") + " + " +
           (use_edges ? "edge exchange" : "node exchange") + " + " +
           (greedy_start ? "weighted NN start" : "random start");
}

inline void write_local_search_report(
    const std::string& filename,
    const std::vector<std::pair<std::string, const ResultManager*>>& methods) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening local-search report: " << filename << '\n';
        return;
    }

    file << "Objective values (average (minimum - maximum))\n";
    file << std::left << std::setw(48) << "Method" << std::right
         << std::setw(18) << "Average (min - max)" << '\n';
    for (const auto& [name, result] : methods) {
        double average = result->evaluations_count == 0
                             ? 0.0
                             : static_cast<double>(result->total_cost) /
                                   result->evaluations_count;
        file << std::left << std::setw(48) << name << std::right << std::fixed
             << std::setprecision(2) << std::setw(8) << average << " ("
             << result->best_cost << " - " << result->worst_cost << ")\n";
    }

    file << "\nRunning times in milliseconds (average (minimum - maximum))\n";
    file << std::left << std::setw(48) << "Method" << std::right
         << std::setw(18) << "Average (min - max)" << '\n';
    for (const auto& [name, result] : methods) {
        double average = 0.0;
        double minimum = 0.0;
        double maximum = 0.0;
        if (!result->run_times_ms.empty()) {
            average = std::accumulate(result->run_times_ms.begin(),
                                      result->run_times_ms.end(), 0.0) /
                      result->run_times_ms.size();
            auto [min_it, max_it] = std::minmax_element(
                result->run_times_ms.begin(), result->run_times_ms.end());
            minimum = *min_it;
            maximum = *max_it;
        }
        file << std::left << std::setw(48) << name << std::right << std::fixed
             << std::setprecision(3) << std::setw(8) << average << " ("
             << minimum << " - " << maximum << ")\n";
    }
}
