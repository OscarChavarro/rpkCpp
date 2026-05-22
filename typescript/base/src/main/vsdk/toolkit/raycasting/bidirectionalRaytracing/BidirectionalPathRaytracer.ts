import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { StratifiedSampling2D } from "../../raycasting/common/StratifiedSampling2D";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { ImageOutputHandle } from "../../io/image/ImageOutputHandle";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongEmittanceDistributionFunction } from "../../material/PhongEmittanceDistributionFunction";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { BsdfComp } from "../common/BsdfComp";
import { PathRayType } from "../common/PathRayType";
import { RayTools } from "../common/RayTools";
import { RayTracer } from "../common/RayTracer";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { BsdfSampler } from "../raytracing/BsdfSampler";
import { EyeSampler } from "../raytracing/EyeSampler";
import { PixelSampler } from "../raytracing/PixelSampler";
import { SampleConnectionFlags } from "../raytracing/SampleConnectionFlags";
import { Sampler } from "../raytracing/Sampler";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { ScreenIterate } from "../raytracing/ScreenIterate";
import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";
import { BidirectionalPathTracingConfiguration } from "./BidirectionalPathTracingConfiguration";
import { BidirectionalPathTracingState } from "./BidirectionalPathTracingState";
import { BiPath } from "./BiPath";
import { DensityBuffer } from "./DensityBuffer";
import { ImportantLightSampler } from "./ImportantLightSampler";
import { LDSpar } from "./LDSpar";
import { LeSpar } from "./LeSpar";
import { LightDirSampler } from "./LightDirSampler";
import { LightList } from "./LightList";
import { SparList } from "./SparList";
import { UniformLightSampler } from "./UniformLightSampler";

export class BidirectionalPathRaytracer extends RayTracer {
  private static readonly STRINGS_SIZE = 300;
  private static readonly name = "Bidirectional Path Tracing";

  private bidirectionalPathState: BidirectionalPathTracingState;
  private lightList: LightList | null;

  private static copyBsdfComp(source: BsdfComp | null): BsdfComp {
    const copy = new BsdfComp();

    if (source === null) {
      return copy;
    }

    const src = source.asArray();
    const dst = copy.asArray();

    for (let i = 0; i < src.length && i < dst.length; i++) {
      dst[i].set(src[i].r, src[i].g, src[i].b);
    }

    return copy;
  }

  private static assignBsdfComp(destination: BsdfComp | null, source: BsdfComp | null): void {
    if (destination === null || source === null) {
      return;
    }

    const src = source.asArray();
    const dst = destination.asArray();

    for (let i = 0; i < src.length && i < dst.length; i++) {
      dst[i].set(src[i].r, src[i].g, src[i].b);
    }
  }

  private static formatWithInt(format: string, value: number): string {
    return format.replace("%d", `${value}`);
  }

  public constructor(inBidirectionalPathState: BidirectionalPathTracingState, inLightList: LightList | null) {
    super();
    this.bidirectionalPathState = inBidirectionalPathState;
    this.lightList = inLightList;
  }

  public override defaults(): void {
    // Defaults are owned by the caller-provided BidirectionalPathTracingState instance.
  }

  public override getName(): string {
    return BidirectionalPathRaytracer.name;
  }

  public override initialize(lightPatches: ArrayList<Patch>): void {
    this.lightList = new LightList(lightPatches);
  }

