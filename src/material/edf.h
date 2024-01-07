/* edf.h: Emittance Distribution Functions: the seflemitted radiance
 * distriution of light sources. */

#ifndef _EDF_H_
#define _EDF_H_

#include <cstdio>

#include "material/edf_methods.h"
#include "material/xxdf.h"

class EDF {
  public:
    void *data;
    EDF_METHODS *methods;
};

/**
Creates a EDF instance with given data and methods.
*/
extern EDF *EdfCreate(void *data, EDF_METHODS *methods);

/* disposes of the memory occupied by the EDF instance */
extern void EdfDestroy(EDF *edf);

/* Print the edf data the to specified file pointer */
extern void EdfPrint(FILE *out, EDF *edf);

/* Returns the emittance (self-emitted radiant exitance) [W/m^2] of the EDF */
extern COLOR EdfEmittance(EDF *edf, HITREC *hit, XXDFFLAGS flags);

extern int EdfIsTextured(EDF *edf);

/* Evaluates the edf: return exitant radiance [W/m^2 sr] into the direction
 * out. If pdf is not null, the probability density of the direction is 
 * computed and returned in pdf. */
extern COLOR EdfEval(EDF *edf, HITREC *hit, Vector3D *out,
                     XXDFFLAGS flags, double *pdf);

/* samples a direction according to the specified edf and emission range determined
 * by flags. If emitted_radiance is not a null pointer, the emitted radiance along
 * the generated direction is returned. If pdf is not null, the probability density
 * of the generated direction is computed and returned in pdf. */
extern Vector3D EdfSample(EDF *edf, HITREC *hit, XXDFFLAGS flags,
                          double xi1, double xi2,
                          COLOR *emitted_radiance, double *pdf);

/* returns diffusely emitted radiant exitance [W/m^2] */
#define EdfDiffuseEmittance(edf, hit) EdfEmittance((edf), hit, DIFFUSE_COMPONENT)

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
 * routine - materialShadingFrame() in material.[ch] constructs such a frame if
 * needed) */
extern int EdfShadingFrame(EDF *edf, HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z);

#endif
