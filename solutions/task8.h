#pragma once

#include "task3.h"
#include "task4.h"
#include "task6.h"
#include "../utils/plotter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace global_convexity_detail {

using local_search_detail::solution_size;
using local_search_detail::total_nodes;

struct LocalOptimum {
    int objective = 0;
    std::vector<int> nodes;
    std::vector<int> edges;
};

struct ReferenceSolution {
    std::vector<int> cycle;
    std::string method;
    int objective = 0;
    std::string source;
};

struct Correlation {
    double pearson = std::numeric_limits<double>::quiet_NaN();
    double spearman = std::numeric_limits<double>::quiet_NaN();
    int points = 0;
};

inline int edge_id(int first, int second) {
    if (first > second) std::swap(first, second);
    return first * total_nodes + second;
}

inline std::vector<int> sorted_nodes(const std::vector<int>& cycle) {
    std::vector<int> nodes = cycle;
    std::sort(nodes.begin(), nodes.end());
    return nodes;
}

inline std::vector<int> sorted_edges(const std::vector<int>& cycle) {
    std::vector<int> edges;
    edges.reserve(cycle.size());
    for (size_t i = 0; i < cycle.size(); ++i) {
        edges.push_back(edge_id(cycle[i], cycle[(i + 1) % cycle.size()]));
    }
    std::sort(edges.begin(), edges.end());
    return edges;
}

inline int intersection_size(const std::vector<int>& first,
                             const std::vector<int>& second) {
    int count = 0;
    size_t i = 0;
    size_t j = 0;
    while (i < first.size() && j < second.size()) {
        if (first[i] == second[j]) {
            ++count;
            ++i;
            ++j;
        } else if (first[i] < second[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return count;
}

inline std::vector<int> parse_cycle(const std::string& line) {
    std::vector<int> cycle;
    std::stringstream stream(line);
    std::string value;
    while (std::getline(stream, value, ',')) {
        if (value.empty()) continue;
        cycle.push_back(std::stoi(value));
    }
    if (cycle.size() != solution_size) {
        throw std::runtime_error("Reference solution has an invalid cycle length.");
    }
    std::array<bool, total_nodes> seen{};
    for (int node : cycle) {
        if (node < 0 || node >= total_nodes || seen[node]) {
            throw std::runtime_error("Reference solution contains invalid or repeated nodes.");
        }
        seen[node] = true;
    }
    return cycle;
}

inline ReferenceSolution load_reference(int dataset) {
    const std::string instance = dataset == 0 ? "tspa" : "tspb";
    const std::string source =
        "calculated_solutions/task6/task6-solution-" + instance + ".txt";
    const task6_detail::PerturbConfig reference_config{2, 30, 10};
    const std::string method = task6_detail::perturb_name(reference_config);

    std::ifstream file(source);
    if (!file.is_open()) {
        throw std::runtime_error("Required stored ILS reference is missing: " + source);
    }

    const std::string marker = " | best cost ";
    const std::string prefix = method + marker;
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, prefix.size(), prefix) != 0) continue;
        const size_t objective_position = prefix.size();
        size_t parsed = 0;
        const int recorded_objective =
            std::stoi(line.substr(objective_position), &parsed);
        if (objective_position + parsed != line.size()) {
            throw std::runtime_error("Malformed ILS objective in " + source);
        }
        std::string cycle_line;
        if (!std::getline(file, cycle_line)) {
            throw std::runtime_error("Reference solution cycle is missing in " + source);
        }
        ReferenceSolution reference{
            parse_cycle(cycle_line), method, recorded_objective, source};
        const int computed = task4_detail::objective(reference.cycle, dataset);
        if (computed != recorded_objective) {
            throw std::runtime_error(
                "Stored reference objective does not match its cycle in " + source +
                " (entry: " + method + ")");
        }
        return reference;
    }
    throw std::runtime_error("Required ILS reference entry was not found in " + source +
                             ": " + method);
}

inline std::string number_or_na(double value) {
    if (!std::isfinite(value)) return "NA";
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << value;
    return stream.str();
}

