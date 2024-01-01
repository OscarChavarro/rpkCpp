/* cluster.h: operations specific for cluster elements. */

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include "GALERKIN/galerkinP.h"

/* Creates a cluster for the Geometry, recurses for the children GEOMs, initializes and
 * returns the created cluster. */
extern ELEMENT *GalerkinCreateClusterHierarchy(Geometry *geom);

/* Disposes of the cluster hierarchy */
extern void GalerkinDestroyClusterHierarchy(ELEMENT *cluster);

/* Returns the radiance or unshot radiance (depending on the
 * iteration method) emitted by the src element, a cluster,
 * towards the sample point. */
extern COLOR ClusterRadianceToSamplePoint(ELEMENT *src, Vector3D sample);

/* Determines the average radiance or unshot radiance (depending on
 * the iteration method) emitted by the source cluster towards the 
 * receiver in the link. The source should be a cluster. */
extern COLOR SourceClusterRadiance(INTERACTION *link);

/* Executes func for every surface element in the cluster. */
extern void IterateOverSurfaceElementsInCluster(ELEMENT *clus, void (*func)(ELEMENT *elem));

/* Computes projected area of receiver as seen from the midpoint of the source,
 * ignoring intra-receiver visibility. */
extern double ReceiverClusterArea(INTERACTION *link);

/* Distributes the srcrad radiance to the surface elements in the
 * receiver cluster */
extern void ClusterGatherRadiance(INTERACTION *link, COLOR *srcrad);

extern COLOR MaxClusterRadiance(ELEMENT *clus);

#endif
