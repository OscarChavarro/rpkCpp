#ifndef _HWRCAST_H_
#define _HWRCAST_H_

#include "skin/radianceinterfaces.h"
#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"

class RayCaster {
  private:
    ScreenBuffer *scrn;
    bool interrupt_requested;
    bool doDeleteScreen;

    void ClipUV(int nrvertices, double *u, double *v);

    /**
    Determines the radiance of the nearest patch visible through the pixel
    (x,y). P shall be the nearest patch visible in the pixel.
    */
    COLOR
    get_radiance_at_pixel(int x, int y, Patch *P, GETRADIANCE_FT getrad) {
        COLOR rad;
        colorClear(rad);
        if ( P && getrad ) {
            // Ray pointing from the eye through the center of the pixel.
            Ray ray;
            ray.pos = GLOBAL_camera_mainCamera.eyep;
            ray.dir = scrn->GetPixelVector(x, y);
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
            ClipUV(P->numberOfVertices, &u, &v);

            // Reverse ray direction and get radiance emited at hit point towards the eye
            Vector3D dir(-ray.dir.x, -ray.dir.y, -ray.dir.z);
            rad = getrad(P, u, v, dir);
        }
        return rad;
    }

  public:
    RayCaster(ScreenBuffer *inscrn = nullptr) {
        if ( inscrn == nullptr ) {
            scrn = new ScreenBuffer(nullptr);
            doDeleteScreen = true;
        } else {
            scrn = inscrn;
            doDeleteScreen = true;
        }

        interrupt_requested = false;
    }

    virtual ~RayCaster() {
        if ( doDeleteScreen ) {
            delete scrn;
        }
    }

    void save(ImageOutputHandle *ip);
    void render(GETRADIANCE_FT getrad);
    void display();
    void interrupt();
};

extern Raytracer GLOBAL_rayCasting_RayCasting;

extern void RayCast(char *fname, FILE *fp, int ispipe);

#endif
