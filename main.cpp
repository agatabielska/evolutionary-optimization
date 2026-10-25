#include "utils/reader.h"
#include "utils/result_manager.h"
#include "utils/plotter.h"
#include "solutions/task1.h"
#include "solutions/task2.h"
#include "solutions/task3.h"
#include <iostream>
#include <cmath>
#include <memory>

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

        generate_dataset_plot(dataset, {
            {"Random", random_result_manager},
            {"Nearest neighbor (end)", nn_result_manager},
            {"Nearest neighbor (any position)", nn_anywhere_at_path_result_manager},
            {"Greedy cycle", greedy_manager},
        });
        ResultManager task2_nn_regret_manager;
        task2_nn_regret_manager.dataset = dataset;
        task2_nn_anywhere_regret(task2_nn_regret_manager);
        task2_nn_regret_manager.save_best_solution_to_file("calculated_solutions/task2_nn_anywhere_regret_dataset_" + std::to_string(dataset) + ".txt", 100);

        ResultManager task2_nn_weighted_manager;
        task2_nn_weighted_manager.dataset = dataset;
        task2_nn_anywhere_weighted(task2_nn_weighted_manager);
        task2_nn_weighted_manager.save_best_solution_to_file("calculated_solutions/task2_nn_anywhere_weighted_dataset_" + std::to_string(dataset) + ".txt", 100);

        ResultManager task2_cycle_regret_manager;
        task2_cycle_regret_manager.dataset = dataset;
        task2_greedy_cycle_regret(task2_cycle_regret_manager);
        task2_cycle_regret_manager.save_best_solution_to_file("calculated_solutions/task2_greedy_cycle_regret_dataset_" + std::to_string(dataset) + ".txt", 100);

        ResultManager task2_cycle_weighted_manager;
        task2_cycle_weighted_manager.dataset = dataset;
        task2_greedy_cycle_weighted(task2_cycle_weighted_manager);
        task2_cycle_weighted_manager.save_best_solution_to_file("calculated_solutions/task2_greedy_cycle_weighted_dataset_" + std::to_string(dataset) + ".txt", 100);

        generate_task2_dataset_plot(dataset, {
            {"Nearest neighbor (regret)", task2_nn_regret_manager},
            {"Nearest neighbor (weighted)", task2_nn_weighted_manager},
            {"Greedy cycle (regret)", task2_cycle_regret_manager},
            {"Greedy cycle (weighted)", task2_cycle_weighted_manager},
        });

        std::vector<std::unique_ptr<ResultManager>> local_results;
        std::vector<std::string> local_names;
        std::vector<std::pair<std::string, const ResultManager*>> report_methods;
        std::vector<PlotMethod> plot_methods;
        local_names.reserve(8);
        report_methods.reserve(8);
        plot_methods.reserve(8);
        for (bool steepest : {false, true}) {
            for (bool use_edges : {false, true}) {
                for (bool greedy_start : {false, true}) {
                    auto result = std::make_unique<ResultManager>();
                    result->dataset = dataset;
                    run_local_search(*result, steepest, use_edges, greedy_start);
                    const std::string method_name =
                        local_search_method_name(steepest, use_edges, greedy_start);
                    local_names.push_back(method_name);
                    const std::string dataset_name = dataset == 0 ? "TSPA" : "TSPB";
                    const std::string search_name = steepest ? "steepest" : "greedy";
                    const std::string move_name =
                        use_edges ? "edge_exchange" : "node_exchange";
                    const std::string start_name =
                        greedy_start ? "weighted_nn_start" : "random_start";
                    result->save_best_solution_to_file(
                        "calculated_solutions/local_search_" + dataset_name + "_" +
                            search_name + "_" + move_name + "_" + start_name + ".txt",
                        100, method_name);
                    report_methods.emplace_back(local_names.back(), result.get());
                    plot_methods.push_back(
                        {local_names.back().c_str(), *result});
                    local_results.push_back(std::move(result));
                }
            }
        }
        write_local_search_report(
            std::string("calculated_solutions/local_search_report_") +
                (dataset == 0 ? "TSPA" : "TSPB") + ".txt",
            report_methods);
        generate_local_search_dataset_plot(dataset, plot_methods);
    }
}