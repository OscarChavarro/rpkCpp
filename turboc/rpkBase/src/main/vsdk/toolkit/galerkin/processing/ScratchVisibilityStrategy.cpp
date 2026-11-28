/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityStrategy.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ScratchRendererVisitor.h"

/**
Create a scratch software renderer for various operations on clusters
*/
void
ScratchVisibilityStrategy::scratchInit(GalerkinState *galerkinState) {
    galerkinState->scratchVisibilityWorkspace = new ScratchVisibilityWorkspace(galerkinState->scratchFrameBufferSize);
}

/**
Terminates scratch rendering
*/
void
ScratchVisibilityStrategy::scratchTerminate(GalerkinState *galerkinState) {
    if ( galerkinState->scratchVisibilityWorkspace != NULL ) {
        delete galerkinState->scratchVisibilityWorkspace;
        galerkinState->scratchVisibilityWorkspace = NULL;
    }
}

/**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer of the workspace and returns the bounding
box containing the size of the virtual screen. The cluster nicely fits
into the virtual screen
*/
RpkAxisAlignedBoundingBox
ScratchVisibilityStrategy::scratchRenderElements(
    GalerkinElement *cluster,
    Vector3Dd eye,
    ScratchVisibilityWorkspace *scratchWorkspace,
    GalerkinState *galerkinState)
{
    RpkAxisAlignedBoundingBox &boundingBox = scratchWorkspace->getBounds();

    if ( scratchWorkspace->holdsRendering(cluster, eye) ) {
        return boundingBox;
    }

    // Cache previously rendered cluster and eye point in order to
    // avoid re-rendering the same situation next time
    scratchWorkspace->setRendering(cluster, eye);

    const Vector3Dd center = cluster->midPoint();
    Vector3Dd up(0.0, 0.0, 1.0);
    Vector3Dd viewDirection;

    viewDirection = center.subtract(eye);
    viewDirection = viewDirection.normalized(Numeric::EPSILON);
    if ( Math::abs(up.dotProduct(viewDirection)) > 1.0 - VSDK::EPSILON ) {
        up = Vector3Dd(0.0, 1.0, 0.0);
    }

    const Matrix4x4d lookAt = Camera::lookAtMatrix(eye, center, up);

    Camera::transformBoundingBox(cluster->geometry->getBoundingBox(), lookAt, &boundingBox);

    ScratchVisibilityBuffer &scratch = scratchWorkspace->getBuffer();
    const Matrix4x4d projection = Camera::projectionMatrixFromBoundingBox(boundingBox);
    scratch.setTransform(projection.multiply(lookAt));

    // Choose a viewport based on the smallest surface element in the cluster.
    int viewportSize = ((int)((boundingBox.dx() * boundingBox.dy()) / cluster->minimumArea));
    if ( viewportSize < 32 ) {
        viewportSize = 32;
    }
    if ( viewportSize > scratch.getWidth() ) {
        viewportSize = scratch.getWidth();
    }
    scratch.setViewport(0, 0, viewportSize, viewportSize);
    scratch.clear();

    ScratchRendererVisitor leafVisitor(eye, scratch);
    ClusterTraversalStrategy::traverseAllLeafElements(&leafVisitor, cluster, galerkinState);

    return boundingBox;
}

/**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
ColorRgbMutable
ScratchVisibilityStrategy::scratchRadiance(const ScratchVisibilityWorkspace *scratchWorkspace, const GalerkinState *galerkinState) {
    ColorRgbMutable rad;

    rad.clear();
    const ScratchVisibilityBuffer &scratch = scratchWorkspace->getBuffer();
    const RasterRectangle2D &viewport = scratch.getViewport();

    int nonBackground = 0;
    for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
        for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
            const GalerkinElement *element = scratch.getSample(x, y);
            if ( element != NULL ) {
                if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
                     galerkinState->isJacobi() ) {
                    rad.add(rad, element->radiance[0]);
                } else {
                    rad.add(rad, element->unShotRadiance[0]);
                }
                nonBackground++;
            }
        }
    }
    if ( nonBackground > 0 ) {
        rad.scale(1.0 / ((double)(scratch.getViewportWidth() * scratch.getViewportHeight())));
    }
    return rad;
}

/** Computes the number of covered samples in the scratch viewport. */
int
ScratchVisibilityStrategy::scratchNonBackgroundPixels(const ScratchVisibilityWorkspace *scratchWorkspace) {
    const ScratchVisibilityBuffer &scratch = scratchWorkspace->getBuffer();
    const RasterRectangle2D &viewport = scratch.getViewport();
    int nonBackground = 0;

    for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
        for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
            if ( scratch.getSample(x, y) != NULL ) {
                nonBackground++;
            }
        }
    }
    return nonBackground;
}

/**
Counts, in the workspace, the samples covered by each of the solver's
top-level surface elements in the last rendering
*/
void
ScratchVisibilityStrategy::scratchPixelsPerElement(ScratchVisibilityWorkspace *scratchWorkspace, const GalerkinState *galerkinState) {
    const ScratchVisibilityBuffer &scratch = scratchWorkspace->getBuffer();
    const RasterRectangle2D &viewport = scratch.getViewport();

    scratchWorkspace->startCounting();
    // Neighbouring samples mostly show the same element: count runs of equal
    // samples and add each run to its element at once
    const GalerkinElement *runElement = NULL;
    int runLength = 0;
    for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
        for ( int x = viewport.getMinX(); x <= viewport.getMaxX(); x++ ) {
            const GalerkinElement *element = scratch.getSample(x, y);
            if ( element == runElement ) {
                runLength++;
            } else {
                addSamples(scratchWorkspace, runElement, runLength, galerkinState);
                runElement = element;
                runLength = 1;
            }
        }
    }
    addSamples(scratchWorkspace, runElement, runLength, galerkinState);
}

/** Adds the samples of a run showing the element (none for the background) */
void
ScratchVisibilityStrategy::addSamples(
    ScratchVisibilityWorkspace *scratchWorkspace,
    const GalerkinElement *element,
    int samples,
    const GalerkinState *galerkinState)
{
    if ( element == NULL || samples == 0 ) {
        return;
    }
    // Cluster leaves are the same top-level elements bound to the scene faces.
    GalerkinElement *topLevel = galerkinState->topLevelElement(element->patch);
    scratchWorkspace->addSamples(topLevel, topLevel->id, samples);
}

/** Area of the virtual screen that one sample of the last rendering covers */
double
ScratchVisibilityStrategy::scratchPixelArea(const RpkAxisAlignedBoundingBox &boundingBox, const ScratchVisibilityWorkspace *scratchWorkspace) {
    const ScratchVisibilityBuffer &scratch = scratchWorkspace->getBuffer();
    return boundingBox.dx() * boundingBox.dy() /
        ((double)(scratch.getViewportWidth() * scratch.getViewportHeight()));
}
