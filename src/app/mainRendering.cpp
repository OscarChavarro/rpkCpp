#include "java/util/ArrayList.txx"
#include "shared/canvas.h"
#include "shared/render.h"
#include "app/mainModel.h"
#include "app/mainRendering.h"
#include "app/batch.h"

void
mainExecuteRendering(java::ArrayList<Patch *> *scenePatches) {
    // Create the window in which to render (canvas window)
    if ( globalImageOutputWidth <= 0 ) {
        globalImageOutputWidth = 1920;
    }
    if ( globalImageOutputHeight <= 0 ) {
        globalImageOutputHeight = 1080;
    }
    createOffscreenCanvasWindow(globalImageOutputWidth, globalImageOutputHeight, scenePatches);

    while ( !renderInitialized() );
    renderScene(scenePatches);

    batch(scenePatches);
}
