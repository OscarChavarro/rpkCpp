import { GalerkinRadianceMethod } from "vitral/dist/vsdk/toolkit/galerkin/GalerkinRadianceMethod";
import { BidirectionalPathTracingState } from "vitral/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { PhotonMapConfig } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapConfig";
import { PhotonMapRadianceMethod } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapRadianceMethod";
import { PhotonMapState } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapState";
import { RayMatterState } from "vitral/dist/vsdk/toolkit/raycasting/simple/RayMatterState";
import { ElementHierarchyState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState";
import { RandomWalkRadianceMethod } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/RandomWalkRadianceMethod";
import { StochasticJacobiRadianceMethod } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod";
import { StochasticRadiosityBasisState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasisState";
import { StochasticRayTracingState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState";
import { StochasticRelaxation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { RadianceMethod } from "vitral/dist/vsdk/toolkit/scene/RadianceMethod";
import { OptionTextUtils } from "./OptionTextUtils";
import { OptionsGroupBidirectionalRaytracing } from "./OptionsGroupBidirectionalRaytracing";
import { OptionsGroupGalerkin } from "./OptionsGroupGalerkin";
import { OptionsGroupPhotonMap } from "./OptionsGroupPhotonMap";
import { OptionsGroupRadianceMethod } from "./OptionsGroupRadianceMethod";
import { OptionsGroupRandomWalkRadiosity } from "./OptionsGroupRandomWalkRadiosity";
import { OptionsGroupRayMatter } from "./OptionsGroupRayMatter";
import { OptionsGroupStochasticRaytracing } from "./OptionsGroupStochasticRaytracing";
import { OptionsGroupStochasticRelaxationRadiosity } from "./OptionsGroupStochasticRelaxationRadiosity";

export class OptionsGroupRadiance {
  private static readonly RADIANCE_METHODS_STRING_LENGTH = 1000;

  private constructor() {
  }

  private static selectRadianceMethod(
    name: string,
    newRadianceMethod: Array<RadianceMethod | null>,
    stochasticRelaxationState: StochasticRelaxation,
    elementHierarchyState: ElementHierarchyState,
    stochasticRadiosityBasisState: StochasticRadiosityBasisState,
    photonMapState: PhotonMapState,
    photonMapConfig: PhotonMapConfig
  ): void {
    if (name !== null && name.length > 0) {
      if (newRadianceMethod !== null && newRadianceMethod.length > 0 && newRadianceMethod[0] !== null) {
        newRadianceMethod[0] = null;
      }

      if (OptionTextUtils.equalsIgnoreCasePrefix(name, "Galerkin", 4)) {
        newRadianceMethod[0] = new GalerkinRadianceMethod();
      }
      else if (OptionTextUtils.equalsIgnoreCasePrefix(name, "PMAP", 4)) {
        newRadianceMethod[0] = new PhotonMapRadianceMethod(photonMapState, photonMapConfig);
      }
      else if (OptionTextUtils.equalsIgnoreCasePrefix(name, "StochJacobi", 4)) {
        newRadianceMethod[0] = new StochasticJacobiRadianceMethod(
          stochasticRelaxationState,
          elementHierarchyState,
          stochasticRadiosityBasisState
        );
      }
      else if (OptionTextUtils.equalsIgnoreCasePrefix(name, "RandomWalk", 4)) {
        newRadianceMethod[0] = new RandomWalkRadianceMethod(
          stochasticRelaxationState,
          elementHierarchyState,
          stochasticRadiosityBasisState
        );
      }
    }
  }

  public static parse(
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
    const radianceMethodsString = [""];
    if (radianceMethodsString[0]!.length > OptionsGroupRadiance.RADIANCE_METHODS_STRING_LENGTH) {
      radianceMethodsString[0] = radianceMethodsString[0]!.substring(0, OptionsGroupRadiance.RADIANCE_METHODS_STRING_LENGTH);
    }

    OptionsGroupRadianceMethod.radianceMethodParseOptions(argc, argv, radianceMethodsString);

    OptionsGroupRadiance.selectRadianceMethod(
      radianceMethodsString[0]!,
      newRadianceMethod,
      stochasticRelaxationState,
      elementHierarchyState,
      stochasticRadiosityBasisState,
      photonMapState,
      photonMapConfig
    );

    if (newRadianceMethod === null || newRadianceMethod.length === 0 || newRadianceMethod[0] === null) {
      process.stderr.write(
        "ERROR: You must select a radiance mode using '-radiance-method'. Supported values: Galerkin, PMAP, StochJacobi, RandomWalk.\n"
      );
      process.stderr.flush?.();
      process.exit(1);
    }

    OptionsGroupStochasticRelaxationRadiosity.parse(argc, argv, stochasticRelaxationState, elementHierarchyState);
    OptionsGroupRandomWalkRadiosity.parse(argc, argv, stochasticRelaxationState);
    OptionsGroupRayMatter.rayMattingParseOptions(argc, argv, rayMatterState);
    OptionsGroupBidirectionalRaytracing.parse(argc, argv, bidirectionalPathState);
    OptionsGroupStochasticRaytracing.parse(argc, argv, stochasticRayTracingState);
    OptionsGroupPhotonMap.parse(argc, argv, photonMapState);

    OptionsGroupGalerkin.galerkinParseOptions(argc, argv);
  }
}
