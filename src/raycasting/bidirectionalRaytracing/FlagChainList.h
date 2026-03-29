#ifndef __FLAG_CHAIN_LIST__
#define __FLAG_CHAIN_LIST__

#include "common/ColorRgb.h"
#include "common/dataStructures/CircularList.h"

class BiPath;
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

typedef CircularListIterator<FlagChain> FlagChainIterator;

#endif