  public override execute(
    ip: ImageOutputHandle | null,
    scene: Scene,
    radianceMethod: RadianceMethod,
    toneMapOptions: ToneMappingContext,
    renderOptions: RendererConfiguration
  ): void {
    void renderOptions;

    const camera = scene.camera;
    const sceneVoxelGrid = scene.voxelGrid;
    const sceneBackground = scene.background;

    if (camera === null || sceneVoxelGrid === null) {
      VsdkLogger.fatal(-1, "BidirectionalPathRaytracer::execute", "Scene camera/voxel grid not available");
      return;
    }

    const config = new BidirectionalPathTracingConfiguration();
    config.toneMapOptions = toneMapOptions;

    if (config.toneMapOptions === null) {
      VsdkLogger.fatal(-1, "BidirectionalPathRaytracer::execute", "Tone mapping context not provided");
      return;
    }

    config.baseConfig = new BidirectionalPathRaytracerConfig();
    config.baseConfig.copyFrom(this.bidirectionalPathState.baseConfig);
    config.baseConfig.totalSamples =
      this.bidirectionalPathState.baseConfig.samplesPerPixel * camera.xSize * camera.ySize;

    config.dBuffer = null;

    config.eyeConfig.pointSampler = new EyeSampler();
    config.eyeConfig.dirSampler = new PixelSampler();
    config.eyeConfig.surfaceSampler = new BsdfSampler();
    config.eyeConfig.surfaceSampler.SetComputeFromNextPdf(true);
    config.eyeConfig.surfaceSampler.SetComputeBsdfComponents(this.bidirectionalPathState.baseConfig.useSpars !== 0);

    if (this.bidirectionalPathState.baseConfig.sampleImportantLights !== 0) {
      config.eyeConfig.neSampler = new ImportantLightSampler(this.lightList);
    }
    else {
      config.eyeConfig.neSampler = new UniformLightSampler(this.lightList);
    }

    config.eyeConfig.minDepth = this.bidirectionalPathState.baseConfig.minimumPathDepth;

    if (this.bidirectionalPathState.baseConfig.maximumEyePathDepth < 1) {
      process.stderr.write("Maximum Eye Path Length too small (<1), using 1\n");
      config.eyeConfig.maxDepth = 1;
    }
    else {
      config.eyeConfig.maxDepth = this.bidirectionalPathState.baseConfig.maximumEyePathDepth;
    }

    config.lightConfig.pointSampler = new UniformLightSampler(this.lightList);
    config.lightConfig.dirSampler = new LightDirSampler();
    config.lightConfig.surfaceSampler = new BsdfSampler();
    config.lightConfig.surfaceSampler.SetComputeFromNextPdf(true);
    config.lightConfig.surfaceSampler.SetComputeBsdfComponents(this.bidirectionalPathState.baseConfig.useSpars !== 0);

    config.lightConfig.minDepth = this.bidirectionalPathState.baseConfig.minimumPathDepth;
    config.lightConfig.maxDepth = this.bidirectionalPathState.baseConfig.maximumLightPathDepth;
    config.lightConfig.neSampler = null;

    config.screen = new ScreenBuffer(null, camera, config.toneMapOptions);
    config.screen.setFactor(1.0);

    config.eyePath = null;
    config.lightPath = null;

    let leSpar: LeSpar | null = null;
    let ldSpar: LDSpar | null = null;

    if (this.bidirectionalPathState.baseConfig.useSpars !== 0) {
      const sc = config.sparConfig;

      sc.baseConfig = config.baseConfig;

      config.sparList = new SparList();

      leSpar = new LeSpar();
      ldSpar = new LDSpar();

      sc.leSpar = leSpar;
      sc.ldSpar = ldSpar;

      leSpar.init(sc, radianceMethod);
      ldSpar.init(sc, radianceMethod);

      config.sparList!.add(leSpar);
      config.sparList!.add(ldSpar);
    }

    if (this.bidirectionalPathState.saveSubsequentImages !== 0) {
      this.doBptAndSubsequentImages(camera, sceneVoxelGrid, sceneBackground, config);
    }
    else if (config.baseConfig.doDensityEstimation !== 0) {
      this.doBptDensityEstimation(camera, sceneVoxelGrid, sceneBackground, config);
    }
    else if (this.bidirectionalPathState.baseConfig.progressiveTracing === 0) {
      ScreenIterate.sequential(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        BidirectionalPathRaytracer.bpCalcPixel,
        config,
        config.toneMapOptions
      );
    }
    else {
      ScreenIterate.progressive(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        BidirectionalPathRaytracer.bpCalcPixel,
        config,
        config.toneMapOptions
      );
    }

    config.screen.render();

    if (ip !== null) {
      config.screen.writeFile(ip);
    }

    this.bidirectionalPathState.lastScreen = config.screen;

    if (this.bidirectionalPathState.baseConfig.useSpars !== 0) {
      config.sparList = null;
      leSpar = null;
      ldSpar = null;
    }

    config.eyeConfig.pointSampler = null;
    config.eyeConfig.dirSampler = null;
    config.eyeConfig.surfaceSampler = null;
    config.eyeConfig.neSampler = null;
    config.lightConfig.pointSampler = null;
    config.lightConfig.dirSampler = null;
    config.lightConfig.surfaceSampler = null;

    if (config.dBuffer !== null) {
      config.dBuffer = null;
    }

    config.baseConfig = null;
  }

  public override saveImage(imageOutputHandle: ImageOutputHandle): boolean {
    if (imageOutputHandle !== null && this.bidirectionalPathState.lastScreen !== null) {
      this.bidirectionalPathState.lastScreen.sync();
      this.bidirectionalPathState.lastScreen.writeFile(imageOutputHandle);
      return true;
    }
    return false;
  }

  public override terminate(): void {
    this.bidirectionalPathState.lastScreen = null;
    if (this.lightList !== null) {
      this.lightList = null;
    }
  }

  private static spikeCheck(color: ColorRgb): boolean {
    const colAvg = color.average();

    if (colAvg > 60000.0) {
      process.stdout.write("Spike\n");
      return true;
    }

    if (colAvg < 0.0) {
      process.stdout.write("Negative");
      return true;
    }

    return false;
  }

