#ifndef __GALERKIN_CLUSTER_CREATION__
#define __GALERKIN_CLUSTER_CREATION__

#include "GALERKIN/GalerkinElement.h"

class ClusterCreationStrategy {
  private:
    static java::ArrayList<GalerkinElement *> *irregularElementsToDelete;
    static java::ArrayList<GalerkinElement *> *hierarchyElements;

    static void
    patchAddClusterChild(Patch *patch, GalerkinElement *galerkinElement);

    static void
    clusterInit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);

    static void
    addElementToIrregularChildrenDeletionCache(GalerkinElement *galerkinElement);

    static void
    addElementToHierarchiesDeletionCache(GalerkinElement *galerkinElement);

    static void
    geomAddClusterChild(Geometry *geometry, GalerkinElement *galerkinElement, GalerkinState *galerkinState);

  public:
    static GalerkinElement *createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState);
    static void freeClusterElements();
    static void printGalerkinElementHierarchy(const GalerkinElement *galerkinElement, int level);
};

#endif
