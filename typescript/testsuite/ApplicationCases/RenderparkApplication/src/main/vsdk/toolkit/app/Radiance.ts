import { OptionsGroupRadiance } from "./options/OptionsGroupRadiance";
import { GalerkinRadianceMethod } from "vitral/dist/vsdk/toolkit/galerkin/GalerkinRadianceMethod";
import { BidirectionalPathTracingState } from "vitral/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { PhotonMapConfig } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapConfig";
import { PhotonMapState } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapState";
import { RayMatterState } from "vitral/dist/vsdk/toolkit/raycasting/simple/RayMatterState";
import { ElementHierarchyState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState";
import { StochasticRadiosityBasisState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasisState";
import { StochasticRayTracingState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState";
import { StochasticRelaxation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { RadianceMethod } from "vitral/dist/vsdk/toolkit/scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "vitral/dist/vsdk/toolkit/scene/RadianceMethodAlgorithm";
import { Scene } from "vitral/dist/vsdk/toolkit/scene/Scene";
import { ToneMappingContext } from "vitral/dist/vsdk/toolkit/tonemap/ToneMappingContext";

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
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        if (patch !== null && patch !== undefined) {
          radianceMethod.destroyPatchData(patch!);
        }
      }
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        if (patch !== null && patch !== undefined) {
          patch.radianceData = radianceMethod.createPatchData(patch!);
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
