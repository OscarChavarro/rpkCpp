#ifndef __RAY_CASTER__
#define __RAY_CASTER__

#include "common/RenderOptions.h"
#include "java/util/ArrayList.h"
#include "skin/radianceinterfaces.h"
#include "render/ScreenBuffer.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
#endif

class RayCaster {
  private:
    ScreenBuffer *screenBuffer;
    bool interrupt_requested;
    bool doDeleteScreen;

    void clipUv(int numberOfVertices, double *u, double *v);

    /**
    Determines the radiance of the nearest patch visible through the pixel
    (x,y). P shall be the nearest patch visible in the pixel.
    */
    COLOR
    getRadianceAtPixel(int x, int y, Patch *P, GETRADIANCE_FT getRadiance) {
        COLOR rad;
        colorClear(rad);
        if ( P && getRadiance ) {
            // Ray pointing from the eye through the center of the pixel.
            Ray ray;
            ray.pos = GLOBAL_camera_mainCamera.eyePosition;
            ray.dir = screenBuffer->getPixelVector(x, y);
            VECTORNORMALIZE(ray.dir);

            // Find intersection point of ray with patch P
            Vector3D point;
            float dist = VECTORDOTPRODUCT(P->normal, ray.dir);
            dist = -(VECTORDOTPRODUCT(P->normal, ray.pos) + P->planeConstant) / dist;
            VECTORSUMSCALED(ray.pos, dist, ray.dir, point);

            // Find surface coordinates of hit point on patch
            double u, v;
            patchUv(P, &point, &u, &v);

            // Boundary check is necessary because Z-buffer algorithm does
            // not yield exactly the same result as ray tracing at patch
            // boundaries.
            clipUv(P->numberOfVertices, &u, &v);

            // Reverse ray direction and get radiance emitted at hit point towards the eye
            Vector3D dir(-ray.dir.x, -ray.dir.y, -ray.dir.z);
            rad = getRadiance(P, u, v, dir);
        }
        return rad;
    }

  public:
    explicit RayCaster(ScreenBuffer *inScreen = nullptr) {
        if ( inScreen == nullptr ) {
            screenBuffer = new ScreenBuffer(nullptr);
            doDeleteScreen = true;
        } else {
            screenBuffer = inScreen;
            doDeleteScreen = true;
        }

        interrupt_requested = false;
    }

    virtual ~RayCaster() {
        if ( doDeleteScreen ) {
            delete screenBuffer;
        }
    }

    void render(GETRADIANCE_FT getRadiance, java::ArrayList<Patch *> *scenePatches);
    void display();
    void save(ImageOutputHandle *ip);

#ifdef RAYTRACING_ENABLED
    void interrupt();
#endif
};

#ifdef RAYTRACING_ENABLED
    extern Raytracer GLOBAL_rayCasting_RayCasting;
#endif

extern void rayCast(char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);

#endif
