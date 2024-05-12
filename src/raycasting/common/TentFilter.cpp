#include <cmath>

#include "java/lang/Math.h"
#include "raycasting/common/TentFilter.h"

TentFilter::TentFilter() {};

TentFilter::~TentFilter() {};

void
TentFilter::sample(double *xi1, double *xi2) {
    double x = java::Math::abs(2 * (*xi1) - 1.0);
    double sx = *xi1 < 0.5 ? -1 : +1;
    double y = java::Math::abs(2 * (*xi2) - 1.0);
    double sy = *xi2 < 0.5 ? -1 : +1;

    if ( x > y ) {
        *xi1 = (sx * java::Math::sqrt(x)) + 0.5;
        *xi2 = (*xi1 * y) + 0.5;
    } else {
        *xi2 = (sy * java::Math::sqrt(y)) + 0.5;
        *xi1 = (*xi2 * x) + 0.5;
    }
}
