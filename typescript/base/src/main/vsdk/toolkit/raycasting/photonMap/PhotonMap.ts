import { StringBuilder } from "../../../../java/lang/StringBuilder";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { KDTree } from "../../common/dataStructures/KDTree";
import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { Camera } from "../../scene/Camera";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { IrrPhoton } from "./IrrPhoton";
import { Photon } from "./Photon";
import { PhotonFlags } from "./PhotonFlags";
import { PhotonKDTree } from "./PhotonKDTree";
import { PhotonMapDCAcceptPDFType } from "./PhotonMapDCAcceptPDFType";
import { PhotonMapState } from "./PhotonMapState";
import { SampleGrid2D } from "./SampleGrid2D";

export class PhotonMap {
  protected photonMapState: PhotonMapState;
  protected m_balanced: boolean;
  protected m_doBalancing: boolean;

  protected m_precomputeIrradiance: boolean;
  protected m_irradianceComputed: boolean;

  protected m_estimate_nrp: number[];
  protected m_sample_nrp: number;
  protected m_nrPhotons: number;
  protected m_totalPhotons: number;
  protected m_totalPaths: number;

  protected m_kdtree: PhotonKDTree;

  protected m_grid: SampleGrid2D;
  protected m_sampleLastPos: Vector3D;

  protected m_nrpFound: number;
  protected m_nrpCosinePos: number;

  protected m_photons: Photon[];
  protected m_distances: number[];
  protected m_cosines: number[];
  protected m_cosinesOk: boolean;

  protected doQuery(
    position: Vector3D,
    numberOfPhotons: number,
    maximumRadius: number,
    excludeFlags: number
  ): number;
  protected doQuery(pos: Vector3D): number;
  protected doQuery(
    positionOrPos: Vector3D,
    numberOfPhotons?: number,
    maximumRadius?: number,
    excludeFlags?: number
  ): number {
    this.m_cosinesOk = false;

    if (numberOfPhotons !== undefined && maximumRadius !== undefined && excludeFlags !== undefined) {
      return this.m_kdtree.query(
        [positionOrPos.x, positionOrPos.y, positionOrPos.z],
        numberOfPhotons,
        this.m_photons as unknown[],
        this.m_distances,
        maximumRadius,
        excludeFlags
      );
    }

    return this.m_kdtree.query(
      [positionOrPos.x, positionOrPos.y, positionOrPos.z],
      this.m_estimate_nrp[0]!,
      this.m_photons as unknown[],
      this.m_distances,
      this.GetMaxR2(),
      0
    );
  }

  protected DoIrradianceQuery(position: Vector3D, normal: Vector3D, maxR2: number): IrrPhoton | null;
  protected DoIrradianceQuery(position: Vector3D, normal: Vector3D): IrrPhoton | null;
  protected DoIrradianceQuery(position: Vector3D, normal: Vector3D, maxR2 = Numeric.HUGE_FLOAT_VALUE): IrrPhoton | null {
    return this.m_kdtree.normalPhotonQuery(position, normal, 0.8, maxR2);
  }

  protected computeCosines(normal: Vector3D): void {
    if (!this.m_cosinesOk) {
      this.m_nrpCosinePos = 0;

      for (let i = 0; i < this.m_nrpFound; i++) {
        const dir = this.m_photons[i]!.dir();
        this.m_cosines[i] = dir.dotProduct(normal);
        if (this.m_cosines[i]! > 0) {
          this.m_nrpCosinePos++;
        }
      }

      this.m_cosinesOk = true;
    }
  }

  protected doAddPhoton(photon: Photon, normal: Vector3D, flags: number): void {
    if (this.m_precomputeIrradiance) {
      const irrPhoton = new IrrPhoton();
      irrPhoton.copy(photon);
      irrPhoton.setNormal(normal);
      this.m_kdtree.addPoint(irrPhoton, flags);
    }
    else {
      this.m_kdtree.addPoint(photon, flags);
    }
  }

  public static zeroAlbedo(
    bsdf: PhongBidirectionalScatteringDistributionFunction | null,
    hit: RayHit,
    flags: number
  ): boolean {
    let color: ColorRgb;
    if (bsdf === null) {
      color = new ColorRgb();
      color.clear();
    }
    else {
      const shctxOk = [false];
      const shctx = hit.shadingContext(shctxOk);
      if (shctxOk[0]) {
        color = bsdf.splitBsdfScatteredPower(shctx, flags & 0xFF);
      }
      else {
        color = new ColorRgb();
        color.clear();
      }
    }
    return color.average() < Numeric.EPSILON;
  }

