#include "common/RenderOptions.h"

const ColorRgb RenderOptions::DEFAULT_OUTLINE_COLOR = {0.5, 0.0, 0.0};
const ColorRgb RenderOptions::DEFAULT_BOUNDING_BOX_COLOR = {0.5, 0.0, 1.0};
const ColorRgb RenderOptions::DEFAULT_CLUSTER_COLOR = {1.0, 0.5, 0.0};

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
    trace(),
    toneMapOptions()
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
    toneMapOptions = nullptr;
}

RenderOptions::~RenderOptions() {
}
