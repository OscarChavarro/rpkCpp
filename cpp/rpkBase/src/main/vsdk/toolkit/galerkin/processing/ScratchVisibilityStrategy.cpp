/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityStrategy.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ScratchRendererVisitor.h"

/**
Create a scratch software renderer for various operations on clusters
*/
void
ScratchVisibilityStrategy::scratchInit(GalerkinState *galerkinState) {
    galerkinState->scratch = new GalerkinElementVisibilityBuffer(
        galerkinState->scratchFrameBufferSize, galerkinState->scratchFrameBufferSize, nullptr);
}

/**
Terminates scratch rendering
*/
void
ScratchVisibilityStrategy::scratchTerminate(GalerkinState *galerkinState) {
    if ( galerkinState->scratch != nullptr ) {
        delete galerkinState->scratch;
        galerkinState->scratch = nullptr;
    }
}

/**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer and returns pointer to a bounding box
containing the size of the virtual screen. The cluster nicely fits
into the virtual screen
*/
const RpkAxisAlignedBoundingBox *
ScratchVisibilityStrategy::scratchRenderElements(GalerkinElement *cluster, Vector3Df eye, GalerkinState *galerkinState) {
    RpkAxisAlignedBoundingBox &boundingBox = galerkinState->getScratchBounds();

    if ( cluster->id == galerkinState->lastClusterId &&
         Numeric::vectorsEqual(eye, galerkinState->lastEye, Numeric::EPSILON_FLOAT) ) {
        return &boundingBox;
    }

    // Cache previously rendered cluster and eye point in order to
    // avoid re-rendering the same situation next time
    galerkinState->lastClusterId = cluster->id;
    galerkinState->lastEye = eye;

    const Vector3Df center = cluster->midPoint();
    Vector3Df up = {0.0, 0.0, 1.0};
    Vector3Df viewDirection;

    viewDirection = center.subtract(eye);
    viewDirection = viewDirection.normalized(Numeric::EPSILON_FLOAT);
    if ( java::Math::abs(up.dotProduct(viewDirection)) > 1.0 - VSDK::EPSILON ) {
        up = Vector3Df(0.0, 1.0, 0.0);
    }

    const Matrix4x4f lookAt = Camera::lookAtMatrix(eye, center, up);

    Camera::transformBoundingBox(cluster->geometry->getBoundingBox(), lookAt, &boundingBox);

    GalerkinElementVisibilityBuffer &scratch = *galerkinState->scratch;
    const Matrix4x4f projection = Camera::projectionMatrixFromBoundingBox(boundingBox);
    scratch.setTransform(projection.multiply(lookAt));

    // Choose a viewport based on the smallest surface element in the cluster.
    int viewportSize = static_cast<int>((boundingBox.dx() * boundingBox.dy()) / cluster->minimumArea);
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

    return &boundingBox;
}

/**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
ColorRgbMutable
ScratchVisibilityStrategy::scratchRadiance(const GalerkinState *galerkinState) {
    ColorRgbMutable rad(0.0, 0.0, 0.0);

    const GalerkinElementVisibilityBuffer &scratch = *galerkinState->scratch;
    const Window &viewport = scratch.getViewport();

    int nonBackground = 0;
    for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
        for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
            const GalerkinElement *element = scratch.getSample(x, y);
            if ( element != nullptr ) {
                if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
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
        rad.scale(1.0F / static_cast<float>(scratch.getViewportWidth() * scratch.getViewportHeight()));
    }
    return rad;
}

/** Computes the number of covered samples in the scratch viewport. */
int
ScratchVisibilityStrategy::scratchNonBackgroundPixels(const GalerkinState *galerkinState) {
    const GalerkinElementVisibilityBuffer &scratch = *galerkinState->scratch;
    const Window &viewport = scratch.getViewport();
    int nonBackground = 0;

    for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
        for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
            if ( scratch.getSample(x, y) != nullptr ) {
                nonBackground++;
            }
        }
    }
    return nonBackground;
}

/** Accumulates coverage on the solver's top-level surface elements. */
void
ScratchVisibilityStrategy::scratchPixelsPerElement(GalerkinState *galerkinState) {
    const GalerkinElementVisibilityBuffer &scratch = *galerkinState->scratch;
    const Window &viewport = scratch.getViewport();

    for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
        for ( int x = viewport.x0; x <= viewport.x1; x++ ) {
            const GalerkinElement *element = scratch.getSample(x, y);
            if ( element != nullptr ) {
                // Cluster leaves are the same top-level elements bound to the scene faces.
                galerkinState->topLevelElement(element->patch)->scratchVisibilityUsageCounter++;
            }
        }
    }
}
