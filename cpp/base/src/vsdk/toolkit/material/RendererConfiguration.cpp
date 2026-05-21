#include "vsdk/toolkit/material/RendererConfiguration.h"

const ColorRgb RendererConfiguration::DEFAULT_OUTLINE_COLOR = {0.5, 0.0, 0.0};
const ColorRgb RendererConfiguration::DEFAULT_BOUNDING_BOX_COLOR = {0.5, 0.0, 1.0};
const ColorRgb RendererConfiguration::DEFAULT_CLUSTER_COLOR = {1.0, 0.5, 0.0};

RendererConfiguration::RendererConfiguration():
    outlineColor(DEFAULT_OUTLINE_COLOR),
    boundingBoxColor(DEFAULT_BOUNDING_BOX_COLOR),
    clusterColor(DEFAULT_CLUSTER_COLOR),
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
    frustumCulling = false;
    noShading = false;
    lineWidth = 1.0;
    renderRayTracedImage = false;
}

RendererConfiguration::~RendererConfiguration() {
}

void
RendererConfiguration::setLineWidth(const float width) {
    lineWidth = width;
}

void
RendererConfiguration::setDrawOutlines(const bool enabled) {
    drawOutlines = enabled;
}

void
RendererConfiguration::setDrawSurfaces(const bool enabled) {
    drawSurfaces = enabled;
}

void
RendererConfiguration::setNoShading(const char enabled) {
    noShading = enabled;
}

void
RendererConfiguration::setSmoothShading(const char enabled) {
    smoothShading = enabled;
}

void
RendererConfiguration::setBackfaceCulling(const char enabled) {
    backfaceCulling = enabled;
}

void
RendererConfiguration::setDrawBoundingBoxes(const bool enabled) {
    drawBoundingBoxes = enabled;
}

void
RendererConfiguration::setDrawClusters(const bool enabled) {
    drawClusters = enabled;
}

void
RendererConfiguration::setFrustumCulling(const char enabled) {
    frustumCulling = enabled;
}

void
RendererConfiguration::setRenderRayTracedImage(const char enabled) {
    renderRayTracedImage = enabled;
}

void
RendererConfiguration::setTrace(const char enabled) {
    trace = enabled;
}

void
RendererConfiguration::setOutlineColor(const ColorRgb &color) {
    outlineColor = color;
}

void
RendererConfiguration::setBoundingBoxColor(const ColorRgb &color) {
    boundingBoxColor = color;
}

void
RendererConfiguration::setClusterColor(const ColorRgb &color) {
    clusterColor = color;
}
