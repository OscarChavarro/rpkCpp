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
    GLOBAL_galerkin_state.scratch = sglOpen(GLOBAL_galerkin_state.scratchFbSize, GLOBAL_galerkin_state.scratchFbSize);
    sglDepthTesting(GLOBAL_sgl_currentContext, true);
}

/**
Terminates scratch rendering
*/
void
scratchTerminate() {
    sglClose(GLOBAL_galerkin_state.scratch);
}

static void
scratchRenderElementPtr(GalerkinElement *elem) {
    Patch *patch = elem->patch;
    Vector3D v[4];
    int i;

    // Backface culling test: only render the element if it is turned towards
    // the current eye point
    if ( VECTORDOTPRODUCT(patch->normal, globalEyePoint) + patch->planeConstant < EPSILON ) {
        return;
    }

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        v[i] = *patch->vertex[i]->point;
    }

    // TODO: Extend SGL_CONTEXT to support Element*
    sglSetColor(GLOBAL_sgl_currentContext, (SGL_PIXEL)elem);
    sglPolygon(GLOBAL_sgl_currentContext, patch->numberOfVertices, v);
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
    Vector3D centre = galerkinElementMidPoint(cluster);
    Vector3D up = {0.0, 0.0, 1.0};
    Vector3D viewDirection;
    static BOUNDINGBOX bbx;
    Matrix4x4 lookAt{};
    SGL_CONTEXT *prev_sgl_context;
    int vp_size;

    if ( cluster->id == GLOBAL_galerkin_state.lastClusterId && VECTOREQUAL(eye, GLOBAL_galerkin_state.lastEye, EPSILON)) {
        return bbx;
    } else {
        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        GLOBAL_galerkin_state.lastClusterId = cluster->id;
        GLOBAL_galerkin_state.lastEye = eye;
    }

    VECTORSUBTRACT(centre, eye, viewDirection);
    VECTORNORMALIZE(viewDirection);
    if ( fabs(VECTORDOTPRODUCT(up, viewDirection)) > 1. - EPSILON ) VECTORSET(up, 0., 1., 0.);
    lookAt = lookAtMatrix(eye, centre, up);

    boundsTransform(geomBounds(cluster->geom), &lookAt, bbx);

    prev_sgl_context = sglMakeCurrent(GLOBAL_galerkin_state.scratch);
    sglLoadMatrix(GLOBAL_sgl_currentContext, orthogonalViewMatrix(bbx[MIN_X], bbx[MAX_X], bbx[MIN_Y], bbx[MAX_Y], -bbx[MAX_Z], -bbx[MIN_Z]));
    sglMultiplyMatrix(GLOBAL_sgl_currentContext, lookAt);

    // Choose a viewport depending on the relative size of the smallest
    // surface element in the cluster to be rendered
    vp_size = (int) ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) / cluster->minimumArea);
    if ( vp_size > GLOBAL_galerkin_state.scratch->width ) {
        vp_size = GLOBAL_galerkin_state.scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    sglViewport(GLOBAL_sgl_currentContext, 0, 0, vp_size, vp_size);

    // Render element pointers in the scratch frame buffer
    globalEyePoint = eye; // Needed for backface culling test
    sglClear(GLOBAL_sgl_currentContext, (SGL_PIXEL) 0x00, SGL_MAXIMUM_Z);
    iterateOverSurfaceElementsInCluster(cluster, scratchRenderElementPtr);

    sglMakeCurrent(prev_sgl_context);
    return bbx;
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
