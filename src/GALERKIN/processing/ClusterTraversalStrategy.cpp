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
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/processing/visitors/MaximumRadianceVisitor.h"
#include "GALERKIN/processing/visitors/PowerAccumulatorVisitor.h"
#include "GALERKIN/processing/visitors/ProjectedAreaAccumulatorVisitor.h"
#include "GALERKIN/processing/visitors/OrientedGathererVisitor.h"
#include "GALERKIN/processing/visitors/DepthVisibilityGathererVisitor.h"
#include "GALERKIN/processing/ScratchVisibilityStrategy.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"

/**
Executes func for every surface element in the cluster
*/
void
ClusterTraversalStrategy::traverseAllLeafElements(
    ClusterLeafVisitor *leafVisitor,
    GalerkinElement *parentElement,
    GalerkinState *galerkinState)
{
    if ( !parentElement->isCluster() && leafVisitor != nullptr ) {
        leafVisitor->visit(parentElement, galerkinState);
    } else {
        for ( int i = 0;
            parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size();
            i++ ) {
            GalerkinElement *childCluster = (GalerkinElement *)parentElement->irregularSubElements->get(i);
            ClusterTraversalStrategy::traverseAllLeafElements(
                leafVisitor, childCluster, galerkinState);
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
                galerkinState);
            sourceRadiance = leafVisitor->getAccumulatedRadiance();
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
                const float *boundingBox =
                    ScratchVisibilityStrategy::scratchRenderElements(sourceElement, samplePoint, galerkinState);

                // Compute average radiance on the virtual screen
                ColorRgb sourceRadiance = ScratchVisibilityStrategy::scratchRadiance(galerkinState);

                // Area factor = area of virtual screen / source cluster area used for
                // form factor computation
                areaFactor = ((boundingBox[MAX_X] - boundingBox[MIN_X]) * (boundingBox[MAX_Y] - boundingBox[MIN_Y])) /
                             (0.25 * sourceElement->area);
                sourceRadiance.scale((float) areaFactor);
                return sourceRadiance;
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
    GalerkinElement *sourceElement = link->sourceElement;
    const GalerkinElement *receiverElement = link->receiverElement;

    if ( !sourceElement->isCluster() || sourceElement == receiverElement ) {
        logFatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    // Take a sample point on the receiver
    return ClusterTraversalStrategy::clusterRadianceToSamplePoint(
        sourceElement, receiverElement->midPoint(), galerkinState);
}

/**
Computes projected area of receiver surface element towards the sample point
(global variable). Ignores intra cluster visibility
*/
double
ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(const GalerkinElement *receiverElement) {
    double rcvCos;
    double distance;
    Vector3D dir;
    Vector3D samplePoint;

    dir.subtraction(samplePoint, receiverElement->patch->midPoint);
    distance = dir.norm();
    if ( distance < Numeric::EPSILON ) {
        rcvCos = 1.0;
    } else {
        rcvCos = dir.dotProduct(receiverElement->patch->normal) / distance;
    }
    if ( rcvCos <= 0.0 ) {
        // Sample point is behind the receiverElement
        return 0.0;
    }

    return rcvCos * receiverElement->area;
}

/**
Computes projected area of receiver cluster as seen from the midpoint of the source,
ignoring intra-receiver visibility
*/
double
ClusterTraversalStrategy::receiverArea(Interaction *link, GalerkinState *galerkinState) {
    const GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;
    Vector3D samplePoint;
    double projectedArea;

    if ( !receiverElement->isCluster() || sourceElement == receiverElement ) {
        return receiverElement->area;
    }

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            return receiverElement->area;

        case GalerkinClusteringStrategy::ORIENTED: {
            samplePoint = sourceElement->midPoint();
            ProjectedAreaAccumulatorVisitor *leafVisitor = new ProjectedAreaAccumulatorVisitor();
            ClusterTraversalStrategy::traverseAllLeafElements(
                leafVisitor,
                receiverElement,
                galerkinState);
            projectedArea = leafVisitor->getTotalProjectedArea();
            delete leafVisitor;
            return projectedArea;
        }

        case GalerkinClusteringStrategy::Z_VISIBILITY:
            samplePoint = sourceElement->midPoint();
            if ( !receiverElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
                return receiverElement->area;
            } else {
                const float *boundingBox =
                    ScratchVisibilityStrategy::scratchRenderElements(receiverElement, samplePoint, galerkinState);

                // Projected area is the number of non-background pixels over
                // the total number of pixels * area of the virtual screen
                projectedArea =
                    (double)ScratchVisibilityStrategy::scratchNonBackgroundPixels(galerkinState) *
                    (boundingBox[MAX_X] - boundingBox[MIN_X]) * (boundingBox[MAX_Y] - boundingBox[MIN_Y]) /
                    (double)(galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);
                return projectedArea;
            }

        default:
            logFatal(-1, "receiverArea", "Invalid clustering strategy %d", galerkinState->clusteringStrategy);
    }
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
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
ClusterTraversalStrategy::gatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState) {
    const GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    if ( !receiverElement->isCluster() || sourceElement == receiverElement ) {
        logFatal(-1, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    Vector3D samplePoint = sourceElement->midPoint();
    OrientedGathererVisitor *leafVisitor = new OrientedGathererVisitor(link, srcRad);

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            ClusterTraversalStrategy::isotropicGatherRadiance(receiverElement, 1.0, link, srcRad);
            break;
        case GalerkinClusteringStrategy::ORIENTED:
            ClusterTraversalStrategy::traverseAllLeafElements(
                    leafVisitor,
                    receiverElement,
                    galerkinState);
            break;
        case GalerkinClusteringStrategy::Z_VISIBILITY:
            if ( !receiverElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
                ClusterTraversalStrategy::traverseAllLeafElements(
                        leafVisitor,
                        receiverElement,
                        galerkinState);
            } else {
                const float *boundingBox =
                    ScratchVisibilityStrategy::scratchRenderElements(receiverElement, samplePoint, galerkinState);

                // Count how many pixels each element occupies in the scratch frame buffer
                ScratchVisibilityStrategy::scratchPixelsPerElement(galerkinState);

                // Area corresponding to one pixel in the scratch frame buffer (virtual screen)
                double pixelArea =
                    (boundingBox[MAX_X] - boundingBox[MIN_X]) * (boundingBox[MAX_Y] - boundingBox[MIN_Y]) /
                    (double)(galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);

                // Gathers the radiance to each element that occupies at least one
                // pixel in the scratch frame buffer and sets elem->tmp back to zero
                // for those elements
                DepthVisibilityGathererVisitor *depthVisibilityLeafVisitor =
                    new DepthVisibilityGathererVisitor(link, srcRad, pixelArea);
                ClusterTraversalStrategy::traverseAllLeafElements(
                    depthVisibilityLeafVisitor,
                    receiverElement,
                    galerkinState);
                delete depthVisibilityLeafVisitor;
            }
            break;
        default:
            logFatal(-1, "gatherRadiance", "Invalid clustering strategy %d",
                     galerkinState->clusteringStrategy);
    }
    delete leafVisitor;
}

ColorRgb
ClusterTraversalStrategy::maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState) {
    ColorRgb radiance;
    MaximumRadianceVisitor *leafVisitor = new MaximumRadianceVisitor();
    ClusterTraversalStrategy::traverseAllLeafElements(leafVisitor, cluster, galerkinState);
    radiance = leafVisitor->getAccumulatedRadiance();
    delete leafVisitor;
    return radiance;
}