inline double pearson(const std::vector<double>& x,
                      const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
    const double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
    double covariance = 0.0;
    double variance_x = 0.0;
    double variance_y = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        const double dx = x[i] - mean_x;
        const double dy = y[i] - mean_y;
        covariance += dx * dy;
        variance_x += dx * dx;
        variance_y += dy * dy;
    }
    if (variance_x == 0.0 || variance_y == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return covariance / std::sqrt(variance_x * variance_y);
}

inline std::vector<double> average_ranks(const std::vector<double>& values) {
    std::vector<size_t> order(values.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(),
                     [&](size_t left, size_t right) {
                         return values[left] < values[right];
                     });
    std::vector<double> ranks(values.size());
    size_t first = 0;
    while (first < order.size()) {
        size_t last = first + 1;
        while (last < order.size() &&
               values[order[last]] == values[order[first]]) {
            ++last;
        }
        const double rank = (static_cast<double>(first + 1) +
                             static_cast<double>(last)) /
                            2.0;
        for (size_t i = first; i < last; ++i) ranks[order[i]] = rank;
        first = last;
    }
    return ranks;
}

inline Correlation summarize(const std::vector<double>& objectives,
                            const std::vector<double>& similarities) {
    Correlation result;
    result.points = static_cast<int>(objectives.size());
    result.pearson = pearson(objectives, similarities);
    result.spearman =
        pearson(average_ranks(objectives), average_ranks(similarities));
    return result;
}

inline std::string unique_key(const LocalOptimum& optimum) {
    std::ostringstream key;
    for (int node : optimum.nodes) key << node << ',';
    key << '|';
    for (int edge : optimum.edges) key << edge << ',';
    return key.str();
}

inline void save_local_optima(const std::string& filename,
                              const std::string& instance, int count,
                              unsigned seed,
                              const std::vector<LocalOptimum>& optima) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not write local optima file: " + filename);
    }
    file << "{\n  \"instance\": \"" << instance << "\",\n"
         << "  \"count\": " << count << ",\n"
         << "  \"seed\": " << seed << ",\n"
         << "  \"objective_direction\": \"minimize\",\n"
         << "  \"optima\": [\n";
    for (size_t index = 0; index < optima.size(); ++index) {
        const LocalOptimum& optimum = optima[index];
        file << "    {\"objective\": " << optimum.objective << ", \"nodes\": [";
        for (size_t i = 0; i < optimum.nodes.size(); ++i) {
            if (i) file << ',';
            file << optimum.nodes[i];
        }
        file << "], \"edges\": [";
        for (size_t i = 0; i < optimum.edges.size(); ++i) {
            if (i) file << ',';
            const int first = optimum.edges[i] / total_nodes;
            const int second = optimum.edges[i] % total_nodes;
            file << '[' << first << ',' << second << ']';
        }
        file << "]}" << (index + 1 == optima.size() ? "\n" : ",\n");
    }
    file << "  ]\n}\n";
    if (!file) {
        throw std::runtime_error("Failed while writing local optima file: " + filename);
    }
}

inline void add_analysis(
    const std::string& instance, const std::string& version,
    const std::string& measure, const std::vector<double>& objectives,
    const std::vector<double>& similarities,
    const std::string& output_directory,
    std::vector<std::pair<std::string, Correlation>>& summary) {
    const Correlation correlation = summarize(objectives, similarities);
    std::vector<ScatterPlotPoint> points;
    points.reserve(objectives.size());
    for (size_t i = 0; i < objectives.size(); ++i) {
        points.push_back({objectives[i], similarities[i]});
    }
    const std::string title = instance + " - " + version + " - " + measure;
    const std::string filename =
        output_directory + "/" + instance + "_" + version + "_" + measure + ".svg";
    generate_scatter_plot(points, title, "Objective value (minimized)",
                          "Common " + measure, correlation.pearson, filename);
    summary.emplace_back(instance + "," + version + "," + measure,
                         correlation);
}

