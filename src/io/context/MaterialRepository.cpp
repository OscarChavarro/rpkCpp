#include "io/context/MaterialRepository.h"

MgfMaterialContext
MaterialRepository::createDefaultMgfMaterialContext() {
    return {
        1,
        false,
        1.0f,
        0.0f,
        0.0f,
        DEFAULT_COLOR_CONTEXT,
        0.0f,
        DEFAULT_COLOR_CONTEXT,
        0.0f,
        DEFAULT_COLOR_CONTEXT,
        0.0f,
        DEFAULT_COLOR_CONTEXT,
        0.0f,
        0.0f,
        DEFAULT_COLOR_CONTEXT,
        0.0f
    };
}

MaterialRepository::MaterialRepository():
    materialLookUpTable(new LookUpTable(LookUpBehaviors::OWNING)),
    defaultMaterialContext(MaterialRepository::createDefaultMgfMaterialContext()),
    unNamedMaterialContext(defaultMaterialContext),
    currentMaterialContext(&unNamedMaterialContext)
{
}

MaterialRepository::~MaterialRepository() {
    if ( materialLookUpTable != nullptr ) {
        delete materialLookUpTable;
        materialLookUpTable = nullptr;
    }
    currentMaterialContext = nullptr;
}

void
MaterialRepository::reset() {
    unNamedMaterialContext = defaultMaterialContext;
    currentMaterialContext = &unNamedMaterialContext;
    materialLookUpTable->lookUpDone();
}
