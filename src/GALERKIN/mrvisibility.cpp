/**
Multi-Resolution Visibility ala Sillion & Drettakis, "Multi-Resolution
Control of Visibility Error", SIGGRAPH '95 p145
*/

#include "common/error.h"
#include "SGL/sgl.h"
#include "GALERKIN/shadowcaching.h"
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
    float rcvDist,
    float srcSize,
    float minimumFeatureSize)
{
    Vector3D vectorTmp;
    float tMinimum;
    float tMaximum;
    float t;
    float fSize;
    float *bbx;
    GalerkinElement *cluster = (GalerkinElement *) (geom->radianceData);
    RayHit hitStore;

    if ( geom == GLOBAL_geom_excludedGeom1 || geom == GLOBAL_geom_excludedGeom2 ) {
        return 1.0;
    }

    if ( !geom->bounded ) {
        logFatal(-1, "geomMultiResolutionVisibility", "Don't know what to do with unbounded geoms");
    }

    fSize = HUGE;
    tMinimum = rcvDist * ((float)EPSILON);
    tMaximum = rcvDist;
    bbx = geom->bounds.coordinates;

    /* Check ray/bounding volume intersection and compute feature size of
     * occluder. */
    vectorSumScaled(ray->pos, tMinimum, ray->dir, vectorTmp);
    if ( outOfBounds(&vectorTmp, bbx)) {
        if ( !boundsIntersectingSegment(ray, bbx, &tMinimum, &tMaximum) ) {
            // Ray doesn't intersect the bounding box of the Geometry within
		    // distance interval tMinimum ... tMaximum
            return 1.0;
        }

        if ( cluster ) {
            // Compute feature size using equivalent blocker size of the occluder
            t = (tMinimum + tMaximum) / 2.0f; // Put the centre of the equivalent blocker halfway tMinimum and tMaximum
            fSize = srcSize + rcvDist / t * (cluster->blockerSize - srcSize);
        }
    }

    if ( fSize < minimumFeatureSize ) {
        double kappa = 0.0;
        double vol;
        vol = (bbx[MAX_X] - bbx[MIN_X] + EPSILON) * (bbx[MAX_Y] - bbx[MIN_Y] + EPSILON) *
              (bbx[MAX_Z] - bbx[MIN_Z] + EPSILON);
        if ( cluster != nullptr ) {
            kappa = cluster->area / (4.0 * vol);
        }
        return std::exp(-kappa * (tMaximum - tMinimum));
    } else {
        if ( geomIsAggregate(geom) ) {
            java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geom);
            double visibility = geomListMultiResolutionVisibility(geometryList, ray, rcvDist, srcSize, minimumFeatureSize);
            delete geometryList;
            return visibility;
        } else {
            RayHit *hit = patchListIntersect(
                    geomPatchArrayListReference(geom),
                    ray,
                    rcvDist * ((float) EPSILON), &rcvDist, HIT_FRONT | HIT_ANY, &hitStore);
            if ( hit != nullptr ) {
                addToShadowCache(hit->patch);
                return 0.0;
            } else {
                return 1.0;
            }
        }
    }
}

double
geomListMultiResolutionVisibility(
    java::ArrayList<Geometry *> *geometryOccluderList,
    Ray *ray,
    float rcvdist,
    float srcSize,
    float minimumFeatureSize)
{
    double vis = 1.0;

    for ( int i = 0; geometryOccluderList != nullptr && i < geometryOccluderList->size(); i++ ) {
        double v;
        v = geomMultiResolutionVisibility(geometryOccluderList->get(i), ray, rcvdist, srcSize, minimumFeatureSize);
        if ( v < EPSILON ) {
            return 0.0;
        } else {
            vis *= v;
        }
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
    globalSglContext = new SGL_CONTEXT(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS, FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);

    globalBuffer1 = new unsigned char [FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS];
    globalBuffer2 = new unsigned char [FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS];
}

/**
Destroys the sgl context created by blockerInit()
*/
void
blockerTerminate() {
    delete[] globalBuffer2;
    delete[] globalBuffer1;
    delete globalSglContext;
}
