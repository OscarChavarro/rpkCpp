#include "common/RenderOptions.h"

#define DEFAULT_SMOOTH_SHADING true
#define DEFAULT_BACKFACE_CULLING true
#define DEFAULT_OUTLINE_DRAWING false
#define DEFAULT_BOUNDING_BOX_DRAWING false
#define DEFAULT_CLUSTER_DRAWING false
#define DEFAULT_OUTLINE_COLOR {0.5, 0.0, 0.0}
#define DEFAULT_BOUNDING_BOX_COLOR {0.5, 0.0, 1.0}
#define DEFAULT_CLUSTER_COLOR {1.0, 0.5, 0.0}

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
    drawOutlines = DEFAULT_OUTLINE_DRAWING;
    drawBoundingBoxes = DEFAULT_BOUNDING_BOX_DRAWING;
    drawClusters = DEFAULT_CLUSTER_DRAWING;
    outlineColor = DEFAULT_OUTLINE_COLOR;
    boundingBoxColor = DEFAULT_BOUNDING_BOX_COLOR;
    clusterColor = DEFAULT_CLUSTER_COLOR;
    frustumCulling = true;
    noShading = false;
    lineWidth = 1.0;
    renderRayTracedImage = false;
}

RenderOptions::~RenderOptions() {
}
