#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/processing/LinkingClusteredStrategy.h"

MemoryPool<float> LinkingClusteredStrategy::linkingClusteredPool;
bool LinkingClusteredStrategy::linkingClusteredPoolInitialized = false;

void
LinkingClusteredStrategy::ensureLinkingClusteredPool() {
    if ( !linkingClusteredPoolInitialized ) {
        linkingClusteredPool.init(2 * 1024 * 1024);
        linkingClusteredPoolInitialized = true;
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
    GalerkinElement *receiverElement;
    GalerkinElement *sourceElement;

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
            Logger::fatal(-1, "createInitialLinkWithTopCluster", "Invalid role");
    }

    if ( receiverElement == nullptr || sourceElement == nullptr ) {
        return;
    }

    // Assume no light transport (overlapping receiver and source)
    float *K;
    float *deltaK;
    bool kFromPool = false;
    bool deltaFromPool = false;
    ensureLinkingClusteredPool();

    if ( receiverElement->basisSize * sourceElement->basisSize == 1 ) {
        K = linkingClusteredPool.allocate(1);
        if ( K == nullptr ) {
            if ( linkingClusteredPool.expand(1024) ) {
                K = linkingClusteredPool.allocate(1);
            }
        }
        if ( K != nullptr ) {
            kFromPool = true;
        } else {
            K = new float[1];
        }
        K[0] = 0.0;
    } else {
        constexpr int KSize = GalerkinBasis::MAX_BASIS_SIZE * GalerkinBasis::MAX_BASIS_SIZE;
        K = linkingClusteredPool.allocate(KSize);
        if ( K == nullptr ) {
            if ( linkingClusteredPool.expand(KSize * 128) ) {
                K = linkingClusteredPool.allocate(KSize);
            }
        }
        if ( K != nullptr ) {
            kFromPool = true;
        } else {
            K = new float[KSize];
        }
        for ( int i = 0; i < receiverElement->basisSize * sourceElement->basisSize; i++ ) {
            K[i] = 0.0;
        }
    }
    deltaK = linkingClusteredPool.allocate(1);
    if ( deltaK == nullptr ) {
        if ( linkingClusteredPool.expand(1024) ) {
            deltaK = linkingClusteredPool.allocate(1);
        }
    }
    if ( deltaK != nullptr ) {
        deltaFromPool = true;
    } else {
        deltaK = new float[1];
    }
    deltaK[0] = Numeric::HUGE_FLOAT_VALUE; // Huge value error on the form factor

    Interaction *newLink = new Interaction(
        receiverElement,
        sourceElement,
        K,
        deltaK[0],
        receiverElement->basisSize,
        sourceElement->basisSize,
        1,
        128
    );

    if ( kFromPool ) {
        if ( receiverElement->basisSize * sourceElement->basisSize == 1 ) {
            linkingClusteredPool.free(1);
        } else {
            linkingClusteredPool.free(GalerkinBasis::MAX_BASIS_SIZE * GalerkinBasis::MAX_BASIS_SIZE);
        }
    } else {
        delete[] K;
    }
    if ( deltaFromPool ) {
        linkingClusteredPool.free(1);
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
