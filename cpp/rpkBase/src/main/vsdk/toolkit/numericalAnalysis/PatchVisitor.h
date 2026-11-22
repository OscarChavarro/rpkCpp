#ifndef PATCH_VISITOR__
#define PATCH_VISITOR__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class PatchVisitor {
  private:
    static int getNumberOfSamples(const Patch *patch);

  public:
    static ColorRgbMutable averageNormalAlbedo(const Patch *patch, char components);
    static ColorRgbMutable averageEmittance(const Patch *patch, char components);
};

#endif
