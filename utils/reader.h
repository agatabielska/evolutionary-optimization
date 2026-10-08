#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "euclidean.h"

inline std::vector<std::string> FILE_PATHS = {"./data/TSPA.csv", "./data/TSPB.csv"};

inline int DISTANCE_MATRIX[200][200][2];
inline int NODE_COST[200][2];

inline void read_distance_matrix() {
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < 200; j++) {
            DISTANCE_MATRIX[i][j][0] = -1;
            DISTANCE_MATRIX[i][j][1] = -1;
        }
    }

    for (int k = 0; k < FILE_PATHS.size(); k++) {

        std::string file_path;
        file_path = FILE_PATHS[k];

        std::vector<std::pair<int, int>> coordinates;

        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << file_path << std::endl;
            continue;
        }

        std::string line;
        for (int i = 0; i < 200 && std::getline(file, line); i++) {
            std::istringstream iss(line);
            int x1, y1, cost;
            char sep1, sep2;
            if (!(iss >> x1 >> sep1 >> y1 >> sep2 >> cost)) {
                std::cerr << "Error reading line: " << line << std::endl;
                continue;
            }
            coordinates.push_back({x1, y1});
            NODE_COST[i][k] = cost;
        }
        file.close();

        for (int i = 0; i < coordinates.size(); i++) {
            for (int j = 0; j < coordinates.size(); j++) {
                if (i != j) {
                    DISTANCE_MATRIX[i][j][k] = calculate_euclidean_distance(coordinates[i].first, coordinates[i].second, coordinates[j].first, coordinates[j].second);
                } else {
                    DISTANCE_MATRIX[i][j][k] = 0;
                }
            }
        }

    }
}