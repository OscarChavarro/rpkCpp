/**
Samples a point on a light source. Two implementations are given : uniform sampling and
importance sampling
*/

#ifndef __LIGHT_SAMPLER__
#define __LIGHT_SAMPLER__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/UniformLightSampler.h"
#include "raycasting/bidirectionalRaytracing/ImportantLightSampler.h"

#endif

#endif
