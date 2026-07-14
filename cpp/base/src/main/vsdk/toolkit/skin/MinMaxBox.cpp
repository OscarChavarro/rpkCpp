#include "vsdk/toolkit/skin/MinMaxBox.h"

void
MinMaxBox::updateFromBoundingBox(const AxisAlignedBoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == nullptr ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}
