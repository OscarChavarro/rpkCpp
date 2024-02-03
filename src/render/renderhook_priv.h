#ifndef _RENDERHOOK_PRIV_H_
#define _RENDERHOOK_PRIV_H_

#include "common/dataStructures/List.h"
#include "render/renderhook.h"

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

#define RenderHookListAdd(renderHookList, hook) \
        (RENDERHOOKLIST *)ListAdd((LIST *)renderHookList, (void *)hook)

#define RenderHookListRemove(renderHookList, hook) \
        (RENDERHOOKLIST *)ListRemove((LIST *)renderHookList, (void *)hook)

#endif
