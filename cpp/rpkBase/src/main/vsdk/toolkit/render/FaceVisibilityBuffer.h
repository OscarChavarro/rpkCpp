#ifndef __FACE_VISIBILITY_BUFFER__
#define __FACE_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer.h"

/** A visible face in the scene's current face table. The default is background. */
class FaceIndexSample {
  private:
    int index;

  public:
    explicit FaceIndexSample(int faceIndex = -1) : index(faceIndex) {}

    bool isBackground() const { return index < 0; }
    int getIndex() const { return index; }
};

// Instantiated once, in FaceVisibilityBuffer.cpp
extern template class PolygonVisibilityBuffer<FaceIndexSample>;
using FaceVisibilityBuffer = PolygonVisibilityBuffer<FaceIndexSample>;

#endif
