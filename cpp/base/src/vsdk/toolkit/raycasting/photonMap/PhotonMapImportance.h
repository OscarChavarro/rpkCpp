#ifndef PHOTON_MAP_IMPORTANCE__
#define PHOTON_MAP_IMPORTANCE__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/raycasting/photonMap/ImportanceMap.h"

class PhotonMapImportance {
  private:
    static bool hasDiffuseOrGlossy(SimpleRaytracingPathNode *node);
    static bool bounceDiffuseOrGlossy(const SimpleRaytracingPathNode *node);
    static bool doImportanceStore(ImportanceMap *map, SimpleRaytracingPathNode *node, ColorRgb importance);
    static bool
    tracePotentialPath(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig);

  public:
    static void
    tracePotentialPaths(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        int numberOfPaths,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig);
};

#endif
