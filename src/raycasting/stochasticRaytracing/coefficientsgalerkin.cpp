#include "raycasting/stochasticRaytracing/mcradP.h"

/**
Disposes previously allocated coefficients
*/
void
disposeCoefficients(StochasticRadiosityElement *elem) {
    if ( elem->basis && elem->basis != &GLOBAL_stochasticRadiosity_dummyBasis && elem->radiance ) {
        free(elem->radiance);
        free(elem->unShotRadiance);
        free(elem->receivedRadiance);
    }
    initCoefficients(elem);
}

/* determines basis based on element type and currently desired approximation */
static GalerkinBasis *
ActualBasis(StochasticRadiosityElement *elem) {
    if ( elem->isClusterFlag ) {
        return &GLOBAL_stochasticRadiosity_clusterBasis;
    } else {
        return &GLOBAL_stochasticRadiosity_basis[NR_VERTICES(elem) == 3 ? ET_TRIANGLE : ET_QUAD][GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType];
    }
}

/**
Allocates memory for radiance coefficients
*/
void
allocCoefficients(StochasticRadiosityElement *elem) {
    disposeCoefficients(elem);
    elem->basis = ActualBasis(elem);
    elem->radiance = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
    elem->unShotRadiance = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
    elem->receivedRadiance = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
}

/**
Re-allocates memory for radiance coefficients if
the currently desired approximation order is not the same
as the approximation order for which the element has
been initialised before
*/
void
reAllocCoefficients(StochasticRadiosityElement *elem) {
    if ( elem->basis != ActualBasis(elem)) {
        allocCoefficients(elem);
    }
}
