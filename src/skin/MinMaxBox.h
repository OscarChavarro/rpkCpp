#ifndef __MIN_MAX_BOX__
#define __MIN_MAX_BOX__

#include "common/Ray.h"
#include "skin/BoundingBox.h"

class MinMaxBox {
  private:
    BoundingBox *boundingBox;

  public:
    explicit MinMaxBox(const BoundingBox *sourceBoundingBox);
    ~MinMaxBox();

    MinMaxBox(const MinMaxBox &) = delete;
    MinMaxBox &operator=(const MinMaxBox &) = delete;

    void updateFromBoundingBox(const BoundingBox *sourceBoundingBox) const;

    bool intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const;

    bool intersectingSegment(const Ray *ray, float *tMin, float *tMax) const;
};

#endif
