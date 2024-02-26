#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/galerkinP.h"

inline void
clusterGalerkinClearCoefficients(COLOR *color, char n) {
    int i;
    COLOR *c;
    for ( i = 0, c = color; i < n; i++, c++ ) {
        colorClear(*c);
    }
}

inline void
clusterGalerkinCopyCoefficients(COLOR *dst, COLOR *src, char n) {
    int i;
    COLOR *d;
    COLOR *s;
    for ( i = 0, d = dst, s = src; i < n; i++, d++, s++ ) {
        *d = *s;
    }
}

inline void
clusterGalerkinAddCoefficients(COLOR *dst, COLOR *extra, char n) {
    int i;
    COLOR *d;
    COLOR *s;

    for ( i = 0, d = dst, s = extra; i < n; i++, d++, s++ ) {
        colorAdd(*d, *s, *d);
    }
}

extern GalerkinElement *galerkinCreateClusterHierarchy(Geometry *geom);
extern void galerkinDestroyClusterHierarchy(GalerkinElement *cluster);
extern COLOR clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample);
extern COLOR sourceClusterRadiance(Interaction *link);
extern void iterateOverSurfaceElementsInCluster(GalerkinElement *galerkinElement, void (*func)(GalerkinElement *elem));
extern double receiverClusterArea(Interaction *link);
extern void clusterGatherRadiance(Interaction *link, COLOR *srcRad);
extern COLOR maxClusterRadiance(GalerkinElement *cluster);

#endif
