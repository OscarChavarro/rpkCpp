#ifndef __RAYTRACER__
#define __RAYTRACER__

#include "java/util/ArrayList.h"
#include "IMAGE/imagec.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"

/**
TODO: This should be converted on to the Raytracer interface for inheriting the current four
ray-tracers: RayMatter, RayCaster, BidirectionalPathRaytracer and StochasticRaytracer.
*/

class RayTracer {
  public:
    virtual void defaults() = 0;
    virtual const char *getName() const = 0;

    // Initializes the current scene for raytracing computations.
    // Called when a new scene is loaded or when selecting a particular
    // raytracing algorithm
    virtual void initialize(const java::ArrayList<Patch *> *lightPatches) const = 0;

    virtual ~RayTracer() {}
};

class Raytracer {
  public:
    // Short name for the raytracing method, for use as argument of
    // -raytracing command line option
    const char *shortName;

    // How short can the short name be abbreviated?
    int nameAbbrev;

    // Raytrace the current scene as seen with the current camera. If 'ip'
    // is not a nullptr pointer, write the ray-traced image using the image output
    // handle pointed by 'ip'
    void (*Raytrace)(ImageOutputHandle *ip, Scene *scene, RadianceMethod *radianceMethod, RenderOptions *renderOptions);

    // Re-displays last ray-traced image. Returns FALSE if there is no
    // previous ray-traced image and TRUE there is
    int (*Redisplay)();

    // Saves last ray-traced image in the file describe dby the image output handle
    int (*SaveImage)(ImageOutputHandle *ip);

    // terminate raytracing computations
    void (*Terminate)();
};

extern Raytracer *GLOBAL_raytracer_activeRaytracer;
extern double GLOBAL_raytracer_totalTime; // Statistics: raytracing time
extern long GLOBAL_raytracer_rayCount; // Statistics: number of rays traced
extern long GLOBAL_raytracer_pixelCount; // Statistics: number of pixels drawn

extern void
rayTrace(
    const char *fileName,
    FILE *fp,
    int isPipe,
    const Raytracer *activeRayTracer,
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions);

#endif
