/* brdf.h: Bidirectional Reflectance Distribution Functions. */

#ifndef _BRDF_H_
#define _BRDF_H_

#include <cstdio>

#include "common/linealAlgebra/vectorMacros.h"
#include "material/xxdf.h"
#include "material/brdf_methods.h"
#include "material/hit.h"

class BRDF {
  public:
    void *data;
    BRDF_METHODS *methods;
};

/**
Creates a BRDF instance with given data and methods
*/
extern BRDF *BrdfCreate(void *data, BRDF_METHODS *methods);

/* Print the brdf data the to specified file pointer */
extern void BrdfPrint(FILE *out, BRDF *brdf);

/* Returns the reflectance of hte BRDF according to the flags */
extern COLOR BrdfReflectance(BRDF *brdf, XXDFFLAGS flags);

/************* BRDF Evaluation functions ****************/

/* BRDF evaluation functions : 
 *
 *   Vector3D in : incoming ray direction (to patch)
 *   Vector3D out : reflected ray direction (from patch)
 *   Vector3D normal : normal vector
 *   XXDFFLAGS flags : flags indicating which components must be
 *     evaluated
 */

extern COLOR BrdfEval(BRDF *brdf, Vector3D *in, Vector3D *out, Vector3D *normal,
                      XXDFFLAGS flags);

extern Vector3D BrdfSample(BRDF *brdf, Vector3D *in,
                           Vector3D *normal, int doRussianRoulette,
                           XXDFFLAGS flags, double x_1, double x_2,
                           double *pdf);

extern void BrdfEvalPdf(BRDF *brdf, Vector3D *in,
                        Vector3D *out, Vector3D *normal,
                        XXDFFLAGS flags, double *pdf, double *pdfRR);

#endif
