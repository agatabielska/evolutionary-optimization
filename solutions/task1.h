// #pragma once
#include "../utils/result_manager.h"
#include <random>
#include <numeric>

void task1_random_solver(ResultManager& result_manager) {
    const int num_nodes = 100;
    int solution[num_nodes];

    for (int i = 0; i < result_manager.repetitions; ++i) {
        std::random_device rd;
        std::mt19937 g(rd());

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
    const int num_nodes = 100;
    int solution[num_nodes];

    for (int start_node = 0; start_node < num_nodes; ++start_node) {
        for (int rep = 0; rep < result_manager.repetitions; ++rep) {
            std::vector<bool> visited(200, false);
            int current_node = start_node;
            solution[0] = current_node;
            visited[current_node] = true;

            for (int j = 1; j < num_nodes; ++j) {
                int nearest_node = -1;
                int nearest_distance = std::numeric_limits<int>::max();

                for (int k = 0; k < 200; ++k) {
                    if (!visited[k]) {
                        int distance = DISTANCE_MATRIX[current_node][k][result_manager.dataset];
                        if (distance < nearest_distance) {
                            nearest_distance = distance;
                            nearest_node = k;
                        }
                    }
                }

                solution[j] = nearest_node;
                visited[nearest_node] = true;
                current_node = nearest_node;
            }

            result_manager.check_solution(solution, num_nodes);
        }
    }
}