inline void run(int dataset, int count, unsigned base_seed,
                const std::string& output_directory) {
    if (count < 3) {
        throw std::invalid_argument(
            "Global convexity correlations require at least three runs.");
    }
    const std::string instance = dataset == 0 ? "TSPA" : "TSPB";
    const unsigned seed = base_seed + static_cast<unsigned>(dataset);
    const ReferenceSolution reference = load_reference(dataset);
    std::cout << "\nGlobal convexity: " << instance << " (N=" << count
              << ", seed=" << seed << ")\n"
              << "Reference: " << reference.source << " entry \"" << reference.method
              << "\" (objective " << reference.objective << ")\n";

    std::mt19937 generator(seed);
    std::vector<LocalOptimum> optima;
    optima.reserve(count);
    std::vector<int> objectives;
    objectives.reserve(count);
    for (int run_index = 0; run_index < count; ++run_index) {
        std::vector<int> cycle = local_search_detail::random_solution(generator);
        local_search_detail::improve(cycle, dataset, false, true, generator);
        const int objective = task4_detail::objective(cycle, dataset);
        optima.push_back({objective, sorted_nodes(cycle), sorted_edges(cycle)});
        objectives.push_back(objective);
        if ((run_index + 1) % 100 == 0 || run_index + 1 == count) {
            std::cout << "  generated " << run_index + 1 << '/' << count << '\n';
        }
    }

    const auto [minimum, maximum] =
        std::minmax_element(objectives.begin(), objectives.end());
    std::set<std::string> distinct;
    for (const LocalOptimum& optimum : optima) distinct.insert(unique_key(optimum));
    std::cout << "Objective range: " << *minimum << " .. " << *maximum << '\n'
              << "Distinct local optima: " << distinct.size() << " / " << count
              << " (duplicates retained)\n";

    const int best_index = static_cast<int>(
        std::min_element(objectives.begin(), objectives.end()) - objectives.begin());
    std::vector<double> average_edges(count, 0.0);
    std::vector<double> average_nodes(count, 0.0);
    long long total_pair_edges = 0;
    long long total_pair_nodes = 0;
    for (int first = 0; first < count; ++first) {
        for (int second = first + 1; second < count; ++second) {
            const int common_edges =
                intersection_size(optima[first].edges, optima[second].edges);
            const int common_nodes =
                intersection_size(optima[first].nodes, optima[second].nodes);
            average_edges[first] += common_edges;
            average_edges[second] += common_edges;
            average_nodes[first] += common_nodes;
            average_nodes[second] += common_nodes;
            total_pair_edges += common_edges;
            total_pair_nodes += common_nodes;
        }
    }
    for (int i = 0; i < count; ++i) {
        average_edges[i] /= count - 1;
        average_nodes[i] /= count - 1;
    }

    const double expected_random_edges =
        solution_size * (solution_size / static_cast<double>(total_nodes)) *
        ((solution_size - 1.0) / (total_nodes - 1.0)) *
        (2.0 / (solution_size - 1.0));
    const double expected_random_nodes =
        solution_size * solution_size / static_cast<double>(total_nodes);
    const double pair_count = count * (count - 1) / 2.0;
    std::cout << "Mean pair similarity (edges): "
              << total_pair_edges / pair_count << " (independent-random baseline "
              << expected_random_edges << ")\n"
              << "Mean pair similarity (nodes): "
              << total_pair_nodes / pair_count << " (independent-random baseline "
              << expected_random_nodes << ")\n"
              << "Best-of-1000 chart check: " << count - 1 << " points; best index "
              << best_index << " excluded: yes\n";

    std::vector<double> objective_values(objectives.begin(), objectives.end());
    std::vector<double> best_edges;
    std::vector<double> best_nodes;
    std::vector<double> best_objectives;
    std::vector<double> reference_edges;
    std::vector<double> reference_nodes;
    best_edges.reserve(count - 1);
    best_nodes.reserve(count - 1);
    best_objectives.reserve(count - 1);
    reference_edges.reserve(count);
    reference_nodes.reserve(count);
    const LocalOptimum best = optima[best_index];
    int identical_to_reference = 0;
    const LocalOptimum reference_features{
        reference.objective, sorted_nodes(reference.cycle), sorted_edges(reference.cycle)};
    for (int i = 0; i < count; ++i) {
        if (i != best_index) {
            best_objectives.push_back(objective_values[i]);
            best_edges.push_back(
                intersection_size(optima[i].edges, best.edges));
            best_nodes.push_back(
                intersection_size(optima[i].nodes, best.nodes));
        }
        reference_edges.push_back(
            intersection_size(optima[i].edges, reference_features.edges));
        reference_nodes.push_back(
            intersection_size(optima[i].nodes, reference_features.nodes));
        if (optima[i].nodes == reference_features.nodes &&
            optima[i].edges == reference_features.edges) {
            ++identical_to_reference;
        }
    }
    std::cout << "Local optima identical to reference: " << identical_to_reference
              << '\n';

    std::filesystem::create_directories(output_directory);
    const std::string data_file =
        output_directory + "/" + instance + "_local_optima.json";
    save_local_optima(data_file, instance, count, seed, optima);
    std::cout << "Saved local optima: " << data_file << '\n';

    std::vector<std::pair<std::string, Correlation>> summary;
    summary.reserve(6);
    const std::vector<std::pair<std::string, const std::vector<double>*>> measures = {
        {"edges", &average_edges}, {"nodes", &average_nodes}};
    for (const auto& [measure, average] : measures) {
        add_analysis(instance, "avg", measure, objective_values, *average,
                     output_directory, summary);
        add_analysis(instance, "best1000", measure, best_objectives,
                     measure == "edges" ? best_edges : best_nodes,
                     output_directory, summary);
        add_analysis(instance, "reference", measure, objective_values,
                     measure == "edges" ? reference_edges : reference_nodes,
                     output_directory, summary);
    }
    const std::string summary_file = output_directory + "/summary.csv";
    std::filesystem::create_directories(output_directory);
    std::ofstream csv;
    if (dataset == 0) csv.open(summary_file);
    else csv.open(summary_file, std::ios::app);
    if (!csv.is_open()) {
        throw std::runtime_error("Could not write summary table: " + summary_file);
    }
    if (dataset == 0) {
        csv << "instance,version,measure,pearson_r,spearman_r,points\n";
    }
    for (const auto& [key, correlation] : summary) {
        csv << instance << ',' << key.substr(instance.size() + 1) << ','
            << number_or_na(correlation.pearson) << ','
            << number_or_na(correlation.spearman) << ',' << correlation.points
            << '\n';
    }
    if (!csv) throw std::runtime_error("Failed while writing summary table: " + summary_file);
    if (dataset == 0) {
        std::cout << "Correlation summary\n"
                  << "instance,version,measure,Pearson r,Spearman rho,points,sign check\n";
    }
    for (const auto& [key, correlation] : summary) {
        const size_t start = instance.size() + 1;
        const std::string version_measure = key.substr(start);
        const size_t separator = version_measure.find(',');
        const std::string sign =
            !std::isfinite(correlation.pearson) ? "undefined"
            : correlation.pearson < 0.0               ? "expected"
            : correlation.pearson > 0.0               ? "opposite expected"
                                                       : "no sign";
        std::cout << instance << ',' << version_measure.substr(0, separator) << ','
                  << version_measure.substr(separator + 1) << ','
                  << number_or_na(correlation.pearson) << ','
                  << number_or_na(correlation.spearman) << ','
                  << correlation.points << ',' << sign << '\n';
    }
    std::cout << "Saved summary table: " << summary_file << '\n'
              << "Sign check: objective is minimized; negative correlation is expected.\n";
}

}  // namespace global_convexity_detail

inline void run_global_convexity(int dataset, int count, unsigned seed,
                                 const std::string& output_directory) {
    global_convexity_detail::run(dataset, count, seed, output_directory);
}
