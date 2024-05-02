#include "common/error.h"
#include "GALERKIN/Interaction.h"

int Interaction::totalInteractions = 0;
int Interaction::ccInteractions = 0;
int Interaction::csInteractions = 0;
int Interaction::scInteractions = 0;
int Interaction::ssInteractions = 0;

Interaction::Interaction():
    receiverElement(),
    sourceElement(),
    K(),
    deltaK(),
    numberOfBasisFunctionsOnReceiver(),
    numberOfBasisFunctionsOnSource(),
    numberOfReceiverCubaturePositions(),
    visibility()
{
}

Interaction::Interaction(
    GalerkinElement *inReceiverElement,
    GalerkinElement *inSourceElement,
    const float *inK,
    const float *inDeltaK,
    unsigned char inNumberOfBasisFunctionsOnReceiver,
    unsigned char inNumberOfBasisFunctionsOnSource,
    unsigned char inNumberOfReceiverCubaturePositions,
    unsigned char inVisibility
): K(), deltaK() {
    this->receiverElement = inReceiverElement;
    this->sourceElement = inSourceElement;
    this->numberOfBasisFunctionsOnReceiver = inNumberOfBasisFunctionsOnReceiver;
    this->numberOfBasisFunctionsOnSource = inNumberOfBasisFunctionsOnSource;
    this->numberOfReceiverCubaturePositions = inNumberOfReceiverCubaturePositions;
    this->visibility = inVisibility;

    if ( inNumberOfBasisFunctionsOnReceiver == 1 && inNumberOfBasisFunctionsOnSource == 1 ) {
        this->K = new float[1];
        *K = *inK;
    } else {
        this->K = new float[inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource];
        for ( int i = 0; i < inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource; i++ ) {
            K[i] = inK[i];
        }
    }

    if ( inNumberOfReceiverCubaturePositions > 1 ) {
        logFatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    deltaK = new float[1];
    *deltaK = *inDeltaK;

    totalInteractions++;
    if ( inReceiverElement->isCluster() ) {
        if ( inSourceElement->isCluster() ) {
            ccInteractions++;
        } else {
            scInteractions++;
        }
    } else {
        if ( inSourceElement->isCluster() ) {
            csInteractions++;
        } else {
            ssInteractions++;
        }
    }
}

Interaction::~Interaction() {
    if ( K != nullptr ) {
        delete[] K;
        K = nullptr;
    }
    if ( deltaK != nullptr ) {
        delete[] deltaK;
        deltaK = nullptr;
    }
}

int
Interaction::getNumberOfInteractions() {
    return totalInteractions;
}

int
Interaction::getNumberOfClusterToClusterInteractions() {
    return ccInteractions;
}

int
Interaction::getNumberOfClusterToSurfaceInteractions() {
    return csInteractions;
}

int
Interaction::getNumberOfSurfaceToClusterInteractions() {
    return scInteractions;
}

int
Interaction::getNumberOfSurfaceToSurfaceInteractions() {
    return ssInteractions;
}

Interaction *
Interaction::interactionDuplicate(Interaction *interaction) {
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
    return newInteraction;
}

void
Interaction::interactionDestroy(Interaction *interaction) {
    Interaction::totalInteractions--;
    if ( interaction->receiverElement->isCluster() ) {
        if ( interaction->sourceElement->isCluster() ) {
            Interaction::ccInteractions--;
        } else {
            Interaction::scInteractions--;
        }
    } else {
        if ( interaction->sourceElement->isCluster() ) {
            Interaction::csInteractions--;
        } else {
            Interaction::ssInteractions--;
        }
    }

    if ( interaction->numberOfBasisFunctionsOnReceiver > 1 || interaction->numberOfBasisFunctionsOnSource > 1 ) {
        delete[] interaction->K;
        interaction->K = nullptr;
    }

    delete interaction;
}
