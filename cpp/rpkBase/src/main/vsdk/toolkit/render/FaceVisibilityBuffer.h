#ifndef FACE_VISIBILITY_BUFFER__
#define FACE_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/sgl/SglVisibilityBuffer.h"

/** A visible face in the scene's current face table. The default is background. */
class SglFaceIndex {
  private:
    int index;

  public:
    explicit SglFaceIndex(int faceIndex = -1) : index(faceIndex) {}

    bool isBackground() const { return index < 0; }
    int getIndex() const { return index; }
};

using FaceVisibilityBuffer = SglVisibilityBuffer<SglFaceIndex>;

#endif
