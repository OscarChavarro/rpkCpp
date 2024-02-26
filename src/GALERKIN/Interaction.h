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

class Interaction {
  public:
    GalerkinElement *receiverElement;
    GalerkinElement *sourceElement;
    FloatOrPointer K; // Coupling coefficient(s), stored top to bottom, left to right
    FloatOrPointer deltaK; // Used for approximation error estimation over the link
    unsigned char nrcv; // Number of basis functions on receiver and source
    unsigned char nsrc;
    unsigned char crcv; // Number of cubature positions on receiver
    unsigned char vis; // 255 for full visibility, 0 for full occlusion

    Interaction();
    explicit Interaction(
            GalerkinElement *rcv,
            GalerkinElement *src,
            FloatOrPointer K,
            FloatOrPointer deltaK,
            unsigned char nrcv,
            unsigned char nsrc,
            unsigned char crcv,
            unsigned char vis
    );
};

extern Interaction *interactionDuplicate(Interaction *interaction);
extern void interactionDestroy(Interaction *interaction);
extern void interactionPrint(FILE *out, Interaction *link);
extern int getNumberOfInteractions();
extern int getNumberOfClusterToClusterInteractions();
extern int getNumberOfClusterToSurfaceInteractions();
extern int getNumberOfSurfaceToClusterInteractions();
extern int getNumberOfSurfaceToSurfaceInteractions();

#include "GALERKIN/GalerkinElement.h"

#endif
