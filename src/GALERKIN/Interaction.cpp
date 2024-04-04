#include "common/error.h"
#include "GALERKIN/Interaction.h"

int Interaction::totalInteractions = 0;
int Interaction::ccInteractions = 0;
int Interaction::csInteractions = 0;
int Interaction::scInteractions = 0;
int Interaction::ssInteractions = 0;

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
    GalerkinElement *inReceiverElement,
    GalerkinElement *inSourceElement,
    const float *K,
    const float *deltaK,
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
