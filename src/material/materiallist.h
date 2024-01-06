/* materiallist.h: linear lists of MATERIALs */

#ifndef _MATERIALLIST_H_
#define _MATERIALLIST_H_

#include "material/Material.h"
#include "common/dataStructures/List.h"

class MATERIALLIST {
  public:
    Material *material;
    MATERIALLIST *next;
};

#define MaterialListCreate (MATERIALLIST *)ListCreate

#define MaterialListAdd(materiallist, material)    \
        (MATERIALLIST *)ListAdd((LIST *)materiallist, (void *)material)

#define MaterialListNext(pmateriallist) \
        (Material *)ListNext((LIST **)pmateriallist)

#define MaterialListIterate(materiallist, proc) \
        ListIterate((LIST *)materiallist, (void (*)(void *))proc)

#define MaterialListDestroy(materiallist) \
        ListDestroy((LIST *)materiallist)

#define ForAllMaterials(mat, matlist) ForAllInList(Material, mat, matlist)

#endif
