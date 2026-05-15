#ifndef __PATCH_VISITOR__
#define __PATCH_VISITOR__

#include "environment/geometry/elements/Patch.h"

class PatchVisitor {
  private:
    static int getNumberOfSamples(Patch *patch);

  public:
    static ColorRgb averageNormalAlbedo(Patch *patch, char components);
    static ColorRgb averageEmittance(Patch *patch, char components);
};

#endif
