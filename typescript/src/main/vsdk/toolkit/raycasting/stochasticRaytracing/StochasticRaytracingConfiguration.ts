import { ArrayList } from "../../../../java/util/ArrayList";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { BsdfComponent } from "../../material/BsdfComponent";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Patch } from "../../skin/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { ImportantLightSampler } from "../bidirectionalRaytracing/ImportantLightSampler";
import { LightList } from "../bidirectionalRaytracing/LightList";
import { UniformLightSampler } from "../bidirectionalRaytracing/UniformLightSampler";
import { PhotonMapSampler } from "../photonMap/PhotonMapSampler";
import { BsdfSampler } from "../raytracing/BsdfSampler";
import { EyeSampler } from "../raytracing/EyeSampler";
import { PixelSampler } from "../raytracing/PixelSampler";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { RayTracingLightMode } from "./RayTracingLightMode";
import { RayTracingRadMode } from "./RayTracingRadMode";
import { RayTracingSamplingMode } from "./RayTracingSamplingMode";
import { ScatterInfo } from "./ScatterInfo";
import { SeedConfig } from "./SeedConfig";
import { StochasticRayTracingState } from "./StochasticRayTracingState";
import { StorageReadout } from "./StorageReadout";

export class StochasticRaytracingConfiguration {
  public samplesPerPixel: number;

  public nextEventSamples: number;
  public lightMode: RayTracingLightMode;

  public radMode: RayTracingRadMode;

  public scatterSamples: number;
  public firstDGSamples: number;
  public reflectionSampling: RayTracingSamplingMode;
  public separateSpecular: boolean;

  public backgroundIndirect: boolean;
  public backgroundDirect: boolean;
  public backgroundSampling: boolean;
  public doFrameCoherent: number;
  public doCorrelatedSampling: number;
  public baseSeed: number;

  public screen: ScreenBuffer | null;
  public toneMapOptions: ToneMappingContext | null;

  public samplerConfig: SamplerConfig;
  public seedConfig: SeedConfig;

  public siStorage: ScatterInfo;
  public siOthers: ScatterInfo[];
  public siOthersCount: number;

  public initialReadout: StorageReadout;
  public rayTracingLightList: LightList | null;

  public constructor(
    defaultCamera: Camera,
    state: StochasticRayTracingState,
    lightList: ArrayList<Patch>,
    radianceMethod: RadianceMethod | null,
    inToneMapOptions: ToneMappingContext,
    inRayTracingLightList: LightList | null
  ) {
    this.samplesPerPixel = 0;
    this.nextEventSamples = 0;
    this.lightMode = RayTracingLightMode.ALL_LIGHTS;
    this.radMode = RayTracingRadMode.STORED_NONE;
    this.scatterSamples = 0;
    this.firstDGSamples = 0;
    this.reflectionSampling = RayTracingSamplingMode.BRDF_SAMPLING;
    this.separateSpecular = false;
    this.backgroundIndirect = false;
    this.backgroundDirect = false;
    this.backgroundSampling = false;
    this.doFrameCoherent = 0;
    this.doCorrelatedSampling = 0;
    this.baseSeed = 0;
    this.screen = null;
    this.toneMapOptions = null;
    this.samplerConfig = new SamplerConfig();
    this.seedConfig = new SeedConfig();
    this.siStorage = new ScatterInfo();
    this.siOthers = new Array<ScatterInfo>(6);
    for (let i = 0; i < this.siOthers.length; i++) {
      this.siOthers[i] = new ScatterInfo();
    }
    this.siOthersCount = 0;
    this.initialReadout = StorageReadout.SCATTER;
    this.rayTracingLightList = inRayTracingLightList;

    this.init(defaultCamera, state, lightList, radianceMethod, inToneMapOptions, inRayTracingLightList);
  }

  public release(): void {
    this.samplerConfig.releaseVars();
  }

