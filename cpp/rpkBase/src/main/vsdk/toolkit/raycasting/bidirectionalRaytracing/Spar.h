/**
Specification of the Stored Partial Radiance class
*/

#ifndef SPAR__
#define SPAR__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/raycasting/common/SimpleRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/FlagChain.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/SparPathGroup.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/SparConfig.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/SparList.h"

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

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LeSpar.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LDSpar.h"

#endif
