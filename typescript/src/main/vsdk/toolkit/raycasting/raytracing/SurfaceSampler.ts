import { ColorRgb } from "../../common/color/ColorRgb";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { BsdfComp } from "../common/BsdfComp";
import { PathRayType } from "../common/PathRayType";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "./Sampler";

export abstract class SurfaceSampler extends Sampler {
  protected m_computeFromNextPdf: boolean;
  protected m_computeBsdfComponents: boolean;

  public constructor() {
    super();
    this.m_computeFromNextPdf = false;
    this.m_computeBsdfComponents = false;
  }

  protected static DetermineRayType(
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    dir: Vector3D
  ): void {
    const cosThisPatch = dir.dotProduct(thisNode.m_normal);

    if (cosThisPatch < 0.0) {
      if ((thisNode.m_hit.getFlags() & RayHitFlag.BACK) !== 0) {
        thisNode.m_rayType = PathRayType.LEAVES;
      }
      else {
        thisNode.m_rayType = PathRayType.ENTERS;
      }

      newNode.m_inBsdf = thisNode.m_outBsdf;
    }
    else {
      thisNode.m_rayType = PathRayType.REFLECTS;
      newNode.m_inBsdf = thisNode.m_inBsdf;
    }
  }

  public DoBsdfEval(
    bsdf: PhongBidirectionalScatteringDistributionFunction | null,
    hit: RayHit,
    inBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    outBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    incoming: Vector3D,
    outgoing: Vector3D,
    flags: number,
    bsdfComp: BsdfComp | null
  ): ColorRgb {
    if (this.m_computeBsdfComponents) {
      if (bsdf === null) {
        const black = new ColorRgb();
        black.clear();
        return black;
      }

      const localBsdfComp = bsdfComp !== null ? bsdfComp : new BsdfComp();
      return bsdf.bsdfEvalComponents(hit, inBsdf, outBsdf, incoming, outgoing, flags, localBsdfComp.asArray());
    }

    if (bsdfComp !== null) {
      bsdfComp.Clear();
    }
    let radiance = new ColorRgb();
    if (bsdf === null) {
      radiance.clear();
    }
    else {
      radiance = bsdf.evaluate(hit, inBsdf, outBsdf, incoming, outgoing, flags);
    }
    return radiance;
  }

  public abstract override sample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR?: boolean,
    flags?: number
  ): boolean;

  public abstract override evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags?: number,
    probabilityDensityFunction?: number[] | null,
    probabilityDensityFunctionRR?: number[] | null
  ): number;

  public abstract EvalPDFPrev(
    prevNode: SimpleRaytracingPathNode,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags?: number,
    probabilityDensityFunction?: number[] | null,
    probabilityDensityFunctionRR?: number[] | null
  ): number;

  public SetComputeFromNextPdf(computeFromNextPdf: boolean): void {
    this.m_computeFromNextPdf = computeFromNextPdf;
  }

  public SetComputeBsdfComponents(computeBsdfComponents: boolean): void {
    this.m_computeBsdfComponents = computeBsdfComponents;
  }
}
