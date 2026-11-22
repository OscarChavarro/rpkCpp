#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/RenderHookList.h"
#include "vsdk/toolkit/render/RenderHook.h"

java::ArrayList<RenderHook *> *RenderHookList::renderHookList = new java::ArrayList<RenderHook*>();

void
RenderHookList::renderHooks() {
    for ( int i = 0; renderHookList != nullptr && i < renderHookList->size(); i++ ) {
        RenderHook * const h = renderHookList->get(i);
        h->function(h->data);
    }
}

void
RenderHookList::removeAllRenderHooks() {
    delete renderHookList;
    renderHookList = nullptr;
}
