#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "render/renderhook.h"
#include "render/renderhook_priv.h"
#include "common/error.h"

static java::ArrayList<RENDERHOOK *> *globalRenderHookList = new java::ArrayList<RENDERHOOK*>();

void
renderHooks() {
    for ( int i = 0; globalRenderHookList != nullptr && i < globalRenderHookList->size(); i++ ) {
        RENDERHOOK *h = globalRenderHookList->get(i);
        h->func(h->data);
    }
}

void
addRenderHook(RENDERHOOKFUNCTION func, void *data) {
    if ( globalRenderHookList == nullptr ) {
        globalRenderHookList = nullptr;
    }

    RENDERHOOK *hook = new RENDERHOOK();
    hook->func = func;
    hook->data = data;

    if ( globalRenderHookList != nullptr ) {
        globalRenderHookList->add(0, hook);
    }
}

void
removeRenderHook(RENDERHOOKFUNCTION func, void *data) {
    // Search the element in the list
    RENDERHOOK *hook = nullptr;
    for ( int i = 0; globalRenderHookList != nullptr && i < globalRenderHookList->size(); i++ ) {
        if ( hook->func == func && hook->data == data ) {
            globalRenderHookList->remove(i);
            break;
        }
    }

    if ( hook == nullptr ) {
        logWarning("removeRenderHook", "Hook to remove not found");
    }
}


void
removeAllRenderHooks() {
    delete globalRenderHookList;
    globalRenderHookList = nullptr;
}
