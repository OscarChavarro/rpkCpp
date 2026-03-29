#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "skin/MinMaxBox.h"
#include "galerkin/GalerkinElement.h"
#include "galerkin/processing/FormFactorClusteredStrategy.h"
#include "galerkin/ShadowCache.h"

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
    double gMin = Numeric::HUGE_DOUBLE_VALUE;
    double gMax = -Numeric::HUGE_DOUBLE_VALUE;

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
    link->K[0] = static_cast<float>(receiverElement->area * G);

    link->deltaK = new float[1];
    link->deltaK[0] = static_cast<float>(G - gMin);
    if ( gMax - G > link->deltaK[0] ) {
        link->deltaK[0] = static_cast<float>(gMax - G);
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
        if ( v < Numeric::EPSILON ) {
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
        Error::fatal(-1, "geometryMultiResolutionVisibility", "Don't know what to do with unbounded geoms");
    }

    float fSize = Numeric::HUGE_FLOAT_VALUE;
    float tMinimum = rcvDist * Numeric::EPSILON_FLOAT;
    float tMaximum = rcvDist;
    const BoundingBox *boundingBox = &geometry->boundingBox;
    MinMaxBox *minMaxBox = geometry->getRayIntersectionBox();

    // Check ray/bounding volume intersection and compute feature size of occluder
    Vector3D vectorTmp;
    const GalerkinElement *cluster = static_cast<GalerkinElement *>(geometry->radianceData);

    vectorTmp.sumScaled(ray->position, tMinimum, ray->direction);
    if ( boundingBox->outOfBounds(&vectorTmp) ) {
        if ( !minMaxBox->intersectingSegment(ray, &tMinimum, &tMaximum) ) {
            // Ray doesn't intersect the bounding box of the Geometry within
            // distance interval tMinimum ... tMaximum
            return 1.0;
        }

        if ( cluster ) {
            // Compute feature size using equivalent blocker size of the occluder
            float t = (tMinimum + tMaximum) / 2.0f; // Put the centre of the equivalent blocker halfway tMinimum and tMaximum
            fSize = srcSize + rcvDist / t * (cluster->blockerSize - srcSize);
        }
    }

    if ( fSize < minimumFeatureSize ) {
        double kappa = 0.0;

        double vol =
                (boundingBox->dx() + Numeric::EPSILON) *
                (boundingBox->dy() + Numeric::EPSILON) *
                (boundingBox->dz() + Numeric::EPSILON);

        if ( cluster != nullptr ) {
            kappa = cluster->area / (4.0 * vol);
        }
        return java::Math::exp(-kappa * (tMaximum - tMinimum));
    } else {
        if ( geometry->isCompound() ) {
            java::ArrayList<Geometry *> *geometryList = Geometry::primitiveListCopy(geometry);
            double visibility = FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
                geometryList, shadowCache, ray, rcvDist, srcSize, minimumFeatureSize);
            delete geometryList;
            return visibility;
        } else {
            RayHit hitStore;
            const RayHit *hit = Geometry::patchListIntersect(
                    Geometry::patchListReference(geometry),
                    ray,
                    rcvDist * Numeric::EPSILON_FLOAT,
                    &rcvDist,
                    RayHitFlag::FRONT | RayHitFlag::ANY,
                    &hitStore);

            if ( hit != nullptr ) {
                shadowCache->addToShadowCache(hit->getPatch());
                return 0.0;
            } else {
                return 1.0;
            }
        }
    }
}
