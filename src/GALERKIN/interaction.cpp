#include <cstdlib>

#include "common/error.h"
#include "GALERKIN/interaction.h"

static int globalTotalInteractions = 0;
static int globalCCInteractions = 0;
static int globalCSInteractions = 0;
static int globalSCInteractions = 0;
static int globalSSInteractions = 0;

int
getNumberOfInteractions() {
    return globalTotalInteractions;
}

int
getNumberOfClusterToClusterInteractions() {
    return globalCCInteractions;
}

int
getNumberOfClusterToSurfaceInteractions() {
    return globalCSInteractions;
}

int
getNumberOfSurfaceToClusterInteractions() {
    return globalSCInteractions;
}

int
getNumberOfSurfaceToSurfaceInteractions() {
    return globalSSInteractions;
}

INTERACTION *
interactionCreate(
    GalerkinElement *rcv,
    GalerkinElement *src,
    FloatOrPointer K,
    FloatOrPointer deltaK,
    unsigned char nrcv,
    unsigned char nsrc,
    unsigned char crcv,
    unsigned char vis
) {
    int i;
    INTERACTION *interaction = (INTERACTION *)malloc(sizeof(INTERACTION));
    interaction->receiverElement = rcv;
    interaction->sourceElement = src;
    interaction->nrcv = nrcv;
    interaction->nsrc = nsrc;
    interaction->crcv = crcv;
    interaction->vis = vis;

    if ( nrcv == 1 && nsrc == 1 ) {
        interaction->K.f = K.f;
    } else {
        interaction->K.p = (float *)malloc(nrcv * nsrc * sizeof(float));
        for ( i = 0; i < nrcv * nsrc; i++ ) {
            interaction->K.p[i] = K.p[i];
        }
    }

    if ( crcv > 1 ) {
        logFatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    interaction->deltaK.f = deltaK.f;

    globalTotalInteractions++;
    if ( isCluster(rcv)) {
        if ( isCluster(src)) {
            globalCCInteractions++;
        } else {
            globalSCInteractions++;
        }
    } else {
        if ( isCluster(src)) {
            globalCSInteractions++;
        } else {
            globalSSInteractions++;
        }
    }

    return interaction;
}

INTERACTION *
interactionDuplicate(INTERACTION *interaction) {
    return interactionCreate(interaction->receiverElement, interaction->sourceElement,
                             interaction->K, interaction->deltaK,
                             interaction->nrcv, interaction->nsrc,
                             interaction->crcv, interaction->vis
    );
}

void
interactionDestroy(INTERACTION *interaction) {
    GalerkinElement *src = interaction->sourceElement, *rcv = interaction->receiverElement;

    if ( interaction->nrcv > 1 || interaction->nsrc > 1 ) {
        free(interaction->K.p);
    }

    free(interaction);

    globalTotalInteractions--;
    if ( isCluster(rcv)) {
        if ( isCluster(src)) {
            globalCCInteractions--;
        } else {
            globalSCInteractions--;
        }
    } else {
        if ( isCluster(src)) {
            globalCSInteractions--;
        } else {
            globalSSInteractions--;
        }
    }
}

void
interactionPrint(FILE *out, INTERACTION *link) {
    int a, b, c, alpha, beta, i;

    fprintf(out, "%d (", link->receiverElement->id);
    galerkinElementPrintId(out, link->receiverElement);
    fprintf(out, ") <- %d (", link->sourceElement->id);
    galerkinElementPrintId(out, link->sourceElement);
    fprintf(out, "): vis=%d, nrcv=%d, nsrc=%d, crcv=%d,\n",
            link->vis,
            link->nrcv,
            link->nsrc,
            link->crcv);
    if ( link->vis == 0 ) {
        return;
    }

    a = link->nrcv;
    b = link->nsrc;
    if ( a == 1 && b == 1 ) {
        fprintf(out, "\tK = %f\n", link->K.f);
    } else {
        fprintf(out, "\tK[%d][%d] = \n", a, b);
        for ( alpha = 0; alpha < a; alpha++ ) {
            fprintf(out, "\t\t");
            for ( beta = 0; beta < b; beta++ ) {
                fprintf(out, "%7.7f\t", link->K.p[alpha * b + beta]);
            }
            fprintf(out, "\n");
        }
    }

    c = link->crcv;
    if ( c == 1 ) {
        fprintf(out, "\tdeltaK = %f\n", link->deltaK.f);
    } else {
        fprintf(out, "\tdeltaK[%d] = ", c);
        for ( i = 0; i < c; i++ ) {
            fprintf(out, "%7.7f ", link->deltaK.p[i]);
        }
        fprintf(out, "\n");
    }
}
