#ifndef __RPK_RAYTRACING_RENDERER__
#define __RPK_RAYTRACING_RENDERER__

#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/Raytracer.h"

extern void
openGlRenderScene(
    const Scene *scene,
    const RayTracer *rayTracer,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions);

#endif
