#include "common/Error.h"
#include "galerkin/Interaction.h"

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
    nmbrOBasisFnctnORecv(),
    numberOfBasisFunctionsOnSource(),
    nmbrORecvCbtrPstns(),
    visibility()
{
}

Interaction::Interaction(
    GalerkinElement *inReceiverElement,
    GalerkinElement *inSourceElement,
    const float *inK,
    const float *inDeltaK,
    unsigned char iNumOBasisFnctnORecv,
    unsigned char iNumOBasisFnctnOSrc,
    unsigned char iNumORecvCbtrPstns,
    unsigned char inVisibility
): K(), deltaK() {
    this->receiverElement = inReceiverElement;
    this->sourceElement = inSourceElement;
    this->nmbrOBasisFnctnORecv = iNumOBasisFnctnORecv;
    this->numberOfBasisFunctionsOnSource = iNumOBasisFnctnOSrc;
    this->nmbrORecvCbtrPstns = iNumORecvCbtrPstns;
    this->visibility = inVisibility;

    if ( iNumOBasisFnctnORecv == 1 && iNumOBasisFnctnOSrc == 1 ) {
        this->K = new float[1];
        *K = *inK;
    } else {
        this->K = new float[iNumOBasisFnctnORecv * iNumOBasisFnctnOSrc];
        for ( int i = 0; i < iNumOBasisFnctnORecv * iNumOBasisFnctnOSrc; i++ ) {
            K[i] = inK[i];
        }
    }

    if ( iNumORecvCbtrPstns > 1 ) {
        Error::fatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
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
    if ( K != NULL ) {
        delete[] K;
        K = NULL;
    }
    if ( deltaK != NULL ) {
        delete[] deltaK;
        deltaK = NULL;
    }
}

int
Interaction::getNumberOfInteractions() {
    return totalInteractions;
}

int
Interaction::getNumOClustTClustInters() {
    return ccInteractions;
}

int
Interaction::getNumOClustTSurfInters() {
    return csInteractions;
}

int
Interaction::getNumOSurfTClustInters() {
    return scInteractions;
}

int
Interaction::getNumOSurfTSurfInters() {
    return ssInteractions;
}

Interaction *
Interaction::interactionDuplicate(Interaction *interaction) {
    Interaction *newInteraction = new Interaction(
        interaction->receiverElement,
        interaction->sourceElement,
        interaction->K,
        interaction->deltaK,
        interaction->nmbrOBasisFnctnORecv,
        interaction->numberOfBasisFunctionsOnSource,
        interaction->nmbrORecvCbtrPstns,
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

    if ( interaction->nmbrOBasisFnctnORecv > 1 || interaction->numberOfBasisFunctionsOnSource > 1 ) {
        delete[] interaction->K;
        interaction->K = NULL;
    }

    delete interaction;
}
