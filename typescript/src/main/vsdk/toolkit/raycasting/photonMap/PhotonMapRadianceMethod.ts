import { OutputStream } from "../../../../java/io/OutputStream";
import { StringBuilder } from "../../../../java/lang/StringBuilder";
import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { RenderOptions } from "../../common/RenderOptions";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Element } from "../../skin/Element";
import { Patch } from "../../skin/Patch";
import { RayHit } from "../../skin/RayHit";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { BiPath } from "../bidirectionalRaytracing/BiPath";
import { LightDirSampler } from "../bidirectionalRaytracing/LightDirSampler";
import { LightList } from "../bidirectionalRaytracing/LightList";
import { UniformLightSampler } from "../bidirectionalRaytracing/UniformLightSampler";
import { BsdfComp } from "../common/BsdfComp";
import { RayTools } from "../common/RayTools";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { BsdfSampler } from "../raytracing/BsdfSampler";
import { EyeSampler } from "../raytracing/EyeSampler";
import { SampleConnectionFlags } from "../raytracing/SampleConnectionFlags";
import { Sampler } from "../raytracing/Sampler";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { SurfaceSampler } from "../raytracing/SurfaceSampler";
import { ImportanceMap } from "./ImportanceMap";
import { Photon } from "./Photon";
import { PhotonFlags } from "./PhotonFlags";
import { PhotonMap } from "./PhotonMap";
import { PhotonMapConfig } from "./PhotonMapConfig";
import { PhotonMapDensityControlOption } from "./PhotonMapDensityControlOption";
import { PhotonMapImportance } from "./PhotonMapImportance";
import { PhotonMapSampler } from "./PhotonMapSampler";
import { PhotonMapState } from "./PhotonMapState";
import { RadiosityReturnOption } from "./RadiosityReturnOption";
import { ScreenSampler } from "./ScreenSampler";

const util = require("node:util");

// To adjust photonMapGetRadiance returns
export class PhotonMapRadianceMethod extends RadianceMethod {
  private static readonly STRING_LENGTH = 1000;
  private static doingLocalRayCasting = false;

  private readonly photonMapState: PhotonMapState;
  private readonly photonMapConfig: PhotonMapConfig;

  private static appendStatsText(buffer: StringBuilder, offset: number[], format: string, ...args: unknown[]): void {
    if (offset[0] >= PhotonMapRadianceMethod.STRING_LENGTH - 1) {
      return;
    }

    let text: string;
    try {
      text = util.format(format, ...args);
    }
    catch (_e) {
      text = format;
    }

    const available = PhotonMapRadianceMethod.STRING_LENGTH - offset[0];
    if (available <= 0) {
      return;
    }

    if (text.length >= available) {
      buffer.append(text, available - 1);
      offset[0] = PhotonMapRadianceMethod.STRING_LENGTH - 1;
    }
    else {
      buffer.append(text);
      offset[0] += text.length;
    }
  }

  public constructor(inPhotonMapState: PhotonMapState, inPhotonMapConfig: PhotonMapConfig) {
    super();
    this.photonMapState = inPhotonMapState;
    this.photonMapConfig = inPhotonMapConfig;

    this.photonMapState.setDefaults();
    this.className = RadianceMethodAlgorithm.PHOTON_MAP;
  }

  public override getRadianceMethodName(): string {
    return "Photon map";
  }

  public override parseOptions(argc: number[], argv: string[]): void {
    void argc;
    void argv;
  }

  public override writeVRML(
    camera: Camera,
    outputStream: OutputStream,
    renderOptions: RenderOptions
  ): void {
    void camera;
    void outputStream;
    void renderOptions;
  }

  /**
For counting how much CPU time was used for the computations
*/
  private photonMapRadiosityUpdateCpuSecs(): void {
    const t = Number(process.hrtime.bigint());
    this.photonMapState.cpuSecs += (t - this.photonMapState.lastClock) / 1_000_000_000.0;
    this.photonMapState.lastClock = t;
  }

  public override createPatchData(patch: Patch): Element | null {
    patch.radianceData = null;
    return patch.radianceData;
  }

  public override destroyPatchData(patch: Patch): void {
    patch.radianceData = null;
  }

