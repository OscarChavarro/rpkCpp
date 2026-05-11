#ifndef FLAG_CHAIN_LIST__
#define FLAG_CHAIN_LIST__

#include "common/color/ColorRgb.h"
#include "common/dataStructures/CircularList.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"

class FlagChain;

// A linked list of flag chains.
// Chains in the list are of fixed length !
class FlagChainList final : private CircularList<FlagChain> {
  public:
    int length;
    int count;

    FlagChainList();
    ~FlagChainList() final;
    void add(const FlagChain &chain) final;
    void add(FlagChainList *list);
    void addDisjoint(const FlagChain &chain);
    ColorRgb compute(BiPath *path);
    FlagChainList *simplify();
};

using FlagChainIterator = CircularListIterator<FlagChain>;

#endif
