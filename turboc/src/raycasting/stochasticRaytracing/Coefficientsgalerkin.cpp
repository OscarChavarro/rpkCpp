#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

/**
Disposes previously allocated coefficients
*/
void
Coefficientsmcrad::disposeCoefficients(StochasticRadiosityElement *elem) {
    if ( elem->basis && elem->basis != &StochasticRadiosityBasisState::activeState().dummyBasis && elem->radiance ) {
        delete[] elem->radiance;
        delete[] elem->unShotRadiance;
        delete[] elem->receivedRadiance;
    }
    Coefficientsmcrad::initCoefficients(elem);
}

/**
Determines basis based on element type and currently desired approximation
*/
GalerkinBasis *
Coefficientsmcrad::actualBasis(const StochasticRadiosityElement *elem) {
    if ( elem->isCluster() ) {
        return &StochasticRadiosityBasisState::activeState().clusterBasis;
    } else {
        return &StochasticRadiosityBasisState::activeState().basis[McradP::numberOfVertices(elem) == 3 ? ET_TRIANGLE : ET_QUAD][StochasticRelaxation::activeState().approximationOrderType];
    }
}

/**
Allocates memory for radiance coefficients
*/
void
Coefficientsmcrad::allocCoefficients(StochasticRadiosityElement *elem) {
    Coefficientsmcrad::disposeCoefficients(elem);
    elem->basis = actualBasis(elem);
    elem->radiance = new ColorRgb[elem->basis->size];
    elem->unShotRadiance = new ColorRgb[elem->basis->size];
    elem->receivedRadiance = new ColorRgb[elem->basis->size];
}

/**
Re-allocates memory for radiance coefficients if
the currently desired approximation order is not the same
as the approximation order for which the element has
been initialised before
*/
void
Coefficientsmcrad::reAllocCoefficients(StochasticRadiosityElement *elem) {
    if ( elem != NULL && elem->basis != Coefficientsmcrad::actualBasis(elem) ) {
        Coefficientsmcrad::allocCoefficients(elem);
    }
}

#endif
