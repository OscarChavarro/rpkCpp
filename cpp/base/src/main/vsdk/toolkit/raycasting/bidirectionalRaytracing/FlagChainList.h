#ifndef FLAG_CHAIN_LIST__
#define FLAG_CHAIN_LIST__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/dataStructures/CircularList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"

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
    ColorRgbMutable compute(BiPath *path);
    FlagChainList *simplify();
};

using FlagChainIterator = CircularListIterator<FlagChain>;

#endif
