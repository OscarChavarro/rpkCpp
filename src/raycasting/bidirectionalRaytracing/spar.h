/**
Specification of the Stored Partial Radiance class
*/

#ifndef __SPAR__
#define __SPAR__

#include "common/color.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/flagchain.h"

#define MAXPATHGROUPS 2

// Some global path groups

#define DISJUNCTGROUP 0
#define LDGROUP 1

class CSpar;

// Spar Config stores handy config params
class CSparConfig {
public:
    BP_BASECONFIG *m_bcfg;

    // Needed in weighted multi-pass methods
    CSpar *m_leSpar;
    CSpar *m_ldSpar;
};

class CSparList : public CTSList<CSpar *> {
public:
    virtual void handlePath(CSparConfig *sconfig,
                            CBiPath *path, COLOR *frad, COLOR *fbpt);
    virtual ~CSparList() {};
};

// iterator

typedef CTSList_Iter<CSpar *> CSparListIter;

class CSpar {
public:
    CContribHandler *m_contrib;
    CSparList *m_sparList;

public:
    CSpar();

    // Destructor
    virtual ~CSpar();

    virtual void init(CSparConfig *config);

    // mainInit spar with a comma separated list of regular expressions
    virtual void parseAndInit(int group, char *regExp);

    // photonMapHandlePath : Handles a bidirectional path. Image contribution
    // is returned. Normally this is a contribution for the pixel
    // affected by the path

    virtual COLOR handlePath(CSparConfig *config, CBiPath *path);
};

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/

class CLeSpar : public CSpar {
public:
    virtual void init(CSparConfig *config);
};

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class CLDSpar : public CSpar {
public:
    virtual void init(CSparConfig *config);
};

#endif
