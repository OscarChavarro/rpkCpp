#ifndef __SPLITBSDF__
#define __SPLITBSDF__

#include "material/texture.h"
#include "material/bsdf.h"
#include "material/brdf.h"
#include "material/btdf.h"

class SPLIT_BSDF {
  public:
    BRDF *brdf;
    BTDF *btdf;
    TEXTURE *texture;
};

extern BSDF_METHODS GLOBAL_scene_splitBsdfMethods;

extern SPLIT_BSDF *
splitBsdfCreate(BRDF *brdf, BTDF *btdf, TEXTURE *texture);

#endif