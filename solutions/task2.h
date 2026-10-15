#pragma once
#include "../utils/result_manager.h"
#include <limits>
#include <utility>
#include <vector>

inline std::pair<int, int> task2_find_nearest_neighbor(int current_node, const std::vector<bool>& visited, int dataset, int greedy_mode = 0) {
    int nearest_node = -1;
    int nearest_distance = std::numeric_limits<int>::max();

    for (int k = 0; k < 200; ++k) {
        if (!visited[k]) {
            if (greedy_mode == 0) {
                int distance = DISTANCE_MATRIX[current_node][k][dataset] + NODE_COST[k][dataset];
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_node = k;
                }
            } else {
                int distance = 2 * DISTANCE_MATRIX[current_node][k][dataset] + NODE_COST[k][dataset] + NODE_COST[current_node][dataset];
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_node = k;
                }
            }
        }
    }

    return std::make_pair(nearest_node, nearest_distance);
}

void task2_nn_anywhere_regret(ResultManager& result_manager) {
    const int total_nodes = 200;
    const int solution_size = 100;
    const int dataset = result_manager.dataset;

    for (int start_node = 0; start_node < total_nodes; ++start_node) {
        std::vector<int> path;
        path.reserve(solution_size);
        std::vector<bool> visited(total_nodes, false);

        path.push_back(start_node);
        visited[start_node] = true;

        while (static_cast<int>(path.size()) < solution_size) {
            int best_node = -1;
            int best_pos = -1; // 0 = prepend, path.size() = append, or inside
            int best_c1 = std::numeric_limits<int>::max();
            double best_score = -std::numeric_limits<double>::infinity();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];
                int c1 = std::numeric_limits<int>::max();
                int c2 = std::numeric_limits<int>::max();
                int pos1 = -1;

                // Option A: insert before the first node
                int cost_front = DISTANCE_MATRIX[k][path.front()][dataset] + node_c;
                if (cost_front < c1) {
                    c2 = c1;
                    c1 = cost_front;
                    pos1 = 0;
                } else if (cost_front < c2) {
                    c2 = cost_front;
                }

                // Option B: insert after the last node
                int cost_back = DISTANCE_MATRIX[path.back()][k][dataset] + node_c;
                if (cost_back < c1) {
                    c2 = c1;
                    c1 = cost_back;
                    pos1 = static_cast<int>(path.size());
                } else if (cost_back < c2) {
                    c2 = cost_back;
                }

                // Option C: insert between two consecutive nodes
                for (size_t i = 0; i + 1 < path.size(); ++i) {
                    int u = path[i];
                    int v = path[i + 1];
                    int delta = DISTANCE_MATRIX[u][k][dataset] +
                                DISTANCE_MATRIX[k][v][dataset] -
                                DISTANCE_MATRIX[u][v][dataset] + node_c;

                    if (delta < c1) {
                        c2 = c1;
                        c1 = delta;
                        pos1 = static_cast<int>(i + 1);
                    } else if (delta < c2) {
                        c2 = delta;
                    }
                }

                // c1 = best insertion cost
                // c2 = second-best insertion cost
                // pos1 = position where c1 occurs
                // score = c2 - c1  (regret-based score)
                double score = static_cast<double>(c2 - c1);

                if (score > best_score ||
                    (score == best_score && (c1 < best_c1 || (c1 == best_c1 && k < best_node)))) {
                    best_score = score;
                    best_node = k;
                    best_pos = pos1;
                    best_c1 = c1;
                }
            }

            path.insert(path.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(path.data(), solution_size);
    }
}

void task2_nn_anywhere_weighted(ResultManager& result_manager, double w1 = 0.5, double w2 = 0.5) {
    const int total_nodes = 200;
    const int solution_size = 100;
    const int dataset = result_manager.dataset;

    for (int start_node = 0; start_node < total_nodes; ++start_node) {
        std::vector<int> path;
        path.reserve(solution_size);
        std::vector<bool> visited(total_nodes, false);

        path.push_back(start_node);
        visited[start_node] = true;

        while (static_cast<int>(path.size()) < solution_size) {
            int best_node = -1;
            int best_pos = -1;
            int best_c1 = std::numeric_limits<int>::max();
            double best_score = -std::numeric_limits<double>::infinity();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];
                int c1 = std::numeric_limits<int>::max();
                int c2 = std::numeric_limits<int>::max();
                int pos1 = -1;

                // Option A: insert before the first node
                int cost_front = DISTANCE_MATRIX[k][path.front()][dataset] + node_c;
                if (cost_front < c1) {
                    c2 = c1;
                    c1 = cost_front;
                    pos1 = 0;
                } else if (cost_front < c2) {
                    c2 = cost_front;
                }

                // Option B: insert after the last node
                int cost_back = DISTANCE_MATRIX[path.back()][k][dataset] + node_c;
                if (cost_back < c1) {
                    c2 = c1;
                    c1 = cost_back;
                    pos1 = static_cast<int>(path.size());
                } else if (cost_back < c2) {
                    c2 = cost_back;
                }

                // Option C: insert between two consecutive nodes
                for (size_t i = 0; i + 1 < path.size(); ++i) {
                    int u = path[i];
                    int v = path[i + 1];
                    int delta = DISTANCE_MATRIX[u][k][dataset] +
                                DISTANCE_MATRIX[k][v][dataset] -
                                DISTANCE_MATRIX[u][v][dataset] + node_c;

                    if (delta < c1) {
                        c2 = c1;
                        c1 = delta;
                        pos1 = static_cast<int>(i + 1);
                    } else if (delta < c2) {
                        c2 = delta;
                    }
                }

                // c1 = best insertion cost, c2 = second-best insertion cost, pos1 = best insertion position
                // score = w1 * (c2 - c1) - w2 * c1
                double score = w1 * static_cast<double>(c2 - c1) - w2 * static_cast<double>(c1);

                if (score > best_score ||
                    (score == best_score && (c1 < best_c1 || (c1 == best_c1 && k < best_node)))) {
                    best_score = score;
                    best_node = k;
                    best_pos = pos1;
                    best_c1 = c1;
                }
            }

            path.insert(path.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(path.data(), solution_size);
    }
}

