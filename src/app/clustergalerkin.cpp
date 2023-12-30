/*
clusters of patches. A hierarchy of clusters is automatically built
after loading a scene.

References: Per Christensen, PhD Thesis "Hierarchical Techniques for Glossy
Global Illumination", Univ. of Washington, 1995, p116-117
*/

#include <cstdlib>

#include "skin/patch.h"
#include "skin/patchlist_geom.h"
#include "skin/geom.h"
#include "app/compound.h"
#include "app/cluster.h"

/* No clusters are created with less than this number of patches. */
#ifndef MIN_PATCHES_IN_CLUSTER
    #define MIN_PATCHES_IN_CLUSTER 3
#endif

/* To create the hierarchy */
class CLUSTER {
  public:
    BOUNDINGBOX bounds;        /* bounding box for the cluster */
    Vector3D mid;            /* midpoint of the bounding box */
    PATCHLIST *patches;        /* list of patches in this cluster */
    int nrpatches;        /* nr of patches in this cluster */
    CLUSTER *children[8];    /* 8 subclusters: clusters form an octree */
    void *radiosity_data;        /* radiosity data */
};

/* Creates an empty cluster with initialized bounding box. */
static CLUSTER *CreateCluster() {
    int i;
    CLUSTER *clus = (CLUSTER *)malloc(sizeof(CLUSTER));
    BoundsInit(clus->bounds);
    VECTORSET(clus->mid, 0., 0., 0.);
    clus->patches = PatchListCreate();
    clus->nrpatches = 0;

    for ( i = 0; i < 8; i++ ) {
        clus->children[i] = (CLUSTER *) nullptr;
    }

    clus->radiosity_data = (void *) nullptr;

    return clus;
}

/* Adds a patch to a cluster. The bounding box is enlarged if necessary, but
 * the midpoint is not updated (it's more efficient to do that once, after all
 * patches have been added to the cluster and the bounding box is fully 
 * determined). */
static void ClusterAddPatch(PATCH *patch, CLUSTER *clus) {
    BOUNDINGBOX patchbounds;
    clus->patches = PatchListAdd(clus->patches, patch);
    clus->nrpatches++;
    BoundsEnlarge(clus->bounds, patch->bounds ? patch->bounds : PatchBounds(patch, patchbounds));
}

/* Creates a toplevel CLUSTER. The patch list of the cluster contains all
 * patches. */
static CLUSTER *CreateToplevelCluster(PATCHLIST *patches) {
    CLUSTER *clus;

    clus = CreateCluster();
    PatchListIterate1A(patches, ClusterAddPatch, clus);
    VECTORSET(clus->mid,
              (clus->bounds[MIN_X] + clus->bounds[MAX_X]) * 0.5,
              (clus->bounds[MIN_Y] + clus->bounds[MAX_Y]) * 0.5,
              (clus->bounds[MIN_Z] + clus->bounds[MAX_Z]) * 0.5);

    return clus;
}

/* Checks the size of fl->patch w.r.t. the bounding box of the cluster clus. If
 * the size of the patch is more than half the size of the cluster, the routine
 * returns. If the patch is smaller than half the size of the cluster, the position of
 * its centroid is tested w.r.t. the centroid of the cluster. If the centroids 
 * coincide, the routine returns. If not, fl is moved to the patch list of a subcluster
 * of clus. Which subcluster depends on the position of the patch w.r.t. the centroid
 * of clus. Returns true if the patch pointed to by fl was moved to the subcluster.
 * Returns false if the patch was not moved. prevfl is the PATCHLIST element preceeding
 * fl (chasing pointers!), needed to be able to efficiently remove fl from the
 * patchlist of clus. */
static int ClusterCheckMovePatch(PATCHLIST *fl, PATCHLIST *prevfl, CLUSTER *clus) {
    int subi;
    Vector3D patchmid;
    float *patchbounds;
    CLUSTER *subclus;
    float dx, dy, dz;

    /* All patches that were added to the top cluster, which is being split now,
     * have a bounding box computed for them. */
    patchbounds = fl->patch->bounds;

    /* if the patch is larger than an octant, return */
    dx = patchbounds[MAX_X] - patchbounds[MIN_X];
    dy = patchbounds[MAX_Y] - patchbounds[MIN_Y];
    dz = patchbounds[MAX_Z] - patchbounds[MIN_Z];
    if ((dx > 10 * EPSILON && dx > (clus->bounds[MAX_X] - clus->bounds[MIN_X]) * 0.5) ||
        (dy > 10 * EPSILON && dy > (clus->bounds[MAX_Y] - clus->bounds[MIN_Y]) * 0.5) ||
        (dz > 10 * EPSILON && dz > (clus->bounds[MAX_Z] - clus->bounds[MIN_Z]) * 0.5)) {
        return false;
    }

    /* check the position of the centroid of the boundingbox of the patch w.r.t. the
     * centroid of the cluster */
    VECTORSET(patchmid,
              (patchbounds[MIN_X] + patchbounds[MAX_X]) * 0.5,
              (patchbounds[MIN_Y] + patchbounds[MAX_Y]) * 0.5,
              (patchbounds[MIN_Z] + patchbounds[MAX_Z]) * 0.5);
    subi = VectorCompare(&clus->mid, &patchmid, 0.);

    /* accidently, the centroids (almost) coincide, keep the patch as a direct child of the
     * cluster. */
    if ( subi == 8 ) {
        return false;
    }

    /* otherwise, move the patch to the subcluster with index subi */
    subclus = clus->children[subi];

    if ( fl == clus->patches ) {
        clus->patches = clus->patches->next;
    } else {
        prevfl->next = fl->next;
    }
    clus->nrpatches--;

    fl->next = subclus->patches;
    subclus->patches = fl;
    subclus->nrpatches++;

    /* enlarge the bounding box the of subcluster */
    BoundsEnlarge(subclus->bounds, patchbounds);

    return true; // fl was moved to the subcluster
}

