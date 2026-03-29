#ifndef __PHOTON_MAP_IMPORTANCE__
#define __PHOTON_MAP_IMPORTANCE__

#include "common/ColorRgb.h"
#include "scene/Background.h"
#include "scene/Camera.h"
#include "scene/VoxelGrid.h"

class ImportanceMap;
class SimpleRaytracingPathNode;
class PhotonMapConfig;

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
        PhotonMapConfig *config);

  public:
    static void
    tracePotentialPaths(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        int numberOfPaths);
};

#endif
