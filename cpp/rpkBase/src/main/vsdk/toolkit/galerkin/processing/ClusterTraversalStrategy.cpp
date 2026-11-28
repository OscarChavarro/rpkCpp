/**
Cluster-specific operations

Reference:

[SILL1995a] Sillion & Drettakis, "Featured-based Control of Visibility Error: A Multi-resolution
Clustering Algorithm for Global Illumination", SIGGRAPH '95 p145
*/

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityStrategy.h"
#include "vsdk/toolkit/galerkin/processing/ElementSamples.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityWorkspace.h"
#include "vsdk/toolkit/galerkin/processing/visitors/MaximumRadianceVisitor.h"
#include "vsdk/toolkit/galerkin/processing/visitors/OrientedGathererVisitor.h"
#include "vsdk/toolkit/galerkin/processing/visitors/PowerAccumulatorVisitor.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ProjectedAreaAccumulatorVisitor.h"

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
            GalerkinElement *childCluster = static_cast<GalerkinElement *>(parentElement->irregularSubElements->get(i));
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
    Vector3Dd samplePoint,
    ScratchVisibilityWorkspace *scratch,
    GalerkinState *galerkinState)
{
    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            return sourceElement->radiance[0];

        case GalerkinClusteringStrategy::ORIENTED: {
            // Accumulate the power emitted by the patches in the source cluster
            // towards the samplePoint point
            PowerAccumulatorVisitor leafVisitor(ColorRgb(0.0, 0.0, 0.0), samplePoint);
            ClusterTraversalStrategy::traverseAllLeafElements(
                &leafVisitor,
                sourceElement,
                galerkinState);
            const ColorRgb sourceRadiance = leafVisitor.getAccumulatedRadiance();

            // Divide by the source area used for computing the form factor:
            // sourceElement->area / 4.0 (average projected area)
            return sourceRadiance.multiply(4.0 / sourceElement->area);
        }

        case GalerkinClusteringStrategy::Z_VISIBILITY:
            if ( !sourceElement->isCluster() || !sourceElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
                return sourceElement->radiance[0];
            } else {
                double areaFactor;

                // Render pointers to the elements in the source cluster into the scratch frame
                // buffer, seen from the samplePoint point
                const RpkAxisAlignedBoundingBox boundingBox =
                    ScratchVisibilityStrategy::scratchRenderElements(sourceElement, samplePoint, scratch, galerkinState);

                // Compute average radiance on the virtual screen
                const ColorRgb sourceRadiance = ScratchVisibilityStrategy::scratchRadiance(scratch, galerkinState);

                // Area factor = area of virtual screen / source cluster area used for
                // form factor computation
                areaFactor = (boundingBox.dx() * boundingBox.dy()) / (0.25 * sourceElement->area);
                return sourceRadiance.multiply(static_cast<double>(areaFactor));
            }

        default:
            Logger::reportMessage("ClusterTraversalStrategy", Logger::FATAL_ERROR, "clusterRadianceToSamplePoint", java::String::format("Invalid clustering strategy %d\n", galerkinState->clusteringStrategy));
    }
    return ColorRgb(0.0, 0.0, 0.0); // Not reached: a FATAL_ERROR report throws
}

/**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
ColorRgb
ClusterTraversalStrategy::sourceClusterRadiance(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState) {
    GalerkinElement *sourceElement = link->sourceElement;
    const GalerkinElement *receiverElement = link->receiverElement;

    if ( !sourceElement->isCluster() || sourceElement == receiverElement ) {
        Logger::reportMessage("ClusterTraversalStrategy", Logger::FATAL_ERROR, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    // Take a sample point on the receiver
    return ClusterTraversalStrategy::clusterRadianceToSamplePoint(
        sourceElement, receiverElement->midPoint(), scratch, galerkinState);
}

/**
Computes projected area of receiver surface element towards the sample point.
Ignores intra cluster visibility
*/
double
ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(const GalerkinElement *receiverElement) {
    double rcvCos;
    double distance;
    Vector3Dd dir;
    Vector3Dd samplePoint;

    dir = samplePoint.subtract(receiverElement->patch->midPoint());
    distance = dir.length();
    if ( distance < VSDK::EPSILON ) {
        rcvCos = 1.0;
    } else {
        rcvCos = dir.dotProduct(receiverElement->patch->getNormal()) / distance;
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
ClusterTraversalStrategy::receiverArea(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState) {
    const GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;
    Vector3Dd samplePoint;
    double projectedArea;

    if ( !receiverElement->isCluster() || sourceElement == receiverElement ) {
        return receiverElement->area;
    }

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            return receiverElement->area;

        case GalerkinClusteringStrategy::ORIENTED: {
            samplePoint = sourceElement->midPoint();
            ProjectedAreaAccumulatorVisitor leafVisitor;
            ClusterTraversalStrategy::traverseAllLeafElements(
                &leafVisitor,
                receiverElement,
                galerkinState);
            projectedArea = leafVisitor.getTotalProjectedArea();
            return projectedArea;
        }

        case GalerkinClusteringStrategy::Z_VISIBILITY:
            samplePoint = sourceElement->midPoint();
            if ( !receiverElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
                return receiverElement->area;
            } else {
                const RpkAxisAlignedBoundingBox boundingBox =
                    ScratchVisibilityStrategy::scratchRenderElements(receiverElement, samplePoint, scratch, galerkinState);

                // Projected area is the number of non-background pixels over
                // the total number of pixels * area of the virtual screen
                projectedArea =
                    static_cast<double>(ScratchVisibilityStrategy::scratchNonBackgroundPixels(scratch)) *
                    boundingBox.dx() * boundingBox.dy() /
                    static_cast<double>(scratch->getBuffer().getViewportWidth() * scratch->getBuffer().getViewportHeight());
                return projectedArea;
            }

        default:
            Logger::reportMessage("ClusterTraversalStrategy", Logger::FATAL_ERROR, "receiverArea", java::String::format("Invalid clustering strategy %d", galerkinState->clusteringStrategy));
    }
    return 0.0; // Not reached: a FATAL_ERROR report throws
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
        rcvRad[0] = rcvRad[0].addScaled(static_cast<double>(areaFactor * link->K[0]), sourceRadiance[0]);
    } else {
        int a = java::Math::min(link->numberOfBasisFunctionsOnReceiver, rcv->basisSize);
        int b = java::Math::min(link->numberOfBasisFunctionsOnSource, link->sourceElement->basisSize);
        for ( int alpha = 0; alpha < a; alpha++ ) {
            for ( int beta = 0; beta < b; beta++ ) {
                rcvRad[alpha] = rcvRad[alpha].addScaled(static_cast<double>(areaFactor * link->K[alpha * link->numberOfBasisFunctionsOnSource + beta]), sourceRadiance[beta]);
            }
        }
    }
}

