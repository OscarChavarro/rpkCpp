import { Error as VsdkError } from "../../common/Error";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinShaftCullMode } from "../GalerkinShaftCullMode";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { Shaft } from "../Shaft";
import { Polygon } from "../../scene/Polygon";
import { Scene } from "../../scene/Scene";
import { BoundingBox } from "../../skin/BoundingBox";
import { ElementFlags } from "../../skin/ElementFlags";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../skin/Patch";
import { FormFactorStrategy } from "./FormFactorStrategy";

export class LinkingSimpleStrategy {
  private static createInitialLink(
    scene: Scene,
    galerkinState: GalerkinState,
    role: GalerkinRole,
    candidateList: Array<Geometry[] | null>,
    topElement: GalerkinElement,
    topLevelBoundingBox: BoundingBox,
    patch: Patch,
  ): void {
    if (scene === null || galerkinState === null || candidateList === null || candidateList.length === 0
      || topElement === null || patch === null || topElement.patch === null) {
      return;
    }

    if (!patch.facing(topElement.patch)) {
      return;
    }

    let receiverElement: GalerkinElement;
    let sourceElement: GalerkinElement;
    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      return;
    }

    switch (role) {
      case GalerkinRole.SOURCE:
        receiverElement = topLevelElement;
        sourceElement = topElement;
        break;
      case GalerkinRole.RECEIVER:
        receiverElement = topElement;
        sourceElement = topLevelElement;
        break;
      default:
        VsdkError.fatal(2, "createInitialLink", "Impossible element role");
        return;
    }

    const oldCandidateList = candidateList[0];
    if ((galerkinState.exactVisibility !== 0
        || galerkinState.shaftCullMode === GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING)
      && oldCandidateList !== null) {
      const shaft = new Shaft();

      if (galerkinState.exactVisibility !== 0) {
        const receiverPolygon = new Polygon();
        const sourcePolygon = new Polygon();
        receiverElement.initPolygon(receiverPolygon);
        sourceElement.initPolygon(sourcePolygon);
        shaft.constructFromPolygonToPolygon(receiverPolygon, sourcePolygon);
      }
      else {
        const boundingBox = new BoundingBox();
        patch.computeAndGetBoundingBox(boundingBox);
        shaft.constructFromBoundingBoxes(topLevelBoundingBox, boundingBox);
      }

      shaft.setShaftOmit(topElement.patch);
      shaft.setShaftOmit(patch);
      const arr: Geometry[] = [];
      shaft.doCulling(oldCandidateList, arr, galerkinState.shaftCullStrategy);
      candidateList[0] = arr;

      if (shaft.isCut()) {
        Shaft.freeCandidateList(candidateList[0]);
        candidateList[0] = oldCandidateList;
        return;
      }
    }

    const link = new Interaction();
    link.K = new Array<number>(GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE).fill(0.0);
    link.receiverElement = receiverElement;
    link.sourceElement = sourceElement;

    link.numberOfBasisFunctionsOnReceiver = receiverElement.basisSize;
    link.numberOfBasisFunctionsOnSource = sourceElement.basisSize;

    const isSceneGeometry = candidateList[0] === scene.geometryList;
    const isClusteredGeometry = candidateList[0] === scene.clusteredGeometryList;
    const geometryListReferences = candidateList[0];
    FormFactorStrategy.computeAreaToAreaFormFactorVisibility(
      scene.voxelGrid,
      geometryListReferences,
      isSceneGeometry,
      isClusteredGeometry,
      link,
      galerkinState,
    );

    if (galerkinState.exactVisibility !== 0
      || galerkinState.shaftCullMode === GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING) {
      if (oldCandidateList !== candidateList[0]) {
        Shaft.freeCandidateList(candidateList[0]);
      }
      candidateList[0] = oldCandidateList;
    }

    if ((link.visibility & 0xff) > 0) {
      const newLink = Interaction.interactionDuplicate(link);
      if (newLink === null) {
        return;
      }

      if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        if (sourceElement.interactions === null) {
          sourceElement.interactions = [];
        }
        sourceElement.interactions.push(newLink);
      }
      else {
        if (receiverElement.interactions === null) {
          receiverElement.interactions = [];
        }
        receiverElement.interactions.push(newLink);
      }
    }
  }

  private static geometryLink(
    scene: Scene,
    galerkinState: GalerkinState,
    role: GalerkinRole,
    candidateList: Array<Geometry[] | null>,
    topElement: GalerkinElement,
    topLevelBoundingBox: BoundingBox,
    geometry: Geometry,
  ): void {
    if (scene === null || galerkinState === null || candidateList === null || candidateList.length === 0
      || topElement === null || topElement.patch === null || geometry === null) {
      return;
    }

    if (geometry.bounded
      && geometry.getBoundingBox().behindPlane(topElement.patch.normal, topElement.patch.planeConstant)) {
      return;
    }

    const oldCandidateList = candidateList[0];
    if (geometry.bounded && oldCandidateList !== null) {
      const shaft = new Shaft();
      const boundingBox = geometry.getBoundingBox();
      shaft.constructFromBoundingBoxes(topLevelBoundingBox, boundingBox);
      shaft.setShaftOmit(topElement.patch);
      const arr: Geometry[] = [];
      shaft.doCulling(oldCandidateList, arr, galerkinState.shaftCullStrategy);
      candidateList[0] = arr;
    }

    if (geometry.isCompound()) {
      const geometryList = Geometry.primitiveListCopy(geometry);
      for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
        LinkingSimpleStrategy.geometryLink(
          scene,
          galerkinState,
          role,
          candidateList,
          topElement,
          topLevelBoundingBox,
          geometryList[i],
        );
      }
    }
    else {
      const patchList = Geometry.patchListReference(geometry);
      for (let i = 0; patchList !== null && i < patchList.length; i++) {
        LinkingSimpleStrategy.createInitialLink(
          scene,
          galerkinState,
          role,
          candidateList,
          topElement,
          topLevelBoundingBox,
          patchList[i],
        );
      }
    }

    if (geometry.bounded && oldCandidateList !== null) {
      Shaft.freeCandidateList(candidateList[0]);
    }
    candidateList[0] = oldCandidateList;
  }

  public static createInitialLinks(
    scene: Scene,
    galerkinState: GalerkinState,
    role: GalerkinRole,
    topElement: GalerkinElement,
  ): void {
    if (scene === null || galerkinState === null || topElement === null) {
      return;
    }

    if ((topElement.flags & ElementFlags.IS_CLUSTER_MASK) !== 0) {
      VsdkError.fatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
      return;
    }
    if (topElement.patch === null) {
      return;
    }

    const topLevelBoundingBox = new BoundingBox();
    topElement.patch.computeAndGetBoundingBox(topLevelBoundingBox);

    const candidateList: Array<Geometry[] | null> = [scene.clusteredGeometryList];

    for (let i = 0; scene.geometryList !== null && i < scene.geometryList.length; i++) {
      LinkingSimpleStrategy.geometryLink(
        scene,
        galerkinState,
        role,
        candidateList,
        topElement,
        topLevelBoundingBox,
        scene.geometryList[i],
      );
    }
  }
}
