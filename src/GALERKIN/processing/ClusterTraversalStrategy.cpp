/**
Cluster-specific operations

Reference:

[SILL1995a] Sillion & Drettakis, "Featured-based Control of Visibility Error: A Multi-resolution
Clustering Algorithm for Global Illumination", SIGGRAPH '95 p145
*/

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/processing/ScratchVisibilityStrategy.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/processing/visitors/MaximumRadianceVisitor.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"
#include "GALERKIN/processing/visitors/PowerAccumulatorVisitor.h"

static ColorRgb globalSourceRadiance;
static Vector3D globalSamplePoint;
static double globalProjectedArea;
static ColorRgb *globalPSourceRad;
static Interaction *globalTheLink;

// Area corresponding to one pixel in the scratch frame buffer
static double globalPixelArea;

/**
Executes func for every surface element in the cluster
*/
void
ClusterTraversalStrategy::traverseAllLeafElements(
    const ClusterLeafVisitor *leafVisitor,
    GalerkinElement *parentElement,
    void (*leafElementVisitCallBack)(GalerkinElement *elem, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance),

    GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance)
{
    if ( !parentElement->isCluster() ) {
        if ( leafElementVisitCallBack != nullptr ) {
            leafElementVisitCallBack(parentElement, galerkinState, accumulatedRadiance);
        }
        if ( leafVisitor != nullptr ) {
            leafVisitor->visit(parentElement, galerkinState, accumulatedRadiance);
        }
    } else {
        for ( int i = 0;
            parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size();
            i++ ) {
            GalerkinElement *childCluster = (GalerkinElement *)parentElement->irregularSubElements->get(i);
            ClusterTraversalStrategy::traverseAllLeafElements(
                leafVisitor, childCluster, leafElementVisitCallBack, galerkinState, accumulatedRadiance);
        }
    }
}

/**
Returns the radiance or un-shot radiance (depending on the
iteration method) emitted by the source element, a cluster,
towards the samplePoint point
*/
ColorRgb
ClusterTraversalStrategy::clusterRadianceToSamplePoint(
    GalerkinElement *sourceElement,
    Vector3D samplePoint,
    GalerkinState *galerkinState)
{
    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            return sourceElement->radiance[0];

        case GalerkinClusteringStrategy::ORIENTED: {
            // Accumulate the power emitted by the patches in the source cluster
            // towards the samplePoint point
            ColorRgb sourceRadiance;
            sourceRadiance.clear();

            PowerAccumulatorVisitor *leafVisitor = new PowerAccumulatorVisitor(sourceRadiance, samplePoint);
            ClusterTraversalStrategy::traverseAllLeafElements(
                leafVisitor,
                sourceElement,
                nullptr,
                galerkinState,
                &sourceRadiance);
            delete leafVisitor;

            // Divide by the source area used for computing the form factor:
            // sourceElement->area / 4.0 (average projected area)
            sourceRadiance.scale(4.0f / sourceElement->area);
            return sourceRadiance;
        }

        case GalerkinClusteringStrategy::Z_VISIBILITY:
            if ( !sourceElement->isCluster() || !sourceElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
                return sourceElement->radiance[0];
            } else {
                double areaFactor;

                // Render pointers to the elements in the source cluster into the scratch frame
                // buffer, seen from the samplePoint point
                const float *bbx = ScratchVisibilityStrategy::scratchRenderElements(sourceElement, samplePoint, galerkinState);

                // Compute average radiance on the virtual screen
                globalSourceRadiance = ScratchVisibilityStrategy::scratchRadiance(galerkinState);

                // Area factor = area of virtual screen / source cluster area used for
                // form factor computation
                areaFactor = ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y])) /
                             (0.25 * sourceElement->area);
                globalSourceRadiance.scale((float) areaFactor);
                return globalSourceRadiance;
            }

        default:
            logFatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %d\n",
                     galerkinState->clusteringStrategy);
    }
}

