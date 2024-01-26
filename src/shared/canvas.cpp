#include "common/error.h"
#include "scene/scene.h"
#include "shared/render.h"
#include "shared/canvas.h"

/**
Size of the canvas mode stack. Canvas mode: determines how the program will
react to mouse events and what shape of cursor is displayed in the canvas
window, eg spray can when rendering. When the mode changes, the old mode
is pushed on a stack and restored afterwards
*/
#define CANVAS_MODE_STACK_SIZE 5
static int globalModeStackIndex;

void
createOffscreenCanvasWindow(int width, int height, java::ArrayList<Patch *> *scenePatches) {
    renderCreateOffscreenWindow(width, height);

    // Set correct width and height for the camera
    cameraSet(&GLOBAL_camera_mainCamera, &GLOBAL_camera_mainCamera.eyePosition, &GLOBAL_camera_mainCamera.lookPosition,
              &GLOBAL_camera_mainCamera.upDirection,
              GLOBAL_camera_mainCamera.fov, width, height, &GLOBAL_camera_mainCamera.background);

    // Render the scene (no expose events on the external canvas window!)
    renderScene(scenePatches);
}

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
