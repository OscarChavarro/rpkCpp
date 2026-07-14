/**
Photon map configuration structure, used during construction
*/

#ifndef PHOTON_MAP_CONFIG__
#define PHOTON_MAP_CONFIG__

#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/raytracing/SamplerConfig.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/toolkit/raycasting/photonMap/ImportanceMap.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMap.h"

class PhotonMapConfig {
  public:
    SamplerConfig lightConfig;
    SamplerConfig eyeConfig;
    BiPath biPath;

    ImportanceMap *importanceMap;
    ImportanceMap *importanceCMap;
    PhotonMap *map;
    PhotonMap *causticMap;

    PhotonMap *currentMap; // Map in current use: global or caustic
    ImportanceMap *currentImpMap; // Importance Map in current use: global or caustic

    ScreenBuffer *screen;
    LightList *lightList;

    PhotonMapConfig(): lightConfig(), eyeConfig(), biPath(),
                       importanceMap(), importanceCMap(), map(),
                       causticMap(), currentMap(), currentImpMap(), screen(), lightList() {
        screen = nullptr;
        currentMap = nullptr;
        lightList = nullptr;
    }
};

#endif
