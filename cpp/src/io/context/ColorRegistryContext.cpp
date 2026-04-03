#include "io/context/ColorRegistryContext.h"

ColorRegistryContext::ColorRegistryContext():
    colorLookUpTable(new LookUpTable<char *>(LookUpBehaviors::OWNING)),
    unNamedColorContext(new ColorContext()),
    currentColor(unNamedColorContext)
{
    *unNamedColorContext = ColorContext::DEFAULT_COLOR_CONTEXT;
}

ColorRegistryContext::~ColorRegistryContext() {
    if ( unNamedColorContext != nullptr ) {
        delete unNamedColorContext;
        unNamedColorContext = nullptr;
    }
    if ( colorLookUpTable != nullptr ) {
        delete colorLookUpTable;
        colorLookUpTable = nullptr;
    }
    currentColor = nullptr;
}

void
ColorRegistryContext::reset() {
    *unNamedColorContext = ColorContext::DEFAULT_COLOR_CONTEXT;
    currentColor = unNamedColorContext;
    colorLookUpTable->lookUpDone();
}
