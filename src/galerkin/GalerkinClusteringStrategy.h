#ifndef __GALERKIN_CLUSTERING_STRATEGY__
#define __GALERKIN_CLUSTERING_STRATEGY__

// Determines how source radiance of a source cluster is determined and
// how irradiance is distributed over the patches in a receiver cluster
enum GalerkinClusteringStrategy {
    ISOTROPIC,
    ORIENTED,
    Z_VISIBILITY
};

#endif
