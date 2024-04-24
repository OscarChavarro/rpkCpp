#include "common/RenderOptions.h"

RenderOptions GLOBAL_render_renderOptions;

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
    useDisplayLists(),
    frustumCulling(),
    renderRayTracedImage(),
    trace()
{
}