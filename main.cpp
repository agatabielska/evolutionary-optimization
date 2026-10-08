#include "utils/reader.h"
#include "utils/result_manager.h"
#include "solutions/task1.h"
#include <iostream>
#include <cmath>

int main() {
    // Read the datasets and initialize the distance matrix and node costs
    read_distance_matrix();
    
    for (int dataset = 0; dataset < 2; ++dataset) {

        std::cout << "\n--- Dataset " << (dataset == 0 ? "TSPA" : "TSPB") << " ---" << std::endl;
        ResultManager random_result_manager;
        random_result_manager.repetitions = 200;
        random_result_manager.dataset = dataset;
        task1_random_solver(random_result_manager);

        random_result_manager.save_best_solution_to_file("calculated_solutions/random_solution_dataset_" + std::to_string(dataset) + ".txt", 100);
        
        ResultManager nn_result_manager;
        nn_result_manager.dataset = dataset;
        task1_nn_end_of_path(nn_result_manager);

        nn_result_manager.save_best_solution_to_file("calculated_solutions/nn_solution_dataset_" + std::to_string(dataset) + ".txt", 100);
        
        ResultManager nn_anywhere_at_path_result_manager;
        nn_anywhere_at_path_result_manager.dataset = dataset;
        task1_nn_anywhere_at_path(nn_anywhere_at_path_result_manager);
        nn_anywhere_at_path_result_manager.save_best_solution_to_file("calculated_solutions/nn_anywhere_at_path_solution_dataset_" + std::to_string(dataset) + ".txt", 100);
    
        ResultManager greedy_manager;
        greedy_manager.dataset = dataset;
        task1_greedy_cycle(greedy_manager);
        greedy_manager.save_best_solution_to_file("calculated_solutions/greedy_dataset_" + std::to_string(dataset) + ".txt", 100);
    }
    
}