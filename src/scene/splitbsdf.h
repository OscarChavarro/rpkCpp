
/* **** Split BSDF : the most commonly used BSDF consisting of
   one brdf and one btdf **** */

#ifndef _SPLITBSDF_H_
#define _SPLITBSDF_H_

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

extern SPLIT_BSDF *SplitBSDFCreate(BRDF *brdf, BTDF *btdf, TEXTURE *texture);
extern BSDF_METHODS GLOBAL_scene_splitBsdfMethods;
extern void SplitBsdfPrint(FILE *out, SPLIT_BSDF *bsdf);

#endif