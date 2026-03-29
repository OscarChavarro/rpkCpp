#include "io/context/ColorContext.h"
#include "io/context/ColorState.h"

ColorState::ColorState():
    unNamedColorContext(new ColorContext()),
    currentColor(unNamedColorContext)
{
    *unNamedColorContext = DEFAULT_COLOR_CONTEXT;
}

ColorState::~ColorState() {
    if ( unNamedColorContext != nullptr ) {
        delete unNamedColorContext;
        unNamedColorContext = nullptr;
    }
    currentColor = nullptr;
}
