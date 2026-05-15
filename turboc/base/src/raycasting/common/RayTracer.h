#ifndef __RAYTRACER__
#define __RAYTRACER__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "environment/geometry/elements/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/image/ImageOutputHandle.h"
#include "tonemap/ToneMappingContext.h"

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
    virtual void initialize(const ArrayList<Patch *> *lightPatches) const = 0;

    // Raytrace the current scene as seen with the current camera. If 'ip'
    // is not a NULL pointer, write the ray-traced image using the image output
    // handle pointed by 'ip'
    virtual void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RenderOptions *renderOptions) const = 0;

    // Saves last ray-traced image in the file describe dby the image output handle
    virtual bool saveImage(ImageOutputHandle *imageOutputHandle) const = 0;

    // Terminate raytracing computations
    virtual void terminate() const = 0;

    static void
    rayTrace(
        const char *fileName,
        OutputStream *stream,
        int isPipe,
        const RayTracer *rayTracer,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RenderOptions *renderOptions);
};

#endif
