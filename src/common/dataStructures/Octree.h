/* Octree.h: routines for dealing with octrees. */

#ifndef _OCTREE_H_
#define _OCTREE_H_

class OCTREE {
  public:
    void *pelement;
    OCTREE *child[8];
};

#define OctreeCreate() (OCTREE *)nullptr

/* When installing an element that already occurs in the octree,
 * the octree is returned unchanged */
extern OCTREE *OctreeAdd(OCTREE *octree, void *element, int (*nodecmp)(void *pelem1, void *pelem2));

/* allows duplicate elements in the octree */
extern OCTREE *OctreeAddWithDuplicates(OCTREE *octree, void *element, int (*nodecmp)(void *pelem1, void *pelem2));

/* The nodecmp function returns 8 if the elements are equal and the branch to be
 * investigated if not. */
extern void *OctreeFind(OCTREE *octree, void *element, int (*nodecmp)(void *pelem1, void *pelem2));

/* This routine looks up the element in the octree using the element
 * comparison function specified. If the element is found, a pointer to the
 * OCTREE cell where the element is stored is returned. If it is not found,
 * nullptr is returned. The nodecmp function returns 8 if the elements are 
 * equal and the branch to be investigated if they are different. */
extern OCTREE *OctreeFindSubtree(OCTREE *octree, void *element, int (*nodecmp)(void *pelem1, void *pelem2));

/* Iterators: the functions 'func' is called for each element in
 * the octree, with 0, 1 or 2 extra, fixed, parameters following the
 * pointer to an element. */
extern void OctreeIterate(OCTREE *octree, void (*func)(void *pelem));

extern void OctreeIterate1A(OCTREE *octree, void (*func)(void *pelem, void *parm), void *parm);

extern void
OctreeIterate2A(OCTREE *octree, void (*func)(void *pelem, void *parm1, void *parm2), void *parm1, void *parm2);

/* Disposes of the memory occupied by an OCTREE. Does not distroy the
 * elements pointed to in the octree. */
extern void OctreeDestroy(OCTREE *octree);

#endif
