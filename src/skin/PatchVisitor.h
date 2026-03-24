#ifndef RPK_PATCH_VISITOR_H
#define RPK_PATCH_VISITOR_H

#include "skin/Patch.h"

class PatchVisitor {
  private:
    static int getNumberOfSamples(Patch *patch);

  public:
    static ColorRgb averageNormalAlbedo(Patch *patch, char components);
    static ColorRgb averageEmittance(Patch *patch, char components);
};

#endif
