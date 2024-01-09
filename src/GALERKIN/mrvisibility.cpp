/**
Multi-Resolution Visibility ala Sillion & Drettakis, "Multi-Resolution
Control of Visibility Error", SIGGRAPH '95 p145
 */

#include <cstdlib>

#include "common/error.h"
#include "common/mymath.h"
#include "skin/Geometry.h"
#include "SGL/sgl.h"
#include "shared/shadowcaching.h"
#include "GALERKIN/mrvisibility.h"
#include "GALERKIN/elementgalerkin.h"

static SGL_CONTEXT *sgl; // sgl context for determining equivalent blocker sizes
static unsigned char *buf1, *buf2; // needed for eroding and expanding

/**
Geoms will be rendered into a frame buffer of this size for determining
the equivalent blocker size
*/
#define FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS 30

static double
GeomMultiResolutionVisibility(
        Geometry *geom,
        Ray *ray,
        float rcvdist,
        float srcsize,
        float minfeaturesize)
{
    Vector3D vtmp;
    float tmin, tmax, t, fsize, *bbx;
    ELEMENT *clus = (ELEMENT *) (geom->radiance_data);
    RayHit hitstore;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
        return 1.;
    }

    if ( !geom->bounded ) {
        logFatal(-1, "GeomMultiResolutionVisibility", "Don't know what to do with unbounded geoms");
    }

    fsize = HUGE;
    tmin = rcvdist * ((float)EPSILON);
    tmax = rcvdist;
    bbx = geom->bounds;

    /* Check ray/bounding volume intersection and compute feature size of
     * occluder. */
    VECTORSUMSCALED(ray->pos, tmin, ray->dir, vtmp);
    if ( OutOfBounds(&vtmp, bbx)) {
        if ( !boundsIntersectingSegment(ray, bbx, &tmin, &tmax)) {
            return 1.;
        }  /* ray doesn't intersect the bounding box of the Geometry within
		   * distance interval tmin ... tmax. */

        if ( clus ) {
            /* Compute feature size using equivalent blocker size of the occluder */
            t = (tmin + tmax) / 2.0f;    /* put the centre of the equivalent blocker halfway tmin and tmax */
            fsize = srcsize + rcvdist / t * (clus->bsize - srcsize);
        }
    }

    if ( fsize < minfeaturesize ) {
        double kappa, vol;
        vol = (bbx[MAX_X] - bbx[MIN_X] + EPSILON) * (bbx[MAX_Y] - bbx[MIN_Y] + EPSILON) *
              (bbx[MAX_Z] - bbx[MIN_Z] + EPSILON);
        kappa = clus->area / (4. * vol);
        return exp(-kappa * (tmax - tmin));
    } else {
        if ( geomIsAggregate(geom)) {
            return GeomListMultiResolutionVisibility(geomPrimList(geom), ray, rcvdist, srcsize, minfeaturesize);
        } else {
            RayHit *hit;
            if ((hit = patchListIntersect(geomPatchList(geom), ray, rcvdist * ((float) EPSILON), &rcvdist,
                                          HIT_FRONT | HIT_ANY,
                                          &hitstore))) {
                AddToShadowCache(hit->patch);
                return 0.;
            } else {
                return 1.;
            }
        }
    }
}

double
GeomListMultiResolutionVisibility(
        GeometryListNode *occluderlist,
        Ray *ray, float rcvdist,
        float srcsize,
        float minfeaturesize)
{
    double vis = 1.;

    while ( occluderlist ) {
        double v;
        v = GeomMultiResolutionVisibility(occluderlist->geom, ray, rcvdist, srcsize, minfeaturesize);
        if ( v < EPSILON ) {
            return 0.;
        } else {
            vis *= v;
        }

        occluderlist = occluderlist->next;
    }

    return vis;
}

/* ************* Equivalent blocker size determination. ************** */

/**
Creates an sgl context needed for determining the equivalent blocker size
of some objects
*/
void
BlockerInit() {
    sgl = sglOpen(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS, FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    sglDepthTesting(true);

    buf1 = (unsigned char *)malloc(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    buf2 = (unsigned char *)malloc(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
}

/**
Destroys the sgl context created by BlockerInit()
*/
void
BlockerTerminate() {
    free((char *) buf2);
    free((char *) buf1);

    sglClose(sgl);
}
