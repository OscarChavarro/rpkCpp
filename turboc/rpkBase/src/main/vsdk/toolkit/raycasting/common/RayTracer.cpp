#include "vsdk/toolkit/raycasting/common/RayTracer.h"

/**
Initializes an ImageOutputTarget taking into account the image filename extension,
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
    ImageOutputTarget *img = NULL;

    if ( stream != NULL ) {
        img = ImageOutputTarget::create(fileName, stream, isPipe);
        if ( img == NULL ) {
            return;
        }
    }

    if ( rayTracer != NULL ) {
        rayTracer->execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }

    if ( img ) {
        delete img;
    }
}