  private static getFalseMonochrome(val: number, photonMapState: PhotonMapState): number {
    let max = photonMapState.falseColMax;

    let outVal = val;
    if (photonMapState.falseColLog !== 0) {
      max = globalThis.Math.log(1.0 + max);
      outVal = globalThis.Math.log(1.0 + outVal);
    }

    let tmp = globalThis.Math.min(outVal, max);
    tmp = tmp / max;

    return tmp;
  }

  public static getFalseColor(val: number, photonMapState: PhotonMapState): ColorRgb {
    const col = new ColorRgb();
    let tmp: number;
    let r = 0.0;
    let g = 0.0;
    let b = 0.0;

    if (photonMapState.falseColMono !== 0) {
      tmp = PhotonMap.getFalseMonochrome(val, photonMapState);
      col.set(tmp, tmp, tmp);
      return col;
    }

    let max = photonMapState.falseColMax;
    let outVal = val;

    if (photonMapState.falseColLog !== 0) {
      max = globalThis.Math.log(1.0 + max);
      outVal = globalThis.Math.log(1.0 + outVal);
    }

    tmp = globalThis.Math.min(outVal, max);
    tmp = 3.0 * (tmp / max);

    if (tmp <= 1.0) {
      b = tmp;
    }
    else if (tmp < 2.0) {
      g = tmp - 1.0;
      b = 1.0 - g;
    }
    else {
      r = tmp - 2.0;
      g = 1.0 - r;
    }

    col.set(r, g, b);
    return col;
  }

  public constructor(inPhotonMapState: PhotonMapState, estimate_nrp: number[], doPrecomputeIrradiance: boolean);
  public constructor(inPhotonMapState: PhotonMapState, estimate_nrp: number[]);
  public constructor(inPhotonMapState: PhotonMapState, estimate_nrp: number[], doPrecomputeIrradiance = false) {
    this.photonMapState = inPhotonMapState;
    this.m_sample_nrp = 0;
    this.m_nrpCosinePos = 0;

    this.m_balanced = true;
    this.m_doBalancing = false;

    this.m_estimate_nrp = estimate_nrp ?? [0];

    this.m_precomputeIrradiance = doPrecomputeIrradiance;
    this.m_irradianceComputed = false;

    this.m_kdtree = new PhotonKDTree(0, true);

    this.m_totalPaths = 0;
    this.m_nrPhotons = 0;
    this.m_totalPhotons = 0;

    this.m_grid = new SampleGrid2D(2, 4);
    this.m_sampleLastPos = new Vector3D(Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE, Numeric.HUGE_FLOAT_VALUE);

    this.m_photons = new Array<Photon>(PhotonMapState.MAXIMUM_RECON_PHOTONS);
    for (let i = 0; i < this.m_photons.length; i++) {
      this.m_photons[i] = new Photon();
    }
    this.m_distances = new Array<number>(PhotonMapState.MAXIMUM_RECON_PHOTONS).fill(0.0);
    this.m_cosines = new Array<number>(PhotonMapState.MAXIMUM_RECON_PHOTONS).fill(0.0);

    this.m_nrpFound = 0;
    this.m_cosinesOk = true;
  }

  public dispose(): void {
    this.m_photons = [];
    this.m_distances = [];
    this.m_cosines = [];
  }

  public setTotalPaths(totalPaths: number): void {
    this.m_totalPaths = totalPaths;
  }

  public printStats(stream: { printf: (format: string, ...args: unknown[]) => unknown } | null): void {
    if (stream === null) {
      return;
    }
    stream.printf("%d stored photons\n", this.m_nrPhotons);
  }

  public getStats(p: StringBuilder | null, n: number): void {
    if (p === null || n <= 0) {
      return;
    }

    const text = `${this.m_nrPhotons} stored photons, ${this.m_totalPhotons} total, ${this.m_totalPaths} paths\n`;
    const available = globalThis.Math.max(0, n - p.length());
    if (available <= 0) {
      return;
    }
    if (text.length > available) {
      p.append(text, available);
    }
    else {
      p.append(text);
    }
  }

  /**
Adding photons, returns if photon was added
*/
  public addPhoton(photon: Photon, normal: Vector3D, flags: number): boolean {
    globalThis.Math.random();

    this.doAddPhoton(photon, normal, flags);
    this.m_nrPhotons++;
    this.m_totalPhotons++;
    this.m_balanced = false;
    this.m_irradianceComputed = false;

    return true;
  }