  private photonMapChooseSurfaceSampler(): SurfaceSampler {
    if (this.photonMapState.usePhotonMapSampler !== 0) {
      return new PhotonMapSampler();
    }
    return new BsdfSampler();
  }

  /**
Initializes the computations for the current scene (if any)
*/
  public override initialize(scene: Scene, toneMapOptions: ToneMappingContext): void {
    process.stderr.write("Photon map activated\n");

    if (toneMapOptions === null || scene.camera === null) {
      VsdkError.fatal(-1, "PhotonMapRadianceMethod::initialize", "Tone mapping context or camera not provided");
      return;
    }

    this.photonMapState.lastClock = Number(process.hrtime.bigint());
    this.photonMapState.cpuSecs = 0.0;
    this.photonMapState.gIterationNumber = 0;
    this.photonMapState.cIterationNumber = 0;
    this.photonMapState.i_iteration_nr = 0;
    this.photonMapState.iterationNumber = 0;
    this.photonMapState.runStopNumber = 0;
    this.photonMapState.totalGPaths = 0;
    this.photonMapState.totalCPaths = 0;
    this.photonMapState.totalIPaths = 0;

    this.photonMapConfig.screen = new ScreenBuffer(null, scene.camera, toneMapOptions);

    const lightPatches = new ArrayList<Patch>();
    for (let i = 0; scene.lightSourcePatchList !== null && i < scene.lightSourcePatchList.length; i++) {
      lightPatches.add(scene.lightSourcePatchList[i]);
    }
    this.photonMapConfig.lightList = new LightList(lightPatches);

    this.photonMapConfig.lightConfig.releaseVars();
    this.photonMapConfig.eyeConfig.releaseVars();

    let cfg = this.photonMapConfig.eyeConfig;

    cfg.pointSampler = new EyeSampler();
    cfg.dirSampler = new ScreenSampler();
    cfg.surfaceSampler = this.photonMapChooseSurfaceSampler();
    cfg.surfaceSampler.SetComputeFromNextPdf(false);
    cfg.neSampler = null;

    cfg.minDepth = 1;
    cfg.maxDepth = 1;

    cfg = this.photonMapConfig.lightConfig;

    cfg.pointSampler = new UniformLightSampler(this.photonMapConfig.lightList);
    cfg.dirSampler = new LightDirSampler();
    cfg.surfaceSampler = this.photonMapChooseSurfaceSampler();
    cfg.surfaceSampler.SetComputeFromNextPdf(false);

    cfg.minDepth = this.photonMapState.minimumLightPathDepth;
    cfg.maxDepth = this.photonMapState.maximumLightPathDepth;

    Statistics.instance().rayTracer.rayCount = 0;

    this.photonMapConfig.map = new PhotonMap(
      this.photonMapState,
      [this.photonMapState.reconGPhotons],
      this.photonMapState.precomputeGIrradiance !== 0
    );

    this.photonMapConfig.importanceMap = new ImportanceMap(
      this.photonMapState,
      [this.photonMapState.reconIPhotons],
      [this.photonMapState.gImpScale]
    );

    this.photonMapConfig.importanceCMap = new ImportanceMap(
      this.photonMapState,
      [this.photonMapState.reconIPhotons],
      [this.photonMapState.cImpScale]
    );

    this.photonMapConfig.causticMap = new PhotonMap(this.photonMapState, [this.photonMapState.reconCPhotons]);
  }

  private static cloneColor(c: ColorRgb): ColorRgb {
    return new ColorRgb(c.r, c.g, c.b);
  }

  private static cloneBsdfComp(comp: BsdfComp): BsdfComp {
    const copy = new BsdfComp();
    for (let i = 0; i < copy.comp.length; i++) {
      copy.comp[i].set(comp.comp[i].r, comp.comp[i].g, comp.comp[i].b);
    }
    return copy;
  }

