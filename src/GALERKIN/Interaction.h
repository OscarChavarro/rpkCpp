#ifndef __INTERACTION__
#define __INTERACTION__

class GalerkinElement;

class Interaction {
  private:
    static int totalInteractions;
    static int ccInteractions;
    static int csInteractions;
    static int scInteractions;
    static int ssInteractions;

    friend void interactionDestroy(Interaction *interaction);

  public:
    GalerkinElement *receiverElement;
    GalerkinElement *sourceElement;
    float *K; // Coupling coefficient(s), stored top to bottom, left to right
    float *deltaK; // Used for approximation error estimation over the link
    unsigned char numberOfBasisFunctionsOnReceiver;
    unsigned char numberOfBasisFunctionsOnSource;
    unsigned char numberOfReceiverCubaturePositions;
    unsigned char visibility; // 255 for full visibility, 0 for full occlusion

    bool isDuplicate;

    Interaction();
    explicit Interaction(
        GalerkinElement *inReceiverElement,
        GalerkinElement *inSourceElement,
        const float *K,
        const float *deltaK,
        unsigned char inNumberOfBasisFunctionsOnReceiver,
        unsigned char inNumberOfBasisFunctionsOnSource,
        unsigned char inNumberOfReceiverCubaturePositions,
        unsigned char inVisibility
    );
    virtual ~Interaction();

    static int getNumberOfInteractions();
    static int getNumberOfClusterToClusterInteractions();
    static int getNumberOfClusterToSurfaceInteractions();
    static int getNumberOfSurfaceToClusterInteractions();
    static int getNumberOfSurfaceToSurfaceInteractions();
};

extern Interaction *interactionDuplicate(Interaction *interaction);
extern void interactionDestroy(Interaction *interaction);

#include "GALERKIN/GalerkinElement.h"

#endif
