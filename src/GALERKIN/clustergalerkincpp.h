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

extern GalerkingElement *galerkinCreateClusterHierarchy(Geometry *geom);
extern void galerkinDestroyClusterHierarchy(GalerkingElement *cluster);
extern COLOR clusterRadianceToSamplePoint(GalerkingElement *src, Vector3D sample);
extern COLOR sourceClusterRadiance(INTERACTION *link);
extern void iterateOverSurfaceElementsInCluster(GalerkingElement *clus, void (*func)(GalerkingElement *elem));
extern double receiverClusterArea(INTERACTION *link);
extern void clusterGatherRadiance(INTERACTION *link, COLOR *srcrad);
extern COLOR maxClusterRadiance(GalerkingElement *clus);

#endif
