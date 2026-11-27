#ifndef __INTERACTION__
#define __INTERACTION__

#include "vsdk/toolkit/skin/RpkGeometry.h"

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
    double *K; // Coupling coefficient(s), stored top to bottom, left to right
    double k0; // Inline storage for K when there is a single basis function on both ends,
        // avoiding a heap allocation for the common case; K then points here
    double deltaK; // Used for approximation error estimation over the link (always a single coefficient)
    bool ownsK;
    unsigned char nmbrOBasisFnctnORecv;
    unsigned char numberOfBasisFunctionsOnSource;
    unsigned char nmbrORecvCbtrPstns;
    unsigned char visibility; // 255 for full visibility, 0 for full occlusion

    Interaction();
    explicit Interaction(
        GalerkinElement *inReceiverElement,
        GalerkinElement *inSourceElement,
        const double *inK,
        double inDeltaK,
        unsigned char iNumOBasisFnctnORecv,
        unsigned char iNumOBasisFnctnOSrc,
        unsigned char iNumORecvCbtrPstns,
        unsigned char inVisibility
    );
    virtual ~Interaction();

    static int getNumberOfInteractions();
    static int getNumOClustTClustInters();
    static int getNumOClustTSurfInters();
    static int getNumOSurfTClustInters();
    static int getNumOSurfTSurfInters();
    static void interactionDestroy(Interaction *interaction);
    static Interaction *interactionDuplicate(Interaction *interaction);
    static void countCreated(const Interaction *interaction);
};

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#endif
