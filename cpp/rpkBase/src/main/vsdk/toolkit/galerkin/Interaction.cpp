#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/galerkin/Interaction.h"

int Interaction::totalInteractions = 0;
int Interaction::ccInteractions = 0;
int Interaction::csInteractions = 0;
int Interaction::scInteractions = 0;
int Interaction::ssInteractions = 0;

Interaction::Interaction():
    receiverElement(),
    sourceElement(),
    K(),
    k0(),
    deltaK(),
    ownsK(),
    numberOfBasisFunctionsOnReceiver(),
    numberOfBasisFunctionsOnSource(),
    numberOfReceiverCubaturePositions(),
    visibility()
{
    ownsK = true;
}

Interaction::Interaction(
    GalerkinElement *inReceiverElement,
    GalerkinElement *inSourceElement,
    const double *inK,
    double inDeltaK,
    unsigned char inNumberOfBasisFunctionsOnReceiver,
    unsigned char inNumberOfBasisFunctionsOnSource,
    unsigned char inNumberOfReceiverCubaturePositions,
    unsigned char inVisibility
): K(), k0(), deltaK() {
    this->receiverElement = inReceiverElement;
    this->sourceElement = inSourceElement;
    this->numberOfBasisFunctionsOnReceiver = inNumberOfBasisFunctionsOnReceiver;
    this->numberOfBasisFunctionsOnSource = inNumberOfBasisFunctionsOnSource;
    this->numberOfReceiverCubaturePositions = inNumberOfReceiverCubaturePositions;
    this->visibility = inVisibility;

    if ( inNumberOfBasisFunctionsOnReceiver == 1 && inNumberOfBasisFunctionsOnSource == 1 ) {
        this->k0 = *inK;
        this->K = &this->k0;
        this->ownsK = false;
    } else {
        this->K = new double[inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource];
        for ( int i = 0; i < inNumberOfBasisFunctionsOnReceiver * inNumberOfBasisFunctionsOnSource; i++ ) {
            K[i] = inK[i];
        }
        this->ownsK = true;
    }

    if ( inNumberOfReceiverCubaturePositions > 1 ) {
        Logger::reportMessage("Interaction", Logger::FATAL_ERROR, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    deltaK = inDeltaK;

    countCreated(this);
}

/**
Adds the interaction to the interaction counters
*/
void
Interaction::countCreated(const Interaction *interaction) {
    totalInteractions++;
    if ( interaction->receiverElement->isCluster() ) {
        if ( interaction->sourceElement->isCluster() ) {
            ccInteractions++;
        } else {
            scInteractions++;
        }
    } else {
        if ( interaction->sourceElement->isCluster() ) {
            csInteractions++;
        } else {
            ssInteractions++;
        }
    }
}

/**
Same as interactionDuplicate, but the copy is not counted: for threads that
create interactions which a single thread counts later (countCreated)
*/
Interaction *
Interaction::uncountedDuplicate(const Interaction *interaction) {
    Interaction *newInteraction = new Interaction();
    newInteraction->receiverElement = interaction->receiverElement;
    newInteraction->sourceElement = interaction->sourceElement;
    newInteraction->numberOfBasisFunctionsOnReceiver = interaction->numberOfBasisFunctionsOnReceiver;
    newInteraction->numberOfBasisFunctionsOnSource = interaction->numberOfBasisFunctionsOnSource;
    newInteraction->numberOfReceiverCubaturePositions = interaction->numberOfReceiverCubaturePositions;
    newInteraction->visibility = interaction->visibility;
    if ( interaction->numberOfBasisFunctionsOnReceiver == 1 && interaction->numberOfBasisFunctionsOnSource == 1 ) {
        newInteraction->k0 = *interaction->K;
        newInteraction->K = &newInteraction->k0;
        newInteraction->ownsK = false;
    } else {
        const int size = interaction->numberOfBasisFunctionsOnReceiver * interaction->numberOfBasisFunctionsOnSource;
        newInteraction->K = new double[size];
        for ( int i = 0; i < size; i++ ) {
            newInteraction->K[i] = interaction->K[i];
        }
        newInteraction->ownsK = true;
    }
    if ( interaction->numberOfReceiverCubaturePositions > 1 ) {
        Logger::reportMessage("Interaction", Logger::FATAL_ERROR, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    newInteraction->deltaK = interaction->deltaK;
    return newInteraction;
}

Interaction::~Interaction() {
    if ( ownsK && K != nullptr ) {
        delete[] K;
        K = nullptr;
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

    if ( interaction->ownsK
      && interaction->K != nullptr
      && (interaction->numberOfBasisFunctionsOnReceiver > 1 || interaction->numberOfBasisFunctionsOnSource > 1) ) {
        delete[] interaction->K;
        interaction->K = nullptr;
    }

    delete interaction;
}
