/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flag chain corresponds to a scattering mode
A chain list is a set of scattering modes
*/

#ifndef __FLAG_CHAIN__
#define __FLAG_CHAIN__

#include "common/dataStructures/CSList.h"
#include "common/color.h"
#include "material/bsdf.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"

class CFlagChain {
public:
    BSDFFLAGS *chain;
    int length;
    bool subtract;

    void init(int length, bool subtract = false);

    CFlagChain(int length = 0, bool subtract = false);

    CFlagChain(const CFlagChain &c); // Copy constructor
    ~CFlagChain();

    // Array access operator
    inline BSDFFLAGS &operator[](const int index) {
        return chain[index];
    }

    COLOR compute(CBiPath *path);
    void print();
};


// Compare two flagchains
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
    void addDisjunct(const CFlagChain &chain);
    void print();
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

    // Methods

    CContribHandler();

    virtual void init(int maxLength);
    virtual ~CContribHandler();
    virtual void addRegExp(char *regExp);
    virtual COLOR compute(CBiPath *path);
    virtual void print();

protected:
    virtual void doRegExp(char *regExp, bool subtract);
    void doSyntaxError(const char *errString);
    bool getFlags(char *regExp, int *pos, BSDFFLAGS *flags);
    bool getToken(char *regExp, int *pos, char *token, BSDFFLAGS *flags);
    void doRegExpGeneral(char *regExp, bool subtract);
};

#endif
