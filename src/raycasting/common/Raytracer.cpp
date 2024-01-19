#include "shared/camera.h"
#include "material/statistics.h"
#include "raycasting/common/Raytracer.h"

double GLOBAL_Raytracer_totalTime = 0.0;
long Global_Raytracer_rayCount = 0;
long Global_Raytracer_pixelCount = 0;
Raytracer *Global_Raytracer_activeRaytracer = (Raytracer *) nullptr;

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

    if ( Global_Raytracer_activeRaytracer ) {
        Global_Raytracer_activeRaytracer->Raytrace(img);
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
    if ( Global_Raytracer_activeRaytracer ) {
        Global_Raytracer_activeRaytracer->InterruptRayTracing();
        Global_Raytracer_activeRaytracer->Terminate();
    }

    Global_Raytracer_activeRaytracer = newMethod;
    if ( Global_Raytracer_activeRaytracer ) {
        Global_Raytracer_activeRaytracer->Initialize();
    }
}
