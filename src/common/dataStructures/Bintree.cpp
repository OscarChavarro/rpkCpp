#include <cstdlib>

#include "common/dataStructures/Bintree.h"

void
BinTreeIterate(BINTREE *bintree, void (*func)(void *)) {
    if ( bintree ) {
        BinTreeIterate(bintree->left, func);
        func(bintree->pelement);
        BinTreeIterate(bintree->right, func);
    }
}

void
BinTreeDestroy(BINTREE *bintree) {
    if ( bintree ) {
        BinTreeDestroy(bintree->left);
        BinTreeDestroy(bintree->right);
        free(bintree);
    }
}
