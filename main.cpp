#include "utils/reader.h"
#include "utils/result_manager.h"
#include "utils/plotter.h"
#include "solutions/task1.h"
#include "solutions/task2.h"
#include "solutions/task3.h"
#include "solutions/task4.h"
#include "solutions/task5.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

const std::string kOutputDir = "calculated_solutions/";
constexpr int kSolutionSize = 100;
constexpr int kRepetitions = 200;

std::string dataset_name(int dataset) { return dataset == 0 ? "TSPA" : "TSPB"; }
std::string dataset_tag(int dataset) { return dataset == 0 ? "tspa" : "tspb"; }

// Common naming scheme of every output file:
//   task<N>-<kind>-<dataset>[-<description>].<extension>
// kind is "solution" (best solution found), "report" (result tables) or "plot" (SVG picture).
// Examples: task3-report-tspa.txt, task3-solution-tspa-steepest-edge-random-start.txt,
//           task3-plot-tspa.svg
std::string output_path(int task, const std::string& kind, int dataset,
                        const std::string& description = "",
                        const std::string& extension = "txt") {
    std::string name = "task" + std::to_string(task) + "-" + kind + "-" + dataset_tag(dataset);
    if (!description.empty()) {
        name += "-" + description;
    }
    return kOutputDir + "task" + std::to_string(task) + "/" + name + "." + extension;
}

// One plot per task and dataset, e.g. calculated_solutions/task4-plot-tspa.svg
std::string plot_path(int task, int dataset) {
    return output_path(task, "plot", dataset, "", "svg");
}

// Runs `solver` on the manager and saves the best solution found.
void run_and_save(ResultManager& manager, int dataset, int task, const std::string& description,
                  const std::function<void(ResultManager&)>& solver) {
    manager.dataset = dataset;
    // reset the clock: all managers of a task are constructed together, so without this the
    // "Time:" line of a later method would also include the time of the earlier methods
    manager.start_time = std::chrono::high_resolution_clock::now();
    solver(manager);
    // Task 1 / 2 solvers do not time their runs individually. If nothing was recorded, spread the
    // total time over the runs, so the report shows the average time per run (min = max = average).
    if (manager.run_times_ms.empty() && manager.evaluations_count > 0) {
        const double total_ms = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - manager.start_time).count();
        for (int i = 0; i < manager.evaluations_count; ++i) {
            manager.record_run_time(total_ms / manager.evaluations_count);
        }
    }
    manager.save_best_solution_to_file(output_path(task, "solution", dataset, description),
                                       kSolutionSize);
}

void run_task1(int dataset) {
    ResultManager random_manager, nn_end_manager, nn_any_manager, greedy_manager;
    random_manager.repetitions = kRepetitions;

    run_and_save(random_manager, dataset, 1, "random", task1_random_solver);
    run_and_save(nn_end_manager, dataset, 1, "nn-end", task1_nn_end_of_path);
    run_and_save(nn_any_manager, dataset, 1, "nn-any", task1_nn_anywhere_at_path);
    run_and_save(greedy_manager, dataset, 1, "greedy-cycle", task1_greedy_cycle);

    // result tables (objective + running time), same format as the Task 3 / Task 4 reports
    write_local_search_report(output_path(1, "report", dataset), {
        {"Random", &random_manager},
        {"Nearest neighbor (end)", &nn_end_manager},
        {"Nearest neighbor (any position)", &nn_any_manager},
        {"Greedy cycle", &greedy_manager},
    });

    generate_dataset_plot(dataset, {
        {"Random", random_manager},
        {"Nearest neighbor (end)", nn_end_manager},
        {"Nearest neighbor (any position)", nn_any_manager},
        {"Greedy cycle", greedy_manager},
    }, plot_path(1, dataset));
}

void run_task2(int dataset) {
    ResultManager nn_regret, nn_weighted, cycle_regret, cycle_weighted;

    run_and_save(nn_regret, dataset, 2, "nn-regret", task2_nn_anywhere_regret);
    // the weighted solvers have extra defaulted parameters, so they are wrapped in lambdas
    run_and_save(nn_weighted, dataset, 2, "nn-weighted",
                 [](ResultManager& r) { task2_nn_anywhere_weighted(r); });
    run_and_save(cycle_regret, dataset, 2, "cycle-regret", task2_greedy_cycle_regret);
    run_and_save(cycle_weighted, dataset, 2, "cycle-weighted",
                 [](ResultManager& r) { task2_greedy_cycle_weighted(r); });

    write_local_search_report(output_path(2, "report", dataset), {
        {"Nearest neighbor (regret)", &nn_regret},
        {"Nearest neighbor (weighted)", &nn_weighted},
        {"Greedy cycle (regret)", &cycle_regret},
        {"Greedy cycle (weighted)", &cycle_weighted},
    });

    generate_dataset_plot(dataset, {
        {"Nearest neighbor (regret)", nn_regret},
        {"Nearest neighbor (weighted)", nn_weighted},
        {"Greedy cycle (regret)", cycle_regret},
        {"Greedy cycle (weighted)", cycle_weighted},
    }, plot_path(2, dataset));
}

