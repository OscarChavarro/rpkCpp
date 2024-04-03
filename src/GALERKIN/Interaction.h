#ifndef __INTERACTION__
#define __INTERACTION__

#include <cstdio>

class GalerkinElement;

class Interaction {
  public:
    GalerkinElement *receiverElement;
    GalerkinElement *sourceElement;
    float *K; // Coupling coefficient(s), stored top to bottom, left to right
    float *deltaK; // Used for approximation error estimation over the link
    unsigned char numberOfBasisFunctionsOnReceiver;
    unsigned char numberOfBasisFunctionsOnSource;
    unsigned char numberOfReceiverCubaturePositions;
    unsigned char visibility; // 255 for full visibility, 0 for full occlusion

    Interaction();
    explicit Interaction(
        GalerkinElement *rcv,
        GalerkinElement *src,
        const float *K,
        const float *deltaK,
        unsigned char inNumberOfBasisFunctionsOnReceiver,
        unsigned char inNumberOfBasisFunctionsOnSource,
        unsigned char inNumberOfReceiverCubaturePositions,
        unsigned char inVisibility
    );
};

extern Interaction *interactionDuplicate(Interaction *interaction);
extern void interactionDestroy(Interaction *interaction);
extern int getNumberOfInteractions();
extern int getNumberOfClusterToClusterInteractions();
extern int getNumberOfClusterToSurfaceInteractions();
extern int getNumberOfSurfaceToClusterInteractions();
extern int getNumberOfSurfaceToSurfaceInteractions();

#include "GALERKIN/GalerkinElement.h"

#endif
