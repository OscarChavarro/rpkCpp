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

class Spar;

// Spar Config stores handy config params
class SparConfig {
  public:
    BidirectionalPathRaytracerConfig *baseConfig;

    // Needed in weighted multi-pass methods
    Spar *leSpar;
    Spar *ldSpar;
};

class SparList final : public CircularList<Spar *> {
  public:
    void
    handlePath(
        SparConfig *config,
        CBiPath *path,
        ColorRgb *fRad,
        ColorRgb *fBpt);
    ~SparList() final;
};

typedef CircularListIterator<Spar *> CSparListIter;

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

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
class LeSpar final : public Spar {
  public:
    void init(SparConfig *sparConfig, RadianceMethod *radianceMethod) final;
};

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class LDSpar final : public Spar {
  public:
    void init(SparConfig *sparConfig, RadianceMethod *radianceMethod) final;
};

#endif
