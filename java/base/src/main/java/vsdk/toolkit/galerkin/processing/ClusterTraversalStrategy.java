/**
Cluster-specific operations

Reference:

[SILL1995a] Sillion & Drettakis, "Featured-based Control of Visibility Error: A Multi-resolution
Clustering Algorithm for Global Illumination", SIGGRAPH '95 p145
*/

package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
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
import vsdk.toolkit.skin.BoundingBox;

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
        Vector3D samplePoint,
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
                sourceRadiance.clear();
                PowerAccumulatorVisitor leafVisitor = new PowerAccumulatorVisitor(sourceRadiance, samplePoint);
                traverseAllLeafElements(leafVisitor, sourceElement, galerkinState);
                sourceRadiance = leafVisitor.getAccumulatedRadiance();
                sourceRadiance.scale(sourceElement.area > 0.0f ? (4.0f / sourceElement.area) : 0.0f);
                return sourceRadiance;
            }
            case Z_VISIBILITY:
                return sourceElement.radiance[0];
            default:
                Logger.fatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %s\n", galerkinState.clusteringStrategy);
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
            Logger.fatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
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
        Vector3D dir = new Vector3D();
        Vector3D samplePoint = new Vector3D();

        dir.subtraction(samplePoint, receiverElement.patch.midPoint);
        double distance = dir.norm();
        if ( distance < Numeric.EPSILON ) {
            rcvCos = 1.0;
        }
        else {
            rcvCos = dir.dotProduct(receiverElement.patch.normal) / distance;
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
                Logger.fatal(-1, "receiverArea", "Invalid clustering strategy %s", galerkinState.clusteringStrategy);
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
            rcvRad[0].addScaled(rcvRad[0], (float)(areaFactor * link.K[0]), sourceRadiance[0]);
        }
        else {
            int a = Math.min(link.numberOfBasisFunctionsOnReceiver, rcv.basisSize);
            int b = Math.min(link.numberOfBasisFunctionsOnSource, link.sourceElement.basisSize);
            for ( int alpha = 0; alpha < a; alpha++ ) {
                for ( int beta = 0; beta < b; beta++ ) {
                    rcvRad[alpha].addScaled(
                        rcvRad[alpha],
                        (float)(areaFactor * link.K[alpha * link.numberOfBasisFunctionsOnSource + beta]),
                        sourceRadiance[beta]);
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
            Logger.fatal(-1, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
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
                Vector3D samplePoint = sourceElement.midPoint();
                BoundingBox bb = ScratchVisibilityStrategy.scratchRenderElements(receiverElement, samplePoint, galerkinState);
                ScratchVisibilityStrategy.scratchPixelsPerElement(galerkinState);
                double pixelArea = (bb.dx() * bb.dy()) / (double)(galerkinState.scratch.vp_width * galerkinState.scratch.vp_height);
                DepthVisibilityGathererVisitor dvgv = new DepthVisibilityGathererVisitor(link, srcRad, pixelArea);
                traverseAllLeafElements(dvgv, receiverElement, galerkinState);
                break;
            default:
                Logger.fatal(-1, "gatherRadiance", "Invalid clustering strategy %s", galerkinState.clusteringStrategy);
        }
    }

    public static ColorRgb maxRadiance(GalerkinElement cluster, GalerkinState galerkinState) {
        MaximumRadianceVisitor leafVisitor = new MaximumRadianceVisitor();
        traverseAllLeafElements(leafVisitor, cluster, galerkinState);
        return leafVisitor.getAccumulatedRadiance();
    }
}
