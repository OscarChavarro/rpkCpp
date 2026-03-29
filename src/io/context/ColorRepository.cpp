#include "io/context/ColorRepository.h"

ColorRepository::ColorRepository():
    colorLookUpTable(new LookUpTable(LookUpBehaviors::owningCString())),
    unNamedColorContext(new ColorContext()),
    currentColor(unNamedColorContext)
{
    *unNamedColorContext = DEFAULT_COLOR_CONTEXT;
}

ColorRepository::~ColorRepository() {
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
ColorRepository::reset() {
    *unNamedColorContext = DEFAULT_COLOR_CONTEXT;
    currentColor = unNamedColorContext;
    colorLookUpTable->lookUpDone();
}
