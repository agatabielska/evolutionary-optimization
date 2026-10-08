#pragma once
#include "../utils/result_manager.h"
#include <random>
#include <numeric>
#include <utility>

std::pair<int, int> find_nearest_neighbor(int current_node, const std::vector<bool>& visited, int dataset) {
    int nearest_node = -1;
    int nearest_distance = std::numeric_limits<int>::max();

    for (int k = 0; k < 200; ++k) {
        if (!visited[k]) {
            int distance = DISTANCE_MATRIX[current_node][k][dataset] + NODE_COST[k][dataset];
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest_node = k;
            }
        }
    }

    return std::make_pair(nearest_node, nearest_distance);
}

void task1_random_solver(ResultManager& result_manager) {
    const int num_nodes = 100;
    int solution[num_nodes];

    std::random_device rd;
    std::mt19937 g(rd());
    for (int i = 0; i < result_manager.repetitions; ++i) {

        // choose 100 random nodes from 0 to 199
        std::vector<int> nodes(200);
        std::iota(nodes.begin(), nodes.end(), 0);
        std::shuffle(nodes.begin(), nodes.end(), g);

        // copy the first 100 nodes to the solution
        std::copy_n(nodes.begin(), num_nodes, solution);
        result_manager.check_solution(solution, num_nodes);
    }
}

void task1_nn_end_of_path(ResultManager& result_manager) {
    const int num_nodes = 200;
    const int solution_size = 100;
    int solution[solution_size];

    for (int start_node = 0; start_node < num_nodes; ++start_node) {
        std::vector<bool> visited(200, false);
        int current_node = start_node;
        solution[0] = current_node;
        visited[current_node] = true;

        for (int j = 1; j < solution_size; ++j) {
            auto [nearest_node, distance] = find_nearest_neighbor(current_node, visited, result_manager.dataset);

            solution[j] = nearest_node;
            visited[nearest_node] = true;
            current_node = nearest_node;
        }

        result_manager.check_solution(solution, solution_size);
    }
}

void task1_nn_anywhere_at_path(ResultManager& result_manager) {
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
            int best_delta = std::numeric_limits<int>::max();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];

                // Option A: Prepend at the beginning
                int cost_front = DISTANCE_MATRIX[k][path.front()][dataset] + node_c;
                if (cost_front < best_delta) {
                    best_delta = cost_front;
                    best_node = k;
                    best_pos = 0;
                }

                // Option B: Append at the end
                int cost_back = DISTANCE_MATRIX[path.back()][k][dataset] + node_c;
                if (cost_back < best_delta) {
                    best_delta = cost_back;
                    best_node = k;
                    best_pos = path.size();
                }

                // Option C: Insert inside an existing edge (i, i + 1)
                for (size_t i = 0; i + 1 < path.size(); ++i) {
                    int u = path[i];
                    int v = path[i + 1];
                    int delta = DISTANCE_MATRIX[u][k][dataset] + 
                                DISTANCE_MATRIX[k][v][dataset] - 
                                DISTANCE_MATRIX[u][v][dataset] + node_c;
                    if (delta < best_delta) {
                        best_delta = delta;
                        best_node = k;
                        best_pos = i + 1;
                    }
                }
            }

            path.insert(path.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(path.data(), solution_size);
    }
}

void task1_greedy_cycle(ResultManager& result_manager) {
    const int total_nodes = 200;
    const int solution_size = 100;
    const int dataset = result_manager.dataset;

    for (int start_node = 0; start_node < total_nodes; ++start_node) {
        std::vector<int> cycle;
        cycle.reserve(solution_size);
        std::vector<bool> visited(total_nodes, false);

        cycle.push_back(start_node);
        visited[start_node] = true;

        // Pick 2nd node that minimizes distance + node cost


        auto [second_node, min_cost] = find_nearest_neighbor(start_node, visited, dataset);
        // int min_cost = std::numeric_limits<int>::max();
        // for (int k = 0; k < total_nodes; ++k) {
        //     if (!visited[k]) {
        //         int cost = DISTANCE_MATRIX[start_node][k][dataset] + NODE_COST[k][dataset];
        //         if (cost < min_cost) {
        //             min_cost = cost;
        //             second_node = k;
        //         }
        //     }
        // }
        cycle.push_back(second_node);
        visited[second_node] = true;

        // Build cycle up to 100 nodes
        while (static_cast<int>(cycle.size()) < solution_size) {
            int best_node = -1;
            int best_pos = -1;
            int best_delta = std::numeric_limits<int>::max();

            for (int k = 0; k < total_nodes; ++k) {
                if (visited[k]) continue;

                int node_c = NODE_COST[k][dataset];
                int cycle_len = cycle.size();

                for (int i = 0; i < cycle_len; ++i) {
                    int u = cycle[i];
                    int v = cycle[(i + 1) % cycle_len]; // Closes the cycle

                    int delta = DISTANCE_MATRIX[u][k][dataset] + 
                                DISTANCE_MATRIX[k][v][dataset] - 
                                DISTANCE_MATRIX[u][v][dataset] + node_c;

                    if (delta < best_delta) {
                        best_delta = delta;
                        best_node = k;
                        best_pos = i + 1;
                    }
                }
            }

            cycle.insert(cycle.begin() + best_pos, best_node);
            visited[best_node] = true;
        }

        result_manager.check_solution(cycle.data(), solution_size);
    }
}