#include "common/Statistics.h"
#include "raycasting/common/RayTracer.h"

double GLOBAL_raytracer_totalTime = 0.0;
long GLOBAL_raytracer_rayCount = 0;
long GLOBAL_raytracer_pixelCount = 0;
RayTracer *GLOBAL_rayTracer = nullptr;

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
*/
void
RayTracer::rayTrace(
    const char *fileName,
    java::io::OutputStream *stream,
    int isPipe,
    const RayTracer *rayTracer,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
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
        rayTracer->execute(img, scene, radianceMethod, renderOptions);
    }

    if ( img ) {
        ImageOutputHandle::deleteImageOutputHandle(img);
    }
}
