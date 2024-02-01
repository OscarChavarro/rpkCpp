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
Src is a toplevel surface element. Render the corrsponding patch
with pixel value a pointer to the element. Uses global variable
eyep for backface culling
*/
static Vector3D eyep;

/**
Create a scratch software renderer for various operations on clusters
*/
void
scratchInit() {
    GLOBAL_galerkin_state.scratch = sglOpen(GLOBAL_galerkin_state.scratch_fb_size, GLOBAL_galerkin_state.scratch_fb_size);
    sglDepthTesting(true);
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

    /* Backface culling test: only render the element if it is turned towards
     * the current eye point */
    if ( VECTORDOTPRODUCT(patch->normal, eyep) + patch->planeConstant < EPSILON ) {
        return;
    }

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        v[i] = *patch->vertex[i]->point;
    }

    sglSetColor((SGL_PIXEL) elem);
    sglPolygon(patch->numberOfVertices, v);
}

/**
Sets up an orthographic projection of the cluster as
seen from the eye. Renders the element pointers to the elements
in cluster in the scratch frame buffer and returns pointer to a bounding box
containing the size of the virtual screen. The cluster clus nicely fits
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

    if ( cluster->id == GLOBAL_galerkin_state.lastclusid && VECTOREQUAL(eye, GLOBAL_galerkin_state.lasteye, EPSILON)) {
        return bbx;
    } else {
        // Cache previously rendered cluster and eye point in order to
        // avoid re-rendering the same situation next time
        GLOBAL_galerkin_state.lastclusid = cluster->id;
        GLOBAL_galerkin_state.lasteye = eye;
    }

    VECTORSUBTRACT(centre, eye, viewDirection);
    VECTORNORMALIZE(viewDirection);
    if ( fabs(VECTORDOTPRODUCT(up, viewDirection)) > 1. - EPSILON ) VECTORSET(up, 0., 1., 0.);
    lookAt = lookAtMatrix(eye, centre, up);

    boundsTransform(geomBounds(cluster->geom), &lookAt, bbx);

    prev_sgl_context = sglMakeCurrent(GLOBAL_galerkin_state.scratch);
    sglLoadMatrix(orthogonalViewMatrix(bbx[MIN_X], bbx[MAX_X], bbx[MIN_Y], bbx[MAX_Y], -bbx[MAX_Z], -bbx[MIN_Z]));
    sglMultiplyMatrix(lookAt);

    // Choose a viewport depending on the relative size of the smallest
    // surface element in the cluster to be rendered
    vp_size = (int) ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) / cluster->minimumArea);
    if ( vp_size > GLOBAL_galerkin_state.scratch->width ) {
        vp_size = GLOBAL_galerkin_state.scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    sglViewport(0, 0, vp_size, vp_size);

    // Render element pointers in the scratch frame buffer
    eyep = eye; // Needed for backface culling test
    sglClear((SGL_PIXEL) nullptr, SGL_MAXIMUM_Z);
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
    int nonbkgrnd;
    SGL_PIXEL *pix;
    COLOR rad;
    int i;
    int j;

    colorClear(rad);
    nonbkgrnd = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->frameBuffer + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *) (*pix);
            if ( elem ) {
                if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
                     GLOBAL_galerkin_state.iteration_method == JACOBI ) {
                    colorAdd(rad, elem->radiance[0], rad);
                } else {
                    colorAdd(rad, elem->unShotRadiance[0], rad);
                }
                nonbkgrnd++;
            }
        }
    }
    if ( nonbkgrnd > 0 ) {
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
    int nonbkgrnd;
    SGL_PIXEL *pix;
    int i, j;

    nonbkgrnd = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->frameBuffer + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            GalerkinElement *elem = (GalerkinElement *) (*pix);
            if ( elem ) {
                nonbkgrnd++;
            }
        }
    }
    return nonbkgrnd;
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
