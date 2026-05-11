#include "io/context/MaterialRegistryContext.h"

MaterialContext
MaterialRegistryContext::createDefaultMgfMaterialContext() {
    return {
        1,
        false,
        1.0F,
        0.0F,
        0.0F,
        ColorContext::DEFAULT_COLOR_CONTEXT,
        0.0F,
        ColorContext::DEFAULT_COLOR_CONTEXT,
        0.0F,
        ColorContext::DEFAULT_COLOR_CONTEXT,
        0.0F,
        ColorContext::DEFAULT_COLOR_CONTEXT,
        0.0F,
        0.0F,
        ColorContext::DEFAULT_COLOR_CONTEXT,
        0.0F
    };
}

MaterialRegistryContext::MaterialRegistryContext():
    materialLookUpTable(new LookUpTable<char *>(LookUpBehaviors::OWNING)),
    defaultMaterialContext(MaterialRegistryContext::createDefaultMgfMaterialContext()),
    unNamedMaterialContext(defaultMaterialContext),
    currentMaterialContext(&unNamedMaterialContext)
{
}

MaterialRegistryContext::~MaterialRegistryContext() {
    if ( materialLookUpTable != nullptr ) {
        delete materialLookUpTable;
        materialLookUpTable = nullptr;
    }
    currentMaterialContext = nullptr;
}

void
MaterialRegistryContext::reset() {
    unNamedMaterialContext = defaultMaterialContext;
    currentMaterialContext = &unNamedMaterialContext;
    materialLookUpTable->lookUpDone();
}
