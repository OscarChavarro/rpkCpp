/* bsdf_methods.h: BSDF methods */

#ifndef _BSDF_METHODS_H_
#define _BSDF_METHODS_H_

#include <cstdio>

#include "common/color.h"
#include "material/xxdf.h"
#include "material/hit.h"

/**
BSDF methods: every kind of BSDF needs to have these functions implemented
*/
class BSDF_METHODS {
  public:
    // Returns the scattered power (diffuse/glossy/specular
    // reflectance and/or transmittance) according to flags
    COLOR (*ScatteredPower)(void *data, RayHit *hit, Vector3D *in, BSDFFLAGS flags);

    int (*IsTextured)(void *data);

    // Returns the index of refraction
    void (*IndexOfRefraction)(void *data, RefractionIndex *index);

    // void *incomingBsdf should be BSDF *incomingBsdf
    COLOR (*Eval)(void *data, RayHit *hit, void *inBsdf, void *outBsdf, Vector3D *in, Vector3D *out, BSDFFLAGS flags);

    Vector3D (*Sample)(void *data, RayHit *hit,
                       void *inBsdf, void *outBsdf,
                       Vector3D *in,
                       int doRussianRoulette, BSDFFLAGS flags,
                       double x_1, double x_2,
                       double *pdf);


    void (*EvalPdf)(void *data, RayHit *hit,
                    void *inBsdf, void *outBsdf,
                    Vector3D *in, Vector3D *out,
                    BSDFFLAGS flags,
                    double *pdf, double *pdfRR);

    // Constructs shading frame at hit point. Returns TRUE if successful and
    // FALSE if not. X and Y may be null pointers
    int (*ShadingFrame)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);
};

#endif
