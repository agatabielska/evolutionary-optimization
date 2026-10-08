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