  private static addWithSpikeCheck(
    config: BidirectionalPathTracingConfiguration,
    path: BiPath,
    nx: number,
    ny: number,
    pixX: number,
    pixY: number,
    f: ColorRgb,
    radSample = false
  ): void {
    void path;

    if (config.baseConfig !== null && config.baseConfig.doDensityEstimation !== 0) {
      let rs: ScreenBuffer;
      let ds: ScreenBuffer;
      let db: DensityBuffer | null;
      let baseSize: number;

      if (radSample) {
        rs = config.ref2 as ScreenBuffer;
        ds = config.dest2 as ScreenBuffer;
        db = config.dBuffer2;
        baseSize = 1.5;
      }
      else {
        rs = config.ref as ScreenBuffer;
        ds = config.dest as ScreenBuffer;
        db = config.dBuffer;
        baseSize = 5.0;
      }

      if (config.deStoreHits && db !== null) {
        db.add(pixX, pixY, f);
      }
      else {
        const center = new Vector2D();
        const g = new ColorRgb();

        center.x = pixX;
        center.y = pixY;

        const factor = rs.getPixXSize() * rs.getPixYSize() * config.baseConfig.totalSamples;

        if (f.average() > Numeric.EPSILON) {
          g.scaledCopy(factor, f);

          config.kernel.varCover(
            center,
            g,
            rs,
            ds,
            config.baseConfig.totalSamples,
            config.scaleSamples,
            baseSize
          );
        }
        return;
      }
    }

    if (config.baseConfig !== null && config.baseConfig.eliminateSpikes !== 0) {
      if (!BidirectionalPathRaytracer.spikeCheck(f)) {
        (config.screen as ScreenBuffer).add(nx, ny, f);
      }
    }
    else {
      (config.screen as ScreenBuffer).add(nx, ny, f);
    }
  }

  private static handlePathX0(
    camera: Camera,
    sceneBackground: Background | null,
    config: BidirectionalPathTracingConfiguration,
    path: BiPath
  ): void {
    if (config.baseConfig === null || path.m_eyeEndNode === null) {
      return;
    }

    const endingEdf: PhongEmittanceDistributionFunction | null =
      path.m_eyeEndNode.m_hit.getMaterial() !== null
        ? path.m_eyeEndNode.m_hit.getMaterial()!.getEdf()
        : null;

    const endingInEnvironment = path.m_eyeEndNode.m_rayType === PathRayType.ENVIRONMENT;
    const hasEnvironmentBackground = endingInEnvironment && sceneBackground !== null;

    const oldBsdfEval = new ColorRgb();
    const fRad = new ColorRgb();

    let f: ColorRgb;
    let factor: number;
    let pdfLNE: number;
    let oldPdfLNE: number;
    let oldPDFLightEval: number;
    let oldPDFDirEval: number;
    let oldRRPDFLightEval: number;
    let oldRRPDFDirEval: number;

    const pdf = [1.0];
    const weight = [1.0];

    let oldBsdfComp: BsdfComp;
    let eyePrevNode: SimpleRaytracingPathNode;

    if (path.m_eyeSize > config.baseConfig.maximumPathDepth) {
      return;
    }

    if (endingEdf !== null || hasEnvironmentBackground || config.baseConfig.useSpars !== 0) {
      const eyeEndNode = path.m_eyeEndNode;

      eyePrevNode = eyeEndNode.previous() as SimpleRaytracingPathNode;

      oldBsdfEval.set(eyeEndNode.m_bsdfEval.r, eyeEndNode.m_bsdfEval.g, eyeEndNode.m_bsdfEval.b);
      oldBsdfComp = BidirectionalPathRaytracer.copyBsdfComp(eyeEndNode.m_bsdfComp);
      oldPDFLightEval = eyeEndNode.m_pdfFromNext;

      oldPDFDirEval = eyePrevNode.m_pdfFromNext;

      oldRRPDFLightEval = eyeEndNode.m_rrPdfFromNext;
      oldRRPDFDirEval = eyePrevNode.m_rrPdfFromNext;

      oldPdfLNE = path.m_pdfLNE;

      eyeEndNode.m_bsdfComp.Clear();

      if (endingEdf !== null) {
        const endingContextOk = [false];
        const endingContext = eyeEndNode.m_hit.shadingContext(endingContextOk);
        eyeEndNode.m_bsdfEval = endingEdf.phongEdfEval(
          endingContextOk[0] ? endingContext : null,
          eyeEndNode.m_inDirF,
          XxdfComponentFlag.DIFFUSE_COMPONENT
            | XxdfComponentFlag.GLOSSY_COMPONENT
            | XxdfComponentFlag.SPECULAR_COMPONENT,
          null
        );

        eyeEndNode.m_bsdfComp.Fill(
          eyeEndNode.m_bsdfEval,
          BsdfComponent.BRDF_DIFFUSE_COMPONENT
        );

        if (config.lightConfig.maxDepth > 0) {
          eyeEndNode.m_pdfFromNext = (config.lightConfig.pointSampler as Sampler).evalPDF(camera, eyeEndNode, eyeEndNode);
          eyeEndNode.m_rrPdfFromNext = 1.0;
        }
        else {
          eyeEndNode.m_pdfFromNext = 0.0;
          eyeEndNode.m_rrPdfFromNext = 0.0;
        }

        if (config.lightConfig.maxDepth > 1) {
          eyePrevNode.m_pdfFromNext = (config.lightConfig.dirSampler as Sampler).evalPDF(camera, eyeEndNode, eyePrevNode);
          eyePrevNode.m_rrPdfFromNext = 1.0;
        }
        else {
          eyePrevNode.m_pdfFromNext = 0.0;
          eyePrevNode.m_rrPdfFromNext = 0.0;
        }

        if (
          config.baseConfig.sampleImportantLights !== 0
          && config.lightConfig.maxDepth > 0
          && path.m_eyeSize > 2
        ) {
          pdfLNE = (config.eyeConfig.neSampler as Sampler).evalPDF(camera, eyePrevNode, eyeEndNode);
        }
        else {
          pdfLNE = eyeEndNode.m_pdfFromNext;
        }

        path.m_pdfLNE = pdfLNE;
      }
      else if (hasEnvironmentBackground) {
        eyeEndNode.m_bsdfEval = Background.backgroundRadiance(
          sceneBackground,
          eyeEndNode.m_hit.getPoint(),
          eyeEndNode.m_inDirF,
          null
        );
        eyeEndNode.m_bsdfComp.Fill(
          eyeEndNode.m_bsdfEval,
          BsdfComponent.BRDF_DIFFUSE_COMPONENT
        );

        eyeEndNode.m_pdfFromNext = 0.0;
        eyeEndNode.m_rrPdfFromNext = 0.0;

        eyePrevNode.m_pdfFromNext = 0.0;
        eyePrevNode.m_rrPdfFromNext = 0.0;

        path.m_pdfLNE = 0.0;
      }

      path.m_geomConnect = 1.0;

      if (config.baseConfig.useSpars !== 0) {
        f = new ColorRgb();
        (config.sparList as SparList).handlePath(config.sparConfig, path, fRad, f);
        factor = 1.0;
      }
      else {
        f = path.evalRadiance();
        factor = path.evalPdfAndWeight(config.baseConfig, pdf, weight);
      }

      factor *= config.fluxToRadFactor / config.baseConfig.samplesPerPixel;

      if (config.baseConfig.useSpars !== 0) {
        fRad.scale(factor);
        BidirectionalPathRaytracer.addWithSpikeCheck(
          config,
          path,
          config.nx,
          config.ny,
          config.xSample,
          config.ySample,
          fRad,
          true
        );
        f.scale(factor);
        BidirectionalPathRaytracer.addWithSpikeCheck(
          config,
          path,
          config.nx,
          config.ny,
          config.xSample,
          config.ySample,
          f,
          false
        );
      }
      else {
        f.scale(factor);
        BidirectionalPathRaytracer.addWithSpikeCheck(
          config,
          path,
          config.nx,
          config.ny,
          config.xSample,
          config.ySample,
          f
        );
      }

      path.m_pdfLNE = oldPdfLNE;
      eyeEndNode.m_bsdfEval.set(oldBsdfEval.r, oldBsdfEval.g, oldBsdfEval.b);
      BidirectionalPathRaytracer.assignBsdfComp(eyeEndNode.m_bsdfComp, oldBsdfComp);
      eyeEndNode.m_pdfFromNext = oldPDFLightEval;
      eyePrevNode.m_pdfFromNext = oldPDFDirEval;
      eyeEndNode.m_rrPdfFromNext = oldRRPDFLightEval;
      eyePrevNode.m_rrPdfFromNext = oldRRPDFDirEval;
    }
  }

