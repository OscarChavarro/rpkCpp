#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/render/Canvas.h"

int Canvas::modeStackIndex = 0;

/**
Pushes the current canvas mode on the canvas mode stack, so it can be restored later
*/
void
Canvas::canvasPushMode() {
    modeStackIndex++;
    if ( modeStackIndex >= CANVAS_MODE_STACK_SIZE ) {
        Logger::fatal(4, "canvasPushMode", "Mode stack size (%d) exceeded.", CANVAS_MODE_STACK_SIZE);
    }
}

/**
Restores the last saved canvas mode
*/
void
Canvas::canvasPullMode() {
    modeStackIndex--;
    if ( modeStackIndex < 0 ) {
        Logger::fatal(4, "canvasPullMode", "Canvas mode stack underflow.\n");
    }
}
