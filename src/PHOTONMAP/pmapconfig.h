// GLOBAL_photonMap_config.H : class for Photon map configuration
/* photon map configuration structure, used during construction */

#ifndef _PMAPCONFIG_H_
#define _PMAPCONFIG_H_

#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/samplertools.h"
#include "PHOTONMAP/photonmap.h"
#include "PHOTONMAP/importancemap.h"

class PMAPCONFIG {
public:
    CSamplerConfig lightConfig;
    CSamplerConfig eyeConfig;
    CBiPath bipath;

    CImportanceMap *importanceMap;
    CImportanceMap *importanceCMap;
    CPhotonMap *globalMap;
    CPhotonMap *causticMap;

    CPhotonMap *currentMap; // Map in current use: global or caustic
    CImportanceMap *currentImpMap; // Importance Map in current use: global or caustic

    ScreenBuffer *screen;

    PMAPCONFIG() {
        screen = nullptr;
        currentMap = nullptr;
    }
};

extern PMAPCONFIG GLOBAL_photonMap_config;

#endif
