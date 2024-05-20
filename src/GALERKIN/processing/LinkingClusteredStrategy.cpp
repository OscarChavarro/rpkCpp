#include "java/util/ArrayList.txx"
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

    if ( receiverElement == nullptr || sourceElement == nullptr ) {
        return;
    }

    // Assume no light transport (overlapping receiver and source)
    float *K;
    float *deltaK;

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
    deltaK[0] = Numeric::HUGE_FLOAT_VALUE; // Huge value error on the form factor

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

    delete[] K;
    delete[] deltaK;

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        sourceElement->interactions->add(newLink);
    } else {
        receiverElement->interactions->add(newLink);
    }
}
