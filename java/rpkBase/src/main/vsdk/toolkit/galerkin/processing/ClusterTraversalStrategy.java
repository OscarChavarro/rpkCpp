/**
Cluster-specific operations

Reference:

[SILL1995a] Sillion & Drettakis, "Featured-based Control of Visibility Error: A Multi-resolution
Clustering Algorithm for Global Illumination", SIGGRAPH '95 p145
*/

package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinClusteringStrategy;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.processing.visitors.ClusterLeafVisitor;
import vsdk.toolkit.galerkin.processing.visitors.DepthVisibilityGathererVisitor;
import vsdk.toolkit.galerkin.processing.visitors.MaximumRadianceVisitor;
import vsdk.toolkit.galerkin.processing.visitors.OrientedGathererVisitor;
import vsdk.toolkit.galerkin.processing.visitors.PowerAccumulatorVisitor;
import vsdk.toolkit.galerkin.processing.visitors.ProjectedAreaAccumulatorVisitor;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

public class ClusterTraversalStrategy {
    /**
Executes func for every surface element in the cluster
*/
    public static void traverseAllLeafElements(
        ClusterLeafVisitor leafVisitor,
        GalerkinElement parentElement,
        GalerkinState galerkinState)
    {
        if ( parentElement == null ) {
            return;
        }
        if ( !parentElement.isCluster() && leafVisitor != null ) {
            leafVisitor.visit(parentElement, galerkinState);
        }
        else {
            for ( int i = 0;
                parentElement.irregularSubElements != null && i < parentElement.irregularSubElements.size();
                i++ ) {
                if ( parentElement.irregularSubElements.get(i) instanceof GalerkinElement ) {
                    GalerkinElement childCluster = (GalerkinElement)parentElement.irregularSubElements.get(i);
                    traverseAllLeafElements(leafVisitor, childCluster, galerkinState);
                }
            }
        }
    }

