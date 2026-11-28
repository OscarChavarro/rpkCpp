#include "vsdk/toolkit/raycasting/common/RayTracer.h"

/**
Initializes an ImageOutputTarget taking into account the image filename extension,
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
    ImageOutputTarget *img = nullptr;

    if ( stream != nullptr ) {
        img = ImageOutputTarget::create(fileName, stream, isPipe);
        if ( img == nullptr ) {
            return;
        }
    }

    if ( rayTracer != nullptr ) {
        rayTracer->execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
    }

    if ( img ) {
        delete img;
    }
}
