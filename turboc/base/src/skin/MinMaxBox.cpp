#include "skin/MinMaxBox.h"

void
MinMaxBox::updateFromBoundingBox(const BoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == NULL ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}
