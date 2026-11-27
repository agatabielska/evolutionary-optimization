#pragma once

#include "reader.h"
#include "result_manager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
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

// Draws the best solution of every method as one panel (two panels per row) and saves
// the whole picture to `output_path` (an .svg file; missing directories are created).
inline void generate_dataset_plot(int dataset, const std::vector<PlotMethod>& methods,
                                  const std::string& output_path) {
    const std::filesystem::path parent = std::filesystem::path(output_path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

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
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error opening SVG plot: " << output_path << '\n';
        return;
    }

    const int columns = 2;
    const int rows = static_cast<int>((methods.size() + columns - 1) / columns);
    const int canvas_height = std::max(1, rows) * panel_height;
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1400\" height=\""
         << canvas_height << "\" viewBox=\"0 0 1400 " << canvas_height << "\">\n";
    file << "<rect width=\"100%\" height=\"100%\" fill=\"#eeeeee\"/>\n";
    for (size_t i = 0; i < methods.size(); ++i) {
        write_plot_panel(file, methods[i], static_cast<int>(i % columns) * panel_width,
                         static_cast<int>(i / columns) * panel_height, panel_width,
                         panel_height, min_x, max_x, min_y, max_y, min_cost,
                         max_cost);
    }

    file << "</svg>\n";
    std::cout << "Saved plot: " << output_path << '\n';
}

struct ScatterPlotPoint {
    double x;
    double y;
};

inline std::string escape_svg_text(const std::string& text) {
    std::string escaped;
    for (char character : text) {
        if (character == '&') escaped += "&amp;";
        else if (character == '<') escaped += "&lt;";
        else if (character == '>') escaped += "&gt;";
        else if (character == '"') escaped += "&quot;";
        else escaped += character;
    }
    return escaped;
}

inline void generate_scatter_plot(const std::vector<ScatterPlotPoint>& points,
                                  const std::string& title,
                                  const std::string& x_label,
                                  const std::string& y_label,
                                  double pearson_r,
                                  const std::string& output_path) {
    constexpr int width = 900;
    constexpr int height = 650;
    constexpr double left = 95.0;
    constexpr double right = 35.0;
    constexpr double top = 75.0;
    constexpr double bottom = 85.0;
    const double plot_width = width - left - right;
    const double plot_height = height - top - bottom;

    const std::filesystem::path parent =
        std::filesystem::path(output_path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);

    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error opening scatter plot: " << output_path << '\n';
        return;
    }

    double min_x = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    for (const ScatterPlotPoint& point : points) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }
    const double x_padding = max_x == min_x ? 1.0 : (max_x - min_x) * 0.04;
    const double y_padding = max_y == min_y ? 1.0 : (max_y - min_y) * 0.08;
    min_x -= x_padding;
    max_x += x_padding;
    min_y -= y_padding;
    max_y += y_padding;

    auto x_position = [&](double value) {
        return left + (value - min_x) * plot_width / (max_x - min_x);
    };
    auto y_position = [&](double value) {
        return top + plot_height -
               (value - min_y) * plot_height / (max_y - min_y);
    };

    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
         << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << ' '
         << height << "\">\n"
         << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
         << "<text x=\"" << width / 2 << "\" y=\"35\" text-anchor=\"middle\" "
            "font-family=\"sans-serif\" font-size=\"20\">"
         << escape_svg_text(title) << " (Pearson r=";
    if (std::isfinite(pearson_r)) {
        file << std::fixed << std::setprecision(4) << pearson_r;
    } else {
        file << "NA";
    }
    file << ")</text>\n";

    for (int tick = 0; tick <= 5; ++tick) {
        const double fraction = tick / 5.0;
        const double x = left + fraction * plot_width;
        const double y = top + fraction * plot_height;
        const double x_value = min_x + fraction * (max_x - min_x);
        const double y_value = max_y - fraction * (max_y - min_y);
        file << "<line x1=\"" << x << "\" y1=\"" << top << "\" x2=\""
             << x << "\" y2=\"" << top + plot_height
             << "\" stroke=\"#e5e5e5\"/>\n"
             << "<line x1=\"" << left << "\" y1=\"" << y << "\" x2=\""
             << left + plot_width << "\" y2=\"" << y
             << "\" stroke=\"#e5e5e5\"/>\n"
             << "<text x=\"" << x << "\" y=\"" << top + plot_height + 22
             << "\" text-anchor=\"middle\" font-family=\"sans-serif\" "
                "font-size=\"12\">"
             << std::fixed << std::setprecision(1) << x_value << "</text>\n"
             << "<text x=\"" << left - 10 << "\" y=\"" << y + 4
             << "\" text-anchor=\"end\" font-family=\"sans-serif\" "
                "font-size=\"12\">"
             << std::fixed << std::setprecision(1) << y_value << "</text>\n";
    }
    file << "<line x1=\"" << left << "\" y1=\"" << top + plot_height
         << "\" x2=\"" << left + plot_width << "\" y2=\"" << top + plot_height
         << "\" stroke=\"#333\"/>\n"
         << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\""
         << left << "\" y2=\"" << top + plot_height
         << "\" stroke=\"#333\"/>\n"
         << "<text x=\"" << left + plot_width / 2 << "\" y=\"" << height - 25
         << "\" text-anchor=\"middle\" font-family=\"sans-serif\" "
            "font-size=\"15\">"
         << escape_svg_text(x_label) << "</text>\n"
         << "<text x=\"22\" y=\"" << top + plot_height / 2
         << "\" transform=\"rotate(-90 22 " << top + plot_height / 2
         << ")\" text-anchor=\"middle\" font-family=\"sans-serif\" "
            "font-size=\"15\">"
         << escape_svg_text(y_label) << "</text>\n";

    for (const ScatterPlotPoint& point : points) {
        file << "<circle cx=\"" << x_position(point.x) << "\" cy=\""
             << y_position(point.y)
             << "\" r=\"2.5\" fill=\"#2864a5\" fill-opacity=\"0.55\"/>\n";
    }
    file << "</svg>\n";
    std::cout << "Saved plot: " << output_path << '\n';
}