void task2_greedy_cycle_regret(ResultManager& result_manager) {
    const int total_nodes = 200;
    const int solution_size = 100;
    const int dataset = result_manager.dataset;

    for (int start_node = 0; start_node < total_nodes; ++start_node) {
        std::vector<int> cycle;
        cycle.reserve(solution_size);
        std::vector<bool> visited(total_nodes, false);

        cycle.push_back(start_node);
        visited[start_node] = true;

        int second_node = task2_find_nearest_neighbor(start_node, visited, dataset).first;
        cycle.push_back(second_node);
        visited[second_node] = true;

        while (static_cast<int>(cycle.size()) < solution_size) {
            int best_node = -1;
            int best_pos = -1;
            int best_c1 = std::numeric_limits<int>::max();
            double best_score = -std::numeric_limits<double>::infinity();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];
                int c1 = std::numeric_limits<int>::max();
                int c2 = std::numeric_limits<int>::max();
                int pos1 = -1;

                int cycle_len = static_cast<int>(cycle.size());
                for (int i = 0; i < cycle_len; ++i) {
                    int u = cycle[i];
                    int v = cycle[(i + 1) % cycle_len];
                    int delta = DISTANCE_MATRIX[u][k][dataset] +
                                DISTANCE_MATRIX[k][v][dataset] -
                                DISTANCE_MATRIX[u][v][dataset] + node_c;

                    if (delta < c1) {
                        c2 = c1;
                        c1 = delta;
                        pos1 = i + 1;
                    } else if (delta < c2) {
                        c2 = delta;
                    }
                }

                // c1 = best insertion cost
                // c2 = second-best insertion cost
                // pos1 = position of the best insertion in the cycle
                // score = c2 - c1  (regret-based score)
                double score = static_cast<double>(c2 - c1);

                if (score > best_score ||
                    (score == best_score && (c1 < best_c1 || (c1 == best_c1 && k < best_node)))) {
                    best_score = score;
                    best_node = k;
                    best_pos = pos1;
                    best_c1 = c1;
                }
            }

            cycle.insert(cycle.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(cycle.data(), solution_size);
    }
}

void task2_greedy_cycle_weighted(ResultManager& result_manager, double w1 = 0.5, double w2 = 0.5) {
    const int total_nodes = 200;
    const int solution_size = 100;
    const int dataset = result_manager.dataset;

    for (int start_node = 0; start_node < total_nodes; ++start_node) {
        std::vector<int> cycle;
        cycle.reserve(solution_size);
        std::vector<bool> visited(total_nodes, false);

        cycle.push_back(start_node);
        visited[start_node] = true;

        int second_node = task2_find_nearest_neighbor(start_node, visited, dataset).first;
        cycle.push_back(second_node);
        visited[second_node] = true;

        while (static_cast<int>(cycle.size()) < solution_size) {
            int best_node = -1;
            int best_pos = -1;
            int best_c1 = std::numeric_limits<int>::max();
            double best_score = -std::numeric_limits<double>::infinity();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];
                int c1 = std::numeric_limits<int>::max();
                int c2 = std::numeric_limits<int>::max();
                int pos1 = -1;

                int cycle_len = static_cast<int>(cycle.size());
                for (int i = 0; i < cycle_len; ++i) {
                    int u = cycle[i];
                    int v = cycle[(i + 1) % cycle_len];
                    int delta = DISTANCE_MATRIX[u][k][dataset] +
                                DISTANCE_MATRIX[k][v][dataset] -
                                DISTANCE_MATRIX[u][v][dataset] + node_c;

                    if (delta < c1) {
                        c2 = c1;
                        c1 = delta;
                        pos1 = i + 1;
                    } else if (delta < c2) {
                        c2 = delta;
                    }
                }

                // c1 = best insertion cost, c2 = second-best insertion cost, pos1 = best insertion position
                // score = w1 * (c2 - c1) - w2 * c1
                double score = w1 * static_cast<double>(c2 - c1) - w2 * static_cast<double>(c1);

                if (score > best_score ||
                    (score == best_score && (c1 < best_c1 || (c1 == best_c1 && k < best_node)))) {
                    best_score = score;
                    best_node = k;
                    best_pos = pos1;
                    best_c1 = c1;
                }
            }

            cycle.insert(cycle.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(cycle.data(), solution_size);
    }
}