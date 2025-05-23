/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#include "GALERKIN/processing/ClusterTraversalStrategy.h"
#include "GALERKIN/processing/ScratchVisibilityStrategy.h"
#include "GALERKIN/processing/visitors/ScratchRendererVisitor.h"

/**
Src is a toplevel surface element. Render the corresponding patch
with pixel value a pointer to the element. Uses global variable
eyePoint for backface culling
*/
static Vector3D globalEyePoint;

/**
Create a scratch software renderer for various operations on clusters
*/
void
ScratchVisibilityStrategy::scratchInit(GalerkinState *galerkinState) {
    galerkinState->scratch = new SGL_CONTEXT(galerkinState->scratchFrameBufferSize, galerkinState->scratchFrameBufferSize);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);
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
float *
ScratchVisibilityStrategy::scratchRenderElements(GalerkinElement *cluster, Vector3D eye, GalerkinState *galerkinState) {
    Vector3D centre = cluster->midPoint();
    Vector3D up = {0.0, 0.0, 1.0};
    Vector3D viewDirection;
    static BoundingBox bbx;
    Matrix4x4 lookAt{};
    SGL_CONTEXT *prev_sgl_context;
    int vp_size;

    if ( cluster->id == galerkinState->lastClusterId && eye.equals(galerkinState->lastEye, Numeric::EPSILON_FLOAT) ) {
        return bbx.coordinates;
    } else {
        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        galerkinState->lastClusterId = cluster->id;
        galerkinState->lastEye = eye;
    }

    viewDirection.subtraction(centre, eye);
    viewDirection.normalize(Numeric::EPSILON_FLOAT);
    if ( java::Math::abs(up.dotProduct(viewDirection)) > 1.0 - Numeric::EPSILON ) {
        up.set(0.0, 1.0, 0.0);
    }
    lookAt = Matrix4x4::createLookAtMatrix(eye, centre, up);

    cluster->geometry->getBoundingBox().transformTo(&lookAt, &bbx);

    prev_sgl_context = sglMakeCurrent(galerkinState->scratch);
    Matrix4x4 o = Matrix4x4::createOrthogonalViewMatrix(
            bbx.coordinates[MIN_X],
            bbx.coordinates[MAX_X],
            bbx.coordinates[MIN_Y],
            bbx.coordinates[MAX_Y],
            -bbx.coordinates[MAX_Z],
            -bbx.coordinates[MIN_Z]);
    GLOBAL_sgl_currentContext->sglLoadMatrix(&o);
    GLOBAL_sgl_currentContext->sglMultiplyMatrix(&lookAt);

    // Choose a viewport depending on the relative size of the smallest
    // surface element in the cluster to be rendered
    vp_size = (int)(
        (bbx.coordinates[MAX_X] - bbx.coordinates[MIN_X]) * (bbx.coordinates[MAX_Y] - bbx.coordinates[MIN_Y]
    ) / cluster->minimumArea);
    if ( vp_size > galerkinState->scratch->width ) {
        vp_size = galerkinState->scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    GLOBAL_sgl_currentContext->sglViewport(0, 0, vp_size, vp_size);

    // Render element pointers in the scratch frame buffer
    globalEyePoint = eye; // Needed for backface culling test
    GLOBAL_sgl_currentContext->sglClear((SGL_PIXEL)0x00, SGL_MAXIMUM_Z);
    ScratchRendererVisitor *leafVisitor = new ScratchRendererVisitor(globalEyePoint);
    ClusterTraversalStrategy::traverseAllLeafElements(leafVisitor, cluster, galerkinState);
    delete leafVisitor;

    sglMakeCurrent(prev_sgl_context);
    return bbx.coordinates;
}

/**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
ColorRgb
ScratchVisibilityStrategy::scratchRadiance(const GalerkinState *galerkinState) {
    int nonBackGround;
    const SGL_PIXEL *pix;
    ColorRgb rad;

    rad.clear();
    nonBackGround = 0;
    for ( int j = 0; j < galerkinState->scratch->vp_height; j++ ) {
        pix = galerkinState->scratch->frameBuffer + j * galerkinState->scratch->width;
        for ( int i = 0; i < galerkinState->scratch->vp_width; i++, pix++ ) {
            const GalerkinElement *element = (GalerkinElement *) (*pix);
            if ( element != nullptr ) {
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
        rad.scale(1.0f / (float) (galerkinState->scratch->vp_width * galerkinState->scratch->vp_height));
    }
    return rad;
}

/**
Computes the number of non background pixels
*/
int
ScratchVisibilityStrategy::scratchNonBackgroundPixels(const GalerkinState *galerkinState) {
    const SGL_PIXEL *pix;
    int nonBackGround = 0;

    for ( int j = 0; j < galerkinState->scratch->vp_height; j++ ) {
        pix = galerkinState->scratch->frameBuffer + j * galerkinState->scratch->width;
        for ( int i = 0; i < galerkinState->scratch->vp_width; i++, pix++ ) {
            const GalerkinElement *elem = (GalerkinElement *) (*pix);
            if ( elem ) {
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
        const SGL_PIXEL *pix = galerkinState->scratch->frameBuffer + i * galerkinState->scratch->width;
        for ( int j = 0; j < galerkinState->scratch->vp_width; j++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *)(*pix);
            if ( elem != nullptr ) {
                elem->scratchVisibilityUsageCounter++;
            }
        }
    }
}
