#ifndef _PMAP_H_
#define _PMAP_H_

#include "java/util/ArrayList.h"
#include "skin/radianceinterfaces.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

extern RADIANCEMETHOD GLOBAL_photonMapMethods;

COLOR photonMapGetNodeGRadiance(SimpleRaytracingPathNode *node);
COLOR photonMapGetNodeCRadiance(SimpleRaytracingPathNode *node);

#endif
