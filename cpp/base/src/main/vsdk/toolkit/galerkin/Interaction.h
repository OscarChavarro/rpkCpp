#ifndef INTERACTION__
#define INTERACTION__

#include "vsdk/toolkit/skin/Geometry.h"

class GalerkinElement;

class Interaction {
  private:
    static int totalInteractions;
    static int ccInteractions;
    static int csInteractions;
    static int scInteractions;
    static int ssInteractions;

  public:
    GalerkinElement *receiverElement;
    GalerkinElement *sourceElement;
    float *K; // Coupling coefficient(s), stored top to bottom, left to right
    float k0; // Inline storage for K when there is a single basis function on both ends,
        // avoiding a heap allocation for the common case; K then points here
    float deltaK; // Used for approximation error estimation over the link (always a single coefficient)
    bool ownsK;
    unsigned char numberOfBasisFunctionsOnReceiver;
    unsigned char numberOfBasisFunctionsOnSource;
    unsigned char numberOfReceiverCubaturePositions;
    unsigned char visibility; // 255 for full visibility, 0 for full occlusion

    Interaction();
    explicit Interaction(
        GalerkinElement *inReceiverElement,
        GalerkinElement *inSourceElement,
        const float *inK,
        float inDeltaK,
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
    static void interactionDestroy(Interaction *interaction);
    static Interaction *interactionDuplicate(Interaction *interaction);
};

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#endif