/**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
ClusterTraversalStrategy::gatherRadiance(Interaction *link, ColorRgb *srcRad, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState) {
    const GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    if ( !receiverElement->isCluster() || sourceElement == receiverElement ) {
        Logger::reportMessage("ClusterTraversalStrategy", Logger::FATAL_ERROR, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    switch ( galerkinState->clusteringStrategy ) {
        case GalerkinClusteringStrategy::ISOTROPIC:
            ClusterTraversalStrategy::isotropicGatherRadiance(receiverElement, 1.0, link, srcRad);
            break;
        case GalerkinClusteringStrategy::ORIENTED: {
            OrientedGathererVisitor leafVisitor(link, srcRad);
            ClusterTraversalStrategy::traverseAllLeafElements(
                    &leafVisitor,
                    receiverElement,
                    galerkinState);
            break;
        }
        case GalerkinClusteringStrategy::Z_VISIBILITY: {
            double pixelArea;
            if ( ClusterTraversalStrategy::receiverSamples(link, scratch, galerkinState, &pixelArea) ) {
                const java::ArrayList<ElementSamples> &visible = scratch->getVisibleElements();
                ClusterTraversalStrategy::gatherRadianceFromSamples(
                    link, srcRad, pixelArea, visible.data(), visible.size());
            } else {
                OrientedGathererVisitor leafVisitor(link, srcRad);
                ClusterTraversalStrategy::traverseAllLeafElements(
                        &leafVisitor,
                        receiverElement,
                        galerkinState);
            }
            break;
        }
        default:
            Logger::reportMessage("ClusterTraversalStrategy", Logger::FATAL_ERROR, "gatherRadiance", java::String::format("Invalid clustering strategy %d", galerkinState->clusteringStrategy));
    }
}

/**
Z-visibility clustering: when the receiver cluster of the link is seen from
outside (the midpoint of the source lies outside its bounding box), renders
its elements seen from that point and counts, in the workspace, how many
samples each one covers. Returns false, rendering nothing, otherwise.
@param pixelArea the area of the virtual screen a sample covers
*/
bool
ClusterTraversalStrategy::receiverSamples(
    Interaction *link,
    ScratchVisibilityWorkspace *scratch,
    GalerkinState *galerkinState,
    double *pixelArea)
{
    GalerkinElement *receiverElement = link->receiverElement;
    Vector3Dd samplePoint = link->sourceElement->midPoint();
    if ( !receiverElement->geometry->boundingBox.outOfBounds(&samplePoint) ) {
        return false;
    }

    const RpkAxisAlignedBoundingBox boundingBox =
        ScratchVisibilityStrategy::scratchRenderElements(receiverElement, samplePoint, scratch, galerkinState);

    // Count how many pixels each element occupies in the scratch frame buffer
    ScratchVisibilityStrategy::scratchPixelsPerElement(scratch, galerkinState);

    // Area corresponding to one pixel in the scratch frame buffer (virtual screen)
    *pixelArea = ScratchVisibilityStrategy::scratchPixelArea(boundingBox, scratch);
    return true;
}

/**
Gathers the radiance to each element of the receiver cluster that occupies at
least one sample of its rendering, as counted by receiverSamples. The visible
fraction of an element is estimated from its samples times pixelArea,
normalized by one quarter of the receiver area. Each element receives one
sum, so the order of the elements does not change the result: only the
visible ones are visited, not every leaf of the cluster
*/
void
ClusterTraversalStrategy::gatherRadianceFromSamples(
    Interaction *link,
    ColorRgb *srcRad,
    double pixelArea,
    const ElementSamples *visibleElements,
    long int numberOfVisibleElements)
{
    for ( long int i = 0; i < numberOfVisibleElements; i++ ) {
        const ElementSamples &visible = visibleElements[i];
        double areaFactor = pixelArea * static_cast<double>(visible.getSamples()) / (0.25 * link->receiverElement->area);
        ClusterTraversalStrategy::isotropicGatherRadiance(visible.getElement(), areaFactor, link, srcRad);
    }
}

ColorRgb
ClusterTraversalStrategy::maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState) {
    MaximumRadianceVisitor leafVisitor;
    ClusterTraversalStrategy::traverseAllLeafElements(&leafVisitor, cluster, galerkinState);
    return leafVisitor.getAccumulatedRadiance();
}
