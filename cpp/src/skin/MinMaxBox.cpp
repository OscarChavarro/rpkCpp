#include "skin/MinMaxBox.h"

void
MinMaxBox::updateFromBoundingBox(const BoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == nullptr ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}
