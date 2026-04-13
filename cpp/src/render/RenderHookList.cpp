#include "java/util/ArrayList.txx"
#include "render/RenderHookList.h"
#include "render/RenderHook.h"

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
