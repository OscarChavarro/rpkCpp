#ifndef __PHOTON_MAP_IMPORTANCE__
#define __PHOTON_MAP_IMPORTANCE__

#include "common/ColorRgb.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/PhotonMapState.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "scene/Background.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"
#include "photonMap/ImportanceMap.h"

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
