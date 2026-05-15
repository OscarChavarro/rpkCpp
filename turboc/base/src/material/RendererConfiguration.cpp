#include "material/RendererConfiguration.h"

const ColorRgb RenderOptions::DEFAULT_OUTLINE_COLOR = ColorRgb(0.5, 0.0, 0.0);
const ColorRgb RenderOptions::DEFAULT_BOUNDING_BOX_COLOR = ColorRgb(0.5, 0.0, 1.0);
const ColorRgb RenderOptions::DEFAULT_CLUSTER_COLOR = ColorRgb(1.0, 0.5, 0.0);

RenderOptions::RenderOptions():
    outlineColor(DEFAULT_OUTLINE_COLOR),
    boundingBoxColor(DEFAULT_BOUNDING_BOX_COLOR),
    clusterColor(DEFAULT_CLUSTER_COLOR),
    lineWidth(),
    drawOutlines(),
    drawSurfaces(),
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
    frustumCulling = false;
    noShading = false;
    lineWidth = 1.0;
    renderRayTracedImage = false;
}

RenderOptions::~RenderOptions() {
}

void
RenderOptions::setLineWidth(const float width) {
    lineWidth = width;
}

void
RenderOptions::setDrawOutlines(const bool enabled) {
    drawOutlines = enabled;
}

void
RenderOptions::setDrawSurfaces(const bool enabled) {
    drawSurfaces = enabled;
}

void
RenderOptions::setNoShading(const char enabled) {
    noShading = enabled;
}

void
RenderOptions::setSmoothShading(const char enabled) {
    smoothShading = enabled;
}

void
RenderOptions::setBackfaceCulling(const char enabled) {
    backfaceCulling = enabled;
}

void
RenderOptions::setDrawBoundingBoxes(const bool enabled) {
    drawBoundingBoxes = enabled;
}

void
RenderOptions::setDrawClusters(const bool enabled) {
    drawClusters = enabled;
}

void
RenderOptions::setFrustumCulling(const char enabled) {
    frustumCulling = enabled;
}

void
RenderOptions::setRenderRayTracedImage(const char enabled) {
    renderRayTracedImage = enabled;
}

void
RenderOptions::setTrace(const char enabled) {
    trace = enabled;
}

void
RenderOptions::setOutlineColor(const ColorRgb &color) {
    outlineColor = color;
}

void
RenderOptions::setBoundingBoxColor(const ColorRgb &color) {
    boundingBoxColor = color;
}

void
RenderOptions::setClusterColor(const ColorRgb &color) {
    clusterColor = color;
}
