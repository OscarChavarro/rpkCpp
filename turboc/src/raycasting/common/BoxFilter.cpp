#include "raycasting/common/BoxFilter.h"

BoxFilter::BoxFilter() {
}

BoxFilter::~BoxFilter() {
}

void
BoxFilter::sample(double * /*dx*/, double * /*dy*/) {
    // Box filter means not changing original (dx, dy) point.
}
