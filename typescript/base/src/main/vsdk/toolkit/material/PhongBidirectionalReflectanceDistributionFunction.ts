import { ColorRgb } from "../common/color/ColorRgb";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Xxdf } from "./Xxdf";
import { XxdfComponentFlag } from "./XxdfComponentFlag";

export class PhongBidirectionalReflectanceDistributionFunction {
  private readonly Kd: ColorRgb;
  private readonly Ks: ColorRgb;
  private readonly avgKd: number;
  private readonly avgKs: number;
  private readonly Ns: number;

  private isSpecular(): boolean {
    return this.Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
  }

  public constructor(inKd: ColorRgb, inKs: ColorRgb, inNs: number) {
    this.Kd = new ColorRgb(inKd.r, inKd.g, inKd.b);
    this.avgKd = this.Kd.average();
    this.Ks = new ColorRgb(inKs.r, inKs.g, inKs.b);
    this.avgKs = this.Ks.average();
    this.Ns = inNs;
  }

  public getKd(): ColorRgb {
    return this.Kd;
  }

  public getKs(): ColorRgb {
    return this.Ks;
  }

  public getNs(): number {
    return this.Ns;
  }

  public reflectance(flags: number): ColorRgb {
    const result = new ColorRgb();
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

  public evaluate(inDirection: Vector3D, out: Vector3D, normal: Vector3D, flags: number): ColorRgb {
    const result = new ColorRgb();
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
      const inRev = new Vector3D();
      inRev.scaledCopy(-1.0, inDirection);
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
    inDirection: Vector3D,
    normal: Vector3D,
    doRussianRoulette: number,
    flags: number,
    x1: number,
    x2: number,
    probabilityDensityFunction: number[] | null
  ): Vector3D {
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);

    const inRev = new Vector3D();
    inRev.scaledCopy(-1.0, inDirection);

    const localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) ? this.avgKd : 0.0;
    const nonDiffuseFlag = this.isSpecular()
      ? XxdfComponentFlag.SPECULAR_COMPONENT
      : XxdfComponentFlag.GLOSSY_COMPONENT;
    const localAverageKs = ((flags & nonDiffuseFlag) !== 0) ? this.avgKs : 0.0;

    const scatteredPower = localAverageKd + localAverageKs;
    let newDir = new Vector3D(0.0, 0.0, 0.0);
    if (scatteredPower < Numeric.EPSILON) {
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
    inDirection: Vector3D,
    out: Vector3D,
    normal: Vector3D,
    flags: number,
    probabilityDensityFunction: number[] | null,
    probabilityDensityFunctionRR: number[] | null
  ): void {
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);
    PhongBidirectionalReflectanceDistributionFunction.setOut(probabilityDensityFunctionRR, 0.0);

    const inRev = new Vector3D();
    inRev.scaledCopy(-1.0, inDirection);

    const goodNormal = new Vector3D();
    const cosIn = inDirection.dotProduct(normal);
    if (cosIn >= 0.0) {
      goodNormal.copy(normal);
    }
    else {
      goodNormal.scaledCopy(-1.0, normal);
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
    if (scatteredPower < Numeric.EPSILON) {
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
