import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Xxdf } from "./Xxdf";
import { XxdfComponentFlag } from "./XxdfComponentFlag";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class PhongBidirectionalReflectanceDistributionFunction {
  private readonly Kd: ColorRgbMutable;
  private readonly Ks: ColorRgbMutable;
  private readonly avgKd: number;
  private readonly avgKs: number;
  private readonly Ns: number;

  private isSpecular(): boolean {
    return this.Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
  }

  public constructor(inKd: ColorRgbMutable, inKs: ColorRgbMutable, inNs: number) {
    this.Kd = new ColorRgbMutable(inKd.r, inKd.g, inKd.b);
    this.avgKd = this.Kd.average();
    this.Ks = new ColorRgbMutable(inKs.r, inKs.g, inKs.b);
    this.avgKs = this.Ks.average();
    this.Ns = inNs;
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

  public reflectance(flags: number): ColorRgbMutable {
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

    return result;
  }

  public evaluate(inDirection: Vector3Df, out: Vector3Df, normal: Vector3Df, flags: number): ColorRgbMutable {
    const result = new ColorRgbMutable();
    result.clear();

    if (out.dotProduct(normal) < 0.0) {
      return result;
    }

    if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) && (this.avgKd > 0.0)) {
      result.addScaled(result, 1.0 / globalThis.Math.PI, this.Kd);
    }

    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;

    if (((flags & nonDiffuseFlag) !== 0) && (this.avgKs > 0.0)) {
      let inRev = new Vector3Df();
      inRev = inDirection.multiply(-1.0);
      const idealReflected = Xxdf.idealReflectedDirection(inRev, normal);
      const localDotProduct = idealReflected.dotProduct(out);

      if (localDotProduct > 0.0) {
        let tmpFloat = globalThis.Math.pow(localDotProduct, this.Ns);
        tmpFloat *= (this.Ns + 2.0) / (2.0 * globalThis.Math.PI);
        result.addScaled(result, tmpFloat, this.Ks);
      }
    }

    return result;
  }

  public sample(
    inDirection: Vector3Df,
    normal: Vector3Df,
    doRussianRoulette: number,
    flags: number,
    x1: number,
    x2: number,
    probabilityDensityFunction: number[] | null
  ): Vector3Df {
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);

    let inRev = new Vector3Df();
    inRev = inDirection.multiply(-1.0);

    const localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) ? this.avgKd : 0.0;
    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    const localAverageKs = ((flags & nonDiffuseFlag) !== 0) ? this.avgKs : 0.0;

    const scatteredPower = localAverageKd + localAverageKs;
    let newDir = new Vector3Df(0.0, 0.0, 0.0);
    if (scatteredPower < VSDK.EPSILON) {
      return newDir;
    }

    if (doRussianRoulette !== 0) {
      if (x1 > scatteredPower) {
        return newDir;
      }
      x1 /= scatteredPower;
    }

    const idealDir = Xxdf.idealReflectedDirection(inRev, normal);
    const coord = new CoordinateSystem();
    let diffPdf: number;
    let nonDiffPdf: number;

    if (x1 < (localAverageKd / scatteredPower)) {
      x1 = x1 / (localAverageKd / scatteredPower);
      coord.setFromZAxis(normal);
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
      if (cosTheta <= 0.0) {
        return newDir;
      }
      diffPdf = cosTheta / globalThis.Math.PI;
    }

    let pdf = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;
    if (doRussianRoulette === 0) {
      pdf /= scatteredPower;
    }

    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunction, pdf);
    return newDir;
  }

  public evaluateProbabilityDensityFunction(
    inDirection: Vector3Df,
    out: Vector3Df,
    normal: Vector3Df,
    flags: number,
    probabilityDensityFunction: number[] | null,
    probabilityDensityFunctionRR: number[] | null
  ): void {
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunctionRR, 0.0);

    let inRev = new Vector3Df();
    inRev = inDirection.multiply(-1.0);

    let goodNormal = new Vector3Df();
    const cosIn = inDirection.dotProduct(normal);
    if (cosIn >= 0.0) {
      goodNormal = normal;
    }
    else {
      goodNormal = normal.multiply(-1.0);
    }

    const cosTheta = goodNormal.dotProduct(out);
    if (cosTheta < 0.0) {
      return;
    }

    const localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) ? this.avgKd : 0.0;
    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    const localAverageKs = ((flags & nonDiffuseFlag) !== 0) ? this.avgKs : 0.0;

    const scatteredPower = localAverageKd + localAverageKs;
    if (scatteredPower < VSDK.EPSILON) {
      return;
    }

    let diffPdf = 0.0;
    if (this.avgKd > 0.0) {
      diffPdf = cosTheta / globalThis.Math.PI;
    }

    let nonDiffPdf = 0.0;
    if (this.avgKs > 0.0) {
      const idealDir = Xxdf.idealReflectedDirection(inRev, goodNormal);
      const cosAlpha = idealDir.dotProduct(out);
      if (cosAlpha > 0.0) {
        nonDiffPdf = (this.Ns + 1.0) * globalThis.Math.pow(cosAlpha, this.Ns) / (2.0 * globalThis.Math.PI);
      }
    }

    PhongBidirectionalReflectanceDistributionFunction.setOut(
      probabilityDensityFunction,
      (this.avgKd * diffPdf + this.avgKs * nonDiffPdf) / scatteredPower
    );
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunctionRR, scatteredPower);
  }

  private static setOut(out: number[] | null, value: number): void {
    if (out !== null && out.length > 0) {
      out[0] = value;
    }
  }
}
