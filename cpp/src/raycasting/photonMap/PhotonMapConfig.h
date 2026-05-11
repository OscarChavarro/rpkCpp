/**
Photon map configuration structure, used during construction
*/

#ifndef PHOTON_MAP_CONFIG__
#define PHOTON_MAP_CONFIG__

#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/SamplerConfig.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/photonMap/ImportanceMap.h"
#include "raycasting/photonMap/PhotonMap.h"

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
