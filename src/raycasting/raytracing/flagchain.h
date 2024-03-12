/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flag chain corresponds to a scattering mode
A chain list is a set of scattering modes
*/

#ifndef __FLAG_CHAIN__
#define __FLAG_CHAIN__

#include "common/dataStructures/CircularList.h"
#include "common/color.h"
#include "material/bsdf.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"

class CFlagChain {
  public:
    BSDF_FLAGS *chain;
    int length;
    bool subtract;

    void init(int paramLength, bool paramSubtract = false);

    explicit CFlagChain(int paramLength = 0, bool paramSubtract = false);

    CFlagChain(const CFlagChain &c); // Copy constructor
    ~CFlagChain();

    // Array access operator
    inline BSDF_FLAGS &operator[](const int index) const {
        return chain[index];
    }

    COLOR compute(CBiPath *path) const;
};


// Compare two flag chains
bool FlagChainCompare(const CFlagChain *c1,
                      const CFlagChain *c2);

// try to combine two flag chains into one equivalent chain
// Only one element in the chain may differ.
CFlagChain *FlagChainCombine(const CFlagChain *chain1,
                             const CFlagChain *chain2);


// A linked list of flag chains.
// Chains in the list are of fixed length !

class CChainList : private CTSList<CFlagChain> {
  public:
    int length;
    int count;

    CChainList();
    ~CChainList();
    void add(const CFlagChain &chain);
    void add(CChainList *list);
    void addDisjoint(const CFlagChain &chain);
    COLOR compute(CBiPath *path);
    CChainList *simplify();
};

// iterator

typedef CTSList_Iter<CFlagChain> CFlagChainIter;

// An array of chain lists indexed by length
class CContribHandler {
  public:
    CChainList *array;
    int maxLength;

    CContribHandler();
    virtual void init(int paramMaxLength);
    virtual ~CContribHandler();
    virtual void addRegExp(char *regExp);
    virtual COLOR compute(CBiPath *path);

  protected:
    virtual void doRegExp(char *regExp, bool subtract);
    void doSyntaxError(const char *errString);
    bool getFlags(const char *regExp, int *pos, BSDF_FLAGS *flags);
    bool getToken(char *regExp, int *pos, char *token, BSDF_FLAGS *flags);
    void doRegExpGeneral(char *regExp, bool subtract);
};

#endif
