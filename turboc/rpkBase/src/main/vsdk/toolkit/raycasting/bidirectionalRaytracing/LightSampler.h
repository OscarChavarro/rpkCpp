/**
Samples a point on a light source. Two implementations are given : uniform sampling and
importance sampling
*/

#ifndef __LIGHT_SAMPLER__
#define __LIGHT_SAMPLER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/UniformLightSampler.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/ImportantLightSampler.h"

#endif

#endif
