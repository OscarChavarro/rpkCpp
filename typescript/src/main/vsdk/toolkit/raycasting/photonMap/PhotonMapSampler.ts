/**
A sampler specifically designed for use with photon maps.
Specular materials are treated as Fresnel reflectors/refractors.

NO DIFFUSE OR GLOSSY TRANSMITTING SURFACES SUPPORTED YET!
*/

import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { RefractionIndex } from "../../material/RefractionIndex";
import { Xxdf } from "../../material/Xxdf";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { BsdfSampler } from "../raytracing/BsdfSampler";
import { Sampler } from "../raytracing/Sampler";
import { PhotonMap } from "./PhotonMap";

/**
This is a hack to get fresnel factors for perfect specular reflection and refraction
*/
export class PhotonMapSampler extends BsdfSampler {
  private m_photonMap: PhotonMap | null;

  public constructor() {
    super();
    this.m_photonMap = null;
  }

  /**
Returns true a component was chosen, false if absorbed
*/
  private static chooseComponent(
    flags1: number,
    flags2: number,
    bsdf: PhongBidirectionalScatteringDistributionFunction | null,
    hit: RayHit,
    doRR: boolean,
    x: number[],
    probabilityDensityFunction: number[],
    chose1: boolean[]
  ): boolean {
    let color = new ColorRgb();
    let power1: number;
    let power2: number;
    let totalPower: number;

    color.clear();
    if (bsdf !== null) {
      color = bsdf.splitBsdfScatteredPower(hit, flags1 & 0xFF);
    }
    power1 = color.average();

    color.clear();
    if (bsdf !== null) {
      color = bsdf.splitBsdfScatteredPower(hit, flags2 & 0xFF);
    }
    power2 = color.average();

    totalPower = power1 + power2;

    if (totalPower < Numeric.EPSILON) {
      return false;
    }

    if (!doRR) {
      power1 /= totalPower;
      power2 /= totalPower;
      totalPower = 1.0;
    }

    if (x[0] < power1) {
      chose1[0] = true;
      probabilityDensityFunction[0] = power1;
      x[0] = x[0] / power1;
    }
    else if (x[0] < totalPower) {
      chose1[0] = false;
      probabilityDensityFunction[0] = power2;
      x[0] = (x[0] - power1) / power2;
    }
    else {
      return false;
    }

    return true;
  }

