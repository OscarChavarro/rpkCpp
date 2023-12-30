/* btdf.h: Bidirectional Transmittance Distribution Functions. */

#ifndef _BTDF_H_
#define _BTDF_H_

#include <cstdio>

#include "material/btdf_methods.h"
#include "material/xxdf.h"

/* index of refraction data type. Normally when using BSDF's
   this should not be needed. In C++ this would of course
   be a plain complex number */

class BTDF {
  public:
    void *data;
    BTDF_METHODS *methods;
};

/**
Creates a BTDF instance with given data and methods
*/
extern BTDF *BtdfCreate(void *data, BTDF_METHODS *methods);

/* Print the btdf data the to specified file pointer */
extern void BtdfPrint(FILE *out, BTDF *btdf);

/* Returns the transmittance of the BTDF */
extern COLOR BtdfTransmittance(BTDF *btdf, XXDFFLAGS flags);

/* Returns the index of refraction of the BTDF */
extern void BtdfIndexOfRefraction(BTDF *btdf, REFRACTIONINDEX *index);


/************* BTDF Evaluation functions ****************/

/* All components of the Btdf
 *
 * Vector directions :
 *
 * in : towards patch
 * out : from patch
 * normal : leaving from patch, on the incoming side.
 *          So in.normal < 0 !!!
 */

extern COLOR BtdfEval(BTDF *btdf, REFRACTIONINDEX inIndex,
                      REFRACTIONINDEX outIndex, Vector3D *in,
                      Vector3D *out, Vector3D *normal, XXDFFLAGS flags);

extern Vector3D BtdfSample(BTDF *btdf, REFRACTIONINDEX inIndex,
                           REFRACTIONINDEX outIndex, Vector3D *in,
                           Vector3D *normal, int doRussianRoulette,
                           XXDFFLAGS flags, double x_1, double x_2,
                           double *pdf);

extern void BtdfEvalPdf(BTDF *btdf, REFRACTIONINDEX inIndex,
                        REFRACTIONINDEX outIndex, Vector3D *in,
                        Vector3D *out, Vector3D *normal,
                        XXDFFLAGS flags, double *pdf, double *pdfRR);

#endif
