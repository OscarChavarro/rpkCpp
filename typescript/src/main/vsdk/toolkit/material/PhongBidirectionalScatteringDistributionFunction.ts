import { ColorRgb } from "../common/color/ColorRgb";
import { Error as VsdkError } from "../common/Error";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { RayHit } from "../skin/RayHit";
import { BsdfComponent } from "./BsdfComponent";
import { BsdfComponentFlag } from "./BsdfComponentFlag";
import { BsdfComponentInfo } from "./BsdfComponent";
import { RayHitFlag } from "../skin/RayHitFlag";
import { PhongBidirectionalReflectanceDistributionFunction } from "./PhongBidirectionalReflectanceDistributionFunction";
import { PhongBidirectionalTransmittanceDistributionFunction } from "./PhongBidirectionalTransmittanceDistributionFunction";
import { RefractionIndex } from "./RefractionIndex";
import { ShadingContext } from "./ShadingContext";
import { SplitBSDFSamplingMode } from "./SplitBSDFSamplingMode";
import { Texture } from "./Texture";

export class PhongBidirectionalScatteringDistributionFunction {
  private static readonly TEXTURED_COMPONENT = BsdfComponent.BRDF_DIFFUSE_COMPONENT;

  private readonly brdf: PhongBidirectionalReflectanceDistributionFunction | null;
  private readonly btdf: PhongBidirectionalTransmittanceDistributionFunction | null;
  private readonly texture: Texture | null;

  public constructor(
    brdf: PhongBidirectionalReflectanceDistributionFunction | null,
    btdf: PhongBidirectionalTransmittanceDistributionFunction | null,
    texture: Texture | null
  ) {
    this.brdf = brdf;
    this.btdf = btdf;
    this.texture = texture;
  }

  public getBrdf(): PhongBidirectionalReflectanceDistributionFunction | null {
    return this.brdf;
  }

  public getBtdf(): PhongBidirectionalTransmittanceDistributionFunction | null {
    return this.btdf;
  }

  public getTexture(): Texture | null {
    return this.texture;
  }

  public static bsdfShadingFrame(hitOrContext: RayHit | ShadingContext, X: Vector3D | null, Y: Vector3D | null, Z: Vector3D | null): boolean {
    void hitOrContext;
    void X;
    void Y;
    void Z;
    return false;
  }

  private static extractHitData(hit: RayHit | null, normal: Vector3D, texCoord: Vector3D, flags: number[]): boolean {
    if (hit === null || flags.length < 1) {
      return false;
    }
    if (!hit.shadingNormal(normal)) {
      return false;
    }
    flags[0] = RayHitFlag.NORMAL;
    if (hit.getTexCoord(texCoord)) {
      flags[0] |= RayHitFlag.TEXTURE_COORDINATE;
    }
    else {
      texCoord.set(0.0, 0.0, 0.0);
    }
    return true;
  }

  private static splitBsdfEvalTexture(texture: Texture | null, hitOrContext: RayHit | ShadingContext): ColorRgb {
    const texCoord = new Vector3D();
    const col = new ColorRgb();
    col.clear();

    if (texture === null) {
      return col;
    }

    if (hitOrContext instanceof ShadingContext) {
      if (!hitOrContext.hasFlag(RayHitFlag.TEXTURE_COORDINATE)) {
        VsdkError.warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
      }
      const ctxTexCoord = hitOrContext.getTexCoord();
      return texture.evaluateColor(ctxTexCoord.x, ctxTexCoord.y);
    }
    else {
      if (!hitOrContext.getTexCoord(texCoord)) {
        VsdkError.warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
      }
      return texture.evaluateColor(texCoord.x, texCoord.y);
    }
  }

