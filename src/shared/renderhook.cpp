#include <cstdlib>

#include "shared/renderhook.h"
#include "shared/renderhook_priv.h"
#include "common/error.h"

static RENDERHOOKLIST *oRenderHookList = nullptr;

void
renderHooks() {
    if ( oRenderHookList != nullptr) {
        ForAllHooks(h, oRenderHookList)
                    {
                        h->func(h->data);
                    }
        EndForAll
    }
}

void
addRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOK *hook;

    if ( oRenderHookList == nullptr) {
        oRenderHookList = RenderHookListCreate();
    }

    hook = (RENDERHOOK *)malloc(sizeof(RENDERHOOK));

    hook->func = func;
    hook->data = data;

    oRenderHookList = RenderHookListAdd(oRenderHookList, hook);
}

void
removeRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOKLIST *list = oRenderHookList;
    RENDERHOOK *hook;

    // Search the element in the list
    do {
        hook = (RENDERHOOK *)
            (list ? (GLOBAL_listHandler = list->renderhook, list = list->next, GLOBAL_listHandler) : nullptr);
    } while ( hook != nullptr && (hook->func != func || hook->data != data));

    if ( hook == nullptr) {
        logWarning("removeRenderHook", "Hook to remove not found");
    } else {
        oRenderHookList = RenderHookListRemove(oRenderHookList, hook);
    }
}


void
removeAllRenderHooks() {
    RenderHookListDestroy(oRenderHookList);
    oRenderHookList = nullptr;
}
