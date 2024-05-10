#include <cmath>

#include "raycasting/common/NormalFilter.h"

/**
GAUSSIAN/NORMAL filter
*/
NormalFilter::NormalFilter(double s, double d) {
    sigma = s;
    dist = d;
}

NormalFilter::~NormalFilter() {
}

void
NormalFilter::sample(double *xi1, double *xi2) {
    double s = dist / sigma;
    double r = *xi1 * std::exp(s * s * (-0.5));
    double a = *xi2;

    *xi1 = sigma * (std::sqrt(-2.0 * std::log(r)) * std::cos(2.0 * M_PI * a)) + 0.5;
    *xi2 = sigma * (std::sqrt(-2.0 * std::log(r)) * std::sin(2.0 * M_PI * a)) + 0.5;
}
