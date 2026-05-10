#ifndef __RENDER_OPTIONS__
#define __RENDER_OPTIONS__

#include "common/color/ColorRgb.h"

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTON MAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class

Build-time feature flags are configured from CMake.
*/

class RendererConfiguration {
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

    RendererConfiguration();
    virtual ~RendererConfiguration();

  private:
    static constexpr bool DEFAULT_SMOOTH_SHADING = true;
    static constexpr bool DEFAULT_BACKFACE_CULLING = true;
    static constexpr bool DEFAULT_OUTLINE_DRAWING = false;
    static constexpr bool DEFAULT_SURFACE_DRAWING = true;
    static constexpr bool DEFAULT_BOUNDING_BOX_DRAWING = false;
    static constexpr bool DEFAULT_CLUSTER_DRAWING = false;
    static const ColorRgb DEFAULT_OUTLINE_COLOR;
    static const ColorRgb DEFAULT_BOUNDING_BOX_COLOR;
    static const ColorRgb DEFAULT_CLUSTER_COLOR;
};

#endif
