import { String as JavaString } from "vitral/dist/java/lang/String";
import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { GalerkinClusteringStrategy } from "../GalerkinClusteringStrategy";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { ClusterLeafVisitor } from "./visitors/ClusterLeafVisitor";
import { MaximumRadianceVisitor } from "./visitors/MaximumRadianceVisitor";
import { OrientedGathererVisitor } from "./visitors/OrientedGathererVisitor";
import { PowerAccumulatorVisitor } from "./visitors/PowerAccumulatorVisitor";
import { ProjectedAreaAccumulatorVisitor } from "./visitors/ProjectedAreaAccumulatorVisitor";
import { ElementSamples } from "./ElementSamples";
import { ScratchVisibilityStrategy } from "./ScratchVisibilityStrategy";
import type { ScratchVisibilityWorkspace } from "./ScratchVisibilityWorkspace";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

/**
The scratch workspace, used by the Z-visibility clustering strategy only (it
may be null for the others), is the one of the calling thread
*/
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
    samplePoint: Vector3Dd,
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): ColorRgbMutable {
    if (sourceElement === null || galerkinState === null) {
      return new ColorRgbMutable();
    }
    switch (galerkinState.clusteringStrategy) {
      case GalerkinClusteringStrategy.ISOTROPIC:
        return (sourceElement.radiance as ColorRgbMutable[])[0]!;
      case GalerkinClusteringStrategy.ORIENTED: {
        let sourceRadiance = new ColorRgbMutable();
        sourceRadiance.clear();
        const leafVisitor = new PowerAccumulatorVisitor(sourceRadiance, samplePoint);
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, sourceElement, galerkinState);
        sourceRadiance = leafVisitor.getAccumulatedRadiance();
        sourceRadiance.scale(sourceElement.area > 0.0 ? (4.0 / sourceElement.area) : 0.0);
        return sourceRadiance;
      }
      case GalerkinClusteringStrategy.Z_VISIBILITY:
        if (!sourceElement.isCluster() || !sourceElement.geometry!.boundingBox.outOfBounds(samplePoint)) {
          return (sourceElement.radiance as ColorRgbMutable[])[0]!;
        }
        else {
          // Render pointers to the elements in the source cluster into the scratch frame
          // buffer, seen from the samplePoint point
          const boundingBox = ScratchVisibilityStrategy.scratchRenderElements(
            sourceElement, samplePoint, scratch!, galerkinState);

          // Compute average radiance on the virtual screen
          const sourceRadiance = ScratchVisibilityStrategy.scratchRadiance(scratch!, galerkinState);

          // Area factor = area of virtual screen / source cluster area used for
          // form factor computation
          const areaFactor = (boundingBox.dx() * boundingBox.dy()) / (0.25 * sourceElement.area);
          sourceRadiance.scale(areaFactor);
          return sourceRadiance;
        }
      default:
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "clusterRadianceToSamplePoint", JavaString.vformat("Invalid clustering strategy %s\n", [galerkinState.clusteringStrategy]));
        return new ColorRgbMutable();
    }
  }

  public static sourceClusterRadiance(
    link: Interaction,
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): ColorRgbMutable {
    const sourceElement = link.sourceElement;
    const receiverElement = link.receiverElement;
    if (sourceElement === null || receiverElement === null || !sourceElement.isCluster() || sourceElement === receiverElement) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }
    return ClusterTraversalStrategy.clusterRadianceToSamplePoint(sourceElement, receiverElement.midPoint(), scratch, galerkinState);
  }

  public static surfaceProjectedAreaToSamplePoint(receiverElement: GalerkinElement): number {
    if (receiverElement === null || receiverElement.patch === null) {
      return 0.0;
    }
    let rcvCos: number;
    let dir = new Vector3Dd();
    const samplePoint = new Vector3Dd();

    dir = samplePoint.subtract(receiverElement.patch.midPoint());
    const distance = dir.length();
    if (distance < VSDK.EPSILON) {
      rcvCos = 1.0;
    }
    else {
      rcvCos = dir.dotProduct(receiverElement.patch.getNormal()) / distance;
    }
    if (rcvCos <= 0.0) {
      return 0.0;
    }
    return rcvCos * receiverElement.area;
  }

  public static receiverArea(
    link: Interaction,
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): number {
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
      case GalerkinClusteringStrategy.Z_VISIBILITY: {
        const samplePoint = sourceElement.midPoint();
        if (!receiverElement.geometry!.boundingBox.outOfBounds(samplePoint)) {
          return receiverElement.area;
        }
        else {
          const boundingBox = ScratchVisibilityStrategy.scratchRenderElements(
            receiverElement, samplePoint, scratch!, galerkinState);

          // Projected area is the number of non-background pixels over
          // the total number of pixels * area of the virtual screen
          const projectedArea =
            ScratchVisibilityStrategy.scratchNonBackgroundPixels(scratch!)
            * boundingBox.dx() * boundingBox.dy()
            / (scratch!.getBuffer().getViewportWidth() * scratch!.getBuffer().getViewportHeight());
          return projectedArea;
        }
      }
      default:
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "receiverArea", JavaString.vformat("Invalid clustering strategy %s", [galerkinState.clusteringStrategy]));
        return receiverElement.area;
    }
  }

  public static isotropicGatherRadiance(
    rcv: GalerkinElement,
    areaFactor: number,
    link: Interaction,
    sourceRadiance: ColorRgbMutable[],
  ): void {
    const rcvRad = rcv.receivedRadiance as ColorRgbMutable[];

    if (link.numberOfBasisFunctionsOnReceiver === 1 && link.numberOfBasisFunctionsOnSource === 1) {
      rcvRad[0]!.addScaled(rcvRad[0]!, areaFactor * link.K[0]!, sourceRadiance[0]!);
    }
    else {
      const a = globalThis.Math.min(link.numberOfBasisFunctionsOnReceiver, rcv.basisSize);
      const b = globalThis.Math.min(link.numberOfBasisFunctionsOnSource, link.sourceElement.basisSize);
      for (let alpha = 0; alpha < a; alpha++) {
        for (let beta = 0; beta < b; beta++) {
          rcvRad[alpha]!.addScaled(
            rcvRad[alpha]!,
            areaFactor * link.K[alpha * link.numberOfBasisFunctionsOnSource + beta]!,
            sourceRadiance[beta]!,
          );
        }
      }
    }
  }

  public static gatherRadiance(
    link: Interaction,
    srcRad: ColorRgbMutable[],
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): void {
    const sourceElement = link.sourceElement;
    const receiverElement = link.receiverElement;
    if (!receiverElement.isCluster() || sourceElement === receiverElement) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    switch (galerkinState.clusteringStrategy) {
      case GalerkinClusteringStrategy.ISOTROPIC:
        ClusterTraversalStrategy.isotropicGatherRadiance(receiverElement, 1.0, link, srcRad);
        break;
      case GalerkinClusteringStrategy.ORIENTED: {
        const leafVisitor = new OrientedGathererVisitor(link, srcRad);
        ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
        break;
      }
      case GalerkinClusteringStrategy.Z_VISIBILITY: {
        const pixelArea = [0.0];
        if (ClusterTraversalStrategy.receiverSamples(link, scratch!, galerkinState, pixelArea)) {
          const visible = scratch!.getVisibleElements();
          ClusterTraversalStrategy.gatherRadianceFromSamples(
            link, srcRad, pixelArea[0]!, visible, 0, visible.length);
        }
        else {
          const leafVisitor = new OrientedGathererVisitor(link, srcRad);
          ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, receiverElement, galerkinState);
        }
        break;
      }
      default:
        VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "gatherRadiance", JavaString.vformat("Invalid clustering strategy %s", [galerkinState.clusteringStrategy]));
    }
  }

  /**
  Z-visibility clustering: when the receiver cluster of the link is seen from
  outside (the midpoint of the source lies outside its bounding box), renders
  its elements seen from that point and counts, in the workspace, how many
  samples each one covers. Returns false, rendering nothing, otherwise.
  @param pixelArea out: [0] is the area of the virtual screen a sample covers
  */
  public static receiverSamples(
    link: Interaction,
    scratch: ScratchVisibilityWorkspace,
    galerkinState: GalerkinState,
    pixelArea: number[],
  ): boolean {
    const receiverElement = link.receiverElement;
    const samplePoint = link.sourceElement.midPoint();
    if (!receiverElement.geometry!.boundingBox.outOfBounds(samplePoint)) {
      return false;
    }

    const boundingBox = ScratchVisibilityStrategy.scratchRenderElements(receiverElement, samplePoint, scratch, galerkinState);

    // Count how many pixels each element occupies in the scratch frame buffer
    ScratchVisibilityStrategy.scratchPixelsPerElement(scratch, galerkinState);

    // Area corresponding to one pixel in the scratch frame buffer (virtual screen)
    pixelArea[0] = ScratchVisibilityStrategy.scratchPixelArea(boundingBox, scratch);
    return true;
  }

  /**
  Gathers the radiance to each element of the receiver cluster that occupies at
  least one sample of its rendering, as counted by receiverSamples. The visible
  fraction of an element is estimated from its samples times pixelArea,
  normalized by one quarter of the receiver area. Each element receives one
  sum, so the order of the elements does not change the result: only the
  visible ones are visited, not every leaf of the cluster.
  TypeScript: the elements are numberOfVisibleElements of visibleElements from
  firstVisibleElement on (C++ takes a pointer to the first)
  */
  public static gatherRadianceFromSamples(
    link: Interaction,
    srcRad: ColorRgbMutable[],
    pixelArea: number,
    visibleElements: readonly ElementSamples[],
    firstVisibleElement: number,
    numberOfVisibleElements: number,
  ): void {
    for (let i = 0; i < numberOfVisibleElements; i++) {
      const visible = visibleElements[firstVisibleElement + i]!;
      const areaFactor = pixelArea * visible.getSamples() / (0.25 * link.receiverElement.area);
      ClusterTraversalStrategy.isotropicGatherRadiance(visible.getElement(), areaFactor, link, srcRad);
    }
  }

  public static maxRadiance(cluster: GalerkinElement, galerkinState: GalerkinState): ColorRgbMutable {
    const leafVisitor = new MaximumRadianceVisitor();
    ClusterTraversalStrategy.traverseAllLeafElements(leafVisitor, cluster, galerkinState);
    return leafVisitor.getAccumulatedRadiance();
  }
}
