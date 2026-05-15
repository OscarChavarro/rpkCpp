#include "io/context/ColorRegistryContext.h"

ColorRegistryContext::ColorRegistryContext():
    colorLookUpTable(new LookUpTable<char *>(OWNING)),
    unNamedColorContext(new ColorContext()),
    currentColor(unNamedColorContext)
{
    *unNamedColorContext = ColorContext::DEFAULT_COLOR_CONTEXT;
}

ColorRegistryContext::~ColorRegistryContext() {
    if ( unNamedColorContext != NULL ) {
        delete unNamedColorContext;
        unNamedColorContext = NULL;
    }
    if ( colorLookUpTable != NULL ) {
        delete colorLookUpTable;
        colorLookUpTable = NULL;
    }
    currentColor = NULL;
}

void
ColorRegistryContext::reset() {
    *unNamedColorContext = ColorContext::DEFAULT_COLOR_CONTEXT;
    currentColor = unNamedColorContext;
    colorLookUpTable->lookUpDone();
}