  private static computeAcceptProb(currentD: number, requiredD: number, photonMapState: PhotonMapState): number {
    if (photonMapState.acceptPdfType === PhotonMapDCAcceptPDFType.STEP) {
      if (currentD > requiredD) {
        return 0.0;
      }
      return 1.0;
    }

    if (photonMapState.acceptPdfType === PhotonMapDCAcceptPDFType.TRANS_COSINE) {
      const ratio = globalThis.Math.min(1.0, currentD / requiredD);
      return 0.5 * (1.0 + globalThis.Math.cos(ratio * globalThis.Math.PI));
    }

    VsdkLogger.error("PhotonMap::computeAcceptProb", "Unknown accept pdf type");
    return 0.0;
  }

  public redistribute(photon: Photon): void {
    const deltaPower = new ColorRgb();
    const factor = 1.0 / this.m_nrpCosinePos;

    const pow = photon.power();
    deltaPower.scaledCopy(factor, pow);

    for (let i = 0; i < this.m_nrpFound; i++) {
      if (this.m_cosines[i]! > 0.0) {
        this.m_photons[i]!.addPower(deltaPower);
      }
    }
  }

  public DC_AddPhoton(photon: Photon, hit: RayHit, requiredD: number, flags: number): boolean {
    let stored: boolean;

    const currentD = this.getCurrentDensity(hit, this.photonMapState.distribPhotons);

    const acceptProb = PhotonMap.computeAcceptProb(currentD, requiredD, this.photonMapState);

    if (globalThis.Math.random() < acceptProb) {
      this.doAddPhoton(photon, hit.getNormal(), flags);
      this.m_nrPhotons++;
      this.m_balanced = false;
      this.m_irradianceComputed = false;
      stored = true;
    }
    else {
      stored = false;
      this.redistribute(photon);
    }

    this.m_totalPhotons++;

    return stored;
  }

  /**
Get a maximum radius^2 to be used when locating photons
*/
  public GetMaxR2(): number {
    const radFraction = 0.03;

    if (this.m_totalPaths <= 0) {
      return Numeric.HUGE_DOUBLE_VALUE;
    }

    return (
      (this.m_estimate_nrp[0]! * Statistics.instance().radiance.totalArea)
      / (globalThis.Math.PI * this.m_totalPaths * radFraction)
    );
  }

  public photonPrecomputeIrradiance(camera: Camera | null, photon: IrrPhoton): void {
    void camera;

    const irradiance = new ColorRgb();
    irradiance.clear();

    const pos = photon.pos();
    this.m_nrpFound = this.doQuery(pos);

    if (this.m_nrpFound > 3) {
      const maxDistance = this.m_distances[0]!;

      for (let i = 0; i < this.m_nrpFound; i++) {
        if (photon.Normal().dotProduct(this.m_photons[i]!.dir()) > 0) {
          const power = this.m_photons[i]!.power();
          irradiance.add(irradiance, power);
        }
      }

      const factor = 1.0 / (globalThis.Math.PI * globalThis.Math.PI * maxDistance * this.m_totalPaths);
      irradiance.scale(factor);
    }

    photon.SetIrradiance(irradiance);
  }

  public precomputeIrradiance(): void {
    process.stderr.write("PhotonMap::precomputeIrradiance\n");
    if (this.m_precomputeIrradiance && !this.m_irradianceComputed) {
      this.m_kdtree.iterateNodes((data: unknown, nodeData: unknown): void => {
        const map = data as PhotonMap;
        const photon = nodeData as IrrPhoton;
        map.photonPrecomputeIrradiance(null, photon);
      }, this);
      this.m_irradianceComputed = true;
    }
  }

  public irradianceReconstruct(
    hit: RayHit,
    outDir: Vector3D,
    diffuseAlbedo: ColorRgb,
    result: ColorRgb
  ): boolean {
    void outDir;

    if (!this.m_irradianceComputed) {
      this.precomputeIrradiance();
    }

    const normal = hit.getNormal();
    const position = hit.getPoint();
    const photon = this.DoIrradianceQuery(position, normal);
    hit.setNormal(normal);

    if (photon !== null) {
      result.scalarProduct(photon.m_irradiance, diffuseAlbedo);
      return true;
    }
    return false;
  }

