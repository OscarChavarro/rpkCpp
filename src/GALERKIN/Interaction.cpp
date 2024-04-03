#include "common/error.h"
#include "GALERKIN/Interaction.h"

static int globalTotalInteractions = 0;
static int globalCCInteractions = 0;
static int globalCSInteractions = 0;
static int globalSCInteractions = 0;
static int globalSSInteractions = 0;

int
getNumberOfInteractions() {
    return globalTotalInteractions;
}

int
getNumberOfClusterToClusterInteractions() {
    return globalCCInteractions;
}

int
getNumberOfClusterToSurfaceInteractions() {
    return globalCSInteractions;
}

int
getNumberOfSurfaceToClusterInteractions() {
    return globalSCInteractions;
}

int
getNumberOfSurfaceToSurfaceInteractions() {
    return globalSSInteractions;
}

Interaction::Interaction():
    receiverElement(),
    sourceElement(),
    K(),
    deltaK(),
    numberOfBasisFunctionsOnReceiver(),
    numberOfBasisFunctionsOnSource(),
    numberOfReceiverCubaturePositions(),
    visibility() {
}

Interaction::Interaction(
    GalerkinElement *rcv,
    GalerkinElement *src,
    const float *K,
    FloatOrPointer deltaK,
    unsigned char inNumberOfBasisFunctionsOnReceiver,
    unsigned char inNumberOfBasisFunctionsOnSource,
    unsigned char inNumberOfReceiverCubaturePositions,
    unsigned char inVisibility
): K(), deltaK() {
    this->receiverElement = rcv;
    this->sourceElement = src;
    this->numberOfBasisFunctionsOnReceiver = inNumberOfBasisFunctionsOnReceiver;
    this->numberOfBasisFunctionsOnSource = inNumberOfBasisFunctionsOnSource;
    this->numberOfReceiverCubaturePositions = inNumberOfReceiverCubaturePositions;
    this->visibility = inVisibility;

    if ( inNumberOfBasisFunctionsOnReceiver == 1 && inNumberOfBasisFunctionsOnSource == 1 ) {
        this->K = new float[1];
        *this->K = *K;
    } else {
        this->K = new float[inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource];
        for ( int i = 0; i < inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource; i++ ) {
            this->K[i] = K[i];
        }
    }

    if ( inNumberOfReceiverCubaturePositions > 1 ) {
        logFatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    this->deltaK.f = deltaK.f;

    globalTotalInteractions++;
    if ( rcv->isCluster() ) {
        if ( src->isCluster() ) {
            globalCCInteractions++;
        } else {
            globalSCInteractions++;
        }
    } else {
        if ( src->isCluster() ) {
            globalCSInteractions++;
        } else {
            globalSSInteractions++;
        }
    }
}

Interaction *
interactionDuplicate(Interaction *interaction) {
    return new Interaction(
        interaction->receiverElement,
        interaction->sourceElement,
        interaction->K,
        interaction->deltaK,
        interaction->numberOfBasisFunctionsOnReceiver,
        interaction->numberOfBasisFunctionsOnSource,
        interaction->numberOfReceiverCubaturePositions,
        interaction->visibility
    );
}

void
interactionDestroy(Interaction *interaction) {
    GalerkinElement *src = interaction->sourceElement;
    GalerkinElement *rcv = interaction->receiverElement;

    if ( interaction->numberOfBasisFunctionsOnReceiver > 1 || interaction->numberOfBasisFunctionsOnSource > 1 ) {
        delete[] interaction->K;
    }

    delete interaction;

    globalTotalInteractions--;
    if ( rcv->isCluster() ) {
        if ( src->isCluster() ) {
            globalCCInteractions--;
        } else {
            globalSCInteractions--;
        }
    } else {
        if ( src->isCluster() ) {
            globalCSInteractions--;
        } else {
            globalSSInteractions--;
        }
    }
}