  public init(
    defaultCamera: Camera,
    state: StochasticRayTracingState,
    lightList: ArrayList<Patch>,
    radianceMethod: RadianceMethod | null,
    inToneMapOptions: ToneMappingContext,
    inRayTracingLightList: LightList | null
  ): void {
    this.samplesPerPixel = state.samplesPerPixel;

    this.radMode = state.radMode;

    this.backgroundIndirect = state.backgroundIndirect !== 0;
    this.backgroundDirect = state.backgroundDirect !== 0;
    this.backgroundSampling = state.backgroundSampling !== 0;
    this.doFrameCoherent = state.doFrameCoherent;
    this.doCorrelatedSampling = state.doCorrelatedSampling;
    this.baseSeed = state.baseSeed;

    if (this.radMode !== RayTracingRadMode.STORED_NONE) {
      if (radianceMethod === null) {
        VsdkLogger.error("Stored Radiance", "No radiance method active, using no storage");
      }
      else if ((this.radMode === RayTracingRadMode.STORED_PHOTON_MAP) && (radianceMethod.className !== RadianceMethodAlgorithm.PHOTON_MAP)) {
        VsdkLogger.error("Stored Radiance", "Photon map method not active, using no storage");
      }
      this.radMode = RayTracingRadMode.STORED_NONE;
    }

    if (state.nextEvent !== 0) {
      this.nextEventSamples = state.nextEventSamples;
    }
    else {
      this.nextEventSamples = 0;
    }
    this.lightMode = state.lightMode;

    this.reflectionSampling = state.reflectionSampling;

    if (
      this.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING
      && (this.radMode as RayTracingRadMode) === RayTracingRadMode.STORED_INDIRECT
    ) {
      VsdkLogger.error("Classical raytracing", "Incompatible with extended final gather, using storage directly");
      this.radMode = RayTracingRadMode.STORED_DIRECT;
    }

    this.scatterSamples = state.scatterSamples;
    if (state.differentFirstDG !== 0) {
      this.firstDGSamples = state.firstDGSamples;
    }
    else {
      this.firstDGSamples = this.scatterSamples;
    }

    this.separateSpecular = state.separateSpecular !== 0;

    if (this.reflectionSampling === RayTracingSamplingMode.PHOTON_MAP_SAMPLING) {
      VsdkLogger.warning("Fresnel Specular Sampling", "always uses separate specular");
      this.separateSpecular = true;
    }

    this.samplerConfig.minDepth = state.minPathDepth;
    this.samplerConfig.maxDepth = state.maxPathDepth;

    this.toneMapOptions = inToneMapOptions;
    if (this.toneMapOptions === null) {
      VsdkLogger.fatal(-1, "StochasticRaytracingConfiguration::init", "Tone mapping context not set");
      return;
    }

    this.screen = new ScreenBuffer(null, defaultCamera, this.toneMapOptions);
    this.screen.setFactor(1.0);

    this.initDependentVars(lightList, radianceMethod, inRayTracingLightList);
  }

