/* scratch.c: scratch renderer routines. Used for handling intra-cluster visibility
 * with a Z-buffer visibility algorithm in software. */

#include "SGL/sgl.h"
#include "skin/Geometry.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/clustergalerkincpp.h"

/* create a scratch software renderer for various operations on clusters. */
void ScratchInit() {
    GLOBAL_galerkin_state.scratch = sglOpen(GLOBAL_galerkin_state.scratch_fb_size, GLOBAL_galerkin_state.scratch_fb_size);
    sglDepthTesting(true);
}

/* terminates scratch rendering */
void ScratchTerminate() {
    sglClose(GLOBAL_galerkin_state.scratch);
}

/* src is a toplevel surface element. Render the corrsponding patch 
 * with pixel value a pointer to the element. Uses global variable
 * eyep for backface culling. */
static Vector3D eyep;

static void ScratchRenderElementPtr(ELEMENT *elem) {
    PATCH *patch = elem->pog.patch;
    Vector3D v[4];
    int i;

    /* Backface culling test: only render the element if it is turned towards
     * the current eye point */
    if ( VECTORDOTPRODUCT(patch->normal, eyep) + patch->plane_constant < EPSILON ) {
        return;
    }

    for ( i = 0; i < patch->nrvertices; i++ ) {
        v[i] = *patch->vertex[i]->point;
    }

    sglSetColor((SGL_PIXEL) elem);
    sglPolygon(patch->nrvertices, v);
}

/* Sets up an orthographic projection of the cluster as
 * seen from the eye. Renders the element pointers to the elements
 * in clus in the scratch frame buffer and returns pointer to a boundingbox 
 * containing the size of the virtual screen. The cluster clus nicely fits
 * into the virtual screen. */
float *ScratchRenderElementPtrs(ELEMENT *clus, Vector3D eye) {
    Vector3D centre = ElementMidpoint(clus);
    Vector3D up = {0., 0., 1.}, viewdir;
    static BOUNDINGBOX bbx;
    Matrix4x4 lookat;
    SGL_CONTEXT *prev_sgl_context;
    int vp_size;

    if ( clus->id == GLOBAL_galerkin_state.lastclusid && VECTOREQUAL(eye, GLOBAL_galerkin_state.lasteye, EPSILON)) {
        return bbx;
    } else {
        /* cache previously rendered cluster and eye point in order to
         * avoid rerendering the same situation next time. */
        GLOBAL_galerkin_state.lastclusid = clus->id;
        GLOBAL_galerkin_state.lasteye = eye;
    }

    VECTORSUBTRACT(centre, eye, viewdir);
    VECTORNORMALIZE(viewdir);
    if ( fabs(VECTORDOTPRODUCT(up, viewdir)) > 1. - EPSILON ) VECTORSET(up, 0., 1., 0.);
    lookat = LookAt(eye, centre, up);

    BoundsTransform(geomBounds(clus->pog.geom), &lookat, bbx);

    prev_sgl_context = sglMakeCurrent(GLOBAL_galerkin_state.scratch);
    sglLoadMatrix(Ortho(bbx[MIN_X], bbx[MAX_X], bbx[MIN_Y], bbx[MAX_Y], -bbx[MAX_Z], -bbx[MIN_Z]));
    sglMultMatrix(lookat);

    /* choose a viewport depending on the relative size of the smallest
     * surface element in the cluster to be rendered. */
    vp_size = (int) ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) / clus->minarea);
    if ( vp_size > GLOBAL_galerkin_state.scratch->width ) {
        vp_size = GLOBAL_galerkin_state.scratch->width;
    }
    if ( vp_size < 32 ) {
        vp_size = 32;
    }
    sglViewport(0, 0, vp_size, vp_size);

    /* Render element pointers in the scratch frame buffer. */
    eyep = eye;    /* needed for backface culling test */
    sglClear((SGL_PIXEL) nullptr, SGL_ZMAX);
    iterateOverSurfaceElementsInCluster(clus, ScratchRenderElementPtr);

    sglMakeCurrent(prev_sgl_context);
    return bbx;
}

/* After rendering element pointers in the scratch frame buffer, this routine
 * computes the average radiance of the virtual screen. */
COLOR ScratchRadiance() {
    int nonbkgrnd;
    SGL_PIXEL *pix;
    COLOR rad;
    int i, j;

    colorClear(rad);
    nonbkgrnd = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->fbuf + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            ELEMENT *elem = (ELEMENT *) (*pix);
            if ( elem ) {
                if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
                     GLOBAL_galerkin_state.iteration_method == JACOBI ) {
                    COLORADD(rad, elem->radiance[0], rad);
                } else {
                    COLORADD(rad, elem->unshot_radiance[0], rad);
                }
                nonbkgrnd++;
            }
        }
    }
    if ( nonbkgrnd > 0 ) {
        COLORSCALE(1. / (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height), rad, rad);
    }
    return rad;
}

/* Computes the number of non background pixels. */
int ScratchNonBackgroundPixels() {
    int nonbkgrnd;
    SGL_PIXEL *pix;
    int i, j;

    nonbkgrnd = 0;
    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->fbuf + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            ELEMENT *elem = (ELEMENT *) (*pix);
            if ( elem ) {
                nonbkgrnd++;
            }
        }
    }
    return nonbkgrnd;
}

/* Counts the number of pixels occupied by each element. The result is
 * accumulated in the tmp field of the elements. This field should be
 * initialized to zero before. */
void ScratchPixelsPerElement() {
    SGL_PIXEL *pix;
    int i, j;

    for ( j = 0; j < GLOBAL_galerkin_state.scratch->vp_height; j++ ) {
        pix = GLOBAL_galerkin_state.scratch->fbuf + j * GLOBAL_galerkin_state.scratch->width;
        for ( i = 0; i < GLOBAL_galerkin_state.scratch->vp_width; i++, pix++ ) {
            ELEMENT *elem = (ELEMENT *) (*pix);
            if ( elem ) {
                elem->tmp++;
            }
        }
    }
}
