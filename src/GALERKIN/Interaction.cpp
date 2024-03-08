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
    if ( rcv->isCluster() ) {
        if ( src->isCluster() ) {
            globalCCInteractions++;
        } else {
            globalSCInteractions++;
        }
    } else {
        if ( src->isCluster() ) {
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
    if ( rcv->isCluster() ) {
        if ( src->isCluster() ) {
            globalCCInteractions--;
        } else {
            globalSCInteractions--;
        }
    } else {
        if ( src->isCluster() ) {
            globalCSInteractions--;
        } else {
            globalSSInteractions--;
        }
    }
}
