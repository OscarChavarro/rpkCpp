#ifndef __RENDER_OPTIONS__
#define __RENDER_OPTIONS__

#include "common/ColorRgb.h"

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTON MAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class
*/
#define RAYTRACING_ENABLED

#define OPEN_GL_ENABLED

class RenderOptions {
  public:
    ColorRgb outlineColor; // Color in which to draw outlines
    ColorRgb boundingBoxColor; // Color in which to draw bounding boxes
    ColorRgb clusterColor; // Color in which to show cluster bounding boxes
    float lineWidth;
    bool drawOutlines; // True for drawing facet outlines
    bool drawSurfaces;
    char noShading; // False for using any kind of shading
    char smoothShading;  // True for rendering with Gouraud interpolation
    char backfaceCulling; // True for backface culling
    bool drawBoundingBoxes; // True for showing bounding boxes
    bool drawClusters; // True for showing cluster hierarchy
    char frustumCulling; // Frustum culling accelerates rendering of large scenes.
    char renderRayTracedImage; // For freezing ray-traced image on the screen when appropriate
    char trace; // High-dynamic range ray-traced tiff

    RenderOptions();
    virtual ~RenderOptions();
};

#endif