// Task 3: all 8 combinations of steepest/greedy x node/edge exchange x random/greedy start.
void run_task3(int dataset) {
    std::vector<std::unique_ptr<ResultManager>> results;
    std::vector<std::string> names;  // reserved up front: plot_methods keeps pointers into it
    std::vector<std::pair<std::string, const ResultManager*>> report_methods;
    std::vector<PlotMethod> plot_methods;
    names.reserve(8);
    report_methods.reserve(8);
    plot_methods.reserve(8);

    for (bool steepest : {false, true}) {
        for (bool use_edges : {false, true}) {
            for (bool greedy_start : {false, true}) {
                auto result = std::make_unique<ResultManager>();
                result->dataset = dataset;
                run_local_search(*result, steepest, use_edges, greedy_start);

                names.push_back(local_search_method_name(steepest, use_edges, greedy_start));
                const std::string description = std::string(steepest ? "steepest" : "greedy") + "-" +
                                                (use_edges ? "edge" : "node") + "-" +
                                                (greedy_start ? "greedy-start" : "random-start");
                result->save_best_solution_to_file(output_path(3, "solution", dataset, description),
                                                   kSolutionSize, names.back());

                report_methods.emplace_back(names.back(), result.get());
                plot_methods.push_back({names.back().c_str(), *result});
                results.push_back(std::move(result));
            }
        }
    }

    write_local_search_report(output_path(3, "report", dataset), report_methods);
    generate_dataset_plot(dataset, plot_methods, plot_path(3, dataset));
}

// Task 4: steepest local search (edge exchange + inter-route exchange) with candidate moves.
// Baseline: the unmodified Task 3 steepest local search with random starts.
void run_task4(int dataset) {
    // the same random starting solutions for all methods (paired comparison)
    const auto starts = task4_make_random_starts(kRepetitions, 20260 + dataset);

    std::vector<std::unique_ptr<ResultManager>> results;
    std::vector<std::string> names;  // reserved up front: plot_methods keeps pointers into it
    std::vector<std::pair<std::string, const ResultManager*>> report_methods;
    std::vector<std::pair<std::string, Task4Stats>> stats;
    std::vector<PlotMethod> plot_methods;
    constexpr int kMaxMethods = 8;
    names.reserve(kMaxMethods);
    report_methods.reserve(kMaxMethods);
    stats.reserve(kMaxMethods);
    plot_methods.reserve(kMaxMethods);

    auto run_method = [&](const std::string& method_name, const std::string& description,
                          const std::function<Task4Stats(ResultManager&)>& runner) {
        auto result = std::make_unique<ResultManager>();
        result->dataset = dataset;
        Task4Stats method_stats = runner(*result);
        result->save_best_solution_to_file(output_path(4, "solution", dataset, description),
                                           kSolutionSize, method_name);

        names.push_back(method_name);
        report_methods.emplace_back(names.back(), result.get());
        stats.emplace_back(names.back(), method_stats);
        plot_methods.push_back({names.back().c_str(), *result});
        results.push_back(std::move(result));
    };

    run_method("Steepest + edge exchange + random start (baseline)", "steepest-baseline",
               [&](ResultManager& r) { return task4_steepest_baseline(r, starts); });
    for (int k : {5, 10, 15, 20}) {
        run_method("Steepest + candidate moves (k = " + std::to_string(k) + ") + random start",
                   "steepest-candidates-k" + std::to_string(k),
                   [&, k](ResultManager& r) { return task4_steepest_candidates(r, starts, k); });
    }

    write_local_search_report(output_path(4, "report", dataset), report_methods);
    write_task4_stats(output_path(4, "report", dataset, "moves"), stats);
    generate_dataset_plot(dataset, plot_methods, plot_path(4, dataset));
}

