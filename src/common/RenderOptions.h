#ifndef __RENDER_OPTIONS__
#define __RENDER_OPTIONS__

#include "common/ColorRgb.h"

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTON MAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class
*/
#define RAYTRACING_ENABLED true

class RenderOptions {
public:
    ColorRgb outline_color; // Color in which to draw outlines
    ColorRgb bounding_box_color; // Color in which to draw bounding boxes
    ColorRgb cluster_color; // Color in which to show cluster bounding boxes
    ColorRgb camera_color; // Color for drawing alternate cameras
    float cameraSize; // Determines how large alternate cameras are drawn
    float lineWidth;
    char drawOutlines; // True for drawing facet outlines
    char noShading; // False for using any kind of shading
    char smoothShading;  // True for rendering with Gouraud interpolation
    char backfaceCulling; // True for backface culling
    char drawBoundingBoxes; // True for showing bounding boxes
    char drawClusters; // True for showing cluster hierarchy
    char useDisplayLists; // True for using display lists for faster display
    char frustumCulling; // Frustum culling accelerates rendering of large scenes.
    char renderRayTracedImage; // For freezing ray-traced image on the screen when appropriate
    char trace; // High-dynamic range ray-traced tiff
};

extern RenderOptions GLOBAL_render_renderOptions;

#endif
