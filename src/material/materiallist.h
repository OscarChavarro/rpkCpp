/* materiallist.h: linear lists of MATERIALs */

#ifndef _MATERIALLIST_H_
#define _MATERIALLIST_H_

#include "material/material.h"
#include "common/dataStructures/List.h"

class MATERIALLIST {
  public:
    MATERIAL *material;
    MATERIALLIST *next;
};

#define MaterialListCreate (MATERIALLIST *)ListCreate

#define MaterialListAdd(materiallist, material)    \
        (MATERIALLIST *)ListAdd((LIST *)materiallist, (void *)material)

#define MaterialListNext(pmateriallist) \
        (MATERIAL *)ListNext((LIST **)pmateriallist)

#define MaterialListIterate(materiallist, proc) \
        ListIterate((LIST *)materiallist, (void (*)(void *))proc)

#define MaterialListDestroy(materiallist) \
        ListDestroy((LIST *)materiallist)

#define ForAllMaterials(mat, matlist) ForAllInList(MATERIAL, mat, matlist)

/* Looks up a material with given name in the given material list. Returns
 * a pointer to the material if found, or (MATERIAL *)nullptr if not found. */
extern MATERIAL *MaterialLookup(MATERIALLIST *MaterialLib, char *name);

#endif
