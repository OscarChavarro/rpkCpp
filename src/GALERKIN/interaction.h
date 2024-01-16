#ifndef __INTERACTION__
#define __INTERACTION__

#include <cstdio>

class GalerkinElement;

/**
For the coupling coefficients and error estimation coefficients: store
the coefficient directly if there is only one, if there are more,
make an array and store a pointer to it
*/
typedef union FloatOrPointer {
    float f;
    float *p;
} FloatOrPointer;

class INTERACTION {
  public:
    GalerkinElement *rcv; // receiver element
    GalerkinElement *src; // source element
    FloatOrPointer K; // coupling coefficient(s), stored top to bottom, left to right */
    FloatOrPointer deltaK; // used for approximation error estimation over the link */
    unsigned char nrcv; // nr of basis functions on receiver and source */
    unsigned char nsrc;
    unsigned char crcv; // nr of cubature positions on receiver */
    unsigned char vis; // 255 for full visibility, 0 for full occlusion
};

extern INTERACTION *
InteractionCreate(
        GalerkinElement *rcv,
        GalerkinElement *src,
        FloatOrPointer K,
        FloatOrPointer deltaK,
        unsigned char nrcv,
        unsigned char nsrc,
        unsigned char crcv,
        unsigned char vis
);
extern INTERACTION *InteractionDuplicate(INTERACTION *interaction);
extern void InteractionDestroy(INTERACTION *interaction);
extern void InteractionPrint(FILE *out, INTERACTION *interaction);
extern int GetNumberOfInteractions();
extern int GetNumberOfClusterToClusterInteractions();
extern int GetNumberOfClusterToSurfaceInteractions();
extern int GetNumberOfSurfaceToClusterInteractions();
extern int GetNumberOfSurfaceToSurfaceInteractions();

#include "GALERKIN/GalerkinElement.h"

#endif
