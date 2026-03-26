#ifndef __MIN_MAX_BOX__
#define __MIN_MAX_BOX__

#include "common/Ray.h"
#include "skin/BoundingBox.h"

class MinMaxBox {
  private:
    BoundingBox boundingBox;

  public:
    inline explicit MinMaxBox(const BoundingBox *sourceBoundingBox);
    inline ~MinMaxBox();

    inline MinMaxBox(const MinMaxBox &) = delete;
    inline MinMaxBox &operator=(const MinMaxBox &) = delete;

    void updateFromBoundingBox(const BoundingBox *sourceBoundingBox);
    bool intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const;
    bool intersectingSegment(const Ray *ray, float *tMin, float *tMax) const;
};

MinMaxBox::MinMaxBox(const BoundingBox *sourceBoundingBox):
        boundingBox()
{
    if ( sourceBoundingBox != nullptr ) {
        boundingBox.copyFrom(sourceBoundingBox);
    }
}

MinMaxBox::~MinMaxBox() {
}

#endif