  public splitBsdfScatteredPower(hitOrContext: RayHit | ShadingContext, flags: number): ColorRgb {
    if (!(hitOrContext instanceof ShadingContext)) {
      const normal = new Vector3D();
      const texCoord = new Vector3D();
      const localFlags = [0];
      if (!PhongBidirectionalScatteringDistributionFunction.extractHitData(hitOrContext, normal, texCoord, localFlags)) {
        const out = new ColorRgb();
        out.clear();
        return out;
      }
      const context = new ShadingContext(
        hitOrContext.getPoint(),
        hitOrContext.getGeometricNormal(),
        normal,
        texCoord,
        hitOrContext.getUv(),
        hitOrContext.getShadingFrame(),
        hitOrContext.getMaterial(),
        localFlags[0]
      );
      return this.splitBsdfScatteredPower(context, flags);
    }

    const albedo = new ColorRgb();
    albedo.clear();

    if (this.texture !== null && (flags & PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT) !== 0) {
      const textureColor = PhongBidirectionalScatteringDistributionFunction.splitBsdfEvalTexture(this.texture, hitOrContext);
      albedo.add(albedo, textureColor);
      flags &= ~PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT;
    }

    if (this.brdf !== null) {
      const reflectance = this.brdf.reflectance(flags);
      if (!globalThis.Number.isFinite(reflectance.average())) {
        VsdkError.fatal(-1, "brdfReflectance", "Oops - test Rd is not finite!");
      }
      albedo.add(albedo, reflectance);
    }

    if (this.btdf !== null) {
      const transmitted = this.btdf.transmittance(BsdfComponentFlag.getBtdfFlags(flags));
      albedo.add(albedo, transmitted);
    }

    return albedo;
  }

  public splitBsdfIsTextured(): boolean {
    return this.texture !== null;
  }

  private static texturedScattererSample(
    inDirection: Vector3D,
    normal: Vector3D,
    x1: number,
    x2: number,
    probabilityDensityFunction: number[] | null
  ): Vector3D {
    void inDirection;
    const coord = new CoordinateSystem();
    coord.setFromZAxis(normal);
    return coord.sampleHemisphereCosTheta(x1, x2, probabilityDensityFunction);
  }

