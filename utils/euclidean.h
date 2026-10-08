#include<cmath>
#include<cstdlib>

int calculate_euclidean_distance(int x1, int y1, int x2, int y2) {
    return round(std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2)));
}