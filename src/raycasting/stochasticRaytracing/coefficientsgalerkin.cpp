#include "raycasting/stochasticRaytracing/mcradP.h"

static int globalCoefficientPoolsInitialized = false;

static void
initCoefficientPools() {
}

/**
Basically sets rad to nullptr
*/
void
initCoefficients(StochasticRadiosityElement *elem) {
    if ( !globalCoefficientPoolsInitialized ) {
        initCoefficientPools();
        globalCoefficientPoolsInitialized = true;
    }

    elem->rad = elem->unshot_rad = elem->received_rad = (COLOR *) nullptr;
    elem->basis = &GLOBAL_stochasticRadiosisty_dummyBasis;
}

/**
Disposes previously allocated coefficients
*/
void
disposeCoefficients(StochasticRadiosityElement *elem) {
    if ( elem->basis && elem->basis != &GLOBAL_stochasticRadiosisty_dummyBasis && elem->rad ) {
        free(elem->rad);
        free(elem->unshot_rad);
        free(elem->received_rad);
    }
    initCoefficients(elem);
}

/* determines basis based on element type and currently desired approximation */
static GalerkinBasis *
ActualBasis(StochasticRadiosityElement *elem) {
    if ( elem->iscluster ) {
        return &GLOBAL_stochasticRadiosisty_clusterBasis;
    } else {
        return &GLOBAL_stochasticRadiosisty_basis[NR_VERTICES(elem) == 3 ? ET_TRIANGLE : ET_QUAD][GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType];
    }
}

/**
Allocates memory for radiance coefficients
*/
void
allocCoefficients(StochasticRadiosityElement *elem) {
    disposeCoefficients(elem);
    elem->basis = ActualBasis(elem);
    elem->rad = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
    elem->unshot_rad = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
    elem->received_rad = (COLOR *)malloc(elem->basis->size * sizeof(COLOR));
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

void
stochasticRaytracingPrintCoefficients(FILE *fp, COLOR *c, GalerkinBasis *galerkinBasis) {
    int i;

    if ( galerkinBasis->size > 0 ) {
        c[0].print(fp);
    }
    for ( i = 1; i < galerkinBasis->size; i++) {
        fprintf(fp, ", ");
        (c)[i].print(fp);
    }
}