  private initDependentVars(
    lightList: ArrayList<Patch>,
    radianceMethod: RadianceMethod | null,
    inRayTracingLightList: LightList | null
  ): void {
    void inRayTracingLightList;

    this.samplerConfig.pointSampler = new EyeSampler();
    this.samplerConfig.dirSampler = new PixelSampler();

    switch (this.reflectionSampling) {
      case RayTracingSamplingMode.BRDF_SAMPLING:
        this.samplerConfig.surfaceSampler = new BsdfSampler();
        break;
      case RayTracingSamplingMode.PHOTON_MAP_SAMPLING:
        this.samplerConfig.surfaceSampler = new PhotonMapSampler();
        break;
      case RayTracingSamplingMode.CLASSICAL_SAMPLING:
        this.samplerConfig.surfaceSampler = new BsdfSampler();
        break;
      default:
        VsdkLogger.error("SR CONFIG::initDependentVars", "Wrong sampling mode");
        break;
    }

    let storeFlags: number;

    const bsdfGlossy = BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT;
    const bsdfDiffuse = BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BTDF_DIFFUSE_COMPONENT;
    const bsdfAll =
      BsdfComponent.BRDF_DIFFUSE_COMPONENT
      | BsdfComponent.BRDF_GLOSSY_COMPONENT
      | BsdfComponent.BRDF_SPECULAR_COMPONENT
      | BsdfComponent.BTDF_DIFFUSE_COMPONENT
      | BsdfComponent.BTDF_GLOSSY_COMPONENT
      | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    if ((radianceMethod === null) || (this.radMode === RayTracingRadMode.STORED_NONE)) {
      storeFlags = 0;
    }
    else {
      if (radianceMethod.className === RadianceMethodAlgorithm.PHOTON_MAP) {
        storeFlags = bsdfGlossy | bsdfDiffuse;
      }
      else {
        storeFlags = BsdfComponent.BRDF_DIFFUSE_COMPONENT;
      }
    }

    this.initialReadout = StorageReadout.SCATTER;

    switch (this.radMode) {
      case RayTracingRadMode.STORED_NONE:
        this.siStorage.flags = 0;
        this.siStorage.nrSamplesBefore = 0;
        this.siStorage.nrSamplesAfter = 0;
        break;
      case RayTracingRadMode.STORED_DIRECT:
        this.siStorage.flags = storeFlags;
        this.siStorage.nrSamplesBefore = 0;
        this.siStorage.nrSamplesAfter = 0;
        this.initialReadout = StorageReadout.READ_NOW;
        break;
      case RayTracingRadMode.STORED_INDIRECT:
      case RayTracingRadMode.STORED_PHOTON_MAP:
        this.siStorage.flags = storeFlags;
        this.siStorage.nrSamplesBefore = this.firstDGSamples;
        this.siStorage.nrSamplesAfter = 0;
        break;
      default:
        VsdkLogger.error("SR CONFIG::initDependentVars", "Wrong Rad Mode");
        break;
    }

    let remainingFlags = bsdfAll & ~storeFlags;
    let siIndex = 0;

    if (this.separateSpecular) {
      let flags: number;

      if (this.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
        flags = remainingFlags & (BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT);
      }
      else {
        flags = remainingFlags & BsdfComponent.BRDF_SPECULAR_COMPONENT;
      }

      if (flags !== 0) {
        this.siOthers[siIndex].flags = flags;
        this.siOthers[siIndex].nrSamplesBefore = this.scatterSamples;
        this.siOthers[siIndex].nrSamplesAfter = this.scatterSamples;
        siIndex++;
        remainingFlags = remainingFlags & ~flags;
      }

      if (this.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
        flags = remainingFlags & (BsdfComponent.BTDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT);
      }
      else {
        flags = remainingFlags & BsdfComponent.BTDF_SPECULAR_COMPONENT;
      }

      if (flags !== 0) {
        this.siOthers[siIndex].flags = flags;
        this.siOthers[siIndex].nrSamplesBefore = this.scatterSamples;
        this.siOthers[siIndex].nrSamplesAfter = this.scatterSamples;
        siIndex++;
        remainingFlags = remainingFlags & ~flags;
      }
    }

    if (
      this.reflectionSampling !== RayTracingSamplingMode.CLASSICAL_SAMPLING
      && this.scatterSamples !== this.firstDGSamples
    ) {
      const gdFlags = remainingFlags & (bsdfDiffuse | bsdfGlossy);
      if (gdFlags !== 0) {
        this.siOthers[siIndex].flags = gdFlags;
        this.siOthers[siIndex].nrSamplesBefore = this.firstDGSamples;
        this.siOthers[siIndex].nrSamplesAfter = this.scatterSamples;
        siIndex++;
        remainingFlags = remainingFlags & ~gdFlags;
      }
    }

    if (this.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
      const dFlags = remainingFlags & bsdfDiffuse;

      if (dFlags !== 0) {
        this.siOthers[siIndex].flags = dFlags;
        this.siOthers[siIndex].nrSamplesBefore = 0;
        this.siOthers[siIndex].nrSamplesAfter = 0;
        siIndex++;
        remainingFlags = remainingFlags & ~dFlags;
      }
    }

    if (remainingFlags !== 0) {
      this.siOthers[siIndex].flags = remainingFlags;
      this.siOthers[siIndex].nrSamplesBefore = this.scatterSamples;
      this.siOthers[siIndex].nrSamplesAfter = this.scatterSamples;
      siIndex++;
    }

    this.siOthersCount = siIndex;

    this.rayTracingLightList = new LightList(lightList, this.backgroundSampling);

    if (this.lightMode === RayTracingLightMode.IMPORTANT_LIGHTS) {
      this.samplerConfig.neSampler = new ImportantLightSampler(this.rayTracingLightList);
    }
    else {
      this.samplerConfig.neSampler = new UniformLightSampler(this.rayTracingLightList);
    }

    this.seedConfig.init(this.samplerConfig.maxDepth);
  }
}
