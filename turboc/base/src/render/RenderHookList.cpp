#include "java/util/ArrayList.txx"
#include "render/RenderHookList.h"
#include "render/RenderHook.h"

ArrayList<RenderHook *> *RenderHookList::renderHookList = new ArrayList<RenderHook*>();

void
RenderHookList::renderHooks() {
    for ( int i = 0; renderHookList != NULL && i < renderHookList->size(); i++ ) {
        RenderHook *h = renderHookList->get(i);
        h->function(h->data);
    }
}

void
RenderHookList::removeAllRenderHooks() {
    delete renderHookList;
    renderHookList = NULL;
}
