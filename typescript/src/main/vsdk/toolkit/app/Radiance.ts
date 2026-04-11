import { OptionsGroupRadiance } from "./options/OptionsGroupRadiance";
import { GalerkinRadianceMethod } from "../galerkin/GalerkinRadianceMethod";
import { BidirectionalPathTracingState } from "../raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { PhotonMapConfig } from "../raycasting/photonMap/PhotonMapConfig";
import { PhotonMapState } from "../raycasting/photonMap/PhotonMapState";
import { RayMatterState } from "../raycasting/simple/RayMatterState";
import { ElementHierarchyState } from "../raycasting/stochasticRaytracing/ElementHierarchyState";
import { StochasticRadiosityBasisState } from "../raycasting/stochasticRaytracing/StochasticRadiosityBasisState";
import { StochasticRayTracingState } from "../raycasting/stochasticRaytracing/StochasticRayTracingState";
import { StochasticRelaxation } from "../raycasting/stochasticRaytracing/StochasticRelaxation";
import { RadianceMethod } from "../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../scene/RadianceMethodAlgorithm";
import { Scene } from "../scene/Scene";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";

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
        if (patch !== null) {
          radianceMethod.destroyPatchData(patch);
        }
      }
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        if (patch !== null) {
          patch.radianceData = radianceMethod.createPatchData(patch);
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
