#ifndef __RENDEROPTIONS__
#define __RENDEROPTIONS__

#include "common/rgb.h"

class RenderOptions {
public:
    RGB outline_color; // Color in which to draw outlines
    RGB bounding_box_color; // Color in which to draw bounding boxes
    RGB cluster_color; // Color in which to show cluster bounding boxes
    RGB camera_color; // Color for drawing alternate cameras
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
    char drawCameras; // True for drawing alternate viewpoints
    char renderRayTracedImage; // For freezing ray-traced image on the screen when appropriate
    char trace; // High-dynamic range ray-traced tiff
    char useBackground; // Use background image when rendering
};

extern RenderOptions GLOBAL_render_renderOptions;

#endif
