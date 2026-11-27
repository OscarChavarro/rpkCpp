#include "vsdk/toolkit/skin/MinMaxBox.h"

void
MinMaxBox::updateFromBoundingBox(const RpkAxisAlignedBoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == NULL ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}
