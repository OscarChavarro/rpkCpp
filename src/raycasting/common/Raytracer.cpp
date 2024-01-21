#include "shared/Camera.h"
#include "material/statistics.h"
#include "raycasting/common/Raytracer.h"

double GLOBAL_raytracer_totalTime = 0.0;
long GLOBAL_raytracer_rayCount = 0;
long GLOBAL_raytracer_pixelCount = 0;

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
 */
void
rayTrace(char *filename, FILE *fp, int ispipe, Raytracer *activeRayTracer) {
    ImageOutputHandle *img = nullptr;

    if ( fp != nullptr ) {
        img = createRadianceImageOutputHandle(filename, fp, ispipe,
                                              GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize,
                                              (float) (GLOBAL_statistics_referenceLuminance / 179.0));
        if ( img == nullptr ) {
            return;
        }
    }

    if ( activeRayTracer != nullptr ) {
        activeRayTracer->Raytrace(img);
    }

    if ( img ) {
        deleteImageOutputHandle(img);
    }
}