  public reconstruct(
    hit: RayHit,
    outDir: Vector3D,
    bsdf: PhongBidirectionalScatteringDistributionFunction | null,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null
  ): ColorRgb {
    const result = new ColorRgb();
    const evalColor = new ColorRgb();
    const col = new ColorRgb();

    result.clear();

    let diffuseAlbedo = new ColorRgb();
    let glossyAlbedo = new ColorRgb();

    diffuseAlbedo.clear();
    glossyAlbedo.clear();

    const shctxOk = [false];
    const shctx = hit.shadingContext(shctxOk);
    if (bsdf !== null && shctxOk[0]) {
      diffuseAlbedo = bsdf.splitBsdfScatteredPower(shctx, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
      glossyAlbedo = bsdf.splitBsdfScatteredPower(
        shctx,
        BsdfComponent.BTDF_DIFFUSE_COMPONENT
          | BsdfComponent.BRDF_GLOSSY_COMPONENT
          | BsdfComponent.BTDF_GLOSSY_COMPONENT
      );
    }

    this.checkNBalance();

    if (glossyAlbedo.average() < Numeric.EPSILON) {
      if (diffuseAlbedo.average() < Numeric.EPSILON) {
        return result;
      }

      if (this.m_precomputeIrradiance) {
        if (this.irradianceReconstruct(hit, outDir, diffuseAlbedo, result)) {
          return result;
        }
      }
    }

    const position = hit.getPoint();
    this.m_nrpFound = this.doQuery(position);

    if (this.m_nrpFound < 3) {
      return result;
    }

      const maxDistance = this.m_distances[0]!;

    for (let i = 0; i < this.m_nrpFound; i++) {
      const dir = this.m_photons[i]!.dir();

      if (bsdf === null || !shctxOk[0]) {
        evalColor.clear();
      }
      else {
        const evaluated = bsdf.evaluate(
          shctx,
          inBsdf,
          outBsdf,
          outDir,
          dir,
          BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
        );
        evalColor.set(evaluated.r, evaluated.g, evaluated.b);
      }
      const power = this.m_photons[i]!.power();

      col.scalarProduct(evalColor, power);
      result.add(result, col);
    }

    const factor = 1.0 / (globalThis.Math.PI * maxDistance * this.m_totalPaths);
    result.scale(factor);

    return result;
  }

  public getCurrentDensity(hit: RayHit, nrPhotons: number): number {
    let localNrPhotons = nrPhotons;
    if (localNrPhotons === 0) {
      localNrPhotons = this.m_estimate_nrp[0]!;
    }

    if (localNrPhotons === 0) {
      return 0.0;
    }

    const position = hit.getPoint();
    this.m_nrpFound = this.doQuery(position, localNrPhotons, this.GetMaxR2(), 0);

    if (this.m_nrpFound < 3) {
      return 0.0;
    }

    const maxDistance = this.m_distances[0]!;

    this.computeCosines(hit.getGeometricNormal());

    if (this.m_nrpCosinePos <= 3) {
      return 0.0;
    }

    return this.m_nrpCosinePos / (globalThis.Math.PI * maxDistance);
  }

  /**
Return a color coded density of the photon map
*/
  public getDensityColor(hit: RayHit): ColorRgb {
    const density = this.getCurrentDensity(hit, 0);

    return PhotonMap.getFalseColor(density, this.photonMapState);
  }

  public sample(position: Vector3D, r: number[], s: number[], coord: CoordinateSystem, flag: number, n: number): number {
    if (!this.m_sampleLastPos.equals(position, 0.0001)) {
      this.m_grid.init();

      this.m_nrpFound = this.doQuery(
        position,
        this.m_sample_nrp,
        KDTree.KD_MAX_RADIUS,
        PhotonFlags.NO_IMPSAMP_PHOTON
      );

      const pr = [0.0];
      const ps = [0.0];

      for (let i = 0; i < this.m_nrpFound; i++) {
        this.m_photons[i]!.findRS(pr, ps, coord, flag, n);

        const color = this.m_photons[i]!.power();
        this.m_grid.add(pr[0]!, ps[0]!, color.average() / this.m_nrPhotons);
      }

      this.m_grid.EnsureNonZeroEntries();
      this.m_sampleLastPos.copy(position);
    }

    const probabilityDensityFunction = [0.0];
    this.m_grid.sample(r, s, probabilityDensityFunction);

    return probabilityDensityFunction[0]!;
  }

  public Balance(): void {
    this.m_kdtree.balance();
  }

  public checkNBalance(): void {
    if ((!this.m_balanced) && (this.m_doBalancing || this.m_precomputeIrradiance)) {
      this.Balance();
      this.m_balanced = true;
    }
  }

  public doBalancing(state: boolean): void {
    this.m_doBalancing = state;
  }
}