  private static computeNeFluxEstimate(
    camera: Camera,
    config: BidirectionalPathTracingConfiguration,
    path: BiPath,
    pPdf: number[] | null,
    pWeight: number[] | null,
    fRad: ColorRgb | null
  ): ColorRgb {
    const eyeEndNode = path.m_eyeEndNode as SimpleRaytracingPathNode;
    const lightEndNode = path.m_lightEndNode as SimpleRaytracingPathNode;

    const eyePrevNode = eyeEndNode.previous();
    const lightPrevNode = lightEndNode.previous();

    const oldBsdfL = new ColorRgb();
    const oldBsdfE = new ColorRgb();

    let oldPdfLP = 0.0;
    let oldPdfEP = 0.0;
    let oldRRPdfLP = 0.0;
    let oldRRPdfEP = 0.0;

    oldBsdfL.set(lightEndNode.m_bsdfEval.r, lightEndNode.m_bsdfEval.g, lightEndNode.m_bsdfEval.b);
    const oldBsdfCompL = BidirectionalPathRaytracer.copyBsdfComp(lightEndNode.m_bsdfComp);

    oldBsdfE.set(eyeEndNode.m_bsdfEval.r, eyeEndNode.m_bsdfEval.g, eyeEndNode.m_bsdfEval.b);
    const oldBsdfCompE = BidirectionalPathRaytracer.copyBsdfComp(eyeEndNode.m_bsdfComp);

    const oldPdfL = lightEndNode.m_pdfFromNext;
    const oldRRPdfL = lightEndNode.m_rrPdfFromNext;

    if (lightPrevNode !== null) {
      oldPdfLP = lightPrevNode.m_pdfFromNext;
      oldRRPdfLP = lightPrevNode.m_rrPdfFromNext;
    }

    const oldPdfE = eyeEndNode.m_pdfFromNext;
    const oldRRPdfE = eyeEndNode.m_rrPdfFromNext;

    if (eyePrevNode !== null) {
      oldPdfEP = eyePrevNode.m_pdfFromNext;
      oldRRPdfEP = eyePrevNode.m_rrPdfFromNext;
    }

    path.m_geomConnect = SamplerConfig.pathNodeConnect(
      camera,
      eyeEndNode,
      lightEndNode,
      config.eyeConfig,
      config.lightConfig,
      SampleConnectionFlags.CONNECT_EL
        | SampleConnectionFlags.CONNECT_LE
        | SampleConnectionFlags.FILL_OTHER_PDF,
      Sampler.BSDF_ALL_COMPONENTS,
      Sampler.BSDF_ALL_COMPONENTS,
      path.m_dirEL
    );

    path.m_dirLE.scaledCopy(-1.0, path.m_dirEL);

    let f: ColorRgb;

    if (config.baseConfig !== null && config.baseConfig.useSpars !== 0) {
      const localFRad = fRad !== null ? fRad : new ColorRgb();
      f = new ColorRgb();
      (config.sparList as SparList).handlePath(config.sparConfig, path, localFRad, f);
      if (fRad === null) {
        localFRad.clear();
      }
    }
    else {
      f = path.evalRadiance();

      const factor = path.evalPdfAndWeight(config.baseConfig as BidirectionalPathRaytracerConfig, pPdf, pWeight);
      f.scale(factor);
    }

    lightEndNode.m_bsdfEval.set(oldBsdfL.r, oldBsdfL.g, oldBsdfL.b);
    BidirectionalPathRaytracer.assignBsdfComp(lightEndNode.m_bsdfComp, oldBsdfCompL);

    eyeEndNode.m_bsdfEval.set(oldBsdfE.r, oldBsdfE.g, oldBsdfE.b);
    BidirectionalPathRaytracer.assignBsdfComp(eyeEndNode.m_bsdfComp, oldBsdfCompE);

    lightEndNode.m_pdfFromNext = oldPdfL;
    lightEndNode.m_rrPdfFromNext = oldRRPdfL;

    if (lightPrevNode !== null) {
      lightPrevNode.m_pdfFromNext = oldPdfLP;
      lightPrevNode.m_rrPdfFromNext = oldRRPdfLP;
    }

    eyeEndNode.m_pdfFromNext = oldPdfE;
    eyeEndNode.m_rrPdfFromNext = oldRRPdfE;

    if (eyePrevNode !== null) {
      eyePrevNode.m_pdfFromNext = oldPdfEP;
      eyePrevNode.m_rrPdfFromNext = oldRRPdfEP;
    }

    return f;
  }

