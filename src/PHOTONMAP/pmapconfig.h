/**
Photon map configuration structure, used during construction
*/

#ifndef __PHOTON_MAP_CONFIG__
#define __PHOTON_MAP_CONFIG__

#include "render/ScreenBuffer.h"
#include "raycasting/bidirectionalRaytracing/bipath.h"
#include "raycasting/raytracing/samplertools.h"
#include "PHOTONMAP/photonmap.h"
#include "PHOTONMAP/importancemap.h"

class PhotonMapConfig {
  public:
    CSamplerConfig lightConfig;
    CSamplerConfig eyeConfig;
    CBiPath biPath;

    CImportanceMap *importanceMap;
    CImportanceMap *importanceCMap;
    CPhotonMap *globalMap;
    CPhotonMap *causticMap;

    CPhotonMap *currentMap; // Map in current use: global or caustic
    CImportanceMap *currentImpMap; // Importance Map in current use: global or caustic

    ScreenBuffer *screen;

    PhotonMapConfig(): lightConfig(), eyeConfig(), biPath(),
                       importanceMap(), importanceCMap(), globalMap(),
                       causticMap(), currentMap(), currentImpMap(), screen() {
        screen = nullptr;
        currentMap = nullptr;
    }
};

extern PhotonMapConfig GLOBAL_photonMap_config;

#endif
