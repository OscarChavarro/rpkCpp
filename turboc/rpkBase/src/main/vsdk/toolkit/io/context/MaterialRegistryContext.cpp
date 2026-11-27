#include "vsdk/toolkit/io/context/MaterialRegistryContext.h"

MaterialContext
MaterialRegistryContext::createDefaultMgfMaterialContext() {
    MaterialContext context;
    context.clock = 1;
    context.sided = false;
    context.nr = 1.0;
    context.ni = 0.0;
    context.rd = 0.0;
    context.rd_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.td = 0.0;
    context.td_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.ed = 0.0;
    context.ed_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.rs = 0.0;
    context.rs_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.rs_a = 0.0;
    context.ts = 0.0;
    context.ts_c = ColorContext::DEFAULT_COLOR_CONTEXT;
    context.ts_a = 0.0;
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
