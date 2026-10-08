#pragma once
#include <algorithm>
#include <limits>
#include <cassert>
#include <fstream>
#include <iostream>
// #include "reader.h"

class ResultManager {
public:
    int best_cost = std::numeric_limits<int>::max();
    int worst_cost = std::numeric_limits<int>::min();
    int average_cost = 0;
    int repetitions = 0;
    int dataset = 0;
    int best_solution[200]{};

    void update_best_solution(int new_cost, const int* new_solution, int num_nodes) noexcept {
        if (new_cost < best_cost) {
            assert(num_nodes <= 200);
            best_cost = new_cost;
            std::copy_n(new_solution, num_nodes, best_solution);
        }
        if (new_cost > worst_cost) {
            worst_cost = new_cost;
        }
        average_cost += new_cost;
    }

    void check_solution(const int* new_solution, int num_nodes) noexcept {
        int new_cost = 0;

        for (int i = 0; i < num_nodes - 1; ++i) {
            new_cost += DISTANCE_MATRIX[new_solution[i]][new_solution[i + 1]][dataset];
            new_cost += NODE_COST[new_solution[i]][dataset];
        }
        // Add the cost of returning to the starting node
        new_cost += DISTANCE_MATRIX[new_solution[num_nodes - 1]][new_solution[0]][dataset];
        new_cost += NODE_COST[new_solution[num_nodes - 1]][dataset];
        update_best_solution(new_cost, new_solution, num_nodes);
    }

    void save_best_solution_to_file(const std::string& filename, int num_nodes) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        file << "Best Cost: " << best_cost << std::endl;
        file << "Worst Cost: " << worst_cost << std::endl;
        file << "Average Cost: " << std::round(average_cost / repetitions) << std::endl;
        file << "Best Solution: ";
        for (int i = 0; i < num_nodes; ++i) {
            file << best_solution[i];
            if (i < num_nodes - 1) {
                file << ",";
            }
        }
        file << std::endl;
        file.close();
    }
};