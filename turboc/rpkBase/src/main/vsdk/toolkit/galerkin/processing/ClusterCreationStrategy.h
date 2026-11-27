#ifndef __CLUSTER_CREATION_STRATEGY__
#define __CLUSTER_CREATION_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"

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
    geomAddClusterChild(RpkGeometry *geometry, GalerkinElement *galerkinElement, GalerkinState *galerkinState);

  public:
    static GalerkinElement *createClusterHierarchy(RpkGeometry *geometry, GalerkinState *galerkinState);
    static void freeClusterElements();
};

#endif
