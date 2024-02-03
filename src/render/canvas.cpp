#include "common/error.h"
#include "render/canvas.h"

/**
Size of the canvas mode stack. Canvas mode: determines how the program will
react to mouse events and what shape of cursor is displayed in the canvas
window, eg spray can when rendering. When the mode changes, the old mode
is pushed on a stack and restored afterwards
*/
#define CANVAS_MODE_STACK_SIZE 5
static int globalModeStackIndex;

/**
Pushes the current canvas mode on the canvas mode stack, so it can be restored later
*/
void
canvasPushMode() {
    globalModeStackIndex++;
    if ( globalModeStackIndex >= CANVAS_MODE_STACK_SIZE ) {
        logFatal(4, "canvasPushMode", "Mode stack size (%d) exceeded.", CANVAS_MODE_STACK_SIZE);
    }
}

/**
Restores the last saved canvas mode
*/
void
canvasPullMode() {
    globalModeStackIndex--;
    if ( globalModeStackIndex < 0 ) {
        logFatal(4, "canvasPullMode", "Canvas mode stack underflow.\n");
    }
}
