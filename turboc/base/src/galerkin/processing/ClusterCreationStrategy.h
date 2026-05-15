#ifndef __GALERKIN_CLUSTER_CREATION__
#define __GALERKIN_CLUSTER_CREATION__

#include "galerkin/GalerkinElement.h"

class ClusterCreationStrategy {
  private:
    static ArrayList<GalerkinElement *> *irregularElementsToDelete;
    static ArrayList<GalerkinElement *> *hierarchyElements;

    static void
    patchAddClusterChild(Patch *patch, GalerkinElement *galerkinElement);

    static void
    clusterInit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);

    static void
    addElemTIrrChildrenDltnCch(GalerkinElement *galerkinElement);

    static void
    addElemTHrrchDltnCch(GalerkinElement *galerkinElement);

    static void
    geomAddClusterChild(Geometry *geometry, GalerkinElement *galerkinElement, GalerkinState *galerkinState);

  public:
    static GalerkinElement *createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState);
    static void freeClusterElements();
};

#endif
