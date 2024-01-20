#include <cstdlib>

#include "common/dataStructures/Bintree.h"

void
binTreeIterate(BINTREE *bintree, void (*func)(void *)) {
    if ( bintree ) {
        binTreeIterate(bintree->left, func);
        func(bintree->pelement);
        binTreeIterate(bintree->right, func);
    }
}

void
binTreeDestroy(BINTREE *bintree) {
    if ( bintree ) {
        binTreeDestroy(bintree->left);
        binTreeDestroy(bintree->right);
        free(bintree);
    }
}
