#ifndef __GALERKIN_CLUSTER_CREATION__
#define __GALERKIN_CLUSTER_CREATION__

#include "GALERKIN/GalerkinElement.h"

class ClusterCreationStrategy {
  public:
    static GalerkinElement *createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState);
    static void freeClusterElements();
};

#endif
