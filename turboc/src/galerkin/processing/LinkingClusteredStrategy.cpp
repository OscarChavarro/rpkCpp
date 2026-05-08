#include "java/util/ArrayList.txx"
#include "common/MemoryPool.txx"
#include "common/Error.h"
#include "galerkin/GalerkinBasis.h"
#include "galerkin/processing/LinkingClusteredStrategy.h"

static MemoryPool<float> gLinkingClusteredPool;
static bool gLinkingClusteredPoolInitialized = false;

static void ensureLinkingClusteredPool() {
    if ( !gLinkingClusteredPoolInitialized ) {
        gLinkingClusteredPool.init(2 * 1024 * 1024);
        gLinkingClusteredPoolInitialized = true;
    }
}

/**
Creates an initial link between the given element and the top cluster
*/
void
LinkingClusteredStrategy::createInitialLinks(
    GalerkinElement *element,
    GalerkinRole role,
    GalerkinState *galerkinState)
{
    GalerkinElement *receiverElement = NULL;
    GalerkinElement *sourceElement = NULL;

    switch ( role ) {
        case RECEIVER:
            receiverElement = element;
            sourceElement = galerkinState->topCluster;
            break;
        case SOURCE:
            sourceElement = element;
            receiverElement = galerkinState->topCluster;
            break;
        default:
            Error::fatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    if ( receiverElement == NULL || sourceElement == NULL ) {
        return;
    }

    // Assume no light transport (overlapping receiver and source)
    float *K;
    float *deltaK;
    bool kFromPool = false;
    bool deltaFromPool = false;
    ensureLinkingClusteredPool();

    if ( receiverElement->basisSize * sourceElement->basisSize == 1 ) {
        K = gLinkingClusteredPool.allocate(1);
        if ( K == NULL ) {
            if ( gLinkingClusteredPool.expand(1024) ) {
                K = gLinkingClusteredPool.allocate(1);
            }
        }
        if ( K != NULL ) {
            kFromPool = true;
        } else {
            K = new float[1];
        }
        K[0] = 0.0;
    } else {
        const int KSize = GALERKIN_MAX_BASIS_SIZE * GALERKIN_MAX_BASIS_SIZE;
        K = gLinkingClusteredPool.allocate(KSize);
        if ( K == NULL ) {
            if ( gLinkingClusteredPool.expand(KSize * 128) ) {
                K = gLinkingClusteredPool.allocate(KSize);
            }
        }
        if ( K != NULL ) {
            kFromPool = true;
        } else {
            K = new float[KSize];
        }
        for ( int i = 0; i < receiverElement->basisSize * sourceElement->basisSize; i++ ) {
            K[i] = 0.0;
        }
    }
    deltaK = gLinkingClusteredPool.allocate(1);
    if ( deltaK == NULL ) {
        if ( gLinkingClusteredPool.expand(1024) ) {
            deltaK = gLinkingClusteredPool.allocate(1);
        }
    }
    if ( deltaK != NULL ) {
        deltaFromPool = true;
    } else {
        deltaK = new float[1];
    }
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

    if ( kFromPool ) {
        if ( receiverElement->basisSize * sourceElement->basisSize == 1 ) {
            gLinkingClusteredPool.free(1);
        } else {
            gLinkingClusteredPool.free(GALERKIN_MAX_BASIS_SIZE * GALERKIN_MAX_BASIS_SIZE);
        }
    } else {
        delete[] K;
    }
    if ( deltaFromPool ) {
        gLinkingClusteredPool.free(1);
    } else {
        delete[] deltaK;
    }

    // Store interactions with the source patch for the progressive radiosity method
    // and with the receiving patch for gathering methods
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        sourceElement->interactions->add(newLink);
    } else {
        receiverElement->interactions->add(newLink);
    }
}
