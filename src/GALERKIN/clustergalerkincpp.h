#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/galerkinP.h"

#define CLEARCOEFFICIENTS_CG(c, n) {int _i; COLOR *_c; \
  for (_i=0, _c=(c); _i<(n); _i++, _c++) colorClear(*_c); \
}

#define COPYCOEFFICIENTS_CG(dst, src, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(src); _i<(n); _i++, _d++, _s++) *_d=*_s; \
}

#define ADDCOEFFICIENTS_CG(dst, extra, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(extra); _i<(n); _i++, _d++, _s++) { \
    colorAdd(*_d, *_s, *_d); \
}}

/* Creates a cluster for the Geometry, recurses for the children GEOMs, initializes and
 * returns the created cluster. */
extern ELEMENT *galerkinCreateClusterHierarchy(Geometry *geom);

/* Disposes of the cluster hierarchy */
extern void galerkinDestroyClusterHierarchy(ELEMENT *cluster);

/* Returns the radiance or un-shot radiance (depending on the
 * iteration method) emitted by the src element, a cluster,
 * towards the sample point. */
extern COLOR clusterRadianceToSamplePoint(ELEMENT *src, Vector3D sample);

/* Determines the average radiance or un-shot radiance (depending on
 * the iteration method) emitted by the source cluster towards the 
 * receiver in the link. The source should be a cluster. */
extern COLOR sourceClusterRadiance(INTERACTION *link);

/* Executes func for every surface element in the cluster. */
extern void iterateOverSurfaceElementsInCluster(ELEMENT *clus, void (*func)(ELEMENT *elem));

/* Computes projected area of receiver as seen from the midpoint of the source,
 * ignoring intra-receiver visibility. */
extern double receiverClusterArea(INTERACTION *link);

/* Distributes the srcrad radiance to the surface elements in the
 * receiver cluster */
extern void clusterGatherRadiance(INTERACTION *link, COLOR *srcrad);

extern COLOR maxClusterRadiance(ELEMENT *clus);

#endif
