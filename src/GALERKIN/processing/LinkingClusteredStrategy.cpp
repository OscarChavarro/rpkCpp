#include "java/util/ArrayList.txx"
#include "common/mymath.h"
#include "common/error.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/processing/LinkingClusteredStrategy.h"

/**
Creates an initial link between the given element and the top cluster
*/
void
LinkingClusteredStrategy::createInitialLinks(
    GalerkinElement *element,
    GalerkinRole role,
    GalerkinState *galerkinState)
{
    GalerkinElement *receiverElement = nullptr;
    GalerkinElement *sourceElement = nullptr;

    switch ( role ) {
        case GalerkinRole::RECEIVER:
            receiverElement = element;
            sourceElement = galerkinState->topCluster;
            break;
        case GalerkinRole::SOURCE:
            sourceElement = element;
            receiverElement = galerkinState->topCluster;
            break;
        default:
            logFatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    // Assume no light transport (overlapping receiver and source)
    float *K = nullptr;
    float *deltaK = nullptr;

    if ( receiverElement != nullptr && sourceElement != nullptr ) {
        if ( receiverElement->basisSize * sourceElement->basisSize == 1 ) {
            K = new float[1];
            K[0] = 0.0;
        } else {
            K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
            for ( int i = 0; i < receiverElement->basisSize * sourceElement->basisSize; i++ ) {
                K[i] = 0.0;
            }
        }
        deltaK = new float[1];
        deltaK[0] = HUGE_FLOAT_VALUE; // HUGE_DOUBLE_VALUE error on the form factor
    }

    Interaction *newLink = new Interaction(
        receiverElement,
        sourceElement,
        K,
        deltaK,
        receiverElement->basisSize,
        sourceElement->basisSize,
        1,
        128
    );

    if ( K != nullptr ) {
        delete[] K;
    }
    if ( deltaK != nullptr ) {
        delete[] deltaK;
    }

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    bool used = false;
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL && sourceElement != nullptr ) {
        sourceElement->interactions->add(newLink);
        used = true;
    } else if ( receiverElement != nullptr ) {
        receiverElement->interactions->add(newLink);
        used = true;
    }

    if ( !used ) {
        delete newLink;
    }
}
