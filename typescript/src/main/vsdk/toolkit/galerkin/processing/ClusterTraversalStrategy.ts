import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { GalerkinClusteringStrategy } from "../GalerkinClusteringStrategy";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { ClusterLeafVisitor } from "./visitors/ClusterLeafVisitor";
import { DepthVisibilityGathererVisitor } from "./visitors/DepthVisibilityGathererVisitor";
import { MaximumRadianceVisitor } from "./visitors/MaximumRadianceVisitor";
import { OrientedGathererVisitor } from "./visitors/OrientedGathererVisitor";
import { PowerAccumulatorVisitor } from "./visitors/PowerAccumulatorVisitor";
import { ProjectedAreaAccumulatorVisitor } from "./visitors/ProjectedAreaAccumulatorVisitor";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { ScratchVisibilityStrategy } from "./ScratchVisibilityStrategy";

export class ClusterTraversalStrategy {
  public static traverseAllLeafElements(
    leafVisitor: ClusterLeafVisitor,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    if (parentElement === null) {
      return;
    }
    if (!parentElement.isCluster()) {
      leafVisitor.visit(parentElement, galerkinState);
    }
    else {
      for (let i = 0;
        parentElement.irregularSubElements !== null && i < parentElement.irregularSubElements.length;
        i++) {
        if (parentElement.irregularSubElements[i] instanceof GalerkinElement) {
          const childCluster = parentElement.irregularSubElements[i] as GalerkinElement;
          ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, childCluster, galerkinState);
        }
      }
    }
  }

  public static clusterRadianceToSamplePoint(
    sourceElement: GalerkinElement,
    samplePoint: Vector3D,
    galerkinState: GalerkinState,
  ): ColorRgb {
    if (sourceElement === null || galerkinState === null) {
      return new ColorRgb();
    }
    switch (galerkinState.clusteringStrategy) {
      case GalerkinClusteringStrategy.ISOTROPIC:
        return (sourceElement.radiance as ColorRgb[])[0];
      case GalerkinClusteringStrategy.ORIENTED: {
        let sourceRadiance = new ColorRgb();
        sourceRadiance.clear();
        const leafVisitor = new PowerAccumulatorVisitor(sourceRadiance, samplePoint);
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, sourceElement, galerkinState);
        sourceRadiance = leafVisitor.getAccumulatedRadiance();
        sourceRadiance.scale(sourceElement.area > 0.0 ? (4.0 / sourceElement.area) : 0.0);
        return sourceRadiance;
      }
      case GalerkinClusteringStrategy.Z_VISIBILITY:
        return (sourceElement.radiance as ColorRgb[])[0];
      default:
        VsdkLogger.fatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %s\n", galerkinState.clusteringStrategy);
        return new ColorRgb();
    }
  }

  public static sourceClusterRadiance(link: Interaction, galerkinState: GalerkinState): ColorRgb {
    const sourceElement = link.sourceElement;
    const receiverElement = link.receiverElement;
    if (sourceElement === null || receiverElement === null || !sourceElement.isCluster() || sourceElement === receiverElement) {
      VsdkLogger.fatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }
    return ClusterTraversalStrategy.clusterRadianceToSamplePoint(sourceElement, receiverElement.midPoint(), galerkinState);
  }

  public static surfaceProjectedAreaToSamplePoint(receiverElement: GalerkinElement): number {
    if (receiverElement === null || receiverElement.patch === null) {
      return 0.0;
    }
    let rcvCos: number;
    const dir = new Vector3D();
    const samplePoint = new Vector3D();

    dir.subtraction(samplePoint, receiverElement.patch.midPoint);
    const distance = dir.norm();
    if (distance < Numeric.EPSILON) {
      rcvCos = 1.0;
    }
    else {
      rcvCos = dir.dotProduct(receiverElement.patch.normal) / distance;
    }
    if (rcvCos <= 0.0) {
      return 0.0;
    }
    return rcvCos * receiverElement.area;
  }

  public static receiverArea(link: Interaction, galerkinState: GalerkinState): number {
    const sourceElement = link.sourceElement;
    const receiverElement = link.receiverElement;
    if (receiverElement === null || sourceElement === null) {
      return 0.0;
    }
    if (!receiverElement.isCluster() || sourceElement === receiverElement) {
      return receiverElement.area;
    }

    switch (galerkinState.clusteringStrategy) {
      case GalerkinClusteringStrategy.ISOTROPIC:
        return receiverElement.area;
      case GalerkinClusteringStrategy.ORIENTED: {
        const leafVisitor = new ProjectedAreaAccumulatorVisitor();
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
        return leafVisitor.getTotalProjectedArea();
      }
      case GalerkinClusteringStrategy.Z_VISIBILITY:
        return receiverElement.area;
      default:
        VsdkLogger.fatal(-1, "receiverArea", "Invalid clustering strategy %s", galerkinState.clusteringStrategy);
        return receiverElement.area;
    }
  }

  public static isotropicGatherRadiance(
    rcv: GalerkinElement,
    areaFactor: number,
    link: Interaction,
    sourceRadiance: ColorRgb[],
  ): void {
    const rcvRad = rcv.receivedRadiance as ColorRgb[];

    if (link.numberOfBasisFunctionsOnReceiver === 1 && link.numberOfBasisFunctionsOnSource === 1) {
      rcvRad[0].addScaled(rcvRad[0], areaFactor * link.K[0], sourceRadiance[0]);
    }
    else {
      const a = globalThis.Math.min(link.numberOfBasisFunctionsOnReceiver, rcv.basisSize);
      const b = globalThis.Math.min(link.numberOfBasisFunctionsOnSource, link.sourceElement.basisSize);
      for (let alpha = 0; alpha < a; alpha++) {
        for (let beta = 0; beta < b; beta++) {
          rcvRad[alpha].addScaled(
            rcvRad[alpha],
            areaFactor * link.K[alpha * link.numberOfBasisFunctionsOnSource + beta],
            sourceRadiance[beta],
          );
        }
      }
    }
  }

  public static gatherRadiance(link: Interaction, srcRad: ColorRgb[], galerkinState: GalerkinState): void {
    const sourceElement = link.sourceElement;
    const receiverElement = link.receiverElement;
    if (!receiverElement.isCluster() || sourceElement === receiverElement) {
      VsdkLogger.fatal(-1, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    const samplePoint = sourceElement.midPoint();
    const leafVisitor = new OrientedGathererVisitor(link, srcRad);
    switch (galerkinState.clusteringStrategy) {
      case GalerkinClusteringStrategy.ISOTROPIC:
        ClusterTraversalStrategy.isotropicGatherRadiance(receiverElement, 1.0, link, srcRad);
        break;
      case GalerkinClusteringStrategy.ORIENTED:
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
        break;
      case GalerkinClusteringStrategy.Z_VISIBILITY: {
        const bb: BoundingBox = ScratchVisibilityStrategy.scratchRenderElements(receiverElement, samplePoint, galerkinState);
        ScratchVisibilityStrategy.scratchPixelsPerElement(galerkinState);
        const pixelArea = (bb.dx() * bb.dy()) / ((galerkinState.scratch as NonNullable<GalerkinState["scratch"]>).vp_width
          * (galerkinState.scratch as NonNullable<GalerkinState["scratch"]>).vp_height);
        const dvgv = new DepthVisibilityGathererVisitor(link, srcRad, pixelArea);
        ClusterTraversalStrategy.traverseAllLeafElements(dvgv, receiverElement, galerkinState);
        break;
      }
      default:
        VsdkLogger.fatal(-1, "gatherRadiance", "Invalid clustering strategy %s", galerkinState.clusteringStrategy);
    }
  }

  public static maxRadiance(cluster: GalerkinElement, galerkinState: GalerkinState): ColorRgb {
    const leafVisitor = new MaximumRadianceVisitor();
    ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);
    return leafVisitor.getAccumulatedRadiance();
  }
}
