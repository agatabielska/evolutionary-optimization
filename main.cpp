#include "utils/reader.h"
#include <iostream>

int main() {
    std::cout << "--- Testing Utils ---" << std::endl;
    
    // Call the reader function
    read_distance_matrix();

    // Verify distance matrix and node costs for some elements
    std::cout << "\n--- Verification ---" << std::endl;
    std::cout << "Distance from Node 0 to Node 1 on TSPA: " << DISTANCE_MATRIX[0][1][0] << std::endl;
    std::cout << "Distance from Node 0 to Node 1 on TSPB: " << DISTANCE_MATRIX[0][1][1] << std::endl;
    std::cout << "Cost of Node 0 on TSPA: " << NODE_COST[0][0] << std::endl;
    std::cout << "Cost of Node 0 on TSPB: " << NODE_COST[0][1] << std::endl;

    return 0;
}