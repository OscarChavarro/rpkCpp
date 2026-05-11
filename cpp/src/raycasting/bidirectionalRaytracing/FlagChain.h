/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flag chain corresponds to a scattering mode
A chain list is a set of scattering modes
*/

#ifndef FLAG_CHAIN__
#define FLAG_CHAIN__

#include "common/color/ColorRgb.h"
#include "common/dataStructures/CircularList.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"

class FlagChain {
  public:
    char *chain;
    int length;
    bool subtract;

    void init(int inLength, bool inSubtract = false);

    explicit FlagChain(int paramLength = 0, bool paramSubtract = false);

    FlagChain(const FlagChain &c); // Copy constructor
    ~FlagChain();

    ColorRgb compute(BiPath *path) const;
    static bool compare(const FlagChain *c1, const FlagChain *c2);
    static FlagChain *combine(const FlagChain *chain1, const FlagChain *chain2);
};

#include "raycasting/bidirectionalRaytracing/FlagChainList.h"
#include "raycasting/bidirectionalRaytracing/ContribHandler.h"

#endif
