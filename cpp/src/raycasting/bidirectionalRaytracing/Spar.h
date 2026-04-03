/**
Specification of the Stored Partial Radiance class
*/

#ifndef __SPAR__
#define __SPAR__

#include "common/ColorRgb.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "raycasting/bidirectionalRaytracing/FlagChain.h"
#include "raycasting/bidirectionalRaytracing/SparPathGroup.h"
#include "raycasting/bidirectionalRaytracing/SparConfig.h"
#include "raycasting/bidirectionalRaytracing/SparList.h"

class Spar {
  public:
    ContribHandler *m_contrib;
    SparList *m_sparList;

    Spar();
    virtual ~Spar();

    virtual void init(SparConfig *config, RadianceMethod *radianceMethod);
    virtual void parseAndInit(int group, char *regExp);
    virtual ColorRgb handlePath(SparConfig *config, BiPath *path);
};

#include "raycasting/bidirectionalRaytracing/LeSpar.h"
#include "raycasting/bidirectionalRaytracing/LDSpar.h"

#endif
