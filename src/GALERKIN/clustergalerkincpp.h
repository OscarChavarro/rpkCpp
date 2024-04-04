#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/galerkinP.h"

inline void
clusterGalerkinClearCoefficients(ColorRgb *color, char n) {
    int i;
    ColorRgb *c;
    for ( i = 0, c = color; i < n; i++, c++ ) {
        c->clear();
    }
}

inline void
clusterGalerkinCopyCoefficients(ColorRgb *dst, ColorRgb *src, char n) {
    int i;
    ColorRgb *d;
    ColorRgb *s;
    for ( i = 0, d = dst, s = src; i < n; i++, d++, s++ ) {
        *d = *s;
    }
}

inline void
clusterGalerkinAddCoefficients(ColorRgb *dst, ColorRgb *extra, char n) {
    int i;
    ColorRgb *d;
    ColorRgb *s;

    for ( i = 0, d = dst, s = extra; i < n; i++, d++, s++ ) {
        colorAdd(*d, *s, *d);
    }
}

extern GalerkinElement *galerkinCreateClusterHierarchy(Geometry *geom);
extern void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);
extern ColorRgb clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample);
extern ColorRgb sourceClusterRadiance(Interaction *link);
extern void iterateOverSurfaceElementsInCluster(GalerkinElement *galerkinElement, void (*func)(GalerkinElement *elem));
extern double receiverClusterArea(Interaction *link);
extern void clusterGatherRadiance(Interaction *link, ColorRgb *srcRad);
extern ColorRgb maxClusterRadiance(GalerkinElement *cluster);

#endif
