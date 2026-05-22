import { ArrayList } from "../../../../java/util/ArrayList";
import { Random } from "../../../../java/util/Random";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { StratifiedSampling2D } from "../../raycasting/common/StratifiedSampling2D";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { ImageOutputHandle } from "../../io/image/ImageOutputHandle";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongEmittanceDistributionFunction } from "../../material/PhongEmittanceDistributionFunction";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { LightList } from "../bidirectionalRaytracing/LightList";
import { PathRayType } from "../common/PathRayType";
import { RayTools } from "../common/RayTools";
import { RayTracer } from "../common/RayTracer";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { PhotonMapRadianceMethod } from "../photonMap/PhotonMapRadianceMethod";
import { NextEventSampler } from "../raytracing/NextEventSampler";
import { PixelSampler } from "../raytracing/PixelSampler";
import { SampleConnectionFlags } from "../raytracing/SampleConnectionFlags";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { ScreenIterate } from "../raytracing/ScreenIterate";
import { RayTracingLightMode } from "./RayTracingLightMode";
import { RayTracingRadMode } from "./RayTracingRadMode";
import { RayTracingSamplingMode } from "./RayTracingSamplingMode";
import { ScatterInfo } from "./ScatterInfo";
import { StochasticRayTracingState } from "./StochasticRayTracingState";
import { StochasticRaytracerCallbackData } from "./StochasticRaytracerCallbackData";
import { StochasticRaytracingConfiguration } from "./StochasticRaytracingConfiguration";
import { StorageReadout } from "./StorageReadout";

export class StochasticRaytracer extends RayTracer {
  private static readonly PHOTON_MAP_MIN_DIST = 0.02;
  private static readonly PHOTON_MAP_MIN_DIST2 = StochasticRaytracer.PHOTON_MAP_MIN_DIST * StochasticRaytracer.PHOTON_MAP_MIN_DIST;
  private static name = "Stochastic Raytracing & Final Gathers";
  private lightList: LightList | null;
  private rayTracingState: StochasticRayTracingState;
  private static random48: Random = new Random();

  public constructor(inLightList: LightList | null, inRayTracingState: StochasticRayTracingState) {
    super();
    this.lightList = inLightList;
    this.rayTracingState = inRayTracingState;
  }

  public override defaults(): void {
    // Defaults are owned by the caller-provided StochasticRayTracingState instance.
  }

  public override getName(): string {
    return StochasticRaytracer.name;
  }

  public override initialize(lightPatches: ArrayList<Patch>): void {
    void lightPatches;
  }

