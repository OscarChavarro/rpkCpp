#include <cstdio>
#include <cstdlib>

#include "shared/renderhook.h"
#include "shared/renderhook_priv.h"
#include "common/error.h"

static RENDERHOOKLIST *oRenderHookList = nullptr;

void RenderHooks() {
    if ( oRenderHookList != nullptr) {
        ForAllHooks(h, oRenderHookList)
                    {
                        h->func(h->data);
                    }
        EndForAll
    }
}

void AddRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOK *hook;

    if ( oRenderHookList == nullptr) {
        oRenderHookList = RenderHookListCreate();
    }

    hook = (RENDERHOOK *)malloc(sizeof(RENDERHOOK));

    hook->func = func;
    hook->data = data;

    oRenderHookList = RenderHookListAdd(oRenderHookList, hook);
}

void RemoveRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOKLIST *list = oRenderHookList;
    RENDERHOOK *hook;

    /* Search the element in the list */

    do {
        hook = RenderHookListNext(&list);
    } while ( hook != nullptr && (hook->func != func || hook->data != data));

    if ( hook == nullptr) {
        Warning("RemoveRenderHook", "Hook to remove not found");
    } else {
        oRenderHookList = RenderHookListRemove(oRenderHookList, hook);
    }
}


void RemoveAllRenderHooks() {
    RenderHookListDestroy(oRenderHookList);
    oRenderHookList = nullptr;
}
