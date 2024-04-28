#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

extern ColorRgb clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample, GalerkinState *galerkinState);
extern ColorRgb sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState);

extern void
iterateOverSurfaceElementsInCluster(
    GalerkinElement *galerkinElement,
    void (*func)(GalerkinElement *elem, GalerkinState *galerkinState, ColorRgb *accumulatedRadiance),
    GalerkinState *galerkinState, ColorRgb *accumulatedRadiance);

extern double receiverClusterArea(Interaction *link, GalerkinState *galerkinState);
extern void clusterGatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState);
extern ColorRgb maxClusterRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);

#endif
