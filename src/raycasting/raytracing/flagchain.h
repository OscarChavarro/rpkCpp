/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flagchain corresponds to a scatteringmode
A chainlist is a set of scattering modes
*/

#ifndef _FLAGCHAIN_H_
#define _FLAGCHAIN_H_

#include "common/dataStructures/CSList.h"
#include "material/color.h"
#include "material/bsdf.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"

// FlagChain class

class CFlagChain {
public:
    BSDFFLAGS *m_chain;
    int m_length;
    bool m_subtract;

    // Methods

    void Init(const int length, const bool subtract = false);

    CFlagChain(const int length = 0, const bool subtract = false);

    CFlagChain(const CFlagChain &c); // Copy constructor
    ~CFlagChain();

    // Array access operator

    inline BSDFFLAGS &operator[](const int index) {
        return m_chain[index];
    }

    // Compute : calculate the product of bsdf components defined
    //   by the chain. Eye and light node ARE INCLUDED

    COLOR Compute(CBiPath *path);

    void Print();
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
    int m_length, m_count;

    CChainList();

    ~CChainList();

    void Add(const CFlagChain &chain);

    void Add(CChainList *list);

    void AddDisjunct(const CFlagChain &chain);

    void Print();

    COLOR Compute(CBiPath *path);

    // Simplify the chainlist returning the equivalent
    // simplified chainlist. Equal entries MAY be reduced to a
    // single entry! (So in fact no equal entries is advisable)
    CChainList *Simplify();
};

// iterator

typedef CTSList_Iter<CFlagChain> CFlagChainIter;



// An array of chainlists indexed by length

class CContribHandler {
public:
    CChainList *m_array;
    int m_maxLength;

    // Methods

    CContribHandler();

    virtual void Init(int maxLength);

    virtual ~CContribHandler();

    // Add a group of paths
    // regExp indicates the regular expression covered by the sampling strategy
    // The class of covered paths covered by the contribhandler is : (regSPaR)(regPath)
    // regSPar is not needed here. The regExp must ensure disjunct paths !

    virtual void AddRegExp(char *regExp);

    // Same but path contribs are SUBTRACTED !!
    virtual void SubRegExp(char *regExp);

    virtual COLOR Compute(CBiPath *path);

    virtual void Print();

protected:
    virtual void DoRegExp(char *regExp, bool subtract);

    void DoSyntaxError(const char *errString);

    bool GetFlags(char *regExp, int *pos, BSDFFLAGS *flags);

    bool GetToken(char *regExp, int *pos, char *token, BSDFFLAGS *flags);

    void DoRegExp_General(char *regExp, bool subtract);

    // Several hard coded regexp pairs (A = Any = *, M = Many = +, X = all components)
    // For the eye and readout nodes (first vertex of eye and light path)
    // ALL components are included by default.
    void DoRegExp_EX(bool subtract); // Eval : "(EX)" just raycasting readout
    void DoRegExp_DREX(bool subtract); // Eval : "(DR)(EX)" direct diffuse
    void DoRegExp_XA(bool subtract); // Eval : "(X)*"
    void DoRegExp_GRXA(bool subtract); // Eval : "(GR|SR|XT)(X)*" (complements LD*)
    void DoRegExp_DRGRXA(bool subtract); // Eval : "(DR)(GR|SR|XT)(X)*"
    void DoRegExp_SMDR(bool subtract); // Eval : "(SR|ST)+(DR)"
    void DoRegExp_DRSMDR(bool subtract); // Eval : "(DR)(SR|ST)+(DR)"
};

#endif /* _FLAGCHAIN_H_ */