  private static handlePathXx(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: BidirectionalPathTracingConfiguration,
    path: BiPath
  ): void {
    if (
      config.baseConfig === null
      || path.m_eyeEndNode === null
      || path.m_lightEndNode === null
    ) {
      return;
    }

    let oldPdfLNE = 0.0;
    const pdf = [0.0];
    const weight = [0.0];

    const fRad = new ColorRgb();

    const newLightNode = new SimpleRaytracingPathNode();
    let oldLightPath: SimpleRaytracingPathNode | null = null;
    let oldLightEndNode: SimpleRaytracingPathNode | null = null;
    let oldLightSize = 0;

    if ((path.m_eyeSize + path.m_lightSize) > config.baseConfig.maximumPathDepth) {
      return;
    }

    const doLNE = path.m_lightSize === 1 && config.baseConfig.sampleImportantLights !== 0;

    if (doLNE) {
      oldLightPath = path.m_lightPath;
      oldLightSize = path.m_lightSize;
      oldLightEndNode = path.m_lightEndNode;

      path.m_lightPath = newLightNode;

      newLightNode.m_pdfFromPrev = 0.0;
      newLightNode.m_pdfFromNext = 0.0;

      if (!(config.eyeConfig.neSampler as Sampler).sample(
        camera,
        sceneWorldVoxelGrid,
        sceneBackground,
        null,
        path.m_eyeEndNode,
        newLightNode,
        globalThis.Math.random(),
        globalThis.Math.random()
      )) {
        path.m_lightPath = oldLightPath;
        return;
      }

      oldPdfLNE = path.m_pdfLNE;
      path.m_pdfLNE = newLightNode.m_pdfFromPrev;
      newLightNode.m_pdfFromPrev = (config.lightConfig.pointSampler as Sampler).evalPDF(camera, newLightNode, newLightNode);

      path.m_lightEndNode = newLightNode;
    }

    if (RayTools.pathNodesVisible(sceneWorldVoxelGrid, path.m_eyeEndNode, path.m_lightEndNode)) {
      const f = BidirectionalPathRaytracer.computeNeFluxEstimate(camera, config, path, pdf, weight, fRad);

      const factor = config.fluxToRadFactor / config.baseConfig.samplesPerPixel;
      f.scale(factor);
      BidirectionalPathRaytracer.addWithSpikeCheck(
        config,
        path,
        config.nx,
        config.ny,
        config.xSample,
        config.ySample,
        f
      );

      if (config.baseConfig.useSpars !== 0) {
        fRad.scale(factor);
        BidirectionalPathRaytracer.addWithSpikeCheck(
          config,
          path,
          config.nx,
          config.ny,
          config.xSample,
          config.ySample,
          fRad,
          true
        );
      }
    }

    if (doLNE) {
      path.m_lightPath = oldLightPath;
      path.m_lightSize = oldLightSize;
      path.m_lightEndNode = oldLightEndNode;
      path.m_pdfLNE = oldPdfLNE;
    }
  }

