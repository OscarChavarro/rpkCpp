#include "shared/canvas.h"
#include "shared/render.h"
#include "shared/camera.h"
#include "common/error.h"

void createOffscreenCanvasWindow(int hres, int vres) {
    RenderCreateOffscreenWindow(hres, vres);

    /* set correct width and height for the camera */
    CameraSet(&GLOBAL_camera_mainCamera, &GLOBAL_camera_mainCamera.eyep, &GLOBAL_camera_mainCamera.lookp, &GLOBAL_camera_mainCamera.updir,
              GLOBAL_camera_mainCamera.fov, hres, vres, &GLOBAL_camera_mainCamera.background);

    /* render the scene (no expose events on the external canvas window!) */
    RenderScene();
}

/* Size of the canvas mode stack. Canvas mode: determines how the program will
 * react to mouse events and what shape of cursor is displayed in the canvas
 * window, eg spraycan when renderin ... When the mode changes, the old mode
 * is pushed on a stack and restored afterwards */
#define CANVASMODESTACKSIZE 5
static int modestackidx;

/* pushes the current canvas mode on the canvas mode stack, so it can be
 * restored later */
void CanvasPushMode() {
    modestackidx++;
    if ( modestackidx >= CANVASMODESTACKSIZE ) {
        logFatal(4, "CanvasPushMode", "Mode stack size (%d) exceeded.", CANVASMODESTACKSIZE);
    }
}

/* restores the last saved canvas mode */
void CanvasPullMode() {
    modestackidx--;
    if ( modestackidx < 0 ) {
        logFatal(4, "CanvasPullMode", "Canvas mode stack underflow.\n");
    }
}
