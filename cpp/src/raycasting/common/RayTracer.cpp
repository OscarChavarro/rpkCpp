#include "raycasting/common/RayTracer.h"

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
*/
void
RayTracer::rayTrace(
    const char *fileName,
    java::OutputStream *stream,
    int isPipe,
    const RayTracer *rayTracer,
    Scene *scene,
    RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RendererConfiguration *renderOptions)
{
    ImageOutputHandle *img = nullptr;

    if ( stream != nullptr ) {
        img = ImageOutputHandle::createRadianceImageOutputHandle(
            fileName,
            stream,
            isPipe,
            scene->camera->xSize,
            scene->camera->ySize);
        if ( img == nullptr ) {
            return;
        }
    }

    if ( rayTracer != nullptr ) {
        rayTracer->execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }

    if ( img ) {
        ImageOutputHandle::deleteImageOutputHandle(img);
    }
}
