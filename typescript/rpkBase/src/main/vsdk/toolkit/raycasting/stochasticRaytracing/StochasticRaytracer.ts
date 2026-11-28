import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { Random48 } from "../../common/Random48";
import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { StratifiedSampling2D } from "../../raycasting/common/StratifiedSampling2D";
import { SampleIndex } from "../common/SampleIndex";
import { Statistics } from "../../common/statistics/Statistics";
import { RefinementReplicas } from "../../galerkin/processing/RefinementReplicas";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { ImageOutputTarget } from "../../render/ImageOutputTarget";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongEmittanceDistributionFunction } from "../../material/PhongEmittanceDistributionFunction";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Scene } from "../../scene/Scene";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
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
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class StochasticRaytracer extends RayTracer {
  private static readonly PHOTON_MAP_MIN_DIST = 0.02;
  private static readonly PHOTON_MAP_MIN_DIST2 = StochasticRaytracer.PHOTON_MAP_MIN_DIST * StochasticRaytracer.PHOTON_MAP_MIN_DIST;
  private static override name = "Stochastic Raytracing & Final Gathers";
  private lightList: LightList | null;
  private rayTracingState: StochasticRayTracingState;

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
      out.add(scenePatches[i]!);
    }
    return out;
  }

  /**
Raytrace the current scene as seen with the current camera.
*/
  public override execute(
    ip: ImageOutputTarget | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    if (toneMapOptions === null) {
      VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "StochasticRaytracer::execute", "Tone mapping context not provided");
    }

    const config = new StochasticRaytracingConfiguration(
      scene.camera as Camera,
      this.rayTracingState,
      StochasticRaytracer.toArrayList(scene.lightSourcePatchList),
      radianceMethod,
      toneMapOptions,
      this.lightList
    );
    const callbackData = new StochasticRaytracerCallbackData(config, radianceMethod, renderOptions);

    if (this.rayTracingState.doFrameCoherent !== 0) {
      StochasticRaytracer.srand48(this.rayTracingState.baseSeed);
    }

    const parallelRequested = radianceMethod !== null && radianceMethod.className === RadianceMethodAlgorithm.RANDOM_WALK
      && this.rayTracingState.randomWalkFinalThreads > 1;
    const parallelSupported = this.rayTracingState.radMode === RayTracingRadMode.STORED_NONE
      && this.rayTracingState.reflectionSampling === RayTracingSamplingMode.BRDF_SAMPLING
      && this.rayTracingState.doFrameCoherent === 0 && this.rayTracingState.doCorrelatedSampling === 0;
    if (parallelRequested && !parallelSupported) {
      VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "execute",
        "Random Walk parallel final tracing requires BRDF sampling without stored radiance, "
        + "or coherent or correlated sampling; tracing serially");
    }

    if (parallelRequested && parallelSupported) {
      if (!this.traceRandomWalkInParallel(scene, config, callbackData)) {
        // A replica worker: its rows are in the shared memory, and its image
        // is not needed
        config.screen = null;
        this.lightList = config.rayTracingLightList;
        config.release();
        return;
      }
    }
    else if (this.rayTracingState.progressiveTracing === 0) {
      ScreenIterate.sequential(
        scene.camera as Camera,
        scene.getSceneQuery() as SceneQuery,
        scene.getSceneQueryWorkspace() as SceneQueryWorkspace,
        scene.background,
        StochasticRaytracer.calcPixel,
        callbackData,
        toneMapOptions
      );
    }
    else {
      ScreenIterate.progressive(
        scene.camera as Camera,
        scene.getSceneQuery() as SceneQuery,
        scene.getSceneQueryWorkspace() as SceneQueryWorkspace,
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

  /**
  Parallel final tracing after Random Walk (C++ traceRandomWalkInParallel):
  worker i traces the rows i, i + workers, ... and each pixel takes its
  samples from its own sequence (see SampleIndex), so the image does not
  depend on the number of workers. Only the main thread touches the screen.

  TypeScript adaptation: the workers are replicas of the program (see
  RefinementReplicas.forFinalTracing), which run the Random Walk iterations
  again before tracing; each one writes its ray count and its pixels, in the
  order it traced them, to its record in shared memory. Returns false in a
  replica worker, true in the main thread once the screen holds the image
  */
  private traceRandomWalkInParallel(
    scene: Scene,
    config: StochasticRaytracingConfiguration,
    callbackData: StochasticRaytracerCallbackData
  ): boolean {
    const camera = scene.camera as Camera;
    const width = camera.xSize;
    const height = camera.ySize;
    const requested = this.rayTracingState.randomWalkFinalThreads < height
      ? this.rayTracingState.randomWalkFinalThreads : height;
    const start = process.hrtime.bigint();
    const stats = Statistics.instance().rayTracer;
    stats.resetCounters();

    const replicas = RefinementReplicas.forFinalTracing(requested);
    const workers = replicas.getNumberOfWorkers();
    const worker = replicas.getWorker();
    if (worker === 0) {
      process.stderr.write(`Tracing Random Walk final image with ${workers} workers\n`);
    }

    // This worker's rows
    const rows = worker < height ? Math.floor((height - 1 - worker) / workers) + 1 : 0;
    const record = replicas.getRecordEvents(worker);
    RefinementReplicas.ensureLength(record, 1 + 3 * rows * width);
    const out = replicas.getRecordEvents(worker);
    const sceneQuery = scene.getSceneQuery() as SceneQuery;
    const workspace = scene.getSceneQueryWorkspace() as SceneQueryWorkspace;
    const sampleIndex = new SampleIndex();
    config.sampleIndex = sampleIndex;
    // As a C++ worker's configuration, which has no screen: the pixels go to
    // the screen only after all the workers end
    const screen = config.screen;
    config.screen = null;
    let position = 1;
    for (let y = worker; y < height; y += workers) {
      for (let x = 0; x < width; x++) {
        sampleIndex.startPixel(y * width + x, config.baseSeed);
        const pixel = StochasticRaytracer.calcPixel(camera, sceneQuery, workspace, scene.background, x, y, callbackData);
        out[position++] = pixel.r;
        out[position++] = pixel.g;
        out[position++] = pixel.b;
      }
    }
    config.sampleIndex = null;
    config.screen = screen;
    out[0] = stats.rayCount;
    replicas.arrive(null);
    if (worker !== 0) {
      return false;
    }

    let rayCount = 0;
    for (let w = 0; w < workers; w++) {
      rayCount += replicas.getRecordEvents(w)[0]!;
    }
    const color = new ColorRgbMutable();
    for (let y = 0; y < height; y++) {
      const values = replicas.getRecordEvents(y % workers);
      let index = 1 + 3 * Math.floor(y / workers) * width;
      for (let x = 0; x < width; x++) {
        color.set(values[index]!, values[index + 1]!, values[index + 2]!);
        index += 3;
        config.screen!.add(x, y, color);
      }
    }
    stats.rayCount = rayCount;
    stats.pixelCount = width * height;
    stats.totalTime = Number(process.hrtime.bigint() - start) / 1_000_000_000;
    return true;
  }

  public override saveImage(imageOutputTarget: ImageOutputTarget): boolean {
    if (imageOutputTarget !== null && this.rayTracingState.lastScreen !== null) {
      this.rayTracingState.lastScreen.sync();
      this.rayTracingState.lastScreen.writeFile(imageOutputTarget);
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
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    thisNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout,
    radianceMethod: RadianceMethod,
    renderOptions: RendererConfiguration
  ): ColorRgbMutable {
    let siCurrent: number;
    let si: ScatterInfo;

    const newNode = new SimpleRaytracingPathNode();
    thisNode.attach(newNode);

    const result = new ColorRgbMutable();
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
      si = config.siOthers[0]!;
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
        let albedo = new ColorRgbMutable();
        albedo.clear();
        if (thisNode.m_useBsdf !== null) {
          albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(), si.flags);
        }
        if (albedo.average() < VSDK.EPSILON) {
          numberOfSamples = 0;
        }
      }

      if ((numberOfSamples > 0) && (thisNode.m_depth + 1 < config.samplerConfig.maxDepth)) {
        const x1 = [0.0];
        const x2 = [0.0];
        let factor: number;
        const stratified = new StratifiedSampling2D(numberOfSamples);
        let radiance: ColorRgbMutable;
        const doRR = thisNode.m_depth >= config.samplerConfig.minDepth;

        for (let i = 0; i < numberOfSamples; i++) {
          stratified.sample(x1, x2, config.sampleIndex);

          if (
            config.samplerConfig.surfaceSampler!.sample(
              camera,
              sceneQuery,
              sceneSceneQueryWorkspace,
              sceneBackground,
              thisNode.previous(),
              thisNode,
              newNode,
              x1[0]!,
              x2[0]!,
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
                sceneQuery,
                sceneSceneQueryWorkspace,
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
                sceneQuery,
                sceneSceneQueryWorkspace,
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
        si = config.siOthers[siCurrent]!;
      }
    }

    thisNode.setNext(null);
    return result;
  }

  private static srGetDirectRadiance(
    camera: Camera,
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout
  ): ColorRgbMutable {
    const result = new ColorRgbMutable();
    const radiance = new ColorRgbMutable();
    result.clear();
    const dirEL = new Vector3Dd();

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
          stratified.sample(x1, x2, config.sampleIndex);

          if (
            config.samplerConfig.neSampler!.sample(
              camera,
              sceneQuery,
              sceneSceneQueryWorkspace,
              sceneBackground,
              prevNode.previous(),
              prevNode,
              lightNode,
              x1[0]!,
              x2[0]!,
              true,
              bsdfAll
            )
            && (RayTools.pathNodesVisible(sceneQuery, sceneSceneQueryWorkspace, prevNode, lightNode))
          ) {
            let siCurrent: number;
            let si: ScatterInfo;

            if ((config.siStorage.flags !== 0) && (readout === StorageReadout.SCATTER)) {
              si = config.siStorage;
              siCurrent = -1;
            }
            else {
              si = config.siOthers[0]!;
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
                si = config.siOthers[siCurrent]!;
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
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    thisNode: SimpleRaytracingPathNode,
    config: StochasticRaytracingConfiguration,
    readout: StorageReadout,
    usedScatterSamples: number,
    radianceMethod: RadianceMethod,
    renderOptions: RendererConfiguration
  ): ColorRgbMutable {
    const result = new ColorRgbMutable();
    let radiance = new ColorRgbMutable();
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
            const dist2 = thisNode.m_hit.getPoint().distanceSquared(thisNode.previous()!.m_hit.getPoint());

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
            u[0]!,
            v[0]!,
            thisNode.m_inDirF,
            renderOptions
          );

          let diffEmit: ColorRgbMutable;
          if (thisEdf === null) {
            diffEmit = new ColorRgbMutable();
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

      radiance = StochasticRaytracer.srGetDirectRadiance(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, config, readout);
      result.add(result, radiance);

      radiance = StochasticRaytracer.stochasticRaytracerGetScatteredRadiance(
        camera,
        sceneQuery,
        sceneSceneQueryWorkspace,
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
        let col: ColorRgbMutable;
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
          col = new ColorRgbMutable();
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
    sceneQuery: SceneQuery,
    sceneSceneQueryWorkspace: SceneQueryWorkspace,
    sceneBackground: Background | null,
    nx: number,
    ny: number,
    callbackData: StochasticRaytracerCallbackData
  ): ColorRgbMutable {
    const config = callbackData.config;
    const radianceMethod = callbackData.radianceMethod;
    const renderOptions = callbackData.renderOptions;
    const eyeNode = new SimpleRaytracingPathNode();
    const pixelNode = new SimpleRaytracingPathNode();
    const x1 = [0.0];
    const x2 = [0.0];
    let col: ColorRgbMutable;
    const result = new ColorRgbMutable();
    const stratified = new StratifiedSampling2D(config.samplesPerPixel);

    result.clear();

    if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
      if (config.doCorrelatedSampling !== 0) {
        StochasticRaytracer.srand48(config.baseSeed);
      }
      StochasticRaytracer.drand48();
      config.seedConfig.save(0);
    }

    config.samplerConfig.pointSampler!.sample(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, null, null as any, eyeNode, 0, 0);
    (config.samplerConfig.dirSampler as PixelSampler).SetPixel(camera, nx, ny, null);

    eyeNode.attach(pixelNode);

    for (let i = 0; i < config.samplesPerPixel; i++) {
      stratified.sample(x1, x2, config.sampleIndex);

      if (config.samplerConfig.dirSampler!.sample(
        camera,
        sceneQuery,
        sceneSceneQueryWorkspace,
        sceneBackground,
        null,
        eyeNode,
        pixelNode,
        x1[0]!,
        x2[0]!
      )
        && ((pixelNode.m_rayType !== PathRayType.ENVIRONMENT) || (config.backgroundDirect))) {
        pixelNode.assignBsdfAndNormal();

        if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
          config.seedConfig.save(pixelNode.m_depth);
        }

        col = StochasticRaytracer.stochasticRaytracerGetRadiance(
          camera,
          sceneQuery,
          sceneSceneQueryWorkspace,
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
    if (config.screen !== null) {
      config.screen.add(nx, ny, result);
    }

    if (config.doFrameCoherent !== 0 || config.doCorrelatedSampling !== 0) {
      config.seedConfig.Restore(0);
    }

    return result;
  }

  private static srand48(seed: number): void {
    Random48.srand48(seed);
  }

  private static drand48(): number {
    return Random48.drand48();
  }
}
