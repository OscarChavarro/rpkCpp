#include "common/RenderOptions.h"

static const bool DEFAULT_SMOOTH_SHADING = true;
static const bool DEFAULT_BACKFACE_CULLING = true;
static const bool DEFAULT_OUTLINE_DRAWING = false;
static const bool DEFAULT_SURFACE_DRAWING = true;
static const bool DEFAULT_BOUNDING_BOX_DRAWING = false;
static const bool DEFAULT_CLUSTER_DRAWING = false;
static const ColorRgb DEFAULT_OUTLINE_COLOR = {0.5, 0.0, 0.0};
static const ColorRgb DEFAULT_BOUNDING_BOX_COLOR = {0.5, 0.0, 1.0};
static const ColorRgb DEFAULT_CLUSTER_COLOR = {1.0, 0.5, 0.0};

RenderOptions::RenderOptions():
    outlineColor(),
    boundingBoxColor(),
    clusterColor(),
    lineWidth(),
    drawOutlines(),
    noShading(),
    smoothShading(),
    backfaceCulling(),
    drawBoundingBoxes(),
    drawClusters(),
    frustumCulling(),
    renderRayTracedImage(),
    trace()
{
    smoothShading = DEFAULT_SMOOTH_SHADING;
    backfaceCulling = DEFAULT_BACKFACE_CULLING;
    drawSurfaces = DEFAULT_SURFACE_DRAWING;
    drawOutlines = DEFAULT_OUTLINE_DRAWING;
    drawBoundingBoxes = DEFAULT_BOUNDING_BOX_DRAWING;
    drawClusters = DEFAULT_CLUSTER_DRAWING;
    outlineColor = DEFAULT_OUTLINE_COLOR;
    boundingBoxColor = DEFAULT_BOUNDING_BOX_COLOR;
    clusterColor = DEFAULT_CLUSTER_COLOR;
    frustumCulling = false;
    noShading = false;
    lineWidth = 1.0;
    renderRayTracedImage = false;
}

RenderOptions::~RenderOptions() {
}
