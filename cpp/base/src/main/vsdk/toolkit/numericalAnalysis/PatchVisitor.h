#ifndef PATCH_VISITOR__
#define PATCH_VISITOR__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class PatchVisitor {
  private:
    static int getNumberOfSamples(Patch *patch);

  public:
    static ColorRgbMutable averageNormalAlbedo(Patch *patch, char components);
    static ColorRgbMutable averageEmittance(Patch *patch, char components);
};

#endif
