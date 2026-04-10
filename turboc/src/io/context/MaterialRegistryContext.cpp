#include "io/context/MaterialRegistryContext.h"

MaterialContext
MaterialRegistryContext::createDefaultMgfMaterialContext() {
    MaterialContext context;
    context.clock = 1;
    context.sided = false;
    context.nr = 1.0f;
    context.ni = 0.0f;
    context.rd = 0.0f;
    context.rd_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.td = 0.0f;
    context.td_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.ed = 0.0f;
    context.ed_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.rs = 0.0f;
    context.rs_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.rs_a = 0.0f;
    context.ts = 0.0f;
    context.ts_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.ts_a = 0.0f;
    return context;
}

MaterialRegistryContext::MaterialRegistryContext():
    materialLookUpTable(new LookUpTable<char *>(OWNING)),
    defaultMaterialContext(MaterialRegistryContext::createDefaultMgfMaterialContext()),
    unNamedMaterialContext(defaultMaterialContext),
    currentMaterialContext(&unNamedMaterialContext)
{
}

MaterialRegistryContext::~MaterialRegistryContext() {
    if ( materialLookUpTable != NULL ) {
        delete materialLookUpTable;
        materialLookUpTable = NULL;
    }
    currentMaterialContext = NULL;
}

void
MaterialRegistryContext::reset() {
    unNamedMaterialContext = defaultMaterialContext;
    currentMaterialContext = &unNamedMaterialContext;
    materialLookUpTable->lookUpDone();
}
