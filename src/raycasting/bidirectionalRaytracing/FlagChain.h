/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flag chain corresponds to a scattering mode
A chain list is a set of scattering modes
*/

#ifndef __FLAG_CHAIN__
#define __FLAG_CHAIN__

#include "common/dataStructures/CircularList.h"
#include "common/ColorRgb.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/bidirectionalRaytracing/bipath.h"

class FlagChain {
  public:
    char *chain;
    int length;
    bool subtract;

    void init(int inLength, bool inSubtract = false);

    explicit FlagChain(int paramLength = 0, bool paramSubtract = false);

    FlagChain(const FlagChain &c); // Copy constructor
    ~FlagChain();

    ColorRgb compute(CBiPath *path) const;
};


// Compare two flag chains
bool
FlagChainCompare(const FlagChain *c1, const FlagChain *c2);

// try to combine two flag chains into one equivalent chain
// Only one element in the chain may differ.
FlagChain *
FlagChainCombine(const FlagChain *chain1, const FlagChain *chain2);

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
    ColorRgb compute(CBiPath *path);
    FlagChainList *simplify();
};

typedef CircularListIterator<FlagChain> FlagChainIterator;

/**
An array of chain lists indexed by length
*/
class ContribHandler {
  public:
    FlagChainList *array;
    int maxLength;

    ContribHandler();
    virtual void init(int paramMaxLength);
    virtual ~ContribHandler();
    virtual void addRegExp(char *regExp);
    virtual ColorRgb compute(CBiPath *path);

  protected:
    virtual void doRegExp(char *regExp, bool subtract);
    void doSyntaxError(const char *errString);
    bool getFlags(const char *regExp, int *pos, char *flags);
    bool getToken(const char *regExp, int *pos, char *token, char *flags);
    void doRegExpGeneral(const char *regExp, bool subtract);
};

#endif
