import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "../common/linealAlgebra/Vector2Dd";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { ShadingContext } from "./ShadingContext";
import { ShadingContextFlag } from "./ShadingContextFlag";
import { Xxdf } from "./Xxdf";
import { XxdfComponentFlag } from "./XxdfComponentFlag";

export class PhongEmittanceDistributionFunction {
  private readonly Kd: ColorRgbMutable;
  private readonly kd: ColorRgbMutable;
  private readonly Ks: ColorRgbMutable;
  private readonly Ns: number;

  private isSpecular(): boolean {
    return this.Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
  }

  public constructor(KdParameter: ColorRgbMutable, KsParameter: ColorRgbMutable, NsParameter: number) {
    this.Kd = new ColorRgbMutable(KdParameter.r, KdParameter.g, KdParameter.b);
    this.kd = new ColorRgbMutable();
    this.kd.scaledCopy(1.0 / globalThis.Math.PI, this.Kd);
    this.Ks = new ColorRgbMutable(KsParameter.r, KsParameter.g, KsParameter.b);
    if (!this.Ks.isBlack()) {
      VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "phongEdfCreate", "Non-diffuse light sources not yet implemented");
    }
    this.Ns = NsParameter;
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

  public static edfIsTextured(): boolean {
    return false;
  }

  public static edfShadingFrame(context: ShadingContext, X: Vector3D | null, Y: Vector3D | null, Z: Vector3D | null): boolean {
    void context;
    void X;
    void Y;
    void Z;
    return false;
  }

  public phongEmittance(context: ShadingContext | null, flags: number): ColorRgbMutable {
    void context;
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

  public phongEdfEval(
    context: ShadingContext | null,
    out: Vector3D,
    flags: number,
    probabilityDensityFunction: number[] | null
  ): ColorRgbMutable {
    const normal = new Vector3D();
    const result = new ColorRgbMutable();
    result.clear();
    PhongEmittanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);

    if (context === null || !context.hasFlag(ShadingContextFlag.SHCTX_NORMAL)) {
      VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "phongEdfEval", "Couldn't determine shading normal");
      return result;
    }
    normal.copy(context.getShadingNormal());

    const cosL = out.dotProduct(normal);
    if (cosL < 0.0) {
      return result;
    }

    if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) {
      result.add(result, this.kd);
      PhongEmittanceDistributionFunction.setOut(probabilityDensityFunction, cosL / globalThis.Math.PI);
    }

    if ((flags & XxdfComponentFlag.SPECULAR_COMPONENT) !== 0) {
      // Not implemented in original code.
    }

    return result;
  }

  public phongEdfSample(
    context: ShadingContext | null,
    flags: number,
    xi1: number,
    xi2: number,
    selfEmittedRadiance: ColorRgbMutable | null,
    probabilityDensityFunction: number[] | null
  ): Vector3D {
    if (selfEmittedRadiance !== null) {
      selfEmittedRadiance.clear();
    }
    PhongEmittanceDistributionFunction.setOut(probabilityDensityFunction, 0.0);

    let dir = new Vector3D(0.0, 0.0, 1.0);
    if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) !== 0) {
      const coord = new CoordinateSystem();
      const normal = new Vector3D();
      if (context === null || !context.hasFlag(ShadingContextFlag.SHCTX_NORMAL)) {
        VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "phongEdfEval", "Couldn't determine shading normal");
        return dir;
      }
      normal.copy(context.getShadingNormal());

      coord.setFromZAxis(normal);
      const sampledPdf = [0.0];
      dir = coord.sampleHemisphereCosTheta(xi1, xi2, sampledPdf);
      PhongEmittanceDistributionFunction.setOut(probabilityDensityFunction, sampledPdf[0] ?? 0.0);

      if (selfEmittedRadiance !== null) {
        selfEmittedRadiance.scaledCopy(1.0 / globalThis.Math.PI, this.Kd);
      }
    }

    return dir;
  }

  private static setOut(out: number[] | null, value: number): void {
    if (out !== null && out.length > 0) {
      out[0] = value;
    }
  }
}
