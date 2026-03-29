#include "java/util/ArrayList.txx"
#include "render/RenderHookList.h"
#include "render/RenderHook.h"

static java::ArrayList<RenderHook *> *globalRenderHookList = new java::ArrayList<RenderHook*>();

void
RenderHookList::renderHooks() {
    for ( int i = 0; globalRenderHookList != nullptr && i < globalRenderHookList->size(); i++ ) {
        RenderHook *h = globalRenderHookList->get(i);
        h->func(h->data);
    }
}

void
RenderHookList::removeAllRenderHooks() {
    delete globalRenderHookList;
    globalRenderHookList = nullptr;
}
