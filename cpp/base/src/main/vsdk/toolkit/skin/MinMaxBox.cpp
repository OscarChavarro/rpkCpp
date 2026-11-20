#include "vsdk/toolkit/skin/MinMaxBox.h"

void
MinMaxBox::updateFromBoundingBox(const RpkAxisAlignedBoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == nullptr ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}
