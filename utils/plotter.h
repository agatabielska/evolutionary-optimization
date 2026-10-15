#pragma once

#include "reader.h"
#include "result_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

struct PlotMethod {
    const char* name;
    const ResultManager& result;
};

inline std::string cost_color(int cost, int min_cost, int max_cost) {
    const double range = std::max(1, max_cost - min_cost);
    const double normalized = (cost - min_cost) / range;
    const int red = static_cast<int>(255.0 * normalized);
    const int blue = static_cast<int>(255.0 * (1.0 - normalized));
    return "rgb(" + std::to_string(red) + ",80," + std::to_string(blue) + ")";
}

inline void write_plot_panel(std::ofstream& file, const PlotMethod& method,
                             int panel_x, int panel_y, int panel_width,
                             int panel_height, int min_x, int max_x,
                             int min_y, int max_y, int min_cost, int max_cost) {
    constexpr int padding = 55;
    const int dataset = method.result.dataset;
    const int plot_x = panel_x + padding;
    const int plot_y = panel_y + 35;
    const int plot_width = panel_width - padding - 20;
    const int plot_height = panel_height - 70;

    auto x_position = [&](int node) {
        return plot_x + (NODE_X[node][dataset] - min_x) * plot_width /
                             std::max(1, max_x - min_x);
    };
    auto y_position = [&](int node) {
        return plot_y + plot_height -
               (NODE_Y[node][dataset] - min_y) * plot_height /
                   std::max(1, max_y - min_y);
    };

    file << "<rect x=\"" << panel_x << "\" y=\"" << panel_y
         << "\" width=\"" << panel_width << "\" height=\"" << panel_height
         << "\" fill=\"white\" stroke=\"#cccccc\"/>\n";
    file << "<text x=\"" << panel_x + 12 << "\" y=\"" << panel_y + 23
         << "\" font-size=\"16\">" << method.name << " (cost "
         << method.result.best_cost << ")</text>\n";

    for (int i = 0; i < 100; ++i) {
        const int first = method.result.best_solution[i];
        const int second = method.result.best_solution[(i + 1) % 100];
        file << "<line x1=\"" << x_position(first) << "\" y1=\""
             << y_position(first) << "\" x2=\"" << x_position(second)
             << "\" y2=\"" << y_position(second)
             << "\" stroke=\"#333333\" stroke-width=\"1\"/>\n";
    }

    for (int node = 0; node < 200; ++node) {
        file << "<circle cx=\"" << x_position(node) << "\" cy=\""
             << y_position(node) << "\" r=\""
             << (std::find(method.result.best_solution,
                           method.result.best_solution + 100, node) !=
                         method.result.best_solution + 100
                     ? 4
                     : 2)
             << "\" fill=\""
             << cost_color(NODE_COST[node][method.result.dataset], min_cost,
                           max_cost)
             << "\"/>\n";
    }
}

inline void generate_dataset_plot(int dataset, const std::vector<PlotMethod>& methods) {
    std::filesystem::create_directories("calculated_solutions");

    int min_x = NODE_X[0][dataset], max_x = NODE_X[0][dataset];
    int min_y = NODE_Y[0][dataset], max_y = NODE_Y[0][dataset];
    int min_cost = NODE_COST[0][dataset], max_cost = min_cost;
    for (int node = 1; node < 200; ++node) {
        min_x = std::min(min_x, NODE_X[node][dataset]);
        max_x = std::max(max_x, NODE_X[node][dataset]);
        min_y = std::min(min_y, NODE_Y[node][dataset]);
        max_y = std::max(max_y, NODE_Y[node][dataset]);
        min_cost = std::min(min_cost, NODE_COST[node][dataset]);
        max_cost = std::max(max_cost, NODE_COST[node][dataset]);
    }

    const int panel_width = 700;
    const int panel_height = 520;
    std::ofstream file("calculated_solutions/" +
                       std::string(dataset == 0 ? "TSPA" : "TSPB") +
                       "_best_solutions.svg");
    if (!file.is_open()) {
        std::cerr << "Error opening SVG plot for dataset " << dataset << '\n';
        return;
    }

    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1400\" height=\"1040\" "
            "viewBox=\"0 0 1400 1040\">\n";
    file << "<rect width=\"100%\" height=\"100%\" fill=\"#eeeeee\"/>\n";
    for (size_t i = 0; i < methods.size(); ++i) {
        write_plot_panel(file, methods[i], static_cast<int>(i % 2) * panel_width,
                         static_cast<int>(i / 2) * panel_height, panel_width,
                         panel_height, min_x, max_x, min_y, max_y, min_cost,
                         max_cost);
    }
    file << "</svg>\n";
    std::cout << "Saved plot: calculated_solutions/"
              << (dataset == 0 ? "TSPA" : "TSPB") << "_best_solutions.svg\n";
}
