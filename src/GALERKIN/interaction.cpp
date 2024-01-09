#include <cstdlib>

#include "common/error.h"
#include "GALERKIN/interaction.h"

static int TotalInteractions = 0;
static int CCInteractions = 0;
static int CSInteractions = 0;
static int SCInteractions = 0;
static int SSInteractions = 0;

int
GetNumberOfInteractions() {
    return TotalInteractions;
}

int
GetNumberOfClusterToClusterInteractions() {
    return CCInteractions;
}

int
GetNumberOfClusterToSurfaceInteractions() {
    return CSInteractions;
}

int
GetNumberOfSurfaceToClusterInteractions() {
    return SCInteractions;
}

int
GetNumberOfSurfaceToSurfaceInteractions() {
    return SSInteractions;
}

INTERACTION *
InteractionCreate(
    ELEMENT *rcv,
    ELEMENT *src,
    FloatOrPointer K,
    FloatOrPointer deltaK,
    unsigned char nrcv,
    unsigned char nsrc,
    unsigned char crcv,
    unsigned char vis
) {
    int i;
    INTERACTION *interaction = (INTERACTION *)malloc(sizeof(INTERACTION));
    interaction->rcv = rcv;
    interaction->src = src;
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
        logFatal(2, "InteractionCreate", "Not yet implemented for higher order approximations");
    }
    interaction->deltaK.f = deltaK.f;

    TotalInteractions++;
    if ( isCluster(rcv)) {
        if ( isCluster(src)) {
            CCInteractions++;
        } else {
            SCInteractions++;
        }
    } else {
        if ( isCluster(src)) {
            CSInteractions++;
        } else {
            SSInteractions++;
        }
    }

    return interaction;
}

INTERACTION *
InteractionDuplicate(INTERACTION *interaction) {
    return InteractionCreate(interaction->rcv, interaction->src,
                             interaction->K, interaction->deltaK,
                             interaction->nrcv, interaction->nsrc,
                             interaction->crcv, interaction->vis
    );
}

void
InteractionDestroy(INTERACTION *interaction) {
    ELEMENT *src = interaction->src, *rcv = interaction->rcv;

    if ( interaction->nrcv > 1 || interaction->nsrc > 1 ) {
        free(interaction->K.p);
    }

    free(interaction);

    TotalInteractions--;
    if ( isCluster(rcv)) {
        if ( isCluster(src)) {
            CCInteractions--;
        } else {
            SCInteractions--;
        }
    } else {
        if ( isCluster(src)) {
            CSInteractions--;
        } else {
            SSInteractions--;
        }
    }
}

void
InteractionPrint(FILE *out, INTERACTION *link) {
    int a, b, c, alpha, beta, i;

    fprintf(out, "%d (", link->rcv->id);
    galerkinPrintElementId(out, link->rcv);
    fprintf(out, ") <- %d (", link->src->id);
    galerkinPrintElementId(out, link->src);
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
