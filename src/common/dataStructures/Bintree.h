#ifndef _BINTREE_H_
#define _BINTREE_H_

class BINTREE {
  public:
    void *pelement;
    BINTREE *left;
    BINTREE *right;
};

extern void BinTreeIterate(BINTREE *bintree, void (*func)(void *));
extern void BinTreeDestroy(BINTREE *bintree);

#endif