    /**
Returns the radiance or un-shot radiance (depending on the
iteration method) emitted by the source element, a cluster,
towards the samplePoint point
*/
    public static ColorRgb clusterRadianceToSamplePoint(
        GalerkinElement sourceElement,
        Vector3Dd samplePoint,
        GalerkinState galerkinState)
    {
        if ( sourceElement == null || galerkinState == null ) {
            return new ColorRgb();
        }
        switch ( galerkinState.clusteringStrategy ) {
            case ISOTROPIC:
                return sourceElement.radiance[0];
            case ORIENTED: {
                ColorRgb sourceRadiance = new ColorRgb();
                sourceRadiance = new ColorRgb(0.0, 0.0, 0.0);
                PowerAccumulatorVisitor leafVisitor = new PowerAccumulatorVisitor(sourceRadiance, samplePoint);
                traverseAllLeafElements(leafVisitor, sourceElement, galerkinState);
                sourceRadiance = leafVisitor.getAccumulatedRadiance();
                sourceRadiance = sourceRadiance.multiply(sourceElement.area > 0.0 ? (4.0 / sourceElement.area) : 0.0);
                return sourceRadiance;
            }
            case Z_VISIBILITY:
                if ( !sourceElement.isCluster() || !sourceElement.geometry.boundingBox.outOfBounds(samplePoint) ) {
                    return sourceElement.radiance[0];
                } else {
                    double areaFactor;

                    // Render pointers to the elements in the source cluster into the scratch frame
                    // buffer, seen from the samplePoint point
                    final RpkAxisAlignedBoundingBox boundingBox =
                        ScratchVisibilityStrategy.scratchRenderElements(sourceElement, samplePoint, galerkinState);

                    // Compute average radiance on the virtual screen
                    ColorRgb sourceRadiance = ScratchVisibilityStrategy.scratchRadiance(galerkinState);

                    // Area factor = area of virtual screen / source cluster area used for
                    // form factor computation
                    areaFactor = (boundingBox.dx() * boundingBox.dy()) / (0.25 * sourceElement.area);
                    sourceRadiance = sourceRadiance.multiply((double)areaFactor);
                    return sourceRadiance;
                }
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "clusterRadianceToSamplePoint", String.format("Invalid clustering strategy %s\n", galerkinState.clusteringStrategy));
                return new ColorRgb();
        }
    }

    /**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
    public static ColorRgb sourceClusterRadiance(Interaction link, GalerkinState galerkinState) {
        GalerkinElement sourceElement = link.sourceElement;
        GalerkinElement receiverElement = link.receiverElement;
        if ( sourceElement == null || receiverElement == null || !sourceElement.isCluster() || sourceElement == receiverElement ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
        }
        return clusterRadianceToSamplePoint(sourceElement, receiverElement.midPoint(), galerkinState);
    }

    /**
Computes projected area of receiver surface element towards the sample point.
Ignores intra cluster visibility
*/
    public static double surfaceProjectedAreaToSamplePoint(GalerkinElement receiverElement) {
        if ( receiverElement == null || receiverElement.patch == null ) {
            return 0.0;
        }
        double rcvCos;
        Vector3Dd dir = new Vector3Dd();
        Vector3Dd samplePoint = new Vector3Dd();

        dir = samplePoint.subtract(receiverElement.patch.midPoint());
        double distance = dir.length();
        if ( distance < VSDK.EPSILON ) {
            rcvCos = 1.0;
        }
        else {
            rcvCos = dir.dotProduct(receiverElement.patch.getNormal()) / distance;
        }
        if ( rcvCos <= 0.0 ) {
            return 0.0;
        }
        return rcvCos * receiverElement.area;
    }

    /**
Computes projected area of receiver cluster as seen from the midpoint of the source,
ignoring intra-receiver visibility
*/
    public static double receiverArea(Interaction link, GalerkinState galerkinState) {
        GalerkinElement sourceElement = link.sourceElement;
        GalerkinElement receiverElement = link.receiverElement;
        if ( receiverElement == null || sourceElement == null ) {
            return 0.0;
        }
        if ( !receiverElement.isCluster() || sourceElement == receiverElement ) {
            return receiverElement.area;
        }

        switch ( galerkinState.clusteringStrategy ) {
            case ISOTROPIC:
                return receiverElement.area;
            case ORIENTED: {
                ProjectedAreaAccumulatorVisitor leafVisitor = new ProjectedAreaAccumulatorVisitor();
                traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
                return leafVisitor.getTotalProjectedArea();
            }
            case Z_VISIBILITY:
                return receiverElement.area;
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "receiverArea", String.format("Invalid clustering strategy %s", galerkinState.clusteringStrategy));
                return receiverElement.area;
        }
    }

    /**
Gathers radiance over the interaction, from the interaction source to
the specified receiver element. areaFactor is a correction factor
to account for the fact that the receiver cluster area/4 was used in
form factor computations instead of the true receiver area. The source
radiance is explicit given
*/
    public static void isotropicGatherRadiance(
        GalerkinElement rcv,
        double areaFactor,
        Interaction link,
        ColorRgb[] sourceRadiance)
    {
        ColorRgb[] rcvRad = rcv.receivedRadiance;

        if ( link.numberOfBasisFunctionsOnReceiver == 1 && link.numberOfBasisFunctionsOnSource == 1 ) {
            rcvRad[0] = rcvRad[0].addScaled((double)(areaFactor * link.K[0]), sourceRadiance[0]);
        }
        else {
            int a = Math.min(link.numberOfBasisFunctionsOnReceiver, rcv.basisSize);
            int b = Math.min(link.numberOfBasisFunctionsOnSource, link.sourceElement.basisSize);
            for ( int alpha = 0; alpha < a; alpha++ ) {
                for ( int beta = 0; beta < b; beta++ ) {
                    rcvRad[alpha] = rcvRad[alpha].addScaled((double)(areaFactor * link.K[alpha * link.numberOfBasisFunctionsOnSource + beta]), sourceRadiance[beta]);
                }
            }
        }
    }

    /**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
    public static void gatherRadiance(Interaction link, ColorRgb[] srcRad, GalerkinState galerkinState) {
        GalerkinElement sourceElement = link.sourceElement;
        GalerkinElement receiverElement = link.receiverElement;
        if ( !receiverElement.isCluster() || sourceElement == receiverElement ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        }

        switch ( galerkinState.clusteringStrategy ) {
            case ISOTROPIC:
                isotropicGatherRadiance(receiverElement, 1.0, link, srcRad);
                break;
            case ORIENTED: {
                OrientedGathererVisitor leafVisitor = new OrientedGathererVisitor(link, srcRad);
                traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
                break;
            }
            case Z_VISIBILITY:
                Vector3Dd samplePoint = sourceElement.midPoint();
                RpkAxisAlignedBoundingBox bb = ScratchVisibilityStrategy.scratchRenderElements(receiverElement, samplePoint, galerkinState);
                ScratchVisibilityStrategy.scratchPixelsPerElement(galerkinState);
                double pixelArea = (bb.dx() * bb.dy()) / (double)(galerkinState.scratch.getViewportWidth() * galerkinState.scratch.getViewportHeight());
                DepthVisibilityGathererVisitor dvgv = new DepthVisibilityGathererVisitor(link, srcRad, pixelArea);
                traverseAllLeafElements(dvgv, receiverElement, galerkinState);
                break;
            default:
                Logger.reportMessage(null, Logger.FATAL_ERROR, "gatherRadiance", String.format("Invalid clustering strategy %s", galerkinState.clusteringStrategy));
        }
    }

    public static ColorRgb maxRadiance(GalerkinElement cluster, GalerkinState galerkinState) {
        MaximumRadianceVisitor leafVisitor = new MaximumRadianceVisitor();
        traverseAllLeafElements(leafVisitor, cluster, galerkinState);
        return leafVisitor.getAccumulatedRadiance();
    }
}
