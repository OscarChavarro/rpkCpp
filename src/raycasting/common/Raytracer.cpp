#include "shared/camera.h"
#include "material/statistics.h"
#include "raycasting/common/Raytracer.h"

double GLOBAL_raytracer_totalTime = 0.0;
long GLOBAL_raytracer_rayCount = 0;
long GLOBAL_raytracer_pixelCount = 0;
Raytracer *GLOBAL_raytracer_activeRaytracer = (Raytracer *) nullptr;

/**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
 */
void
RayTrace(char *filename, FILE *fp, int ispipe) {
    ImageOutputHandle *img = nullptr;

    if ( fp != nullptr ) {
        img = CreateRadianceImageOutputHandle(filename, fp, ispipe,
                                              GLOBAL_camera_mainCamera.hres, GLOBAL_camera_mainCamera.vres, (float)(GLOBAL_statistics_referenceLuminance / 179.0));
        if ( img == nullptr ) {
            return;
        }
    }

    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Raytrace(img);
    }

    if ( img ) {
        DeleteImageOutputHandle(img);
    }
}

/**
This routine sets the current raytracing method to be used
*/
void
setRayTracing(Raytracer *newMethod) {
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->InterruptRayTracing();
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    GLOBAL_raytracer_activeRaytracer = newMethod;
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Initialize();
    }
}