  private static handlePath1X(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    config: BidirectionalPathTracingConfiguration,
    path: BiPath
  ): void {
    if (
      config.baseConfig === null
      || config.lightPath === null
      || path.m_eyeEndNode === null
      || path.m_lightEndNode === null
      || config.screen === null
    ) {
      return;
    }

    if ((path.m_eyeSize + path.m_lightSize) > config.baseConfig.maximumPathDepth) {
      return;
    }

    const oldPdfLNE = path.m_pdfLNE;
    path.m_pdfLNE = config.lightPath.m_pdfFromPrev;

    const pixX = [0.0];
    const pixY = [0.0];

    if (RayTools.eyeNodeVisible(
      camera,
      sceneVoxelGrid,
      path.m_eyeEndNode,
      path.m_lightEndNode,
      pixX,
      pixY
    )) {
      const nx = [0];
      const ny = [0];

      const fRad = new ColorRgb();
      const pdf = [0.0];
      const weight = [0.0];

      const f = BidirectionalPathRaytracer.computeNeFluxEstimate(camera, config, path, pdf, weight, fRad);

      config.screen.getPixel(pixX[0], pixY[0], nx, ny);

      const factor = ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0]) / config.baseConfig.totalSamples;
      f.scale(factor);

      BidirectionalPathRaytracer.addWithSpikeCheck(config, path, nx[0], ny[0], pixX[0], pixY[0], f);