  private static toArrayList(scenePatches: Patch[] | null): ArrayList<Patch> {
    const out = new ArrayList<Patch>();
    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      out.add(scenePatches[i]);
    }
    return out;
  }

  /**
Raytrace the current scene as seen with the current camera.
*/
  public override execute(
    ip: ImageOutputHandle | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (toneMapOptions === null) {
      VsdkLogger.fatal(-1, "StochasticRaytracer::execute", "Tone mapping context not provided");
    }

    const config = new StochasticRaytracingConfiguration(
      scene.camera as Camera,
      this.rayTracingState,
      StochasticRaytracer.toArrayList(scene.lightSourcePatchList),
      radianceMethod,
      toneMapOptions,
      this.lightList
    );
    const callbackData = new StochasticRaytracerCallbackData();
    callbackData.config = config;
    callbackData.radianceMethod = radianceMethod;
    callbackData.renderOptions = renderOptions;

    if (this.rayTracingState.doFrameCoherent !== 0) {
      StochasticRaytracer.srand48(this.rayTracingState.baseSeed);
    }

    if (this.rayTracingState.progressiveTracing === 0) {
      ScreenIterate.sequential(
        scene.camera as Camera,
        scene.voxelGrid as VoxelGrid,
        scene.background,
        StochasticRaytracer.calcPixel,
        callbackData,
        toneMapOptions
      );
    }
    else {
      ScreenIterate.progressive(
        scene.camera as Camera,
        scene.voxelGrid as VoxelGrid,
        scene.background,
        StochasticRaytracer.calcPixel,
        callbackData,
        toneMapOptions
      );
    }

    config.screen!.render();

    if (ip !== null) {
      config.screen!.writeFile(ip);
    }

    this.rayTracingState.lastScreen = config.screen;
    config.screen = null;
    this.lightList = config.rayTracingLightList;
    config.release();
  }

  public override saveImage(imageOutputHandle: ImageOutputHandle): boolean {
    if (imageOutputHandle !== null && this.rayTracingState.lastScreen !== null) {
      this.rayTracingState.lastScreen.sync();
      this.rayTracingState.lastScreen.writeFile(imageOutputHandle);
      return true;
    }
    else {
      return false;
    }
  }

  public override terminate(): void {
    this.rayTracingState.lastScreen = null;
    this.lightList = null;
  }

  private static stochasticRaytracerGetScatteredRadiance(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    thisNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout,
    radianceMethod: RadianceMethod,
    renderOptions: RendererConfiguration
  ): ColorRgb {
    let siCurrent: number;
    let si: ScatterInfo;

    const newNode = new SimpleRaytracingPathNode();
    thisNode.attach(newNode);

    const result = new ColorRgb();
    result.clear();

    if ((config.samplerConfig.surfaceSampler === null) ||
      (thisNode.m_depth >= config.samplerConfig.maxDepth)) {
      return result;
    }

    if ((config.siStorage.flags !== 0) &&
      (readout === StorageReadout.SCATTER)) {
      si = config.siStorage;
      siCurrent = -1;
    }
    else {
      si = config.siOthers[0];
      siCurrent = 0;
    }

    while (siCurrent < config.siOthersCount) {
      let numberOfSamples: number;

      if (si.DoneSomePreviousBounce(thisNode)) {
        numberOfSamples = si.nrSamplesAfter;
      }
      else {
        numberOfSamples = si.nrSamplesBefore;
      }

      if (numberOfSamples > 2) {
        let albedo = new ColorRgb();
        albedo.clear();
        if (thisNode.m_useBsdf !== null) {
          albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(), si.flags);
        }
        if (albedo.average() < Numeric.EPSILON) {
          numberOfSamples = 0;
        }
      }

      if ((numberOfSamples > 0) && (thisNode.m_depth + 1 < config.samplerConfig.maxDepth)) {
        const x1 = [0.0];
        const x2 = [0.0];
        let factor: number;
        const stratified = new StratifiedSampling2D(numberOfSamples);
        let radiance: ColorRgb;
        const doRR = thisNode.m_depth >= config.samplerConfig.minDepth;

        for (let i = 0; i < numberOfSamples; i++) {
          stratified.sample(x1, x2);

          if (
            config.samplerConfig.surfaceSampler!.sample(
              camera,
              sceneVoxelGrid,
              sceneBackground,
              thisNode.previous(),
              thisNode,
              newNode,
              x1[0],
              x2[0],
              doRR,
              si.flags
            )
            && ((newNode.m_rayType !== PathRayType.ENVIRONMENT) || (config.backgroundIndirect))
          ) {
            if (newNode.m_rayType !== PathRayType.ENVIRONMENT) {
              newNode.assignBsdfAndNormal();
            }

            if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
              config.seedConfig.save(newNode.m_depth);
            }

            if (siCurrent === -1) {
              radiance = StochasticRaytracer.stochasticRaytracerGetRadiance(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                newNode,
                config,
                StorageReadout.READ_NOW,
                numberOfSamples,
                radianceMethod,
                renderOptions
              );
            }
            else {
              radiance = StochasticRaytracer.stochasticRaytracerGetRadiance(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                newNode,
                config,
                readout,
                numberOfSamples,
                radianceMethod,
                renderOptions
              );
            }

            if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
              config.seedConfig.Restore(newNode.m_depth);
            }

            factor = newNode.m_G / (newNode.m_pdfFromPrev * numberOfSamples);

            radiance.scalarProductScaled(radiance, factor, thisNode.m_bsdfEval);
            result.add(radiance, result);
          }
        }
      }

      siCurrent++;
      if (siCurrent < config.siOthersCount) {
        si = config.siOthers[siCurrent];
      }
    }

    thisNode.setNext(null);
    return result;
  }

  private static srGetDirectRadiance(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout
  ): ColorRgb {
    const result = new ColorRgb();
    const radiance = new ColorRgb();
    result.clear();
    const dirEL = new Vector3D();

    if (readout === StorageReadout.READ_NOW && config.radMode === RayTracingRadMode.STORED_PHOTON_MAP) {
      return result;
    }

    const nes = config.samplerConfig.neSampler as NextEventSampler;
    const bsdfAll = BsdfComponent.BRDF_DIFFUSE_COMPONENT
      | BsdfComponent.BRDF_GLOSSY_COMPONENT
      | BsdfComponent.BRDF_SPECULAR_COMPONENT
      | BsdfComponent.BTDF_DIFFUSE_COMPONENT
      | BsdfComponent.BTDF_GLOSSY_COMPONENT
      | BsdfComponent.BTDF_SPECULAR_COMPONENT;
    const bsdfSpec = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    if ((nes !== null) &&
      (config.nextEventSamples > 0) &&
      (prevNode.m_depth + 1 < config.samplerConfig.maxDepth)) {
      const lightNode = new SimpleRaytracingPathNode();
      const x1 = [0.0];
      const x2 = [0.0];
      let geom: number;
      let weight: number;
      let cl: number;
      let cr: number;
      let factor: number;
      let nrs: number;
      let lightsToDo = true;

      if (config.lightMode === RayTracingLightMode.ALL_LIGHTS) {
        lightsToDo = nes.ActivateFirstUnit();
      }

      while (lightsToDo) {
        const stratified = new StratifiedSampling2D(config.nextEventSamples);

        for (let i = 0; i < config.nextEventSamples; i++) {
          stratified.sample(x1, x2);

          if (
            config.samplerConfig.neSampler!.sample(
              camera,
              sceneVoxelGrid,
              sceneBackground,
              prevNode.previous(),
              prevNode,
              lightNode,
              x1[0],
              x2[0],
              true,
              bsdfAll
            )
            && (RayTools.pathNodesVisible(sceneVoxelGrid, prevNode, lightNode))
          ) {
            let siCurrent: number;
            let si: ScatterInfo;

            if ((config.siStorage.flags !== 0) && (readout === StorageReadout.SCATTER)) {
              si = config.siStorage;
              siCurrent = -1;
            }
            else {
              si = config.siOthers[0];
              siCurrent = 0;
            }

            while (siCurrent < config.siOthersCount) {
              let doSi = true;

              if (((config.reflectionSampling === RayTracingSamplingMode.PHOTON_MAP_SAMPLING)
                || (config.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING))
                && ((si.flags & bsdfSpec) !== 0)) {
                doSi = false;
              }

              if (doSi) {
                geom = SamplerConfig.pathNodeConnect(
                  camera,
                  prevNode,
                  lightNode,
                  config.samplerConfig,
                  config.samplerConfig,
                  SampleConnectionFlags.CONNECT_EL,
                  si.flags,
                  bsdfAll,
                  dirEL
                );

                if (config.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
                  weight = 1.0;
                }
                else {
                  cl = SimpleRaytracingPathNode.multipleImportanceSampling(config.nextEventSamples * lightNode.m_pdfFromPrev);

                  if (si.DoneSomePreviousBounce(prevNode)) {
                    nrs = si.nrSamplesAfter;
                  }
                  else {
                    nrs = si.nrSamplesBefore;
                  }

                  cr = SimpleRaytracingPathNode.multipleImportanceSampling(nrs * lightNode.m_pdfFromNext);

                  if (lightNode.m_depth >= config.samplerConfig.minDepth) {
                    cr *= SimpleRaytracingPathNode.multipleImportanceSampling(lightNode.m_rrPdfFromNext);
                  }

                  weight = cl / (cl + cr);
                }

                factor = weight * geom / (lightNode.m_pdfFromPrev * config.nextEventSamples);
                radiance.scalarProductScaled(prevNode.m_bsdfEval, factor, lightNode.m_bsdfEval);

                result.add(result, radiance);
              }

              siCurrent++;
              if (siCurrent < config.siOthersCount) {
                si = config.siOthers[siCurrent];
              }
            }
          }
        }

        if (config.lightMode === RayTracingLightMode.ALL_LIGHTS) {
          lightsToDo = nes.ActivateNextUnit();
        }
        else {
          lightsToDo = false;
        }
      }
    }
    return result;
  }

  private static stochasticRaytracerGetRadiance(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    thisNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout,
    usedScatterSamples: number,
    radianceMethod: RadianceMethod,
    renderOptions: RendererConfiguration
  ): ColorRgb {
    const result = new ColorRgb();
    let radiance = new ColorRgb();
    let edfFlags = 0x01 | 0x02 | 0x04;
    const bsdfSpec = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    if (thisNode.m_rayType === PathRayType.ENVIRONMENT) {
      let weight = 1;
      let cr: number;
      let cl: number;
      let doWeight = true;

      if (thisNode.m_depth <= 1) {
        doWeight = false;
      }

      if (config.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
        doWeight = false;
      }

      if (!config.backgroundSampling) {
        doWeight = false;
      }

      if (doWeight) {
        const prevNode = thisNode.previous() as SimpleRaytracingPathNode;
        cl = config.nextEventSamples *
          config.samplerConfig.neSampler!.evalPDF(camera, prevNode, thisNode);
        cl = SimpleRaytracingPathNode.multipleImportanceSampling(cl);
        cr = usedScatterSamples * thisNode.m_pdfFromPrev;
        cr = SimpleRaytracingPathNode.multipleImportanceSampling(cr);
        weight = cr / (cr + cl);
      }

      const position = thisNode.previous()!.m_hit.getPoint();
      const bkgRad = Background.backgroundRadiance(sceneBackground, position, thisNode.m_inDirF, null);
      bkgRad.scale(weight);
      return bkgRad;
    }
    else {
      const thisEdf = thisNode.m_hit.getMaterial()!.getEdf();

      result.clear();

      if ((readout === StorageReadout.READ_NOW) && (config.siStorage.flags !== 0)) {
        if (radianceMethod.className === RadianceMethodAlgorithm.PHOTON_MAP) {
          const photonMapMethod = radianceMethod as PhotonMapRadianceMethod;
          if (config.radMode === RayTracingRadMode.STORED_PHOTON_MAP) {
            const dist2 = thisNode.m_hit.getPoint().distance2(thisNode.previous()!.m_hit.getPoint());

            if (dist2 > StochasticRaytracer.PHOTON_MAP_MIN_DIST2) {
              radiance = photonMapMethod.getNodeGRadiance(thisNode);
            }
            else {
              radiance.clear();
              readout = StorageReadout.SCATTER;
            }
          }
          else {
            radiance = photonMapMethod.getNodeGRadiance(thisNode);
          }
        }
        else {
          const u = [0.0];
          const v = [0.0];

          const position = thisNode.m_hit.getPoint();
          thisNode.m_hit.getPatch()!.uv(position, u, v);

          radiance = radianceMethod.getRadiance(
            camera,
            thisNode.m_hit.getPatch() as Patch,
            u[0],
            v[0],
            thisNode.m_inDirF,
            renderOptions
          );

          let diffEmit: ColorRgb;
          if (thisEdf === null) {
            diffEmit = new ColorRgb();
            diffEmit.clear();
          }
          else {
            const thisContextOk = [false];
            const thisContext = thisNode.m_hit.shadingContext(thisContextOk);
            diffEmit = thisEdf.phongEdfEval(
              thisContextOk[0] ? thisContext : null,
              thisNode.m_inDirF,
              BsdfComponent.BRDF_DIFFUSE_COMPONENT,
              null
            );
          }

          radiance.subtract(radiance, diffEmit);
        }

        result.add(result, radiance);
      }

      if ((config.radMode === RayTracingRadMode.STORED_PHOTON_MAP) && readout === StorageReadout.SCATTER) {
        const photonMapMethod = radianceMethod as PhotonMapRadianceMethod;
        radiance = photonMapMethod.getNodeCRadiance(thisNode);
        result.add(result, radiance);
      }

      radiance = StochasticRaytracer.srGetDirectRadiance(camera, sceneVoxelGrid, sceneBackground, thisNode, config, readout);
      result.add(result, radiance);

      radiance = StochasticRaytracer.stochasticRaytracerGetScatteredRadiance(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        thisNode,
        config,
        readout,
        radianceMethod,
        renderOptions
      );
      result.add(result, radiance);

      if (config.radMode === RayTracingRadMode.STORED_PHOTON_MAP
        && radianceMethod.className === RadianceMethodAlgorithm.PHOTON_MAP
        && (readout === StorageReadout.READ_NOW)
        && !(config.siStorage.DoneThisBounce(thisNode.previous() as SimpleRaytracingPathNode))) {
        edfFlags = 0;
      }

      if ((thisEdf !== null) && (edfFlags !== 0)) {
        let weight: number;
        let cr: number;
        let cl: number;
        let col: ColorRgb;
        let doWeight = true;

        if (thisNode.m_depth <= 1) {
          doWeight = false;
        }

        if (config.reflectionSampling === RayTracingSamplingMode.CLASSICAL_SAMPLING) {
          doWeight = false;
        }

        if (config.reflectionSampling === RayTracingSamplingMode.PHOTON_MAP_SAMPLING
          && thisNode.m_depth > 1
          && ((thisNode.previous()!.m_usedComponents & bsdfSpec) !== 0)) {
          doWeight = false;
        }

        if (doWeight) {
          const prevNode = thisNode.previous() as SimpleRaytracingPathNode;
          cl = config.nextEventSamples *
            config.samplerConfig.neSampler!.evalPDF(camera, prevNode, thisNode);
          cl = SimpleRaytracingPathNode.multipleImportanceSampling(cl);
          cr = usedScatterSamples * thisNode.m_pdfFromPrev;
          cr = SimpleRaytracingPathNode.multipleImportanceSampling(cr);

          weight = cr / (cr + cl);
        }
        else {
          weight = 1;
        }

        if (thisEdf === null) {
          col = new ColorRgb();
          col.clear();
        }
        else {
          const thisContextOk = [false];
          const thisContext = thisNode.m_hit.shadingContext(thisContextOk);
          col = thisEdf.phongEdfEval(thisContextOk[0] ? thisContext : null, thisNode.m_inDirF, edfFlags, null);
        }

        result.addScaled(result, weight, col);
      }
    }

    return result;
  }

  private static calcPixel(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    nx: number,
    ny: number,
    data: unknown
  ): ColorRgb {
    const callbackData = data as StochasticRaytracerCallbackData;
    const config = callbackData.config as StochasticRaytracingConfiguration;
    const radianceMethod = callbackData.radianceMethod as RadianceMethod;
    const renderOptions = callbackData.renderOptions as RendererConfiguration;
    const eyeNode = new SimpleRaytracingPathNode();
    const pixelNode = new SimpleRaytracingPathNode();
    const x1 = [0.0];
    const x2 = [0.0];
    let col: ColorRgb;
    const result = new ColorRgb();
    const stratified = new StratifiedSampling2D(config.samplesPerPixel);

    result.clear();

    if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
      if (config.doCorrelatedSampling !== 0) {
        StochasticRaytracer.srand48(config.baseSeed);
      }
      StochasticRaytracer.drand48();
      config.seedConfig.save(0);
    }

    config.samplerConfig.pointSampler!.sample(camera, sceneVoxelGrid, sceneBackground, null, null as any, eyeNode, 0, 0);
    (config.samplerConfig.dirSampler as PixelSampler).SetPixel(camera, nx, ny, null);

    eyeNode.attach(pixelNode);

    for (let i = 0; i < config.samplesPerPixel; i++) {
      stratified.sample(x1, x2);

      if (config.samplerConfig.dirSampler!.sample(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        null,
        eyeNode,
        pixelNode,
        x1[0],
        x2[0]
      )
        && ((pixelNode.m_rayType !== PathRayType.ENVIRONMENT) || (config.backgroundDirect))) {
        pixelNode.assignBsdfAndNormal();

        if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
          config.seedConfig.save(pixelNode.m_depth);
        }

        col = StochasticRaytracer.stochasticRaytracerGetRadiance(
          camera,
          sceneVoxelGrid,
          sceneBackground,
          pixelNode,
          config,
          config.initialReadout,
          config.samplesPerPixel,
          radianceMethod,
          renderOptions
        );

        if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
          config.seedConfig.Restore(pixelNode.m_depth);
        }

        col.scale(pixelNode.m_G / pixelNode.m_pdfFromPrev);
        result.add(result, col);
      }
    }

    const factor = ScreenBuffer.computeFluxToRadFactor(camera, nx, ny) / config.samplesPerPixel;

    result.scale(factor);
    config.screen!.add(nx, ny, result);

    if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
      config.seedConfig.Restore(0);
    }

    return result;
  }

  private static srand48(seed: number): void {
    StochasticRaytracer.random48 = new Random(seed);
  }

  private static drand48(): number {
    return StochasticRaytracer.random48.nextDouble();
  }
}
