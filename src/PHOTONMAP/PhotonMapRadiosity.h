#ifndef _PMAP_H_
#define _PMAP_H_

#include "skin/radianceinterfaces.h"
#include "raycasting/common/pathnode.h"
#include "PHOTONMAP/photonmap.h"

extern RADIANCEMETHOD Pmap;

COLOR GetPmapNodeGRadiance(CPathNode *node);
COLOR GetPmapNodeCRadiance(CPathNode *node);

#endif
