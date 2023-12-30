/* cluster.h: patch cluster hierarchies: after loading a scene, a hierarchy of clusters
 * containing patches is built. Since only the position and size of the patches is 
 * relevant for constructing this hierarchy (and not e.g. the material of the patches or
 * the geometry to which they belong ...), this automatically constructed hierarchy 
 * is often more efficient for tracing rays and for use in a clustering radisiosity 
 * method etc... */

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include "skin/geom.h"

/* Creates a hierarchical model of the discretised scene (the patches 
 * in the scene) using the simple algorithm described in
 * - Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
 *   PhD Thesis, University of Washington, 1995, p 116 
 * A pointer to the toplevel "cluster" is returned. */
extern GEOM *CreateClusterHierarchy(PATCHLIST *patches);

#endif