/**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
ColorRgb
ClusterTraversalStrategy::sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState) {
    GalerkinElement *src = link->sourceElement;
    const GalerkinElement *rcv = link->receiverElement;

    if ( !src->isCluster() || src == rcv ) {
        logFatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    // Take a sample point on the receiver
    return ClusterTraversalStrategy::clusterRadianceToSamplePoint(src, rcv->midPoint(), galerkinState);
}

/**
Computes projected area of receiver surface element towards the sample point
(global variable). Ignores intra cluster visibility
*/
double
ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(const GalerkinElement *rcv) {
    double rcvCos;
    double dist;
    Vector3D dir;

    dir.subtraction(globalSamplePoint, rcv->patch->midPoint);
    dist = dir.norm();
    if ( dist < Numeric::EPSILON ) {
        rcvCos = 1.0;
    } else {
        rcvCos = dir.dotProduct(rcv->patch->normal) / dist;
    }
    if ( rcvCos <= 0.0 ) {
        // Sample point is behind the rcv
        return 0.0;
    }

    return rcvCos * rcv->area;
}

void
ClusterTraversalStrategy::accumulateProjectedAreaToSamplePoint(
    GalerkinElement *rcv,
    const GalerkinState * /*galerkinState*/,
    ColorRgb * /*accumulatedRadiance*/)
{
    globalProjectedArea += ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(rcv);
}

/**
Computes projected area of receiver cluster as seen from the midpoint of the source,
ignoring intra-receiver visibility
*/
double
ClusterTraversalStrategy::receiverArea(Interaction *link, GalerkinState *galerkinState) {
    const GalerkinElement *src = link->sourceElement;
    GalerkinElement *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        return rcv->area;
    }

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            return rcv->area;

        case GalerkinClusteringStrategy::ORIENTED: {
            globalSamplePoint = src->midPoint();
            globalProjectedArea = 0.0;
            ClusterTraversalStrategy::traverseAllLeafElements(
                nullptr,
                rcv,
                ClusterTraversalStrategy::accumulateProjectedAreaToSamplePoint,
                galerkinState,
                &globalSourceRadiance);
            return globalProjectedArea;
        }

        case GalerkinClusteringStrategy::Z_VISIBILITY:
            globalSamplePoint = src->midPoint();
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                return rcv->area;
            } else {
                const float *bbx = ScratchVisibilityStrategy::scratchRenderElements(rcv, globalSamplePoint, galerkinState);

                // Projected area is the number of non-background pixels over
                // the total number of pixels * area of the virtual screen
                globalProjectedArea = (double)ScratchVisibilityStrategy::scratchNonBackgroundPixels(galerkinState) *
                                      (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                      (double)(galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);
                return globalProjectedArea;
            }

        default:
            logFatal(-1, "receiverArea", "Invalid clustering strategy %d",
                     galerkinState->clusteringStrategy);
    }

    return 0.0; // This point is never reached and if it is it's your fault
}

/**
Gathers radiance over the interaction, from the interaction source to
the specified receiver element. areaFactor is a correction factor
to account for the fact that the receiver cluster area/4 was used in
form factor computations instead of the true receiver area. The source
radiance is explicit given
*/
void
ClusterTraversalStrategy::isotropicGatherRadiance(
    GalerkinElement *rcv,
    double areaFactor,
    const Interaction *link,
    const ColorRgb *sourceRadiance)
{
    ColorRgb *rcvRad = rcv->receivedRadiance;

    if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
        rcvRad[0].addScaled(rcvRad[0], (float) (areaFactor * link->K[0]), sourceRadiance[0]);
    } else {
        int a;
        int b;
        a = java::Math::min(link->numberOfBasisFunctionsOnReceiver, rcv->basisSize);
        b = java::Math::min(link->numberOfBasisFunctionsOnSource, link->sourceElement->basisSize);
        for ( int alpha = 0; alpha < a; alpha++ ) {
            for ( int beta = 0; beta < b; beta++ ) {
                rcvRad[alpha].addScaled(
                    rcvRad[alpha],
                    (float)(areaFactor * link->K[alpha * link->numberOfBasisFunctionsOnSource + beta]),
                    sourceRadiance[beta]);
            }
        }
    }
}

