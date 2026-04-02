/**
Photon map configuration structure, used during construction
*/

#ifndef __PHOTON_MAP_CONFIG__
#define __PHOTON_MAP_CONFIG__

#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/SamplerConfig.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "photonMap/ImportanceMap.h"
#include "photonMap/PhotonMap.h"

class LightList;

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
