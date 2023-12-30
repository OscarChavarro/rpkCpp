#include <cstdlib>

#include "common/dataStructures/Octree.h"

OCTREE *NewOctreeNode(void *pelement) {
    int i;
    OCTREE *nt = (OCTREE *)malloc(sizeof(OCTREE));
    nt->pelement = pelement;
    for ( i = 0; i < 8; i++ ) {
        nt->child[i] = (OCTREE *) nullptr;
    }

    return nt;
}

OCTREE *OctreeAddWithDuplicates(OCTREE *octree,
                                void *pelement,
                                int (*nodecmp)(void *pelem1, void *pelem2)) {
    OCTREE *o, *p;
    int cmp = 8;

    o = (OCTREE *) nullptr;
    p = octree;
    while ( p ) {
        cmp = nodecmp(p->pelement, pelement);
        o = p;
        p = p->child[cmp % 8];    /* allow duplicate items in the OCTREE */
    }

    /* p is nullptr, o is the octree node where we should add the new child */
    p = NewOctreeNode(pelement);

    if ( o ) {        /* add the new node to the old octree */
        o->child[cmp % 8] = p;
        return octree;
    } else {
        return p;
    }
}

OCTREE *OctreeFindSubtree(OCTREE *octree,
                          void *pelement,
                          int (*nodecmp)(void *pelem1, void *pelem2)) {
    OCTREE *p;
    int cmp;

    p = octree;
    while ( p ) {
        cmp = nodecmp(p->pelement, pelement);
        if ( cmp >= 8 ) {
            return p;
        } else {
            p = p->child[cmp];
        }
    }

    return (OCTREE *) nullptr;
}

void *OctreeFind(OCTREE *octree,
                 void *pelement,
                 int (*nodecmp)(void *pelem1, void *pelem2)) {
    OCTREE *p = OctreeFindSubtree(octree, pelement, nodecmp);

    if ( p ) {
        return p->pelement;
    } else {
        return (void *) nullptr;
    }
}

void OctreeIterate(OCTREE *octree, void (*func)(void *pelem)) {
    int i;

    if ( octree ) {
        for ( i = 0; i < 8; i++ ) {
            OctreeIterate(octree->child[i], func);
        }
        func(octree->pelement);
    }
}

void OctreeIterate1A(OCTREE *octree, void (*func)(void *pelem, void *parm), void *parm) {
    int i;

    if ( octree ) {
        for ( i = 0; i < 8; i++ ) {
            OctreeIterate1A(octree->child[i], func, parm);
        }
        func(octree->pelement, parm);
    }
}

void OctreeIterate2A(OCTREE *octree, void (*func)(void *pelem, void *parm1, void *parm2), void *parm1, void *parm2) {
    int i;

    if ( octree ) {
        for ( i = 0; i < 8; i++ ) {
            OctreeIterate2A(octree->child[i], func, parm1, parm2);
        }
        func(octree->pelement, parm1, parm2);
    }
}

void OctreeDestroy(OCTREE *octree) {
    int i;

    if ( octree ) {
        for ( i = 0; i < 8; i++ ) {
            OctreeDestroy(octree->child[i]);
        }
        free(octree);
    }
}
