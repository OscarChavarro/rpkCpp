#include "java/util/ArrayList.txx"
#include "render/renderhook.h"
#include "render/renderhook_priv.h"

static java::ArrayList<RenderHook *> *globalRenderHookList = new java::ArrayList<RenderHook*>();

void
renderHooks() {
    for ( int i = 0; globalRenderHookList != nullptr && i < globalRenderHookList->size(); i++ ) {
        RenderHook *h = globalRenderHookList->get(i);
        h->func(h->data);
    }
}

void
removeAllRenderHooks() {
    delete globalRenderHookList;
    globalRenderHookList = nullptr;
}
