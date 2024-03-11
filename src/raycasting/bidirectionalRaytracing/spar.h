/**
Specification of the Stored Partial Radiance class
*/

#ifndef __SPAR__
#define __SPAR__

#include "common/color.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/flagchain.h"

#define MAX_PATH_GROUPS 2

// Some global path groups

#define DISJOINT_GROUP 0
#define LD_GROUP 1

class Spar;

// Spar Config stores handy config params
class CSparConfig {
public:
    BP_BASECONFIG *baseConfig;

    // Needed in weighted multi-pass methods
    Spar *leSpar;
    Spar *ldSpar;
};

class CSparList : public CTSList<Spar *> {
public:
    virtual void handlePath(CSparConfig *config,
                            CBiPath *path, COLOR *fRad, COLOR *fBpt);
    virtual ~CSparList() {
    };
};

typedef CTSList_Iter<Spar *> CSparListIter;

class Spar {
  public:
    CContribHandler *m_contrib;
    CSparList *m_sparList;

  public:
    Spar();
    virtual ~Spar();

    virtual void init(CSparConfig *config);
    virtual void parseAndInit(int group, char *regExp);
    virtual COLOR handlePath(CSparConfig *config, CBiPath *path);
};

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
class LeSpar : public Spar {
public:
    virtual void init(CSparConfig *config);
};

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class LDSpar : public Spar {
public:
    virtual void init(CSparConfig *config);
};

#endif
