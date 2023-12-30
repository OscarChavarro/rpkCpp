#include <cstdlib>

#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/mcradP.h"

static int coeffPoolsInited = false;

static void
InitCoeffPools() {
}

void
InitCoefficients(ELEMENT *elem) {
    if ( !coeffPoolsInited ) {
        InitCoeffPools();
        coeffPoolsInited = true;
    }

    elem->rad = elem->unshot_rad = elem->received_rad = (COLOR *) nullptr;
    elem->basis = &dummyBasis;
}

void
DisposeCoefficients(ELEMENT *elem) {
    if ( elem->basis && elem->basis != &dummyBasis && elem->rad ) {
        free(elem->rad);
        free(elem->unshot_rad);
        free(elem->received_rad);
    }
    InitCoefficients(elem);
}

/* determines basis based on element type and currently desired approximation */
static GalerkinBasis *
ActualBasis(ELEMENT *elem) {
    if ( elem->iscluster ) {
        return &clusterBasis;
    } else {
        return &basis[NR_VERTICES(elem) == 3 ? ET_TRIANGLE : ET_QUAD][mcr.approx_type];
    }
}

void
AllocCoefficients(ELEMENT *elem) {
    DisposeCoefficients(elem);
    elem->basis = ActualBasis(elem);
    elem->rad = (COLOR *)malloc(elem->basis->size*sizeof(COLOR));
    elem->unshot_rad = (COLOR *)malloc(elem->basis->size*sizeof(COLOR));
    elem->received_rad = (COLOR *)malloc(elem->basis->size*sizeof(COLOR));
}

void
ReAllocCoefficients(ELEMENT *elem) {
    if ( elem->basis != ActualBasis(elem)) {
        AllocCoefficients(elem);
    }
}
