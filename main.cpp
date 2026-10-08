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
    //     ResultManager random_result_manager;
    //     random_result_manager.repetitions = 200;
    //     random_result_manager.dataset = dataset;
    //     task1_random_solver(random_result_manager);

    //     random_result_manager.save_best_solution_to_file("calculated_solutions/random_solution_dataset_" + std::to_string(dataset) + ".txt", 100);
    // }
        
        ResultManager nn_result_manager;
        nn_result_manager.repetitions = 200;
        nn_result_manager.dataset = dataset;
        nn_result_manager.start_points = 200;
        task1_nn_end_of_path(nn_result_manager);

        nn_result_manager.save_best_solution_to_file("calculated_solutions/nn_solution_dataset_" + std::to_string(dataset) + ".txt", 100);
    }
    
}