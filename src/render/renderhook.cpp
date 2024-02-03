#include <cstdlib>

#include "render/renderhook.h"
#include "render/renderhook_priv.h"
#include "common/error.h"

static RENDERHOOKLIST *globalRenderHookList = nullptr;

void
renderHooks() {
    if ( globalRenderHookList != nullptr) {
        RENDERHOOKLIST *listStart = globalRenderHookList;
        if ( listStart != nullptr ) {
            for ( RENDERHOOKLIST *window = listStart; window != nullptr; window = window->next ) {
                RENDERHOOK *h = window->renderhook;
                h->func(h->data);
            }
        }
    }
}

void
addRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOK *hook;

    if ( globalRenderHookList == nullptr) {
        globalRenderHookList = nullptr;
    }

    hook = (RENDERHOOK *)malloc(sizeof(RENDERHOOK));

    hook->func = func;
    hook->data = data;

    globalRenderHookList = RenderHookListAdd(globalRenderHookList, hook);
}

void
removeRenderHook(RENDERHOOKFUNCTION func, void *data) {
    RENDERHOOKLIST *list = globalRenderHookList;
    RENDERHOOK *hook;

    // Search the element in the list
    RENDERHOOK *listHandler;
    do {
        hook = (list ? (listHandler = list->renderhook, list = list->next, listHandler) : nullptr);
    } while ( hook != nullptr && (hook->func != func || hook->data != data));

    if ( hook == nullptr) {
        logWarning("removeRenderHook", "Hook to remove not found");
    } else {
        globalRenderHookList = RenderHookListRemove(globalRenderHookList, hook);
    }
}


void
removeAllRenderHooks() {
    RENDERHOOKLIST *listWindow = globalRenderHookList;
    while ( listWindow != nullptr ) {
        RENDERHOOKLIST *listNode = listWindow->next;
        free(listWindow);
        listWindow = listNode;
    }
    globalRenderHookList = nullptr;
}
