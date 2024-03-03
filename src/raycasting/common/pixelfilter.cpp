#include <cmath>

#include "raycasting/common/pixelfilter.h"

// super class
pixelFilter::pixelFilter() {}

pixelFilter::~pixelFilter() {}

void pixelFilter::sample(double *xi1, double *xi2) {}


// BOX filter
BoxFilter::BoxFilter() {};

BoxFilter::~BoxFilter() {};

void BoxFilter::sample(double *, double *) {}


// TENT filter
TentFilter::TentFilter() {};

TentFilter::~TentFilter() {};

void TentFilter::sample(double *xi1, double *xi2) {
    double x = fabs(2 * (*xi1) - 1.);
    double sx = *xi1 < .5 ? -1 : +1;
    double y = fabs(2 * (*xi2) - 1.);
    double sy = *xi2 < .5 ? -1 : +1;

    if ( x > y ) {
        *xi1 = (sx * sqrt(x)) + .5;
        *xi2 = (*xi1 * y) + .5;
    } else {
        *xi2 = (sy * sqrt(y)) + .5;
        *xi1 = (*xi2 * x) + .5;
    }
}


// GAUSSIAN/NORMAL filter
NormalFilter::NormalFilter(double s, double d) {
    sigma = s;
    dist = d;
};

NormalFilter::~NormalFilter() {};

void
NormalFilter::sample(double *xi1, double *xi2) {
    double s = dist / sigma;
    double r = *xi1 * std::exp(s * s * (-0.5));
    double a = *xi2;

    *xi1 = sigma * (std::sqrt(-2.0 * std::log(r)) * std::cos(2.0 * M_PI * a)) + 0.5;
    *xi2 = sigma * (std::sqrt(-2.0 * std::log(r)) * std::sin(2.0 * M_PI * a)) + 0.5;
}
