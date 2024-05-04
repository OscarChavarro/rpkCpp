#ifndef __GALERKIN_CLUSTER_CREATION__
#define __GALERKIN_CLUSTER_CREATION__

#include "GALERKIN/GalerkinElement.h"

class ClusterCreationStrategy {
  private:
    static java::ArrayList<GalerkinElement *> *irregularElementsToDelete;
    static java::ArrayList<GalerkinElement *> *hierarchyElements;

    static void
    patchAddClusterChild(Patch *patch, GalerkinElement *cluster);

    static void
    clusterInit(GalerkinElement *cluster, const GalerkinState *galerkinState);

    static void
    addElementToIrregularChildrenDeletionCache(GalerkinElement *element);

    static void
    addElementToHierarchiesDeletionCache(GalerkinElement *element);

    static void
    geomAddClusterChild(Geometry *geometry, GalerkinElement *parentCluster, GalerkinState *galerkinState);

    static GalerkinElement *
    galerkinDoCreateClusterHierarchy(Geometry *parentGeometry, GalerkinState *galerkinState);
  public:
    static GalerkinElement *createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState);
    static void freeClusterElements();
};

#endif
