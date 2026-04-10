#include "raycasting/common/RayTracer.h"

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
*/
void
RayTracer::rayTrace(
    const char *fileName,
    OutputStream *stream,
    int isPipe,
    const RayTracer *rayTracer,
    Scene *scene,
    RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions)
{
    ImageOutputHandle *img = NULL;

    if ( stream != NULL ) {
        img = ImageOutputHandle::createRadianceImageOutputHandle(
            fileName,
            stream,
            isPipe,
            scene->camera->xSize,
            scene->camera->ySize);
        if ( img == NULL ) {
            return;
        }
    }

    if ( rayTracer != NULL ) {
        rayTracer->execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }

    if ( img ) {
        ImageOutputHandle::deleteImageOutputHandle(img);
    }
}
