/**
Specification of the Stored Partial Radiance class
*/

#ifndef __SPAR__
#define __SPAR__

#include "common/ColorRgb.h"

#include "scene/RadianceMethod.h"

#include "raycasting/common/pathnode.h"

#include "raycasting/bidirectionalRaytracing/bipath.h"
#include "raycasting/bidirectionalRaytracing/FlagChain.h"
#include "raycasting/bidirectionalRaytracing/SparPathGroup.h"
#include "raycasting/bidirectionalRaytracing/SparConfig.h"

class Spar;
class SparList;

class Spar {
  public:
    ContribHandler *m_contrib;
    SparList *m_sparList;

    Spar();
    virtual ~Spar();

    virtual void init(SparConfig *config, RadianceMethod *radianceMethod);
    virtual void parseAndInit(int group, char *regExp);
    virtual ColorRgb handlePath(SparConfig *config, CBiPath *path);
};

#include "raycasting/bidirectionalRaytracing/SparList.h"
#include "raycasting/bidirectionalRaytracing/LeSpar.h"
#include "raycasting/bidirectionalRaytracing/LDSpar.h"

#endif
