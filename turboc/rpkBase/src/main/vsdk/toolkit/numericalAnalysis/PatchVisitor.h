#ifndef __PATCH_VISITOR__
#define __PATCH_VISITOR__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class PatchVisitor {
  private:
    static int getNumberOfSamples(const Patch *patch);

  public:
    static ColorRgb averageNormalAlbedo(const Patch *patch, char components);
    static ColorRgb averageEmittance(const Patch *patch, char components);
};

#endif