/**
Requires global variables globalPSourceRad (globalSourceRadiance pointer), globalTheLink (interaction
over which is being gathered) and globalSamplePoint (midpoint of source
element). rcv is a surface element belonging to the receiver cluster
in the interaction. This routines gathers radiance to this receiver
surface, taking into account the projected area of the receiver
towards the midpoint of the source, ignoring visibility in the receiver
cluster
*/
void
ClusterTraversalStrategy::orientedSurfaceGatherRadiance(
    GalerkinElement *rcv,
    const GalerkinState */*galerkinState*/,
    ColorRgb * /*accumulatedRadiance*/)
{
    // globalTheLink->rcv is a cluster, so it's total area divided by 4 (average projected area)
    // was used to compute link->K
    double areaFactor = ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(rcv) /
        (0.25 * globalTheLink->receiverElement->area);

    ClusterTraversalStrategy::isotropicGatherRadiance(rcv, areaFactor, globalTheLink, globalPSourceRad);
}

/**
Same as above, except that the number of pixels in the scratch frame buffer
times the area corresponding one such pixel is used as the visible area
of the element. Uses global variables globalPixelArea, globalTheLink, globalPSourceRad
*/
void
ClusterTraversalStrategy::zVisSurfaceGatherRadiance(
    GalerkinElement *rcv,
    const GalerkinState */*galerkinState*/,
    ColorRgb * /*accumulatedRadiance*/)
{
    if ( rcv->tmp <= 0 ) {
        // Element occupies no pixels in the scratch frame buffer
        return;
    }

    double areaFactor = globalPixelArea * (double) (rcv->tmp) / (0.25 * globalTheLink->receiverElement->area);
    ClusterTraversalStrategy::isotropicGatherRadiance(rcv, areaFactor, globalTheLink, globalPSourceRad);

    rcv->tmp = 0; // Set it to zero for future re-use
}

/**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
ClusterTraversalStrategy::gatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState) {
    const GalerkinElement *src = link->sourceElement;
    GalerkinElement *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        logFatal(-1, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        return;
    }

    globalPSourceRad = srcRad;
    globalTheLink = link;
    globalSamplePoint = src->midPoint();

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            ClusterTraversalStrategy::isotropicGatherRadiance(rcv, 1.0, link, srcRad);
            break;
        case GalerkinClusteringStrategy::ORIENTED:
            ClusterTraversalStrategy::traverseAllLeafElements(
                nullptr,
                rcv,
                ClusterTraversalStrategy::orientedSurfaceGatherRadiance,
                galerkinState,
                &globalSourceRadiance);
            break;
        case GalerkinClusteringStrategy::Z_VISIBILITY:
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                ClusterTraversalStrategy::traverseAllLeafElements(
                    nullptr,
                    rcv,
                    ClusterTraversalStrategy::orientedSurfaceGatherRadiance,
                    galerkinState,
                    &globalSourceRadiance);
            } else {
                const float *boundingBox = ScratchVisibilityStrategy::scratchRenderElements(rcv, globalSamplePoint, galerkinState);

                // Count how many pixels each element occupies in the scratch frame buffer
                ScratchVisibilityStrategy::scratchPixelsPerElement(galerkinState);

                // Area corresponding to one pixel on the virtual screen
                globalPixelArea = (boundingBox[MAX_X] - boundingBox[MIN_X]) * (boundingBox[MAX_Y] - boundingBox[MIN_Y]) /
                                  (double) (galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);

                // Gathers the radiance to each element that occupies at least one
                // pixel in the scratch frame buffer and sets elem->tmp back to zero
                // for those elements
                ClusterTraversalStrategy::traverseAllLeafElements(
                    nullptr,
                    rcv,
                    ClusterTraversalStrategy::zVisSurfaceGatherRadiance,
                    galerkinState,
                    &globalSourceRadiance);
            }
            break;
        default:
            logFatal(-1, "gatherRadiance", "Invalid clustering strategy %d",
                     galerkinState->clusteringStrategy);
    }
}

ColorRgb
ClusterTraversalStrategy::maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState) {
    ColorRgb radiance;
    radiance.clear();
    MaximumRadianceVisitor *leafVisitor = new MaximumRadianceVisitor();
    ClusterTraversalStrategy::traverseAllLeafElements(leafVisitor, cluster, nullptr, galerkinState, &radiance);
    delete leafVisitor;
    return radiance;
}
