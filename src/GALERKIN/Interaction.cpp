#include "common/error.h"
#include "GALERKIN/Interaction.h"

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

Interaction::Interaction():
    receiverElement(),
    sourceElement(),
    K(),
    deltaK(),
    nrcv(),
    nsrc(),
    crcv(),
    vis() {
}

Interaction::Interaction(
    GalerkinElement *rcv,
    GalerkinElement *src,
    FloatOrPointer K,
    FloatOrPointer deltaK,
    unsigned char nrcv,
    unsigned char nsrc,
    unsigned char crcv,
    unsigned char vis
): K(), deltaK() {
    this->receiverElement = rcv;
    this->sourceElement = src;
    this->nrcv = nrcv;
    this->nsrc = nsrc;
    this->crcv = crcv;
    this->vis = vis;

    if ( nrcv == 1 && nsrc == 1 ) {
        this->K.f = K.f;
    } else {
        this->K.p = new float[nrcv * nsrc];
        for ( int i = 0; i < nrcv * nsrc; i++ ) {
            this->K.p[i] = K.p[i];
        }
    }

    if ( crcv > 1 ) {
        logFatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
    }
    this->deltaK.f = deltaK.f;

    globalTotalInteractions++;
    if ( isCluster(rcv) ) {
        if ( isCluster(src) ) {
            globalCCInteractions++;
        } else {
            globalSCInteractions++;
        }
    } else {
        if ( isCluster(src) ) {
            globalCSInteractions++;
        } else {
            globalSSInteractions++;
        }
    }
}

Interaction *
interactionDuplicate(Interaction *interaction) {
    return new Interaction(
        interaction->receiverElement,
        interaction->sourceElement,
        interaction->K,
        interaction->deltaK,
        interaction->nrcv,
        interaction->nsrc,
        interaction->crcv,
        interaction->vis
    );
}

void
interactionDestroy(Interaction *interaction) {
    GalerkinElement *src = interaction->sourceElement;
    GalerkinElement *rcv = interaction->receiverElement;

    if ( interaction->nrcv > 1 || interaction->nsrc > 1 ) {
        delete[] interaction->K.p;
    }

    delete interaction;

    globalTotalInteractions--;
    if ( isCluster(rcv) ) {
        if ( isCluster(src) ) {
            globalCCInteractions--;
        } else {
            globalSCInteractions--;
        }
    } else {
        if ( isCluster(src) ) {
            globalCSInteractions--;
        } else {
            globalSSInteractions--;
        }
    }
}

void
interactionPrint(FILE *out, Interaction *link) {
    int a;
    int b;
    int c;
    int alpha;
    int beta;

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
        for ( int i = 0; i < c; i++ ) {
            fprintf(out, "%7.7f ", link->deltaK.p[i]);
        }
        fprintf(out, "\n");
    }
}
