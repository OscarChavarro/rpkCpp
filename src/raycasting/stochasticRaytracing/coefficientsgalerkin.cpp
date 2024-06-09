#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingState.h"

/**
Disposes previously allocated coefficients
*/
void
disposeCoefficients(StochasticRadiosityElement *elem) {
    if ( elem->basis && elem->basis != &GLOBAL_stochasticRadiosity_dummyBasis && elem->radiance ) {
        delete[] elem->radiance;
        delete[] elem->unShotRadiance;
        delete[] elem->receivedRadiance;
    }
    initCoefficients(elem);
}

/**
Determines basis based on element type and currently desired approximation
*/
static GalerkinBasis *
actualBasis(const StochasticRadiosityElement *elem) {
    if ( elem->isCluster() ) {
        return &GLOBAL_stochasticRadiosity_clusterBasis;
    } else {
        return &GLOBAL_stochasticRadiosity_basis[numberOfVertices(elem) == 3 ? ET_TRIANGLE : ET_QUAD][GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType];
    }
}

/**
Allocates memory for radiance coefficients
*/
void
allocCoefficients(StochasticRadiosityElement *elem) {
    disposeCoefficients(elem);
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
reAllocCoefficients(StochasticRadiosityElement *elem) {
    if ( elem != nullptr && elem->basis != actualBasis(elem) ) {
        allocCoefficients(elem);
    }
}

#endif
