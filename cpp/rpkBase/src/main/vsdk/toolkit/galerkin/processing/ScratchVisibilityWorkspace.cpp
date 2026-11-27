#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer.txx"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityWorkspace.h"

template class PolygonVisibilityBuffer<const GalerkinElement *>;

ScratchVisibilityWorkspace::ScratchVisibilityWorkspace(int frameBufferSize):
    buffer(frameBufferSize, frameBufferSize, nullptr),
    lastCluster(nullptr),
    lastEye(),
    bounds(),
    visibleElements(),
    countingStamps(),
    visibleIndices(),
    countingStamp(0)
{
}

void
ScratchVisibilityWorkspace::startCounting() {
    visibleElements.clear();
    countingStamp++;
}

void
ScratchVisibilityWorkspace::addSamples(GalerkinElement *element, int elementId, int samples) {
    while ( countingStamps.size() <= elementId ) {
        countingStamps.add(0);
        visibleIndices.add(0);
    }
    if ( countingStamps[elementId] == countingStamp ) {
        const int index = visibleIndices[elementId];
        visibleElements.set(index, ElementSamples(element, visibleElements[index].getSamples() + samples));
    } else {
        countingStamps.set(elementId, countingStamp);
        visibleIndices.set(elementId, static_cast<int>(visibleElements.size()));
        visibleElements.add(ElementSamples(element, samples));
    }
}