  // Sample : newNode gets filled, others may change
  // Return true if the node was filled in, false if path Ends
  // When path ends (absorption) the type of thisNode is adjusted to 'Ends'
  public override sample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR = false,
    flags = Sampler.BSDF_ALL_COMPONENTS
  ): boolean {
    const bsdf = thisNode.m_useBsdf;
    const sChosen = [false];
    const pdfChoice = [0.0];
    let sFlagMask: number;

    const sFLAGS = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;
    const gdFLAGS = BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT;

    if (flags === BsdfComponent.BRDF_SPECULAR_COMPONENT) {
      sFlagMask = sFLAGS;
    }
    else {
      sFlagMask = flags;
    }

    const x2a = [x2];
    if (!PhotonMapSampler.chooseComponent(
      sFLAGS & sFlagMask,
      gdFLAGS & flags,
      bsdf,
      thisNode.m_hit,
      doRR,
      x2a,
      pdfChoice,
      sChosen
    )) {
      return false;
    }

    let ok: boolean;

    if (sChosen[0]) {
      ok = this.fresnelSample(sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x2a[0], flags);
    }
    else {
      flags = gdFLAGS & flags;
      ok = this.gdSample(camera, sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x1, x2a[0], flags);
    }

    if (ok) {
      newNode.m_pdfFromPrev *= pdfChoice[0];
      newNode.m_accUsedComponents = thisNode.m_accUsedComponents | thisNode.m_usedComponents;
    }

    return ok;
  }

  /**
The Fresnel sampler works as follows:
1. Index of refractions are taken
2. Reflectance and Transmittance values are taken. Normally one of the two
   would be zero.
2b. Complex index of refraction, converted into geometric iof
3. Perfect reflected and refracted (if necessary) directions are computed
4. cosines and fresnel formulas are computed
5. reflection or refraction is chosen
6. fresnel reflection/refraction multiplied by appropriate scattering powers
7. node filled in.
*/
  private static bsdfGeometricIOR(bsdf: PhongBidirectionalScatteringDistributionFunction | null): RefractionIndex {
    const nc = new RefractionIndex();

    if (bsdf === null) {
      nc.set(1.0, 0.0);
    }
    else {
      bsdf.indexOfRefraction(nc);
    }

    if (nc.getNi() > Numeric.EPSILON) {
      nc.set(nc.complexToGeometricRefractionIndex(), 0.0);
    }

    return nc;
  }

  private static chooseFresnelDirection(
    thisNode: SimpleRaytracingPathNode,
    flags: number,
    x2: number,
    dir: Vector3D,
    pdfDir: number[],
    scatteringColor: ColorRgb,
    doCosInverse: boolean[]
  ): boolean {
    const ncIn = PhotonMapSampler.bsdfGeometricIOR(thisNode.m_inBsdf);
    const ncOut = PhotonMapSampler.bsdfGeometricIOR(thisNode.m_outBsdf);

    const bsdf = thisNode.m_useBsdf;
    let reflectance = new ColorRgb();
    reflectance.clear();
    if (bsdf !== null) {
      reflectance = bsdf.splitBsdfScatteredPower(
        thisNode.m_hit,
        BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT
      );
    }

    let transmittance = new ColorRgb();
    transmittance.clear();
    if (bsdf !== null) {
      transmittance = bsdf.splitBsdfScatteredPower(
        thisNode.m_hit,
        BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT
      );
    }

    const reflective = reflectance.average() > Numeric.EPSILON;
    const trans = transmittance.average() > Numeric.EPSILON;

    if (reflective && trans) {
      VsdkLogger.error(
        "FresnelFactor",
        "Cannot deal with simultaneous reflective & transit materials"
      );
      return false;
    }

    let cosI: number;
    let cost: number;
    let F: number;
    const tir = [false];
    let reflectedDir = new Vector3D();
    let refractedDir = new Vector3D();

    if (reflective) {
      if ((flags & BsdfComponent.BRDF_SPECULAR_COMPONENT) !== 0) {
        F = 1.0;
        reflectedDir = Xxdf.idealReflectedDirection(thisNode.m_inDirT, thisNode.m_normal);
        cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);
        if (cosI < 0) {
          VsdkLogger.error("fresnelSample", "cosI < 0");
        }
      }
      else {
        F = 0.0;
      }
    }
    else {
      refractedDir = Xxdf.idealRefractedDirection(
        thisNode.m_inDirT,
        thisNode.m_normal,
        ncIn,
        ncOut,
        tir
      );

      if (!tir[0]) {
        reflectedDir = Xxdf.idealReflectedDirection(thisNode.m_inDirT, thisNode.m_normal);
      }

      cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);

      if (cosI < 0) {
        VsdkLogger.error("fresnelSample", "cosI < 0");
      }

      if (!tir[0]) {
        cost = -thisNode.m_normal.dotProduct(refractedDir);

        if (cost < 0) {
          VsdkLogger.error("fresnelSample", "cost < 0");
        }

        const nt = ncOut.getNr();
        const ni = ncIn.getNr();

        const rParallel = (nt * cosI - ni * cost) / (nt * cosI + ni * cost);
        const rPerpendicular = (ni * cosI - nt * cost) / (ni * cosI + nt * cost);

        F = 0.5 * (rParallel * rParallel + rPerpendicular * rPerpendicular);
      }
      else {
        F = 0.0;
      }
    }

    let T = 1.0 - F;
    let reflected: boolean;

    let sum = 0.0;

    if ((flags & BsdfComponent.BTDF_SPECULAR_COMPONENT) !== 0) {
      sum += T;
    }
    else {
      T = 0.0;
    }

    if ((flags & BsdfComponent.BRDF_SPECULAR_COMPONENT) !== 0) {
      sum += F;
    }
    else {
      F = 0.0;
    }

    if (sum < Numeric.EPSILON) {
      return false;
    }

    if (x2 < T / sum) {
      reflected = false;
      dir.copy(refractedDir);
      pdfDir[0] = T / sum;
    }
    else {
      reflected = true;
      dir.copy(reflectedDir);
      pdfDir[0] = F / sum;
    }

    if (reflected) {
      if (reflective) {
        scatteringColor.scaledCopy(F, reflectance);
        doCosInverse[0] = false;
      }
      else {
        scatteringColor.scaledCopy(F, transmittance);
        doCosInverse[0] = true;
      }
    }
    else {
      scatteringColor.scaledCopy(T, transmittance);
      doCosInverse[0] = true;
    }

    return true;
  }

  private fresnelSample(
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    x2: number,
    flags: number
  ): boolean {
    const dir = new Vector3D();
    const pdfDir = [0.0];
    const doCosInverse = [false];
    const scatteringColor = new ColorRgb();

    if (!PhotonMapSampler.chooseFresnelDirection(thisNode, flags, x2, dir, pdfDir, scatteringColor, doCosInverse)) {
      return false;
    }

    PhotonMapSampler.DetermineRayType(thisNode, newNode, dir);

    if (!this.sampleTransfer(sceneVoxelGrid, sceneBackground, thisNode, newNode, dir, pdfDir[0])) {
      thisNode.m_rayType = PathRayType.STOPS;
      return false;
    }

    if (doCosInverse[0]) {
      const cosB = globalThis.Math.abs(newNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
      thisNode.m_bsdfEval.scaleInverse(cosB, scatteringColor);
    }
    else {
      thisNode.m_bsdfEval.set(scatteringColor.r, scatteringColor.g, scatteringColor.b);
    }

    if (this.m_computeFromNextPdf && prevNode !== null) {
      VsdkLogger.warning("FresnelSampler", "FromNextPdf not supported");
    }

    if (thisNode.m_rayType === PathRayType.REFLECTS) {
      thisNode.m_usedComponents = BsdfComponent.BRDF_SPECULAR_COMPONENT;
    }
    else {
      thisNode.m_usedComponents = BsdfComponent.BTDF_SPECULAR_COMPONENT;
    }

    return true;
  }

  private gdSample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    flags: number
  ): boolean {
    let ok: boolean;
    const x1Holder = [x1];
    const x2Holder = [x2];

    if (this.m_photonMap === null) {
      ok = super.sample(camera, sceneVoxelGrid, sceneBackground, prevNode, thisNode, newNode, x1, x2, false, flags);
      thisNode.m_usedComponents = flags;
      return ok;
    }

    // -- Currently NEVER reached!

    const bsdf = thisNode.m_useBsdf;
    const dChosen = [false];
    const pdfChoice = [0.0];

    if (!PhotonMapSampler.chooseComponent(
      BsdfComponent.BRDF_DIFFUSE_COMPONENT & flags,
      BsdfComponent.BRDF_GLOSSY_COMPONENT & flags,
      bsdf,
      thisNode.m_hit,
      false,
      x1Holder,
      pdfChoice,
      dChosen
    )) {
      return false;
    }

    const coord = new CoordinateSystem();
    let glossyExponent: number;

    if (dChosen[0]) {
      coord.setFromZAxis(thisNode.m_normal);
      glossyExponent = 1;
      flags = BsdfComponent.BRDF_DIFFUSE_COMPONENT;
    }
    else {
      flags = BsdfComponent.BRDF_GLOSSY_COMPONENT;
      VsdkLogger.error("PhotonMapSampler::gdSample", "Not done yet");
      return false;
    }

    const photonMapPdf = this.m_photonMap.sample(thisNode.m_hit.getPoint(), x1Holder, x2Holder, coord, flags, glossyExponent);

    ok = super.sample(
      camera,
      sceneVoxelGrid,
      sceneBackground,
      prevNode,
      thisNode,
      newNode,
      x1Holder[0],
      x2Holder[0],
      false,
      flags
    );

    if (ok) {
      newNode.m_pdfFromPrev *= pdfChoice[0] * photonMapPdf;
      thisNode.m_usedComponents = flags;
    }

    return ok;
  }
}