// Task 5: steepest local search with a persistent list of improving move deltas.
void run_task5(int dataset) {
    const auto starts = task4_make_random_starts(kRepetitions, 20260 + dataset);
    std::vector<std::unique_ptr<ResultManager>> results;
    std::vector<std::string> names;
    std::vector<std::pair<std::string, const ResultManager*>> report_methods;
    std::vector<std::pair<std::string, Task5Stats>> stats;
    std::vector<PlotMethod> plot_methods;
    names.reserve(2);
    report_methods.reserve(2);
    stats.reserve(2);
    plot_methods.reserve(2);

    auto run_method = [&](const std::string& method_name, const std::string& description,
                          const std::function<void(ResultManager&, Task5Stats&)>& runner) {
        auto result = std::make_unique<ResultManager>();
        result->dataset = dataset;
        Task5Stats method_stats;
        runner(*result, method_stats);
        result->save_best_solution_to_file(output_path(5, "solution", dataset, description),
                                           kSolutionSize, method_name);
        names.push_back(method_name);
        report_methods.emplace_back(names.back(), result.get());
        stats.emplace_back(names.back(), method_stats);
        plot_methods.push_back({names.back().c_str(), *result});
        results.push_back(std::move(result));
    };

    run_method("Steepest + edge exchange + random start (baseline)", "steepest-baseline",
               [&](ResultManager& result, Task5Stats& method_stats) {
                   const Task4Stats baseline = task4_steepest_baseline(result, starts);
                   method_stats.runs = baseline.runs;
                   method_stats.evaluated_moves = baseline.evaluated_moves;
                   method_stats.applied_moves = baseline.applied_moves;
               });
    run_method("Steepest + improving move list + random start", "steepest-improving-list",
               [&](ResultManager& result, Task5Stats& method_stats) {
                   method_stats = task5_improving_list(result, starts);
               });

    write_local_search_report(output_path(5, "report", dataset), report_methods);
    write_task5_stats(output_path(5, "report", dataset, "moves"), stats);
    generate_dataset_plot(dataset, plot_methods, plot_path(5, dataset));
}

// Task 5 (optional): list of improving moves combined with candidate moves.
// Separate experiment, files: task5-*-<dataset>-combined.*
void run_task5_combined(int dataset) {
    const auto starts = task4_make_random_starts(kRepetitions, 20260 + dataset);
    std::vector<std::unique_ptr<ResultManager>> results;
    std::vector<std::string> names;
    std::vector<std::pair<std::string, const ResultManager*>> report_methods;
    std::vector<std::pair<std::string, Task5Stats>> stats;
    std::vector<PlotMethod> plot_methods;
    constexpr int kMaxMethods = 8;
    names.reserve(kMaxMethods);
    report_methods.reserve(kMaxMethods);
    stats.reserve(kMaxMethods);
    plot_methods.reserve(kMaxMethods);

    auto run_method = [&](const std::string& method_name, const std::string& description,
                          const std::function<void(ResultManager&, Task5Stats&)>& runner) {
        auto result = std::make_unique<ResultManager>();
        result->dataset = dataset;
        Task5Stats method_stats;
        runner(*result, method_stats);
        result->save_best_solution_to_file(
            output_path(5, "solution", dataset, "combined-" + description), kSolutionSize,
            method_name);
        names.push_back(method_name);
        report_methods.emplace_back(names.back(), result.get());
        stats.emplace_back(names.back(), method_stats);
        plot_methods.push_back({names.back().c_str(), *result});
        results.push_back(std::move(result));
    };

    run_method("Steepest + edge exchange + random start (baseline)", "steepest-baseline",
               [&](ResultManager& result, Task5Stats& s) {
                   const Task4Stats b = task4_steepest_baseline(result, starts);
                   s.runs = b.runs;
                   s.evaluated_moves = b.evaluated_moves;
                   s.applied_moves = b.applied_moves;
               });
    run_method("Steepest + improving move list + random start", "improving-list",
               [&](ResultManager& result, Task5Stats& s) {
                   s = task5_improving_list(result, starts);
               });
    for (int k : {5, 10, 15, 20}) {
        run_method("Steepest + improving move list + candidate moves (k = " +
                       std::to_string(k) + ") + random start",
                   "improving-list-candidates-k" + std::to_string(k),
                   [&, k](ResultManager& result, Task5Stats& s) {
                       s = task5_improving_list_candidates(result, starts, k);
                   });
    }

    write_local_search_report(output_path(5, "report", dataset, "combined"), report_methods);
    write_task5_stats(output_path(5, "report", dataset, "combined-moves"), stats);
    generate_dataset_plot(dataset, plot_methods,
                          output_path(5, "plot", dataset, "combined", "svg"));
}

}  // namespace

int main(int argc, char** argv) {
    read_distance_matrix();
    for (int task = 1; task <= 5; ++task) {
        std::filesystem::create_directories(kOutputDir + "task" + std::to_string(task));
    }

    for (int dataset = 0; dataset < 2; ++dataset) {
        std::cout << "\n--- Dataset " << dataset_name(dataset) << " ---" << std::endl;
        const bool only_task5 = argc > 1 && std::string(argv[1]) == "--task5";
        if (!only_task5) {
            run_task1(dataset);
            run_task2(dataset);
            run_task3(dataset);
            run_task4(dataset);
        }
        run_task5(dataset);
        run_task5_combined(dataset);
    }
}