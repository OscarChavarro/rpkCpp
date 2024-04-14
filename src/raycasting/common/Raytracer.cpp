#include "scene/Camera.h"
#include "material/statistics.h"
#include "raycasting/common/Raytracer.h"
#include "scene/scene.h"

double GLOBAL_raytracer_totalTime = 0.0;
long GLOBAL_raytracer_rayCount = 0;
long GLOBAL_raytracer_pixelCount = 0;
Raytracer *GLOBAL_raytracer_activeRaytracer = nullptr;

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
*/
void
rayTrace(
    char *fileName,
    FILE *fp,
    int isPipe,
    Raytracer *activeRayTracer,
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    ImageOutputHandle *img = nullptr;

    if ( fp != nullptr ) {
        img = createRadianceImageOutputHandle(fileName, fp, isPipe,
                                              GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize,
                                              (float) (GLOBAL_statistics.referenceLuminance / 179.0));
        if ( img == nullptr ) {
            return;
        }
    }

    if ( activeRayTracer != nullptr ) {
        activeRayTracer->Raytrace(sceneBackground, img, scenePatches, lightPatches, clusteredWorldGeometry, context);
    }

    if ( img ) {
        deleteImageOutputHandle(img);
    }
}