  private static texturedScattererEvalPdf(
    inDirection: Vector3D,
    out: Vector3D,
    normal: Vector3D,
    probabilityDensityFunction: number[] | null
  ): void {
    void inDirection;
    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, normal.dotProduct(out) / globalThis.Math.PI);
  }

  private splitBsdfProbabilities(
    hitOrContext: RayHit | ShadingContext,
    flags: number,
    inTexture: number[] | null,
    reflection: number[] | null,
    transmission: number[] | null,
    brdfFlags: number[] | null,
    btdfFlags: number[] | null
  ): void {
    PhongBidirectionalScatteringDistributionFunction.setOut(inTexture, 0.0);

    if (this.texture !== null && (flags & PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT) !== 0) {
      const textureColor = PhongBidirectionalScatteringDistributionFunction.splitBsdfEvalTexture(this.texture, hitOrContext);
      PhongBidirectionalScatteringDistributionFunction.setOut(inTexture, textureColor.average());
      flags &= ~PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT;
    }

    PhongBidirectionalScatteringDistributionFunction.setOut(brdfFlags, BsdfComponentFlag.getBrdfFlags(flags));
    PhongBidirectionalScatteringDistributionFunction.setOut(btdfFlags, BsdfComponentFlag.getBtdfFlags(flags));

    const reflectance = this.brdf === null ? new ColorRgb() : this.brdf.reflectance(brdfFlags?.[0] ?? 0);
    PhongBidirectionalScatteringDistributionFunction.setOut(reflection, reflectance.average());

    const transmittance = this.btdf === null ? new ColorRgb() : this.btdf.transmittance(btdfFlags?.[0] ?? 0);
    PhongBidirectionalScatteringDistributionFunction.setOut(transmission, transmittance.average());
  }

  private static splitBsdfSamplingMode(
    texture: number,
    reflection: number,
    transmission: number,
    x1: number[]
  ): SplitBSDFSamplingMode {
    let mode = SplitBSDFSamplingMode.SAMPLE_ABSORPTION;

    if (x1[0] < texture) {
      mode = SplitBSDFSamplingMode.SAMPLE_TEXTURE;
      x1[0] /= texture;
    }
    else {
      x1[0] -= texture;
      if (x1[0] < reflection) {
        mode = SplitBSDFSamplingMode.SAMPLE_REFLECTION;
        x1[0] /= reflection;
      }
      else {
        x1[0] -= reflection;
        if (x1[0] < transmission) {
          mode = SplitBSDFSamplingMode.SAMPLE_TRANSMISSION;
          x1[0] /= transmission;
        }
      }
    }

    return mode;
  }

  public indexOfRefraction(index: RefractionIndex): void {
    if (this.btdf === null) {
      index.set(1.0, 0.0);
    }
    else {
      this.btdf.setIndexOfRefraction(index);
    }
  }

  public sample(
    hitOrContext: RayHit | ShadingContext,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    inDirection: Vector3D,
    doRussianRoulette: number,
    flags: number,
    x1: number,
    x2: number,
    probabilityDensityFunction: number[] | null
  ): Vector3D {
    if (!(hitOrContext instanceof ShadingContext)) {
      const normal0 = new Vector3D();
      const texCoord0 = new Vector3D();
      const localFlags0 = [0];
      if (!PhongBidirectionalScatteringDistributionFunction.extractHitData(hitOrContext, normal0, texCoord0, localFlags0)) {
        const out0 = new Vector3D();
        out0.set(0.0, 0.0, 1.0);
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, 0.0);
        return out0;
      }
      const context0 = new ShadingContext(
        hitOrContext.getPoint(),
        hitOrContext.getGeometricNormal(),
        normal0,
        texCoord0,
        hitOrContext.getUv(),
        hitOrContext.getShadingFrame(),
        hitOrContext.getMaterial(),
        localFlags0[0]
      );
      return this.sample(context0, inBsdf, outBsdf, inDirection, doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    }

    const normal = new Vector3D();
    let out = new Vector3D();

    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, 0.0);
    if (!hitOrContext.hasFlag(RayHitFlag.NORMAL)) {
      VsdkError.warning("sample", "Couldn't determine shading normal");
      out.set(0.0, 0.0, 1.0);
      return out;
    }
    normal.copy(hitOrContext.getShadingNormal());

    const localTexture = [0.0];
    const reflection = [0.0];
    const transmission = [0.0];
    const brdfFlags = [0];
    const btdfFlags = [0];

    this.splitBsdfProbabilities(hitOrContext, flags, localTexture, reflection, transmission, brdfFlags, btdfFlags);

    const scattering = localTexture[0] + reflection[0] + transmission[0];
    if (scattering < Numeric.EPSILON) {
      return out;
    }

    if (doRussianRoulette === 0) {
      localTexture[0] /= scattering;
      reflection[0] /= scattering;
      transmission[0] /= scattering;
    }

    const localX1 = [x1];
    const mode = PhongBidirectionalScatteringDistributionFunction.splitBsdfSamplingMode(
      localTexture[0], reflection[0], transmission[0], localX1
    );

    const inIndex = new RefractionIndex();
    const outIndex = new RefractionIndex();

    if (inBsdf !== null) {
      inBsdf.indexOfRefraction(inIndex);
    }
    if (outBsdf !== null) {
      outBsdf.indexOfRefraction(outIndex);
    }

    let p: number;
    switch (mode) {
      case SplitBSDFSamplingMode.SAMPLE_TEXTURE: {
        const pTexture = [0.0];
        out = PhongBidirectionalScatteringDistributionFunction.texturedScattererSample(
          inDirection, normal, localX1[0], x2, pTexture
        );
        p = pTexture[0];
        if (p < Numeric.EPSILON) {
          return out;
        }
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, localTexture[0] * p);
        break;
      }
      case SplitBSDFSamplingMode.SAMPLE_REFLECTION: {
        if (this.brdf === null) {
          p = 0.0;
        }
        else {
          const pReflection = [0.0];
          out = this.brdf.sample(inDirection, normal, 0, brdfFlags[0], localX1[0], x2, pReflection);
          p = pReflection[0];
        }
        if (p < Numeric.EPSILON) {
          return out;
        }
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, reflection[0] * p);
        break;
      }
      case SplitBSDFSamplingMode.SAMPLE_TRANSMISSION: {
        if (this.btdf === null) {
          p = 0.0;
          out.x = 0.0;
          out.y = 0.0;
          out.z = 0.0;
        }
        else {
          const pTransmission = [0.0];
          out = this.btdf.sample(inIndex, outIndex, inDirection, normal, 0, btdfFlags[0], localX1[0], x2, pTransmission);
          p = pTransmission[0];
        }
        if (p < Numeric.EPSILON) {
          return out;
        }
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, transmission[0] * p);
        break;
      }
      case SplitBSDFSamplingMode.SAMPLE_ABSORPTION:
      default:
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, 0.0);
        return out;
    }

    if (mode !== SplitBSDFSamplingMode.SAMPLE_TEXTURE) {
      const pTexture = [0.0];
      PhongBidirectionalScatteringDistributionFunction.texturedScattererEvalPdf(inDirection, out, normal, pTexture);
      PhongBidirectionalScatteringDistributionFunction.setOut(
        probabilityDensityFunction,
        (probabilityDensityFunction?.[0] ?? 0.0) + localTexture[0] * pTexture[0]
      );
    }

    if (mode !== SplitBSDFSamplingMode.SAMPLE_REFLECTION) {
      let pReflection = 0.0;
      if (this.brdf !== null) {
        const pRR = [0.0];
        const outP = [0.0];
        this.brdf.evaluateProbabilityDensityFunction(inDirection, out, normal, brdfFlags[0], outP, pRR);
        pReflection = outP[0];
      }
      PhongBidirectionalScatteringDistributionFunction.setOut(
        probabilityDensityFunction,
        (probabilityDensityFunction?.[0] ?? 0.0) + reflection[0] * pReflection
      );
    }

    if (mode !== SplitBSDFSamplingMode.SAMPLE_TRANSMISSION) {
      let pTransmission = 0.0;
      if (this.btdf !== null) {
        const pRR = [0.0];
        const outP = [0.0];
        this.btdf.evaluateProbabilityDensityFunction(
          inIndex, outIndex, inDirection, out, normal, btdfFlags[0], outP, pRR
        );
        pTransmission = outP[0];
      }
      PhongBidirectionalScatteringDistributionFunction.setOut(
        probabilityDensityFunction,
        (probabilityDensityFunction?.[0] ?? 0.0) + transmission[0] * pTransmission
      );
    }

    return out;
  }

  private static texturedScattererEval(inDirection: Vector3D, out: Vector3D, normal: Vector3D): number {
    void inDirection;
    void out;
    void normal;
    return 1.0 / globalThis.Math.PI;
  }

  public evaluate(
    hitOrContext: RayHit | ShadingContext,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    inDirection: Vector3D,
    out: Vector3D,
    flags: number
  ): ColorRgb {
    if (!(hitOrContext instanceof ShadingContext)) {
      const normal0 = new Vector3D();
      const texCoord0 = new Vector3D();
      const localFlags0 = [0];
      if (!PhongBidirectionalScatteringDistributionFunction.extractHitData(hitOrContext, normal0, texCoord0, localFlags0)) {
        const result0 = new ColorRgb();
        result0.clear();
        return result0;
      }
      const context0 = new ShadingContext(
        hitOrContext.getPoint(),
        hitOrContext.getGeometricNormal(),
        normal0,
        texCoord0,
        hitOrContext.getUv(),
        hitOrContext.getShadingFrame(),
        hitOrContext.getMaterial(),
        localFlags0[0]
      );
      return this.evaluate(context0, inBsdf, outBsdf, inDirection, out, flags);
    }

    const result = new ColorRgb();
    const normal = new Vector3D();

    result.clear();
    if (!hitOrContext.hasFlag(RayHitFlag.NORMAL)) {
      VsdkError.warning("evaluate", "Couldn't determine shading normal");
      return result;
    }
    normal.copy(hitOrContext.getShadingNormal());

    if (this.texture !== null && (flags & PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT) !== 0) {
      const textureBsdf = PhongBidirectionalScatteringDistributionFunction.texturedScattererEval(inDirection, out, normal);
      const textureCol = PhongBidirectionalScatteringDistributionFunction.splitBsdfEvalTexture(this.texture, hitOrContext);
      result.addScaled(result, textureBsdf, textureCol);
      flags &= ~PhongBidirectionalScatteringDistributionFunction.TEXTURED_COMPONENT;
    }

    if (this.brdf !== null) {
      const reflectionCol = this.brdf.evaluate(inDirection, out, normal, BsdfComponentFlag.getBrdfFlags(flags));
      result.add(result, reflectionCol);

      const inIndex = new RefractionIndex();
      const outIndex = new RefractionIndex();
      let refractionCol: ColorRgb;

      if (inBsdf !== null) {
        inBsdf.indexOfRefraction(inIndex);
      }
      if (outBsdf !== null) {
        outBsdf.indexOfRefraction(outIndex);
      }

      if (this.btdf === null) {
        refractionCol = new ColorRgb();
        refractionCol.clear();
      }
      else {
        refractionCol = this.btdf.evaluate(
          inIndex, outIndex, inDirection, out, normal, BsdfComponentFlag.getBtdfFlags(flags)
        );
      }

      result.add(result, refractionCol);
    }

    return result;
  }

  public evaluateProbabilityDensityFunction(
    hitOrContext: RayHit | ShadingContext,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    inDirection: Vector3D,
    out: Vector3D,
    flags: number,
    probabilityDensityFunction: number[] | null,
    probabilityDensityFunctionRR: number[] | null
  ): void {
    if (!(hitOrContext instanceof ShadingContext)) {
      const normal0 = new Vector3D();
      const texCoord0 = new Vector3D();
      const localFlags0 = [0];
      if (!PhongBidirectionalScatteringDistributionFunction.extractHitData(hitOrContext, normal0, texCoord0, localFlags0)) {
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, 0.0);
        PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunctionRR, 0.0);
        return;
      }
      const context0 = new ShadingContext(
        hitOrContext.getPoint(),
        hitOrContext.getGeometricNormal(),
        normal0,
        texCoord0,
        hitOrContext.getUv(),
        hitOrContext.getShadingFrame(),
        hitOrContext.getMaterial(),
        localFlags0[0]
      );
      this.evaluateProbabilityDensityFunction(
        context0, inBsdf, outBsdf, inDirection, out, flags, probabilityDensityFunction, probabilityDensityFunctionRR
      );
      return;
    }

    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, 0.0);
    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunctionRR, 0.0);

    const normal = new Vector3D();
    if (!hitOrContext.hasFlag(RayHitFlag.NORMAL)) {
      VsdkError.warning("evaluateProbabilityDensityFunction", "Couldn't determine shading normal");
      return;
    }
    normal.copy(hitOrContext.getShadingNormal());

    const pTexture = [0.0];
    const pReflection = [0.0];
    const pTransmission = [0.0];
    const brdfFlags = [0];
    const btdfFlags = [0];

    this.splitBsdfProbabilities(hitOrContext, flags, pTexture, pReflection, pTransmission, brdfFlags, btdfFlags);

    const pScattering = pTexture[0] + pReflection[0] + pTransmission[0];
    if (pScattering < Numeric.EPSILON) {
      return;
    }

    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunctionRR, pScattering);

    const inIndex = new RefractionIndex();
    const outIndex = new RefractionIndex();
    if (inBsdf === null) {
      inIndex.set(1.0, 0.0);
    }
    else {
      inBsdf.indexOfRefraction(inIndex);
    }

    if (outBsdf === null) {
      outIndex.set(1.0, 0.0);
    }
    else {
      outBsdf.indexOfRefraction(outIndex);
    }

    const p = [0.0];
    PhongBidirectionalScatteringDistributionFunction.texturedScattererEvalPdf(inDirection, out, normal, p);
    PhongBidirectionalScatteringDistributionFunction.setOut(probabilityDensityFunction, pTexture[0] * p[0]);

    if (this.brdf === null) {
      p[0] = 0.0;
    }
    else {
      const pRR = [0.0];
      this.brdf.evaluateProbabilityDensityFunction(inDirection, out, normal, brdfFlags[0], p, pRR);
    }
    PhongBidirectionalScatteringDistributionFunction.setOut(
      probabilityDensityFunction,
      (probabilityDensityFunction?.[0] ?? 0.0) + pReflection[0] * p[0]
    );

    if (this.btdf === null) {
      p[0] = 0.0;
    }
    else {
      const pRR = [0.0];
      this.btdf.evaluateProbabilityDensityFunction(inIndex, outIndex, inDirection, out, normal, btdfFlags[0], p, pRR);
    }
    PhongBidirectionalScatteringDistributionFunction.setOut(
      probabilityDensityFunction,
      (probabilityDensityFunction?.[0] ?? 0.0) + pTransmission[0] * p[0]
    );

    PhongBidirectionalScatteringDistributionFunction.setOut(
      probabilityDensityFunction,
      (probabilityDensityFunction?.[0] ?? 0.0) / pScattering
    );
  }

  public bsdfEvalComponents(
    hitOrContext: RayHit | ShadingContext,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    inDirection: Vector3D,
    out: Vector3D,
    flags: number,
    colArray: ColorRgb[]
  ): ColorRgb {
    if (!(hitOrContext instanceof ShadingContext)) {
      const normal0 = new Vector3D();
      const texCoord0 = new Vector3D();
      const localFlags0 = [0];
      if (!PhongBidirectionalScatteringDistributionFunction.extractHitData(hitOrContext, normal0, texCoord0, localFlags0)) {
        const result0 = new ColorRgb();
        result0.clear();
        return result0;
      }
      const context0 = new ShadingContext(
        hitOrContext.getPoint(),
        hitOrContext.getGeometricNormal(),
        normal0,
        texCoord0,
        hitOrContext.getUv(),
        hitOrContext.getShadingFrame(),
        hitOrContext.getMaterial(),
        localFlags0[0]
      );
      return this.bsdfEvalComponents(context0, inBsdf, outBsdf, inDirection, out, flags, colArray);
    }

    const result = new ColorRgb();
    const empty = new ColorRgb();
    empty.clear();
    result.clear();

    for (let i = 0; i < BsdfComponentInfo.BSDF_COMPONENTS; i++) {
      const thisFlag = BsdfComponentFlag.bsdfIndexToComp(i);
      if ((flags & thisFlag) !== 0) {
        colArray[i] = this.evaluate(hitOrContext, inBsdf, outBsdf, inDirection, out, thisFlag);
        result.add(result, colArray[i]);
      }
      else {
        colArray[i] = new ColorRgb(empty.r, empty.g, empty.b);
      }
    }

    return result;
  }

  private static setOut(out: number[] | null, value: number): void;
  private static setOut(out: number[] | null, value: number): void {
    if (out !== null && out.length > 0) {
      out[0] = value;
    }
  }

  private static setOutInt(out: number[] | null, value: number): void {
    if (out !== null && out.length > 0) {
      out[0] = value;
    }
  }
}
