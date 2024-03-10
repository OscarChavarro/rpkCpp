#ifndef __PHOTON_MAP_RADIOSITY_
#define __PHOTON_MAP_RADIOSITY_

#include "java/util/ArrayList.h"
#include "skin/RadianceMethod.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

extern RADIANCEMETHOD GLOBAL_photonMapMethods;

COLOR photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node);
COLOR photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node);

#endif
