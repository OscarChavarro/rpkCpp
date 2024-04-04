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
    visibility(),
    isDuplicate(false) {
}

Interaction::Interaction(
    GalerkinElement *rcv,
    GalerkinElement *src,
    const float *K,
    const float *deltaK,
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
    this->deltaK = new float[1];
    *(this->deltaK) = *deltaK;

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
    isDuplicate = false;
}

Interaction::~Interaction() {
    if ( !isDuplicate ) {
        if ( K != nullptr ) {
            delete[] K;
        }
        if ( deltaK != nullptr ) {
            delete[] deltaK;
        }
    }
}

Interaction *
interactionDuplicate(Interaction *interaction) {
    Interaction *newInteraction = new Interaction(
        interaction->receiverElement,
        interaction->sourceElement,
        interaction->K,
        interaction->deltaK,
        interaction->numberOfBasisFunctionsOnReceiver,
        interaction->numberOfBasisFunctionsOnSource,
        interaction->numberOfReceiverCubaturePositions,
        interaction->visibility
    );
    newInteraction->isDuplicate = true;
    return newInteraction;
}

void
interactionDestroy(Interaction *interaction) {
    globalTotalInteractions--;
    if ( interaction->receiverElement->isCluster() ) {
        if ( interaction->sourceElement->isCluster() ) {
            globalCCInteractions--;
        } else {
            globalSCInteractions--;
        }
    } else {
        if ( interaction->sourceElement->isCluster() ) {
            globalCSInteractions--;
        } else {
            globalSSInteractions--;
        }
    }

    if ( interaction->numberOfBasisFunctionsOnReceiver > 1 || interaction->numberOfBasisFunctionsOnSource > 1 ) {
        delete[] interaction->K;
        interaction->K = nullptr;
    }

    delete interaction;
}