  /**
Adapted from bi-directional path, this is a bit overkill for here
*/
  private photonMapDoComputePixelFluxEstimate(
    camera: Camera,
    config: PhotonMapConfig,
    radianceMethod: RadianceMethod
  ): ColorRgb {
    void radianceMethod;
    const bp = config.biPath;
    const eyeEndNode = bp.m_eyeEndNode as SimpleRaytracingPathNode;
    const lightEndNode = bp.m_lightEndNode as SimpleRaytracingPathNode;
    const eyePrevNode = eyeEndNode.previous();
    const lightPrevNode = lightEndNode.previous();

    const oldBsdfL = PhotonMapRadianceMethod.cloneColor(lightEndNode.m_bsdfEval);
    const oldBsdfCompL = PhotonMapRadianceMethod.cloneBsdfComp(lightEndNode.m_bsdfComp);

    const oldBsdfE = PhotonMapRadianceMethod.cloneColor(eyeEndNode.m_bsdfEval);
    const oldBsdfCompE = PhotonMapRadianceMethod.cloneBsdfComp(eyeEndNode.m_bsdfComp);

    const oldPdfL = lightEndNode.m_pdfFromNext;
    const oldRRPdfL = lightEndNode.m_rrPdfFromNext;

    let oldPdfLP = 0.0;
    let oldRRPdfLP = 0.0;
    if (lightPrevNode !== null) {
      oldPdfLP = lightPrevNode.m_pdfFromNext;
      oldRRPdfLP = lightPrevNode.m_rrPdfFromNext;
    }

    const oldPdfE = eyeEndNode.m_pdfFromNext;
    const oldRRPdfE = eyeEndNode.m_rrPdfFromNext;

    let oldPdfEP = 0.0;
    let oldRRPdfEP = 0.0;
    if (eyePrevNode !== null) {
      oldPdfEP = eyePrevNode.m_pdfFromNext;
      oldRRPdfEP = eyePrevNode.m_rrPdfFromNext;
    }

    bp.m_geomConnect = SamplerConfig.pathNodeConnect(
      camera,
      eyeEndNode,
      lightEndNode,
      config.eyeConfig,
      config.lightConfig,
      SampleConnectionFlags.CONNECT_EL | SampleConnectionFlags.CONNECT_LE,
      Sampler.BSDF_ALL_COMPONENTS,
      Sampler.BSDF_ALL_COMPONENTS,
      bp.m_dirEL
    );

    bp.m_dirLE.scaledCopy(-1.0, bp.m_dirEL);

    const f = bp.evalRadiance();
    const factor = 1.0 / bp.evalPdfAcc();
    f.scale(factor);

    lightEndNode.m_bsdfEval.set(oldBsdfL.r, oldBsdfL.g, oldBsdfL.b);
    lightEndNode.m_bsdfComp = oldBsdfCompL;

    eyeEndNode.m_bsdfEval.set(oldBsdfE.r, oldBsdfE.g, oldBsdfE.b);
    eyeEndNode.m_bsdfComp = oldBsdfCompE;

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

  /**
Test next event estimator to the screen. The result is standard
particle tracing, although constructing global & caustic together
does not give correct display
*/
  private photonMapDoScreenNEE(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    config: PhotonMapConfig,
    radianceMethod: RadianceMethod
  ): void {
    if (config.currentMap === config.importanceMap) {
      return;
    }
    if (config.screen === null) {
      return;
    }

    const bp = config.biPath;
    if (bp.m_eyeEndNode === null || bp.m_lightEndNode === null) {
      return;
    }

    const nx = [0];
    const ny = [0];
    const pixX = [0.0];
    const pixY = [0.0];

    if (RayTools.eyeNodeVisible(
      camera,
      sceneWorldVoxelGrid,
      bp.m_eyeEndNode,
      bp.m_lightEndNode,
      pixX,
      pixY
    )) {
      const f = this.photonMapDoComputePixelFluxEstimate(camera, config, radianceMethod);
      config.screen.getPixel(pixX[0], pixY[0], nx, ny);

      let factor: number;
      if (config.currentMap === config.map) {
        factor = ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0]) / this.photonMapState.totalGPaths;
      }
      else {
        factor = ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0]) / this.photonMapState.totalCPaths;
      }

      f.scale(factor);
      config.screen.add(nx[0], ny[0], f);
    }
  }

  /**
Store a photon. Some acceptance tests are performed first
*/
  private photonMapDoPhotonStore(
    camera: Camera,
    node: SimpleRaytracingPathNode,
    power: ColorRgb
  ): boolean {
    if (node.m_hit.getPatch() !== null && node.m_hit.getPatch()!.material !== null) {
      const bsdf = node.m_hit.getPatch()!.material!.getBsdf();

      if (!PhotonMap.zeroAlbedo(
        bsdf,
        node.m_hit,
        BsdfComponent.BRDF_DIFFUSE_COMPONENT
          | BsdfComponent.BTDF_DIFFUSE_COMPONENT
          | BsdfComponent.BRDF_GLOSSY_COMPONENT
          | BsdfComponent.BTDF_GLOSSY_COMPONENT
      )) {
        const photon = new Photon(node.m_hit.getPoint(), power, node.m_inDirF);

        let flags = 0;
        if (node.m_depth === 1) {
          flags |= PhotonFlags.DIRECT_LIGHT_PHOTON;
        }

        if (this.photonMapState.densityControl === PhotonMapDensityControlOption.NO_DENSITY_CONTROL) {
          if (this.photonMapConfig.currentMap === null) {
            return false;
          }
          return this.photonMapConfig.currentMap.addPhoton(photon, node.m_hit.getNormal(), flags);
        }

        let reqDensity: number;
        if (this.photonMapState.densityControl === PhotonMapDensityControlOption.CONSTANT_RD) {
          reqDensity = this.photonMapState.constantRD;
        }
        else {
          if (this.photonMapConfig.currentImpMap === null) {
            return false;
          }
          reqDensity = this.photonMapConfig.currentImpMap.getRequiredDensity(
            camera,
            node.m_hit.getPoint(),
            node.m_hit.getNormal()
          );
        }

        if (this.photonMapConfig.currentMap === null) {
          return false;
        }
        return this.photonMapConfig.currentMap.DC_AddPhoton(photon, node.m_hit, reqDensity, flags);
      }
    }
    return false;
  }

  /**
Handle one path : store at all end positions and for testing, connect to the eye
*/
  private photonMapHandlePath(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    config: PhotonMapConfig,
    radianceMethod: RadianceMethod
  ): void {
    const bp: BiPath = config.biPath;
    const accPower = new ColorRgb();

    bp.m_lightSize = 1;
    let currentNode = bp.m_lightPath;
    if (currentNode === null || bp.m_eyePath === null) {
      return;
    }

    bp.m_eyeSize = 1;
    bp.m_eyeEndNode = bp.m_eyePath;
    bp.m_geomConnect = 1.0;

    let lDone = false;
    accPower.setMonochrome(1.0);

    while (!lDone && currentNode !== null) {
      const factor = currentNode.m_G / currentNode.m_pdfFromPrev;
      accPower.scale(factor);

      if (config.currentMap === config.map) {
        if (bp.m_lightSize > 1 && this.photonMapDoPhotonStore(camera, currentNode, accPower)) {
          bp.m_lightEndNode = currentNode;
          this.photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
        }
      }
      else {
        if (bp.m_lightSize > 2 && this.photonMapDoPhotonStore(camera, currentNode, accPower)) {
          bp.m_lightEndNode = currentNode;
          this.photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
        }
      }

      if (!currentNode.ends()) {
        accPower.selfScalarProduct(currentNode.m_bsdfEval);

        currentNode = currentNode.next();
        bp.m_lightSize++;
      }
      else {
        lDone = true;
      }
    }
  }

  private photonMapTracePath(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: PhotonMapConfig,
    bsdfFlags: number
  ): void {
    config.biPath.m_eyePath = config.eyeConfig.tracePathDefault(camera, sceneVoxelGrid, sceneBackground, config.biPath.m_eyePath);

    let path = config.biPath.m_lightPath;

    let x1 = globalThis.Math.random();
    let x2 = globalThis.Math.random();

    path = config.lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, path, x1, x2, bsdfFlags);
    if (path === null) {
      return;
    }

    config.biPath.m_lightPath = path;

    path.ensureNext();

    const node = path.next() as SimpleRaytracingPathNode;
    x1 = globalThis.Math.random();
    x2 = globalThis.Math.random();

    if (config.lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, node, x1, x2, bsdfFlags) !== null) {
      node.ensureNext();
      config.lightConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, node.next(), bsdfFlags);
    }
  }

  private photonMapTracePaths(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    config: PhotonMapConfig,
    numberOfPaths: number,
    bsdfFlags: number,
    radianceMethod: RadianceMethod
  ): void {
    for (let i = 0; i < numberOfPaths; i++) {
      this.photonMapTracePath(camera, sceneWorldVoxelGrid, sceneBackground, config, bsdfFlags);
      this.photonMapHandlePath(camera, sceneWorldVoxelGrid, config, radianceMethod);
    }
  }

  private photonMapBRRealIteration(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    radianceMethod: RadianceMethod
  ): void {
    this.photonMapState.iterationNumber++;

    process.stderr.write(`PhotonMapRadianceMethod Iteration ${this.photonMapState.iterationNumber}\n`);

    if ((this.photonMapState.iterationNumber > 1)
      && (this.photonMapState.doGlobalMap !== 0 || this.photonMapState.doCausticMap !== 0)) {
      const scaleFactor = (this.photonMapState.iterationNumber - 1.0) / this.photonMapState.iterationNumber;
      this.photonMapConfig.screen?.scaleRadiance(scaleFactor);
    }

    if (this.photonMapState.densityControl === PhotonMapDensityControlOption.IMPORTANCE_RD
      && this.photonMapState.doImportanceMap !== 0) {
      this.photonMapState.i_iteration_nr++;
      this.photonMapConfig.currentMap = this.photonMapConfig.importanceMap;
      this.photonMapState.totalIPaths = this.photonMapState.i_iteration_nr * this.photonMapState.iPathsPerIteration;
      this.photonMapConfig.currentMap?.setTotalPaths(this.photonMapState.totalIPaths);
      this.photonMapConfig.importanceCMap?.setTotalPaths(this.photonMapState.totalIPaths);

      PhotonMapImportance.tracePotentialPaths(
        camera,
        sceneWorldVoxelGrid,
        sceneBackground,
        this.photonMapState.iPathsPerIteration,
        this.photonMapState,
        this.photonMapConfig
      );

      process.stderr.write(
        `Total potential paths : ${this.photonMapState.totalIPaths}, Total rays ${Statistics.instance().rayTracer.rayCount}\n`
      );
    }

    if (this.photonMapState.doGlobalMap !== 0) {
      this.photonMapState.gIterationNumber++;
      this.photonMapConfig.currentMap = this.photonMapConfig.map;
      this.photonMapState.totalGPaths = this.photonMapState.gIterationNumber * this.photonMapState.gPathsPerIteration;
      this.photonMapConfig.currentMap?.setTotalPaths(this.photonMapState.totalGPaths);

      this.photonMapConfig.currentImpMap = this.photonMapConfig.importanceMap;

      this.photonMapTracePaths(
        camera,
        sceneWorldVoxelGrid,
        sceneBackground,
        this.photonMapConfig,
        this.photonMapState.gPathsPerIteration,
        Sampler.BSDF_ALL_COMPONENTS,
        radianceMethod
      );

      process.stderr.write("Global map: ");
      this.photonMapConfig.map?.printStats({
        printf(format: string, ...args: unknown[]): void {
          process.stderr.write(util.format(format, ...args));
        }
      });
    }

    if (this.photonMapState.doCausticMap !== 0) {
      this.photonMapState.cIterationNumber++;
      this.photonMapConfig.currentMap = this.photonMapConfig.causticMap;
      this.photonMapState.totalCPaths = this.photonMapState.cIterationNumber * this.photonMapState.cPathsPerIteration;
      this.photonMapConfig.currentMap?.setTotalPaths(this.photonMapState.totalCPaths);

      this.photonMapConfig.currentImpMap = this.photonMapConfig.importanceCMap;

      this.photonMapTracePaths(
        camera,
        sceneWorldVoxelGrid,
        sceneBackground,
        this.photonMapConfig,
        this.photonMapState.cPathsPerIteration,
        BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT,
        radianceMethod
      );

      process.stderr.write("Caustic map: ");
      this.photonMapConfig.causticMap?.printStats({
        printf(format: string, ...args: unknown[]): void {
          process.stderr.write(util.format(format, ...args));
        }
      });
    }
  }

  /**
Performs one step of the radiance computations. The goal most often is
to fill in a RGB color for display of each patch and/or vertex. These
colors are used for hardware rendering if the default hardware rendering
method is not updated in this file
*/
  public override doStep(scene: Scene, renderOptions: RenderOptions): boolean {
    void renderOptions;

    if (scene.camera === null || scene.voxelGrid === null) {
      VsdkError.fatal(-1, "PhotonMapRadianceMethod::doStep", "Scene camera/voxel grid not available");
      return false;
    }

    this.photonMapState.lastClock = Number(process.hrtime.bigint());

    this.photonMapBRRealIteration(scene.camera, scene.voxelGrid, scene.background, this);
    this.photonMapRadiosityUpdateCpuSecs();

    this.photonMapState.runStopNumber++;

    return false;
  }

  /**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
  public override terminate(scenePatches: Patch[]): void {
    void scenePatches;

    this.photonMapConfig.screen = null;

    this.photonMapConfig.lightConfig.releaseVars();
    this.photonMapConfig.eyeConfig.releaseVars();

    this.photonMapConfig.map = null;
    this.photonMapConfig.importanceMap = null;
    this.photonMapConfig.importanceCMap = null;
    this.photonMapConfig.causticMap = null;
    this.photonMapConfig.lightList = null;
  }

  /**
Returns the radiance emitted in the node related direction
*/
  public getNodeGRadiance(node: SimpleRaytracingPathNode): ColorRgb {
    if (this.photonMapConfig.map === null) {
      const black = new ColorRgb();
      black.clear();
      return black;
    }
    this.photonMapConfig.map.doBalancing(this.photonMapState.balanceKDTree !== 0);
    return this.photonMapConfig.map.reconstruct(
      node.m_hit,
      node.m_inDirF,
      node.m_useBsdf,
      node.m_inBsdf,
      node.m_outBsdf
    );
  }

  /**
Returns the radiance emitted in the node related direction
*/
  public getNodeCRadiance(node: SimpleRaytracingPathNode): ColorRgb {
    if (this.photonMapConfig.causticMap === null) {
      const black = new ColorRgb();
      black.clear();
      return black;
    }
    this.photonMapConfig.causticMap.doBalancing(this.photonMapState.balanceKDTree !== 0);

    return this.photonMapConfig.causticMap.reconstruct(
      node.m_hit,
      node.m_inDirF,
      node.m_useBsdf,
      node.m_inBsdf,
      node.m_outBsdf
    );
  }

  public override getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RenderOptions
  ): ColorRgb {
    void renderOptions;

    const hit = new RayHit();
    const point = new Vector3D();
    const bsdf = patch.material !== null ? patch.material.getBsdf() : null;
    let radiance = new ColorRgb();
    let density: number;

    patch.pointBarycentricMapping(u, v, point);
    hit.init(patch, point, patch.normal, patch.material);
    const normal = hit.getNormal();
    hit.shadingNormal(normal);
    hit.setNormal(normal);

    if (PhotonMap.zeroAlbedo(
      bsdf,
      hit,
      BsdfComponent.BRDF_DIFFUSE_COMPONENT
        | BsdfComponent.BTDF_DIFFUSE_COMPONENT
        | BsdfComponent.BRDF_GLOSSY_COMPONENT
        | BsdfComponent.BTDF_GLOSSY_COMPONENT
    )) {
      radiance.clear();
      return radiance;
    }

    let radiosityReturn = RadiosityReturnOption.GLOBAL_RADIANCE;

    if (PhotonMapRadianceMethod.doingLocalRayCasting) {
      radiosityReturn = this.photonMapState.radianceReturn;
    }

    switch (radiosityReturn) {
      case RadiosityReturnOption.GLOBAL_DENSITY:
        radiance = this.photonMapConfig.map !== null ? this.photonMapConfig.map.getDensityColor(hit) : new ColorRgb();
        break;
      case RadiosityReturnOption.CAUSTIC_DENSITY:
        radiance = this.photonMapConfig.causticMap !== null ? this.photonMapConfig.causticMap.getDensityColor(hit) : new ColorRgb();
        break;
      case RadiosityReturnOption.IMPORTANCE_C_DENSITY:
        radiance = this.photonMapConfig.importanceCMap !== null ? this.photonMapConfig.importanceCMap.getDensityColor(hit) : new ColorRgb();
        break;
      case RadiosityReturnOption.IMPORTANCE_G_DENSITY:
        radiance = this.photonMapConfig.importanceMap !== null ? this.photonMapConfig.importanceMap.getDensityColor(hit) : new ColorRgb();
        break;
      case RadiosityReturnOption.REC_C_DENSITY:
        if (this.photonMapConfig.importanceCMap !== null) {
          const nn = hit.getNormal();
          this.photonMapConfig.importanceCMap.doBalancing(this.photonMapState.balanceKDTree !== 0);
          density = this.photonMapConfig.importanceCMap.getRequiredDensity(camera, hit.getPoint(), nn);
          hit.setNormal(nn);
          radiance = PhotonMap.getFalseColor(density, this.photonMapState);
        }
        else {
          radiance.clear();
        }
        break;
      case RadiosityReturnOption.REC_G_DENSITY:
        if (this.photonMapConfig.importanceMap !== null) {
          this.photonMapConfig.importanceMap.doBalancing(this.photonMapState.balanceKDTree !== 0);
          density = this.photonMapConfig.importanceMap.getRequiredDensity(camera, hit.getPoint(), hit.getNormal());
          radiance = PhotonMap.getFalseColor(density, this.photonMapState);
        }
        else {
          radiance.clear();
        }
        break;
      case RadiosityReturnOption.GLOBAL_RADIANCE:
        if (this.photonMapConfig.map !== null) {
          radiance = this.photonMapConfig.map.reconstruct(hit, dir, bsdf, null, bsdf);
        }
        else {
          radiance.clear();
        }
        break;
      case RadiosityReturnOption.CAUSTIC_RADIANCE:
        if (this.photonMapConfig.causticMap !== null) {
          radiance = this.photonMapConfig.causticMap.reconstruct(hit, dir, bsdf, null, bsdf);
        }
        else {
          radiance.clear();
        }
        break;
      default:
        radiance.clear();
        VsdkError.error("photonMapGetRadiance", "Unknown radiance return");
    }

    return radiance;
  }

  public override getStats(): string {
    const stats = new StringBuilder();
    const statsOffset = [0];

    PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Photon map Statistics:\n\n");
    PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Ray count %d\n", Statistics.instance().rayTracer.rayCount);
    PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Time %g\n", this.photonMapState.cpuSecs);

    if (this.photonMapConfig.map !== null) {
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Global Map: ");
      this.photonMapConfig.map.getStats(stats, PhotonMapRadianceMethod.STRING_LENGTH);
      statsOffset[0] = globalThis.Math.min(stats.length(), PhotonMapRadianceMethod.STRING_LENGTH - 1);
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "\n");
    }
    if (this.photonMapConfig.causticMap !== null) {
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Caustic Map: ");
      this.photonMapConfig.causticMap.getStats(stats, PhotonMapRadianceMethod.STRING_LENGTH);
      statsOffset[0] = globalThis.Math.min(stats.length(), PhotonMapRadianceMethod.STRING_LENGTH - 1);
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "\n");
    }
    if (this.photonMapConfig.importanceMap !== null) {
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Global Importance Map: ");
      this.photonMapConfig.importanceMap.getStats(stats, PhotonMapRadianceMethod.STRING_LENGTH);
      statsOffset[0] = globalThis.Math.min(stats.length(), PhotonMapRadianceMethod.STRING_LENGTH - 1);
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "\n");
    }
    if (this.photonMapConfig.importanceCMap !== null) {
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "Caustic Importance Map: ");
      this.photonMapConfig.importanceCMap.getStats(stats, PhotonMapRadianceMethod.STRING_LENGTH);
      statsOffset[0] = globalThis.Math.min(stats.length(), PhotonMapRadianceMethod.STRING_LENGTH - 1);
      PhotonMapRadianceMethod.appendStatsText(stats, statsOffset, "\n");
    }

    if (stats.length() >= PhotonMapRadianceMethod.STRING_LENGTH) {
      return stats.toString().substring(0, PhotonMapRadianceMethod.STRING_LENGTH - 1);
    }
    return stats.toString();
  }
}
