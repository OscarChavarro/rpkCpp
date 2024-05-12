#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/ShadowCache.h"
#include "GALERKIN/processing/FormFactorClusteredStrategy.h"

/**
Like above, but with a constant approximation on both the receiver and source
element, which makes things slightly simpler
*/
void
FormFactorClusteredStrategy::doConstantAreaToAreaFormFactor(
    Interaction *link,
    const CubatureRule *cubatureRuleRcv,
    const CubatureRule *cubatureRuleSrc,
    double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES])
{
    const GalerkinElement *receiverElement = link->receiverElement;
    const GalerkinElement *sourceElement = link->sourceElement;
    double Gx;
    double G = 0.0;
    double gMin = HUGE;
    double gMax = -HUGE;

    for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
        Gx = 0.0;
        for ( int l = 0; l < cubatureRuleSrc->numberOfNodes; l++ ) {
            Gx += cubatureRuleSrc->w[l] * Gxy[k][l];
        }
        Gx *= sourceElement->area;

        G += cubatureRuleRcv->w[k] * Gx;

        if ( Gx > gMax ) {
            gMax = Gx;
        }
        if ( Gx < gMin ) {
            gMin = Gx;
        }
    }
    link->K[0] = (float)(receiverElement->area * G);

    link->deltaK = new float[1];
    link->deltaK[0] = (float)(G - gMin);
    if ( gMax - G > link->deltaK[0] ) {
        link->deltaK[0] = (float)(gMax - G);
    }

    link->numberOfReceiverCubaturePositions = 1;
}

/**
Return a floating point number in the range [0..1].
0 indicates that there is
full occlusion, 1 that there is full visibility and a number in between
that there is visibility with at least one occluder with feature size
smaller than the specified minimum feature size. (Such occluders have been
replaced by a box containing an isotropic participating medium with
suitable extinction properties.) rcvdist is the distance from the origin of
the ray (assumed to be on the source) to the receiver surface. srcsize
is the diameter of the source surface. min feature size is the minimal
diameter of a feature (umbra or whole lit region on the receiver)
that one is interested in. Approximate visibility computations are allowed
for occluders that cast features with diameter smaller than
min feature size. If there is a "hard" occlusion, the first patch tested that
lead to this conclusion is added to the shadow cache
*/
double
FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
    const java::ArrayList<Geometry *> *geometryOccluderList,
    ShadowCache *shadowCache,
    Ray *ray,
    float rcvDist,
    float srcSize,
    float minimumFeatureSize)
{
    double vis = 1.0;

    for ( int i = 0; geometryOccluderList != nullptr && i < geometryOccluderList->size(); i++ ) {
        double v = FormFactorClusteredStrategy::geometryMultiResolutionVisibility(
            shadowCache, geometryOccluderList->get(i), ray, rcvDist, srcSize, minimumFeatureSize);
        if ( v < EPSILON ) {
            return 0.0;
        } else {
            vis *= v;
        }
    }

    return vis;
}

double
FormFactorClusteredStrategy::geometryMultiResolutionVisibility(
    ShadowCache *shadowCache,
    Geometry *geometry,
    Ray *ray,
    float rcvDist,
    float srcSize,
    float minimumFeatureSize)
{
    if ( geometry->isExcluded() ) {
        return 1.0;
    }

    if ( !geometry->bounded ) {
        logFatal(-1, "geometryMultiResolutionVisibility", "Don't know what to do with unbounded geoms");
    }

    float fSize = HUGE_FLOAT;
    float tMinimum = rcvDist * ((float)EPSILON);
    float tMaximum = rcvDist;
    const BoundingBox *boundingBox = &geometry->boundingBox;

    // Check ray/bounding volume intersection and compute feature size of occluder
    Vector3D vectorTmp;
    const GalerkinElement *cluster = (GalerkinElement *)geometry->radianceData;

    vectorTmp.sumScaled(ray->pos, tMinimum, ray->dir);
    if ( boundingBox->outOfBounds(&vectorTmp) ) {
        if ( !boundingBox->intersectingSegment(ray, &tMinimum, &tMaximum) ) {
            // Ray doesn't intersect the bounding box of the Geometry within
            // distance interval tMinimum ... tMaximum
            return 1.0;
        }

        if ( cluster ) {
            // Compute feature size using equivalent blocker size of the occluder
            float t;
            t = (tMinimum + tMaximum) / 2.0f; // Put the centre of the equivalent blocker halfway tMinimum and tMaximum
            fSize = srcSize + rcvDist / t * (cluster->blockerSize - srcSize);
        }
    }

    if ( fSize < minimumFeatureSize ) {
        double kappa = 0.0;
        double vol;
        vol = (boundingBox->coordinates[MAX_X] - boundingBox->coordinates[MIN_X] + EPSILON)
            * (boundingBox->coordinates[MAX_Y] - boundingBox->coordinates[MIN_Y] + EPSILON)
            * (boundingBox->coordinates[MAX_Z] - boundingBox->coordinates[MIN_Z] + EPSILON);
        if ( cluster != nullptr ) {
            kappa = cluster->area / (4.0 * vol);
        }
        return std::exp(-kappa * (tMaximum - tMinimum));
    } else {
        if ( geometry->isCompound() ) {
            java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
            double visibility = FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
                geometryList, shadowCache, ray, rcvDist, srcSize, minimumFeatureSize);
            delete geometryList;
            return visibility;
        } else {
            RayHit hitStore;
            RayHit *hit = Geometry::patchListIntersect(
                    geomPatchArrayListReference(geometry),
                    ray,
                    rcvDist * ((float)EPSILON), &rcvDist, HIT_FRONT | HIT_ANY, &hitStore);
            if ( hit != nullptr ) {
                shadowCache->addToShadowCache(hit->getPatch());
                return 0.0;
            } else {
                return 1.0;
            }
        }
    }
}
