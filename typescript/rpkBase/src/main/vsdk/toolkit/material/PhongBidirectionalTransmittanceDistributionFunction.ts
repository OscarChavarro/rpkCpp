import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { RefractionIndex } from "./RefractionIndex";
import { Xxdf } from "./Xxdf";
import { XxdfComponentFlag } from "./XxdfComponentFlag";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class PhongBidirectionalTransmittanceDistributionFunction {
  private readonly Kd: ColorRgbMutable;
  private readonly Ks: ColorRgbMutable;
  private readonly avgKd: number;
  private readonly avgKs: number;
  private readonly Ns: number;
  private readonly refractionIndex: RefractionIndex;

  private isSpecular(): boolean {
    return this.Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
  }

  public constructor(inKd: ColorRgbMutable, inKs: ColorRgbMutable, inNs: number, inNr: number, inNi: number) {
    this.Kd = new ColorRgbMutable(inKd.r, inKd.g, inKd.b);
    this.avgKd = this.Kd.average();
    this.Ks = new ColorRgbMutable(inKs.r, inKs.g, inKs.b);
    this.avgKs = this.Ks.average();
    this.Ns = inNs;
    this.refractionIndex = new RefractionIndex();
    this.refractionIndex.set(inNr, inNi);
  }

  public getKd(): ColorRgbMutable {
    return this.Kd;
  }

  public getKs(): ColorRgbMutable {
    return this.Ks;
  }

  public getNs(): number {
    return this.Ns;
  }

  public getRefractionIndex(): RefractionIndex {
    return this.refractionIndex;
  }

  public transmittance(flags: number): ColorRgbMutable {
    const result = new ColorRgbMutable();
    result.clear();

    if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) {
      result.add(result, this.Kd);
    }

    if (this.isSpecular()) {
      if ((flags & XxdfComponentFlag.SPECULAR_COMPONENT) !== 0) {
        result.add(result, this.Ks);
      }
    }
    else if ((flags & XxdfComponentFlag.GLOSSY_COMPONENT) !== 0) {
      result.add(result, this.Ks);
    }

    if (!globalThis.Number.isFinite(result.average())) {
      VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "transmittance", "Oops - result is not finite!");
    }

    return result;
  }

  public evaluate(
    inIndex: RefractionIndex,
    outIndex: RefractionIndex,
    inDirection: Vector3Dd,
    out: Vector3Dd,
    normal: Vector3Dd,
    flags: number
  ): ColorRgbMutable {
    let inRev = new Vector3Dd();
    inRev = inDirection.multiply(-1.0);

    const result = new ColorRgbMutable();
    result.clear();

    if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) && (this.avgKd > 0.0)) {
      const isReflection = normal.dotProduct(out) >= 0.0;
      if (!isReflection) {
        result.set(this.Kd.r, this.Kd.g, this.Kd.b);
        result.scale(1.0 / globalThis.Math.PI);
      }
    }

    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    if (((flags & nonDiffuseFlag) !== 0) && (this.avgKs > 0.0)) {
      const totalIR = [false];
      const idealRefracted = Xxdf.idealRefractedDirection(inRev, normal, inIndex, outIndex, totalIR);
      const localDotProduct = idealRefracted.dotProduct(out);

      if (localDotProduct > 0.0) {
        let tmpFloat = globalThis.Math.pow(localDotProduct, this.Ns);
        tmpFloat *= (this.Ns + 2.0) / (2.0 * globalThis.Math.PI);
        result.addScaled(result, tmpFloat, this.Ks);
      }
    }

    return result;
  }

  public sample(
    inIndex: RefractionIndex,
    outIndex: RefractionIndex,
    inDirection: Vector3Dd,
    normal: Vector3Dd,
    doRussianRoulette: number,
    flags: number,
    x1: number,
    x2: number,
    probabilityDensityFunction: number[] | null
  ): Vector3Dd {
    PhongBidirectionalTransmittanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);

    let newDir = new Vector3Dd(0.0, 0.0, 0.0);
    let inRev = new Vector3Dd();
    inRev = inDirection.multiply(-1.0);

    const localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) ? this.avgKd : 0.0;
    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    const localAverageKs = ((flags & nonDiffuseFlag) !== 0) ? this.avgKs : 0.0;

    const scatteredPower = localAverageKd + localAverageKs;
    if (scatteredPower < VSDK.EPSILON) {
      return newDir;
    }

    if (doRussianRoulette !== 0) {
      if (x1 > scatteredPower) {
        return newDir;
      }
      x1 /= scatteredPower;
    }

    const totalIR = [false];
    const idealDir = Xxdf.idealRefractedDirection(inRev, normal, inIndex, outIndex, totalIR);
    let invNormal = new Vector3Dd();
    invNormal = normal.multiply(-1.0);
    const coord = new CoordinateSystem();

    let diffPdf: number;
    let nonDiffPdf: number;

    if (x1 < (localAverageKd / scatteredPower)) {
      x1 = x1 / (localAverageKd / scatteredPower);
      coord.setFromZAxis(invNormal);
      const outDiffPdf = [0.0];
      newDir = coord.sampleHemisphereCosTheta(x1, x2, outDiffPdf);
      diffPdf = outDiffPdf[0]!;

      const tmpFloat = idealDir.dotProduct(newDir);
      if (tmpFloat > 0.0) {
        nonDiffPdf = (this.Ns + 1.0) * globalThis.Math.pow(tmpFloat, this.Ns) / (2.0 * globalThis.Math.PI);
      }
      else {
        nonDiffPdf = 0.0;
      }
    }
    else {
      x1 = (x1 - (localAverageKd / scatteredPower)) / (localAverageKs / scatteredPower);
      coord.setFromZAxis(idealDir);
      const outNonDiffPdf = [0.0];
      newDir = coord.sampleHemisphereCosNTheta(this.Ns, x1, x2, outNonDiffPdf);
      nonDiffPdf = outNonDiffPdf[0]!;

      const cosTheta = normal.dotProduct(newDir);
      if (cosTheta > 0.0) {
        diffPdf = cosTheta / globalThis.Math.PI;
      }
      else {
        diffPdf = 0.0;
      }
    }

    let pdf = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;
    if (doRussianRoulette === 0) {
      pdf /= scatteredPower;
    }

    PhongBidirectionalTransmittanceDistributionFunction.setOut(probabilityDensityFunction, pdf);
    return newDir;
  }

  public evaluateProbabilityDensityFunction(
    inIndex: RefractionIndex,
    outIndex: RefractionIndex,
    inDirection: Vector3Dd,
    out: Vector3Dd,
    normal: Vector3Dd,
    flags: number,
    probabilityDensityFunction: number[] | null,
    probabilityDensityFunctionRR: number[] | null
  ): void {
    PhongBidirectionalTransmittanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);
    PhongBidirectionalTransmittanceDistributionFunction.setOut(probabilityDensityFunctionRR, 0.0);

    let inRev = new Vector3Dd();
    inRev = inDirection.multiply(-1.0);

    let goodNormal = new Vector3Dd();
    const cosIn = inDirection.dotProduct(normal);
    if (cosIn >= 0.0) {
      goodNormal = normal;
    }
    else {
      goodNormal = normal.multiply(-1.0);
    }

    const cosTheta = goodNormal.dotProduct(out);

    let localAverageKd: number;
    if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) && (cosTheta < 0.0)) {
      localAverageKd = this.avgKd;
    }
    else {
      localAverageKd = 0.0;
    }

    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    const localAverageKs = ((flags & nonDiffuseFlag) !== 0) ? this.avgKs : 0.0;

    const scatteredPower = localAverageKd + localAverageKs;
    if (scatteredPower < VSDK.EPSILON) {
      return;
    }

    const diffPdf = (localAverageKd > 0.0) ? (cosTheta / globalThis.Math.PI) : 0.0;

    let nonDiffPdf = 0.0;
    if (localAverageKs > 0.0) {
      const totalIR = [false];
      let idealDir: Vector3Dd;

      if (cosIn >= 0.0) {
        idealDir = Xxdf.idealRefractedDirection(inRev, goodNormal, inIndex, outIndex, totalIR);
      }
      else {
        idealDir = Xxdf.idealRefractedDirection(inRev, goodNormal, outIndex, inIndex, totalIR);
      }

      const cosAlpha = idealDir.dotProduct(out);
      if (cosAlpha > 0.0) {
        nonDiffPdf = (this.Ns + 1.0) * globalThis.Math.pow(cosAlpha, this.Ns) / (2.0 * globalThis.Math.PI);
      }
    }

    PhongBidirectionalTransmittanceDistributionFunction.setOut(
      probabilityDensityFunction,
      (localAverageKd * diffPdf + localAverageKs * nonDiffPdf) / scatteredPower
    );
    PhongBidirectionalTransmittanceDistributionFunction.setOut(probabilityDensityFunctionRR, scatteredPower);
  }

  public setIndexOfRefraction(index: RefractionIndex): void {
    index.set(this.refractionIndex.getNr(), this.refractionIndex.getNi());
  }

  private static setOut(out: number[] | null, value: number): void {
    if (out !== null && out.length > 0) {
      out[0] = value;
    }
  }
}
