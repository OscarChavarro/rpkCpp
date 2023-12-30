/* bsdf.h : A simple combination of brdf and btdf.
 *    Handles evaluation and sampling and also
 *    functions that relate to brdf or btdf like reflectance etc. 
 *
 *  All new code should be using BSDF's since this is more general.
 *  You should NEVER access brdf or btdf data directly from a
 *  bsdf. Since new models may not make the same distinction for
 *  light scattering.
 */

#ifndef _BSDF_H_
#define _BSDF_H_

#include "common/linealAlgebra/vectorMacros.h"
#include "material/xxdf.h"
#include "material/bsdf_methods.h"

class BSDF {
  public:
    void *data;
    BSDF_METHODS *methods;
};

/**
Creates a BSDF instance with given data and methods
*/
extern BSDF *BsdfCreate(void *data, BSDF_METHODS *methods);

/* disposes of the memory occupied by the BSDF instance */
extern void BsdfDestroy(BSDF *bsdf);

/* *** SCATTERED POWER *** */

extern COLOR BsdfScatteredPower(BSDF *bsdf, HITREC *hit, Vector3D *dir, BSDFFLAGS flags);

/* Returns the reflectance of hte BSDF according to the flags */
# define BsdfReflectance(bsdf, hit, dir, xxflags) BsdfScatteredPower(bsdf, hit, dir, SETBRDFFLAGS(xxflags))
# define BsdfSpecularReflectance(bsdf, hit, dir) BsdfReflectance((bsdf), hit, dir, SPECULAR_COMPONENT)

/* Returns the transmittance of the BSDF */
# define BsdfTransmittance(bsdf, hit, dir, xxflags) BsdfScatteredPower(bsdf, hit, dir, SETBTDFFLAGS(xxflags))
#define BsdfSpecularTransmittance(bsdf, hit, dir) BsdfTransmittance((bsdf), hit, dir, SPECULAR_COMPONENT)

/* Returns the index of refraction of the BSDF */
extern void BsdfIndexOfRefraction(BSDF *bsdf, REFRACTIONINDEX *index);

/* Computes a shading frame at the given hit point. The Z axis of this frame is
 * the shading normal, The X axis is in the tangent plane on the surface at the
 * hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
 * is perpendicular to X and Z. X and Y may be null pointers. In this case,
 * only the shading normal is returned, avoiding computation of the X and 
 * Y axis if possible). 
 * Note: also edf's can have a routine for computing the shading frame. If a
 * material has both an edf and a bsdf, the shading frame shall of course
 * be the same. 
 * This routine returns TRUE if a shading frame could be constructed and FALSE if
 * not. In the latter case, a default frame needs to be used (not computed by this
 * routine - MaterialShadingFrame() in material.[ch] constructs such a frame if
 * needed) */
extern int BsdfShadingFrame(BSDF *bsdf, HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

/************* BSDF Evaluation functions ****************/

/* All components of the Bsdf
 *
 * Vector directions :
 *
 * in : from patch !!
 * out : from patch
 * hit->normal : leaving from patch, on the incoming side.
 *          So in . hit->normal > 0 !!!
 */
extern COLOR BsdfEval(BSDF *bsdf, HITREC *hit, BSDF *inBsdf, BSDF *outBsdf, Vector3D *in, Vector3D *out, BSDFFLAGS flags);


/*
 * BsdfEvalComponents :
 * Evaluates all requested components of the BSDF separately and
 * stores the result in 'colArray'.
 * Total evaluation is returned.
 */
extern COLOR BsdfEvalComponents(BSDF *bsdf, HITREC *hit, BSDF *inBsdf,
                                BSDF *outBsdf, Vector3D *in, Vector3D *out,
                                BSDFFLAGS flags,
                                COLOR *colArray);


/* Sampling routines, parameters as in evaluation, except that two
   random numbers x_1 and x_2 are needed (2D sampling process) */
extern Vector3D BsdfSample(BSDF *bsdf, HITREC *hit,
                           BSDF *inBsdf, BSDF *outBsdf,
                           Vector3D *in,
                           int doRussianRoulette,
                           BSDFFLAGS flags, double x_1, double x_2,
                           double *pdf);

extern void BsdfEvalPdf(BSDF *bsdf, HITREC *hit,
                        BSDF *inBsdf, BSDF *outBsdf,
                        Vector3D *in, Vector3D *out,
                        BSDFFLAGS flags,
                        double *pdf, double *pdfRR);

extern int BsdfIsTextured(BSDF *bsdf);

#endif