/* Splits a cluster into subclusters: it is assumed that clus is a cluster without
 * subclusters yet. If clus is nullptr or there are no more than MIN_PATCHES_IN_CLUSTER 
 * patches in the cluster, the routine simply returns. If there are more patches, 8 
 * subclusters are created for the cluster. Then for each patch, if the patch
 * is smaller than an octant, the patch is moved to the octant containing its
 * centroid. Otherwise, the patch remains a direct child of the cluster. Finally,
 * subclusters with zero patches are disposed off and the procedure recursively repeated
 * for each subcluster. */
static void SplitCluster(CLUSTER *clus) {
    int i;
    PATCHLIST *fl, *next, *prev;

    /* don't split the cluster if it contains too few patches. */
    if ( clus == (CLUSTER *) nullptr || clus->nrpatches <= MIN_PATCHES_IN_CLUSTER ) {
        return;
    }

    /* Create eight subclusters for the cluster with initialized bounding box. */
    for ( i = 0; i < 8; i++ ) {
        clus->children[i] = CreateCluster();
    }

    /* check and possibly move each of the patches in the cluster to a subcluster */
    fl = clus->patches;
    prev = (PATCHLIST *) nullptr;
    while ( fl ) {
        next = fl->next;    /* fl can be moved to a subcluster, after which fl->next will
			 * point to the patches already present in the subcluster. */
        if ( !ClusterCheckMovePatch(fl, prev, clus)) {
            prev = fl;
        }    /* fl was not moved to a subcluster. If fl is moved,
			* prev remains the same. */
        fl = next;
    }

    /* dispose of subclusters containing no patches, call SplitCluster recursively for
     * non empty subclusters */
    for ( i = 0; i < 8; i++ ) {
        if ( clus->children[i]->nrpatches == 0 ) {
            free(clus->children[i]);
            clus->children[i] = (CLUSTER *) nullptr;
        } else {
            VECTORSET(clus->children[i]->mid,
                      (clus->children[i]->bounds[MIN_X] + clus->children[i]->bounds[MAX_X]) * 0.5,
                      (clus->children[i]->bounds[MIN_Y] + clus->children[i]->bounds[MAX_Y]) * 0.5,
                      (clus->children[i]->bounds[MIN_Z] + clus->children[i]->bounds[MAX_Z]) * 0.5);
            SplitCluster(clus->children[i]);
        }
    }
}

/* Converts a cluster hierarchy to a "normal" GEOM hierarchy. The
 * "normal" routines for raytracing can be used to trace a ray through
 * the cluster hierarchy and shaftculling and such can be done on clusters
 * without extra code and such ... This routine is destructive: 
 * the CLUSTER hierarchy is disposed of (the PATCHLISTs of the clusters
 * are copied to the GEOMs). */
static GEOM *ConvertClusterToGeom(CLUSTER *clus) {
    GEOMLIST *children;
    GEOM *child, *the_patches;
    int i;

    if ( !clus ) {
        return (GEOM *) nullptr;
    }

    the_patches = (GEOM *) nullptr;
    if ( clus->patches ) {
        the_patches = GeomCreate(clus->patches, PatchListMethods());
    }

    children = GeomListCreate();
    for ( i = 0; i < 8; i++ ) {
        if ((child = ConvertClusterToGeom(clus->children[i]))) {
            children = GeomListAdd(children, child);
        }
    }

    free(clus);

    if ( !children ) {
        return the_patches;
    }

    /* the patches in the cluster are the first to be tested for intersection with */
    children = GeomListAdd(children, the_patches);
    return GeomCreate(children, CompoundMethods());
}

/* Creates a hierarchical model of the discrete scene (the patches
 * in the scene) using the simple algorithm described in
 * - Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
 *   PhD Thesis, University of Washington, 1995, p 116 
 * This hierarchy is often much more efficient for tracing rays and
 * clustering radiosity algorithms than the given hierarchy of
 * bounding boxes. A pointer to the toplevel "cluster" is returned. */
GEOM *CreateClusterHierarchy(PATCHLIST *patches) {
    CLUSTER *top;
    GEOM *topgeom;

    /* create a toplevel cluster containing (references to) all the patches in the solid */
    top = CreateToplevelCluster(patches);

    /* split the toplevel cluster recursively into subclusters */
    SplitCluster(top);

    /* convert to a GEOM hierarchy, disposing of the CLUSTERs */
    topgeom = ConvertClusterToGeom(top);

    return topgeom;
}
