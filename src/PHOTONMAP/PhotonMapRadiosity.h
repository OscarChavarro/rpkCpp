#ifndef _PMAP_H_
#define _PMAP_H_

#include "skin/radianceinterfaces.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

extern RADIANCEMETHOD GLOBAL_photonMapMethods;

COLOR photonMapGetNodeGRadiance(CPathNode *node);
COLOR photonMapGetNodeCRadiance(CPathNode *node);

#endif
