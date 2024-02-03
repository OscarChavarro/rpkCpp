/**
Multi-Resolution Visibility ala Sillion & Drettakis, "Multi-Resolution
Control of Visibility Error", SIGGRAPH '95 p145
*/

#include "common/error.h"
#include "SGL/sgl.h"
#include "shared/shadowcaching.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/mrvisibility.h"

static SGL_CONTEXT *globalSglContext; // Sgl context for determining equivalent blocker sizes
static unsigned char *globalBuffer1; // Needed for eroding and expanding
static unsigned char *globalBuffer2;

/**
Geoms will be rendered into a frame buffer of this size for determining
the equivalent blocker size
*/
#define FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS 30

static double
geomMultiResolutionVisibility(
    Geometry *geom,
    Ray *ray,
    float rcvdist,
    float srcSize,
    float minimumFeatureSize)
{
    Vector3D vtmp;
    float tmin;
    float tmax;
    float t;
    float fsize;
    float *bbx;
    GalerkinElement *clus = (GalerkinElement *) (geom->radiance_data);
    RayHit hitstore;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
        return 1.;
    }

    if ( !geom->bounded ) {
        logFatal(-1, "geomMultiResolutionVisibility", "Don't know what to do with unbounded geoms");
    }

    fsize = HUGE;
    tmin = rcvdist * ((float)EPSILON);
    tmax = rcvdist;
    bbx = geom->bounds;

    /* Check ray/bounding volume intersection and compute feature size of
     * occluder. */
    VECTORSUMSCALED(ray->pos, tmin, ray->dir, vtmp);
    if ( outOfBounds(&vtmp, bbx)) {
        if ( !boundsIntersectingSegment(ray, bbx, &tmin, &tmax)) {
            return 1.;
        }  /* ray doesn't intersect the bounding box of the Geometry within
		   * distance interval tmin ... tmax. */

        if ( clus ) {
            /* Compute feature size using equivalent blocker size of the occluder */
            t = (tmin + tmax) / 2.0f;    /* put the centre of the equivalent blocker halfway tmin and tmax */
            fsize = srcSize + rcvdist / t * (clus->bsize - srcSize);
        }
    }

    if ( fsize < minimumFeatureSize ) {
        double kappa, vol;
        vol = (bbx[MAX_X] - bbx[MIN_X] + EPSILON) * (bbx[MAX_Y] - bbx[MIN_Y] + EPSILON) *
              (bbx[MAX_Z] - bbx[MIN_Z] + EPSILON);
        kappa = clus->area / (4. * vol);
        return exp(-kappa * (tmax - tmin));
    } else {
        if ( geomIsAggregate(geom)) {
            return geomListMultiResolutionVisibility(geomPrimList(geom), ray, rcvdist, srcSize, minimumFeatureSize);
        } else {
            RayHit *hit = patchListIntersect(
                geomPatchArrayList(geom),
                ray,
                rcvdist * ((float) EPSILON), &rcvdist, HIT_FRONT | HIT_ANY, &hitstore);
            if ( hit != nullptr ) {
                addToShadowCache(hit->patch);
                return 0.;
            } else {
                return 1.;
            }
        }
    }
}

double
geomListMultiResolutionVisibility(
    GeometryListNode *occluderList,
    Ray *ray,
    float rcvdist,
    float srcSize,
    float minimumFeatureSize)
{
    double vis = 1.0;

    while ( occluderList ) {
        double v;
        v = geomMultiResolutionVisibility(occluderList->geometry, ray, rcvdist, srcSize, minimumFeatureSize);
        if ( v < EPSILON ) {
            return 0.0;
        } else {
            vis *= v;
        }

        occluderList = occluderList->next;
    }

    return vis;
}

/**
Equivalent blocker size determination
*/

/**
Creates an sgl context needed for determining the equivalent blocker size
of some objects
*/
void
blockerInit() {
    globalSglContext = sglOpen(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS, FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    sglDepthTesting(true);

    globalBuffer1 = (unsigned char *)malloc(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    globalBuffer2 = (unsigned char *)malloc(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
}

/**
Destroys the sgl context created by blockerInit()
*/
void
blockerTerminate() {
    free((char *) globalBuffer2);
    free((char *) globalBuffer1);

    sglClose(globalSglContext);
}
