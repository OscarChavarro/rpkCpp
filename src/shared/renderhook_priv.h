#ifndef _RENDERHOOK_PRIV_H_
#define _RENDERHOOK_PRIV_H_

#include "common/dataStructures/List.h"
#include "shared/renderhook.h"

class RENDERHOOK {
public:
    RENDERHOOKFUNCTION func;
    void *data;
};

/* same layout as LIST in dataStructures/List.h in order to be able to use
 * the generic list procedures defined in dataStructures/List.c */
class RENDERHOOKLIST {
public:
    RENDERHOOK *renderhook;
    RENDERHOOKLIST *next;
};

#define RenderHookListCreate    (RENDERHOOKLIST *)ListCreate

#define RenderHookListAdd(renderHookList, hook)    \
        (RENDERHOOKLIST *)ListAdd((LIST *)renderHookList, (void *)hook)

#define RenderHookListNext(prenderHookList) \
        (RENDERHOOK *)ListNext((LIST **)prenderHookList)

#define RenderHookListRemove(renderHookList, hook) \
        (RENDERHOOKLIST *)ListRemove((LIST *)renderHookList, (void *)hook)

#define RenderHookListDestroy(renderHookList) \
        ListDestroy((LIST *)renderHookList)

#define ForAllHooks(h, hooklist) ForAllInList(RENDERHOOK, h, hooklist)

#endif