      if (config.baseConfig.useSpars !== 0) {
        fRad.scale(factor);
        BidirectionalPathRaytracer.addWithSpikeCheck(config, path, nx[0], ny[0], pixX[0], pixY[0], fRad, true);
      }
    }

    path.m_pdfLNE = oldPdfLNE;
  }

  private static bpCombinePaths(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: BidirectionalPathTracingConfiguration
  ): void {
    if (config.eyePath === null || config.baseConfig === null) {
      return;
    }

    let eyeSize: number;
    let lightSize: number;
    let eyeSubPathDone: boolean;
    let lightSubPathDone: boolean;

    let eyeEndNode: SimpleRaytracingPathNode;
    let lightEndNode: SimpleRaytracingPathNode | null;

    const eyePath = config.eyePath;
    const lightPath = config.lightPath;

    const path = new BiPath();

    if (config.lightPath !== null) {
      if (config.baseConfig.sampleImportantLights !== 0 && lightPath!.next() !== null) {
        config.pdfLNE = (config.eyeConfig.neSampler as Sampler).evalPDF(
          camera,
          lightPath!.next() as SimpleRaytracingPathNode,
          lightPath as SimpleRaytracingPathNode
        );
      }
      else {
        config.pdfLNE = lightPath!.m_pdfFromPrev;
      }
    }
    else {
      config.pdfLNE = 0.0;
    }

    path.m_eyePath = eyePath;
    path.m_lightPath = lightPath;
    path.m_pdfLNE = config.pdfLNE;

    eyeSubPathDone = false;
    eyeSize = 1;
    eyeEndNode = eyePath;

    while (!eyeSubPathDone) {
      if (eyeSize > 1) {
        path.m_eyeSize = eyeSize;
        path.m_eyeEndNode = eyeEndNode;
        path.m_lightSize = 0;
        path.m_lightEndNode = null;

        BidirectionalPathRaytracer.handlePathX0(camera, sceneBackground, config, path);
      }

      lightSubPathDone = lightPath === null;
      lightSize = 1;
      lightEndNode = lightPath;

      while (!lightSubPathDone) {
        path.m_eyeSize = eyeSize;
        path.m_eyeEndNode = eyeEndNode;
        path.m_lightSize = lightSize;
        path.m_lightEndNode = lightEndNode;

        if (eyeSize > 1) {
          BidirectionalPathRaytracer.handlePathXx(camera, sceneVoxelGrid, sceneBackground, config, path);
        }
        else {
          BidirectionalPathRaytracer.handlePath1X(camera, sceneVoxelGrid, config, path);
        }

        if ((lightEndNode as SimpleRaytracingPathNode).ends()) {
          lightSubPathDone = true;
        }
        else {
          lightSize++;
          lightEndNode = (lightEndNode as SimpleRaytracingPathNode).next();
        }
      }

      if (eyeEndNode.ends()) {
        eyeSubPathDone = true;
      }
      else {
        eyeSize++;
        eyeEndNode = eyeEndNode.next() as SimpleRaytracingPathNode;
      }
    }
  }

  private static bpCalcPixel(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    nx: number,
    ny: number,
    data: unknown
  ): ColorRgb {
    const config = data as BidirectionalPathTracingConfiguration;

    const x1 = [0.0];
    const x2 = [0.0];

    const result = new ColorRgb();
    result.clear();

    if (config.baseConfig === null || config.screen === null) {
      return result;
    }

    const stratifiedSampling2D = new StratifiedSampling2D(config.baseConfig.samplesPerPixel);

    if (config.eyePath === null) {
      config.eyePath = new SimpleRaytracingPathNode();
    }

    (config.eyeConfig.pointSampler as Sampler).sample(
      camera,
      sceneVoxelGrid,
      sceneBackground,
      null,
      null,
      config.eyePath,
      0,
      0
    );

    (config.eyeConfig.dirSampler as PixelSampler).SetPixel(camera, nx, ny, null);

    let pixNode = config.eyePath.next();

    if (pixNode === null) {
      pixNode = new SimpleRaytracingPathNode();
      config.eyePath.attach(pixNode);
    }

    let nextNode = pixNode.next();
    if (nextNode === null) {
      nextNode = new SimpleRaytracingPathNode();
      pixNode.attach(nextNode);
    }

    config.nx = nx;
    config.ny = ny;
    config.fluxToRadFactor = ScreenBuffer.computeFluxToRadFactor(camera, nx, ny);

    for (let i = 0; i < config.baseConfig.samplesPerPixel; i++) {
      if (config.eyeConfig.maxDepth > 1) {
        stratifiedSampling2D.sample(x1, x2);

        config.eyePath.m_rayType = PathRayType.STARTS;

        const tmpVec2D = config.screen.getPixelCenter(globalThis.Math.trunc(x1[0]), globalThis.Math.trunc(x2[0]));
        config.xSample = tmpVec2D.x;
        config.ySample = tmpVec2D.y;

        if ((config.eyeConfig.dirSampler as Sampler).sample(
          camera,
          sceneVoxelGrid,
          sceneBackground,
          null,
          config.eyePath,
          pixNode,
          x1[0],
          x2[0]
        )) {
          pixNode.assignBsdfAndNormal();
          config.eyeConfig.tracePathDefault(camera, sceneVoxelGrid, sceneBackground, nextNode);
        }
      }
      else {
        config.eyePath.m_rayType = PathRayType.STOPS;
      }

      if (config.lightConfig.maxDepth > 0) {
        config.lightPath = config.lightConfig.tracePathDefault(camera, sceneVoxelGrid, sceneBackground, config.lightPath);
      }
      else {
        config.lightPath = null;
      }

      BidirectionalPathRaytracer.bpCombinePaths(camera, sceneVoxelGrid, sceneBackground, config);
    }

    let src: ColorRgb;
    if (config.baseConfig.doDensityEstimation !== 0) {
      if (config.dBuffer !== null) {
        src = config.screen.get(nx, ny);
      }
      else {
        src = (config.dest as ScreenBuffer).get(nx, ny);
      }
    }
    else {
      src = config.screen.get(nx, ny);
    }

    result.set(src.r, src.g, src.b);
    return result;
  }

  private doBptAndSubsequentImages(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: BidirectionalPathTracingConfiguration
  ): void {
    if (config.baseConfig === null || config.toneMapOptions === null || config.screen === null) {
      return;
    }

    let nrIterations = globalThis.Math.trunc(
      globalThis.Math.log(this.bidirectionalPathState.baseConfig.samplesPerPixel)
      / globalThis.Math.log(2.0)
    );
    const maxSamples = globalThis.Math.pow(2.0, nrIterations);

    nrIterations += 1;

    process.stdout.write(
      `nrIter ${nrIterations}, maxSamples ${globalThis.Math.trunc(maxSamples)}, origSamples ${this.bidirectionalPathState.baseConfig.samplesPerPixel}\n`
    );

    process.stdout.write(`Base name '${this.bidirectionalPathState.baseFilename}'\n`);

    const baseFilename = this.bidirectionalPathState.baseFilename ?? "";
    const lastOcc = baseFilename.lastIndexOf(".");

    let format1: string;
    let format2: string;

    if (lastOcc < 0) {
      format1 = `${baseFilename}%d.tif`;
      format2 = `${baseFilename}%d.ppm.gz`;
    }
    else {
      const prefix = baseFilename.substring(0, lastOcc);
      const suffix = baseFilename.substring(lastOcc);
      format1 = `${prefix}%d${suffix}`;
      format2 = "";
    }

    process.stdout.write(`Format 1 '${format1}'\n`);
    process.stdout.write(`Format 2 '${format2}'\n`);

    let currentSamples = 1;
    let totalSamples = 1;

    for (let i = 0; i < nrIterations; i++) {
      if (i > 0) {
        config.screen.scaleRadiance(0.5);
        config.screen.setAddScaleFactor(0.5);
      }

      config.baseConfig.samplesPerPixel = currentSamples;
      config.baseConfig.totalSamples = currentSamples * camera.xSize * camera.ySize;

      ScreenIterate.sequential(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        BidirectionalPathRaytracer.bpCalcPixel,
        config,
        config.toneMapOptions
      );

      config.screen.render();

      if (format1.length > 0) {
        const filename = BidirectionalPathRaytracer.formatWithInt(format1, totalSamples);
        config.screen.writeFile(filename);
      }

      if (format2.length > 0) {
        const filename = BidirectionalPathRaytracer.formatWithInt(format2, totalSamples);
        config.screen.writeFile(filename);
      }

      if (i > 0) {
        currentSamples *= 2;
      }

      totalSamples += currentSamples;
    }
  }

  private doBptDensityEstimation(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: BidirectionalPathTracingConfiguration
  ): void {
    if (config.baseConfig === null || config.toneMapOptions === null) {
      return;
    }

    let fileName = "";

    config.ref = new ScreenBuffer(null, camera, config.toneMapOptions);
    config.dest = new ScreenBuffer(null, camera, config.toneMapOptions);

    config.ref.setFactor(1.0);
    config.dest.setFactor(1.0);

    config.dBuffer = new DensityBuffer(config.ref, config.baseConfig);

    if (config.baseConfig.useSpars !== 0) {
      config.ref2 = new ScreenBuffer(null, camera, config.toneMapOptions);
      config.dest2 = new ScreenBuffer(null, camera, config.toneMapOptions);

      config.ref2.setFactor(1.0);
      config.dest2.setFactor(1.0);

      config.dBuffer2 = new DensityBuffer(config.ref2, config.baseConfig);
    }
    else {
      config.ref2 = null;
      config.dest2 = null;
      config.dBuffer2 = null;
    }

    config.baseConfig.samplesPerPixel = 1;
    config.baseConfig.totalSamples =
      config.baseConfig.samplesPerPixel * config.ref.getHRes() * config.ref.getVRes();

    config.deStoreHits = true;

    ScreenIterate.sequential(
      camera,
      sceneVoxelGrid,
      sceneBackground,
      BidirectionalPathRaytracer.bpCalcPixel,
      config,
      config.toneMapOptions
    );

    config.dBuffer.reconstruct();
    config.ref.render();

    if (config.dBuffer2 !== null) {
      config.dBuffer2.reconstruct();
      config.ref2!.render();
    }

    config.dBuffer.reconstructVariable(config.dest, 5.0);
    config.dest.render();

    if (config.dBuffer2 !== null) {
      config.dBuffer2.reconstructVariable(config.dest2 as ScreenBuffer, 1.5);
      config.dest2!.render();
    }

    config.dBuffer = null;

    if (config.dBuffer2 !== null) {
      config.dBuffer2 = null;
    }

    config.deStoreHits = false;

    const numberOfIterations = globalThis.Math.floor(
      globalThis.Math.log(this.bidirectionalPathState.baseConfig.samplesPerPixel)
      / globalThis.Math.log(2.0)
    );

    const maxSamples = globalThis.Math.pow(2.0, numberOfIterations);

    process.stdout.write(`Doing ${numberOfIterations} iterations, thus ${globalThis.Math.trunc(maxSamples)} samples per pixel\n`);

    const oldSPP = config.baseConfig.samplesPerPixel;
    let newSPP = oldSPP;
    let oldTotalSPP = oldSPP;
    let newTotalSPP = oldTotalSPP + newSPP;

    for (let i = 1; i <= numberOfIterations; i++) {
      process.stdout.write(`Doing run with ${newSPP} samples, ${oldTotalSPP} samples already done\n`);

      config.ref.copy(config.dest, camera);

      if (config.ref2 !== null) {
        config.ref2.copy(config.dest2 as ScreenBuffer, camera);
      }

      config.dest.scaleRadiance(oldTotalSPP / newTotalSPP);
      config.dest.setAddScaleFactor(newSPP / newTotalSPP);

      if (config.dest2 !== null) {
        config.dest2.scaleRadiance(oldTotalSPP / newTotalSPP);
        config.dest2.setAddScaleFactor(newSPP / newTotalSPP);
      }

      config.baseConfig.samplesPerPixel = newSPP;
      config.baseConfig.totalSamples =
        config.baseConfig.samplesPerPixel * config.ref.getHRes() * config.ref.getVRes();

      config.scaleSamples = newTotalSPP;

      ScreenIterate.sequential(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        BidirectionalPathRaytracer.bpCalcPixel,
        config,
        config.toneMapOptions
      );

      config.dest.render();
      fileName = `deScreen${newTotalSPP}.ppm.gz`;

      if (config.dest2 !== null) {
        config.dest2.render();
        fileName = `de2Screen${newTotalSPP}.ppm.gz`;

        (config.screen as ScreenBuffer).merge(config.dest, config.dest2, camera);
        (config.screen as ScreenBuffer).render();
        fileName = `deMRGScreen${newTotalSPP}.ppm.gz`;
      }
      else {
        (config.screen as ScreenBuffer).copy(config.dest, camera);
      }

      newSPP = newSPP * 2;
      oldTotalSPP = newTotalSPP;
      newTotalSPP = oldTotalSPP + newSPP;
    }

    config.dest = null;
    config.ref = null;

    if (config.ref2 !== null) {
      config.ref2 = null;
    }

    if (config.dest2 !== null) {
      config.dest2 = null;
    }

    if (fileName.length > BidirectionalPathRaytracer.STRINGS_SIZE) {
      fileName = fileName.substring(0, BidirectionalPathRaytracer.STRINGS_SIZE);
    }
  }
}
