/**
Specification of the Stored Partial Radiance class
*/

#ifndef __SPAR__
#define __SPAR__

#include "common/ColorRgb.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/bidirectionalRaytracing/FlagChain.h"

#define MAX_PATH_GROUPS 2

// Some global path groups

#define DISJOINT_GROUP 0
#define LD_GROUP 1

class Spar;

// Spar Config stores handy config params
class SparConfig {
public:
    BP_BASECONFIG *baseConfig;

    // Needed in weighted multi-pass methods
    Spar *leSpar;
    Spar *ldSpar;
};

class CSparList : public CTSList<Spar *> {
public:
    virtual void
    handlePath(
        SparConfig *config,
        CBiPath *path,
        ColorRgb *fRad,
        ColorRgb *fBpt);
    virtual ~CSparList() {
    };
};

typedef CTSList_Iter<Spar *> CSparListIter;

class Spar {
  public:
    ContribHandler *m_contrib;
    CSparList *m_sparList;

    Spar();
    virtual ~Spar();

    virtual void init(SparConfig *config, RadianceMethod *radianceMethod);
    virtual void parseAndInit(int group, char *regExp);
    virtual ColorRgb handlePath(SparConfig *config, CBiPath *path);
};

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
class LeSpar : public Spar {
  public:
    virtual void init(SparConfig *sparConfig, RadianceMethod *radianceMethod);
};

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class LDSpar : public Spar {
  public:
    virtual void init(SparConfig *sparConfig, RadianceMethod *radianceMethod);
};

#endif
