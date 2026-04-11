/**
The real importance map storage
*/

import { Error as VsdkError } from "../../common/Error";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Camera } from "../../scene/Camera";
import { Importon } from "./Importon";
import { IrrPhoton } from "./IrrPhoton";
import { Photon } from "./Photon";
import { PhotonMap } from "./PhotonMap";
import { PhotonMapImportanceOptions } from "./PhotonMapImportanceOptions";
import { PhotonMapState } from "./PhotonMapState";

export class ImportanceMap extends PhotonMap {
  private m_maxImp: number;
  private m_avgImp: number;
  private m_totalMaxDistance: number;
  private m_preReconPhotons: number;
  private m_impScalePtr: number[];

  public constructor(photonMapState: PhotonMapState, estimate_nrp: number[], impScalePtr: number[]) {
    super(photonMapState, estimate_nrp, true);
    this.m_maxImp = 0.0;
    this.m_avgImp = 0.0;
    this.m_totalMaxDistance = 0.0;
    this.m_preReconPhotons = 0;
    this.m_impScalePtr = impScalePtr;
  }

  public override addPhoton(photon: Photon, normal: Vector3D, flags: number): boolean {
    return super.addPhoton(photon, normal, flags);
  }

  public reconstructImportance(pos: Vector3D, normal: Vector3D): number {
    const maxDistance = this.m_distances[0];
    let result = 0.0;

    for (let i = 0; i < this.m_nrpFound; i++) {
      const importon = this.m_photons[i] as Importon;
      const dir = importon.dir();
      const importance = importon.Importance();

      const cosTheta = dir.dotProduct(normal);
      if (cosTheta > 0.0) {
        result += importance;
      }
    }

    if (maxDistance < 1e-5) {
      return 0.0;
    }

    const factor = 1.0 / (globalThis.Math.PI * maxDistance * this.m_totalPaths);
    result *= factor;

    return result;
  }

  public getImpReqDensity(camera: Camera, pos: Vector3D, normal: Vector3D): number {
    let density = this.reconstructImportance(pos, normal);
    density /= camera.pixelWidth * camera.pixelHeight;
    return density;
  }

  public getRequiredDensity(camera: Camera, pos: Vector3D, normal: Vector3D): number {
    if (this.m_nrPhotons === 0) {
      return this.photonMapState.constantRD;
    }

    let density: number;

    this.checkNBalance();

    if (this.m_precomputeIrradiance) {
      if (!this.m_irradianceComputed || (this.m_preReconPhotons !== this.m_estimate_nrp[0])) {
        this.precomputeIrradiance();
      }

      const photon = this.DoIrradianceQuery(pos, normal, this.m_totalMaxDistance) as Importon | null;

      if (photon !== null) {
        switch (this.photonMapState.importanceOption) {
          case PhotonMapImportanceOptions.USE_IMPORTANCE:
            density = photon.PImportance();
            density *= this.m_impScalePtr[0];
            break;
          default:
            VsdkError.error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
            return 0.0;
        }
      }
      else {
        density = 0.0;
      }
    }
    else {
      this.m_nrpFound = this.doQuery(pos);

      if (this.m_nrpFound < 3) {
        return 0.0;
      }

      switch (this.photonMapState.importanceOption) {
        case PhotonMapImportanceOptions.USE_IMPORTANCE:
          density = this.getImpReqDensity(camera, pos, normal);
          density *= this.m_impScalePtr[0];
          break;
        default:
          VsdkError.error("ImportanceMap::getRequiredDensity", "Unsupported importance option");
          return 0.0;
      }
    }

    if (density < this.photonMapState.minimumImpRD) {
      density = this.photonMapState.minimumImpRD;
    }
    return density;
  }

  protected ComputeAllRequiredDensities(
    camera: Camera,
    pos: Vector3D,
    normal: Vector3D,
    imp: number[],
    pot: number[],
    diff: number[]
  ): void {
    this.m_nrpFound = this.doQuery(pos);
    if (this.m_nrpFound < 5) {
      imp[0] = 0.0;
      pot[0] = 0.0;
      diff[0] = 0.0;
      return;
    }

    imp[0] = this.getImpReqDensity(camera, pos, normal);
  }

  public override photonPrecomputeIrradiance(camera: Camera | null, photon: IrrPhoton): void {
    if (camera === null) {
      return;
    }

    const imp = [0.0];
    const pot = [0.0];
    const diff = [0.0];
    const pos = photon.pos();
    const normal = photon.Normal();

    this.ComputeAllRequiredDensities(camera, pos, normal, imp, pot, diff);

    pot[0] = this.m_distances[0];
    this.m_totalMaxDistance = globalThis.Math.max(pot[0], this.m_totalMaxDistance);

    (photon as Importon).PSetAll(imp[0], pot[0], diff[0]);
    if (imp[0] > this.m_maxImp) {
      this.m_maxImp = imp[0];
    }

    this.m_avgImp += imp[0];
  }

  public override precomputeIrradiance(): void {
    process.stderr.write("ImportanceMap::precomputeIrradiance\n");
    this.m_maxImp = 0.0;
    this.m_avgImp = 0.0;
    this.m_preReconPhotons = this.m_estimate_nrp[0];
    this.m_totalMaxDistance = 0.0;
    this.m_irradianceComputed = false;

    super.precomputeIrradiance();

    if (this.m_nrPhotons > 0) {
      this.m_avgImp /= this.m_nrPhotons;
    }
    if (this.m_estimate_nrp[0] > 0) {
      this.m_totalMaxDistance *= 20.0 / this.m_estimate_nrp[0];
    }
  }
}

