#ifndef __RAYTRACER__
#define __RAYTRACER__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/image/ImageOutputHandle.h"

/**
TODO: This should be converted on to the Raytracer interface for inheriting the current four
ray-tracers: RayMatter, RayCaster, BidirectionalPathRaytracer and StochasticRaytracer.
*/

class RayTracer {
  public:
    virtual ~RayTracer() {}

    virtual void defaults() = 0;
    virtual const char *getName() const = 0;

    // Initializes the current scene for raytracing computations.
    // Called when a new scene is loaded or when selecting a particular
    // raytracing algorithm
    virtual void initialize(const java::ArrayList<Patch *> *lightPatches) const = 0;

    // Raytrace the current scene as seen with the current camera. If 'ip'
    // is not a nullptr pointer, write the ray-traced image using the image output
    // handle pointed by 'ip'
    virtual void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions) const = 0;

    // Saves last ray-traced image in the file describe dby the image output handle
    virtual bool saveImage(ImageOutputHandle *imageOutputHandle) const = 0;

    // Terminate raytracing computations
    virtual void terminate() const = 0;
};

extern RayTracer *GLOBAL_rayTracer;
extern double GLOBAL_raytracer_totalTime; // Statistics: raytracing time
extern long GLOBAL_raytracer_rayCount; // Statistics: number of rays traced
extern long GLOBAL_raytracer_pixelCount; // Statistics: number of pixels drawn

extern void
rayTrace(
    const char *fileName,
    FILE *fp,
    int isPipe,
    const RayTracer *rayTracer,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions);

#endif
