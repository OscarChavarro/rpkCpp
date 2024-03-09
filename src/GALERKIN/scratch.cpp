/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#include "SGL/sgl.h"
#include "skin/Geometry.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/clustergalerkincpp.h"

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
scratchInit() {
    GLOBAL_galerkin_state.scratch = new SGL_CONTEXT(GLOBAL_galerkin_state.scratchFbSize, GLOBAL_galerkin_state.scratchFbSize);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);
}

/**
Terminates scratch rendering
*/
void
scratchTerminate() {
    delete GLOBAL_galerkin_state.scratch;
}

static void
scratchRenderElementPtr(GalerkinElement *elem) {
    Patch *patch = elem->patch;
    Vector3D v[4];
    int i;

    // Backface culling test: only render the element if it is turned towards
    // the current eye point
    if ( vectorDotProduct(patch->normal, globalEyePoint) + patch->planeConstant < EPSILON ) {
        return;
    }

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        v[i] = *patch->vertex[i]->point;
    }

    // TODO: Extend SGL_CONTEXT to support Element*
    GLOBAL_sgl_currentContext->sglSetColor((SGL_PIXEL)elem);
    GLOBAL_sgl_currentContext->sglPolygon(patch->numberOfVertices, v);
}

/**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer and returns pointer to a bounding box
containing the size of the virtual screen. The cluster nicely fits
into the virtual screen
*/
float *
scratchRenderElements(GalerkinElement *cluster, Vector3D eye) {
    Vector3D centre = cluster->midPoint();
    Vector3D up = {0.0, 0.0, 1.0};
    Vector3D viewDirection;
    static BoundingBox bbx;
    Matrix4x4 lookAt{};
    SGL_CONTEXT *prev_sgl_context;
    int vp_size;

    if ( cluster->id == GLOBAL_galerkin_state.lastClusterId && vectorEqual(eye, GLOBAL_galerkin_state.lastEye, EPSILON)) {
        return bbx.coordinates;
    } else {
        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        GLOBAL_galerkin_state.lastClusterId = cluster->id;
        GLOBAL_galerkin_state.lastEye = eye;
    }

    vectorSubtract(centre, eye, viewDirection);
    vectorNormalize(viewDirection);
    if ( std::fabs(vectorDotProduct(up, viewDirection)) > 1. - EPSILON ) vectorSet(up, 0.0, 1.0, 0.0);
    lookAt = lookAtMatrix(eye, centre, up);

    geomBounds(cluster->geometry).transformTo(&lookAt, &bbx);

    prev_sgl_context = sglMakeCurrent(GLOBAL_galerkin_state.scratch);
    GLOBAL_sgl_currentContext->sglLoadMatrix(
        orthogonalViewMatrix(
            bbx.coordinates[MIN_X],
            bbx.coordinates[MAX_X],
            bbx.coordinates[MIN_Y],
            bbx.coordinates[MAX_Y],
            -bbx.coordinates[MAX_Z],
            -bbx.coordinates[MIN_Z]));
    GLOBAL_sgl_currentContext->sglMultiplyMatrix(lookAt);

    // Choose a viewport depending on the relative size of the smallest
    // surface element in the cluster to be rendered
    vp_size = (int)(
        (bbx.coordinates[MAX_X] - bbx.coordinates[MIN_X]) * (bbx.coordinates[MAX_Y] - bbx.coordinates[MIN_Y]
    ) / cluster->minimumArea);
    if ( vp_size > GLOBAL_galerkin_state.scratch->width ) {
        vp_size = GLOBAL_galerkin_state.scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    GLOBAL_sgl_currentContext->sglViewport(0, 0, vp_size, vp_size);

    // Render element pointers in the scratch frame buffer
    globalEyePoint = eye; // Needed for backface culling test
    GLOBAL_sgl_currentContext->sglClear((SGL_PIXEL) 0x00, SGL_MAXIMUM_Z);
    iterateOverSurfaceElementsInCluster(cluster, scratchRenderElementPtr);

    sglMakeCurrent(prev_sgl_context);
    return bbx.coordinates;
}

/**
After rendering element pointers in the scratch frame buffer, this routine
computes the average radiance of the virtual screen
*/
COLOR
scratchRadiance() {
    int nonBackGround;
    SGL_PIXEL *pix;
    COLOR rad;
    int i;
    int j;

    colorClear(rad);
    nonBackGround = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->frameBuffer + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *) (*pix);
            if ( elem != nullptr ) {
                if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
                     GLOBAL_galerkin_state.iteration_method == JACOBI ) {
                    colorAdd(rad, elem->radiance[0], rad);
                } else {
                    colorAdd(rad, elem->unShotRadiance[0], rad);
                }
                nonBackGround++;
            }
        }
    }
    if ( nonBackGround > 0 ) {
        colorScale(1.0f / (float) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height),
                   rad, rad);
    }
    return rad;
}

/**
Computes the number of non background pixels
*/
int
scratchNonBackgroundPixels() {
    int nonBackGround;
    SGL_PIXEL *pix;
    int i, j;

    nonBackGround = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->frameBuffer + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *) (*pix);
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
scratchPixelsPerElement() {
    SGL_PIXEL *pix;

    for ( int j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->frameBuffer + j * GLOBAL_galerkin_state.scratch->width;
        for ( int i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *) (*pix);
            if ( elem ) {
                elem->tmp++;
            }
        }
    }
}
