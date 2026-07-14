/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityStrategy.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ScratchRendererVisitor.h"

/**
Src is a toplevel surface element. Render the corresponding patch
with pixel value a pointer to the element.
*/
/**
Create a scratch software renderer for various operations on clusters
*/
void
ScratchVisibilityStrategy::scratchInit(GalerkinState *galerkinState) {
    galerkinState->scratch = new SglContext(galerkinState->scratchFrameBufferSize, galerkinState->scratchFrameBufferSize);
    galerkinState->scratch->sglDepthTesting(true);
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
AxisAlignedBoundingBox *
ScratchVisibilityStrategy::scratchRenderElements(GalerkinElement *cluster, Vector3D eye, GalerkinState *galerkinState) {
    static AxisAlignedBoundingBox boundingBox;

    if ( cluster->id == galerkinState->lastClusterId &&
         eye.equals(galerkinState->lastEye, Numeric::EPSILON_FLOAT) ) {
        return &boundingBox;
    }

    // Cache previously rendered cluster and eye point in order to
    // avoid re-rendering the same situation next time
    galerkinState->lastClusterId = cluster->id;
    galerkinState->lastEye = eye;

    const Vector3D center = cluster->midPoint();
    Vector3D up = {0.0, 0.0, 1.0};
    Vector3D viewDirection;

    viewDirection.subtraction(center, eye);
    viewDirection.normalize(Numeric::EPSILON_FLOAT);
    if ( java::Math::abs(up.dotProduct(viewDirection)) > 1.0 - Numeric::EPSILON ) {
        up.set(0.0, 1.0, 0.0);
    }

    const Matrix4x4 lookAt = Matrix4x4::createLookAtMatrix(eye, center, up);

    Camera::transformBoundingBox(cluster->geometry->getBoundingBox(), lookAt, &boundingBox);

    SglContext *scratch = galerkinState->scratch;

    Matrix4x4 o = Camera::projectionMatrixFromBoundingBox(boundingBox);
    scratch->sglLoadMatrix(&o);
    scratch->sglMultiplyMatrix(&lookAt);

    // Choose a viewport depending on the relative size of the smallest
    // surface element in the cluster to be rendered
    int vp_size = static_cast<int>((boundingBox.dx() * boundingBox.dy()) / cluster->minimumArea);

    if ( vp_size > galerkinState->scratch->width ) {
        vp_size = galerkinState->scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    scratch->sglViewport(0, 0, vp_size, vp_size);

    // Render element pointers in the scratch frame buffer
    scratch->sglClear(static_cast<SGL_PIXEL>(0x00), SglConstants::SGL_MAXIMUM_Z);

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

    rad.clear();
    int nonBackGround = 0;
    for ( int j = 0; j < galerkinState->scratch->vp_height; j++ ) {
        const int rowStart = j * galerkinState->scratch->width;
        for ( int i = 0; i < galerkinState->scratch->vp_width; i++ ) {
            const Element *elementBase = galerkinState->scratch->galerkinElementBuffer[rowStart + i];
            if ( elementBase != nullptr ) {
                const auto *element = static_cast<const GalerkinElement *>(elementBase);
                if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
                     galerkinState->galerkinIterationMethod == GalerkinIterationMethod::JACOBI ) {
                    rad.add(rad, element->radiance[0]);
                } else {
                    rad.add(rad, element->unShotRadiance[0]);
                }
                nonBackGround++;
            }
        }
    }
    if ( nonBackGround > 0 ) {
        rad.scale(1.0F / static_cast<float>(galerkinState->scratch->vp_width * galerkinState->scratch->vp_height));
    }
    return rad;
}

/**
Computes the number of non background pixels
*/
int
ScratchVisibilityStrategy::scratchNonBackgroundPixels(const GalerkinState *galerkinState) {
    int nonBackGround = 0;

    for ( int j = 0; j < galerkinState->scratch->vp_height; j++ ) {
        const int rowStart = j * galerkinState->scratch->width;
        for ( int i = 0; i < galerkinState->scratch->vp_width; i++ ) {
            const Element *elementBase = galerkinState->scratch->galerkinElementBuffer[rowStart + i];
            if ( elementBase != nullptr ) {
                nonBackGround++;
            }
        }
    }
    return nonBackGround;
}

/**
Counts the number of pixels occupied by each element. The result is
accumulated in the tmp field of the elements. This field should be
initialized to zero before
*/
void
ScratchVisibilityStrategy::scratchPixelsPerElement(const GalerkinState *galerkinState) {
    for ( int i = 0; i < galerkinState->scratch->vp_height; i++ ) {
        const int rowStart = i * galerkinState->scratch->width;
        for ( int j = 0; j < galerkinState->scratch->vp_width; j++ ) {
            Element *elementBase = galerkinState->scratch->galerkinElementBuffer[rowStart + j];
            if ( elementBase != nullptr ) {
                auto *elem = static_cast<GalerkinElement *>(elementBase);
                elem->scratchVisibilityUsageCounter++;
            }
        }
    }
}
