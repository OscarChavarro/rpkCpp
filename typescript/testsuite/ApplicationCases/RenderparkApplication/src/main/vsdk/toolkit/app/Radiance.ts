import { OptionsGroupRadiance } from "./options/OptionsGroupRadiance";
import { GalerkinRadianceMethod } from "rpk-base/dist/vsdk/toolkit/galerkin/GalerkinRadianceMethod";
import { BidirectionalPathTracingState } from "rpk-base/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { PhotonMapConfig } from "rpk-base/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapConfig";
import { PhotonMapState } from "rpk-base/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapState";
import { RayMatterState } from "rpk-base/dist/vsdk/toolkit/raycasting/simple/RayMatterState";
import { ElementHierarchyState } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState";
import { StochasticRadiosityBasisState } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasisState";
import { StochasticRayTracingState } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState";
import { StochasticRelaxation } from "rpk-base/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { RadianceMethod } from "rpk-base/dist/vsdk/toolkit/scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "rpk-base/dist/vsdk/toolkit/scene/RadianceMethodAlgorithm";
import { Scene } from "rpk-base/dist/vsdk/toolkit/scene/Scene";
import { ToneMappingContext } from "rpk-base/dist/vsdk/toolkit/tonemap/ToneMappingContext";

/**
Stuff common to all radiance methods
*/
export class Radiance {
  private constructor() {
  }

  /**
  This routine sets the current radiance method to be used + initializes
  */
  public static setRadianceMethod(
    radianceMethod: RadianceMethod | null,
    scene: Scene,
    toneMapOptions: ToneMappingContext
  ): void {
    if (radianceMethod !== null) {
      radianceMethod.terminate(scene.patchList ?? []);
      // Until we have radiance data convertors, we dispose of the old data and
      // allocate new data for the new method
      const radianceElements = scene.getRadianceElements()!;
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        if (patch !== null && patch !== undefined) {
          radianceMethod.destroyPatchData(patch!);
          radianceElements.set(patch!.getFaceIndex(), null);
        }
      }
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        if (patch !== null && patch !== undefined) {
          radianceElements.set(patch!.getFaceIndex(), radianceMethod.createPatchData(patch!));
        }
      }
      radianceMethod.initialize(scene, toneMapOptions);
    }
  }

  /**
  Parses (and consumes) command line options for radiance
  computation
  */
  public static radianceParseOptions(
    argc: number[],
    argv: string[],
    newRadianceMethod: Array<RadianceMethod | null>,
    stochasticRelaxationState: StochasticRelaxation,
    elementHierarchyState: ElementHierarchyState,
    stochasticRadiosityBasisState: StochasticRadiosityBasisState,
    photonMapState: PhotonMapState,
    photonMapConfig: PhotonMapConfig,
    rayMatterState: RayMatterState,
    bidirectionalPathState: BidirectionalPathTracingState,
    stochasticRayTracingState: StochasticRayTracingState
  ): void {
    OptionsGroupRadiance.parse(
      argc,
      argv,
      newRadianceMethod,
      stochasticRelaxationState,
      elementHierarchyState,
      stochasticRadiosityBasisState,
      photonMapState,
      photonMapConfig,
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState
    );

    if (
      newRadianceMethod !== null
      && newRadianceMethod.length > 0
      && newRadianceMethod[0] !== null
    ) {
      if (newRadianceMethod[0]!.className === RadianceMethodAlgorithm.GALERKIN) {
        const galerkinRadianceMethod = newRadianceMethod[0] as GalerkinRadianceMethod;
        galerkinRadianceMethod.setStrategy();
      }
      newRadianceMethod[0]!.parseOptions(argc, argv);
    }
  }
}
