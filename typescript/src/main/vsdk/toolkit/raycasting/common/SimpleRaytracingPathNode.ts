import { PrintStream } from "../../../../java/io/PrintStream";
import { ColorRgb } from "../../common/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Vector3DPrinter } from "../../io/wrapper/Vector3DPrinter";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { RayHitFlag } from "../../material/RayHitFlag";
import { RayHit } from "../../skin/RayHit";
import { BsdfComp } from "./BsdfComp";
import { PathRayType } from "./PathRayType";

export class SimpleRaytracingPathNode {
  public static multipleImportanceSampling(a: number): number {
    return a * a;
  }

  public m_hit: RayHit;
  public m_inDirT: Vector3D;
  public m_inDirF: Vector3D;
  public m_normal: Vector3D;

  public m_G: number;
  public m_pdfFromPrev: number;
  public m_pdfFromNext: number;
  public m_rrPdfFromNext: number;
  public accumulatedRussianRouletteFactors: number;

  public m_bsdfEval: ColorRgb;
  public m_bsdfComp: BsdfComp;

  public m_usedComponents: number;
  public m_accUsedComponents: number;

  public m_useBsdf: PhongBidirectionalScatteringDistributionFunction | null;
  public m_inBsdf: PhongBidirectionalScatteringDistributionFunction | null;
  public m_outBsdf: PhongBidirectionalScatteringDistributionFunction | null;
  public m_rayType: PathRayType;
  public m_depth: number;

  private m_next: SimpleRaytracingPathNode | null;
  private m_previous: SimpleRaytracingPathNode | null;

  public constructor() {
    this.m_hit = new RayHit();
    this.m_inDirT = new Vector3D();
    this.m_inDirF = new Vector3D();
    this.m_normal = new Vector3D();

    this.m_G = 0.0;
    this.m_pdfFromPrev = 0.0;
    this.m_pdfFromNext = 0.0;
    this.m_rrPdfFromNext = 0.0;
    this.accumulatedRussianRouletteFactors = 0.0;

    this.m_bsdfEval = new ColorRgb();
    this.m_bsdfComp = new BsdfComp();

    this.m_usedComponents = 0;
    this.m_accUsedComponents = 0;
    this.m_useBsdf = null;
    this.m_inBsdf = null;
    this.m_outBsdf = null;
    this.m_rayType = PathRayType.STARTS;
    this.m_depth = 0;

    this.m_next = null;
    this.m_previous = null;
  }

  public next(): SimpleRaytracingPathNode | null {
    return this.m_next;
  }

  public previous(): SimpleRaytracingPathNode | null {
    return this.m_previous;
  }

  public setNext(node: SimpleRaytracingPathNode | null): void {
    this.m_next = node;
  }

  public setPrevious(node: SimpleRaytracingPathNode | null): void {
    this.m_previous = node;
  }

  public attach(node: SimpleRaytracingPathNode): void {
    this.m_next = node;
    node.setPrevious(this);
  }

  public ensureNext(): void {
    if (this.m_next === null) {
      this.attach(new SimpleRaytracingPathNode());
    }
  }

  public print(out: PrintStream | null): void {
    if (out === null) {
      return;
    }

    out.printf("Path node at depth %d\n", this.m_depth);
    out.printf("Pos : ");
    Vector3DPrinter.print(this.m_hit.getPoint(), out);
    out.printf("\n");
    out.printf("Norm: ");
    Vector3DPrinter.print(this.m_normal, out);
    out.printf("\n");
    if (this.m_previous !== null) {
      out.printf("InF: ");
      Vector3DPrinter.print(this.m_inDirF, out);
      out.printf("\n");
      out.printf("Cos in  %f\n", this.m_normal.dotProduct(this.m_inDirF));
      const patch = this.m_hit.getPatch();
      if (patch !== null) {
        out.printf("GCos in %f\n", patch.normal.dotProduct(this.m_inDirF));
      }
    }
    if (this.m_next !== null) {
      out.printf("OutF: ");
      Vector3DPrinter.print(this.m_next.m_inDirT, out);
      out.printf("\n");
      out.printf("Cos out %f\n", this.m_normal.dotProduct(this.m_next.m_inDirT));
      const patch = this.m_hit.getPatch();
      if (patch !== null) {
        out.printf("GCos out %f\n", patch.normal.dotProduct(this.m_next.m_inDirT));
      }
    }
  }

  protected GetMatchingNode(): SimpleRaytracingPathNode | null {
    const thisBsdf = this.m_useBsdf;
    let backHits = 1;
    let tmpNode = this.previous();
    let matchedNode: SimpleRaytracingPathNode | null = null;

    while (tmpNode !== null && backHits > 0) {
      switch (tmpNode.m_rayType) {
        case PathRayType.ENTERS: {
          const hitPatch = tmpNode.m_hit.getPatch();
          const hitBsdf = hitPatch !== null && hitPatch.material !== null
            ? hitPatch.material.getBsdf()
            : null;
          if (hitBsdf === thisBsdf) {
            backHits--;
          }
          break;
        }
        case PathRayType.LEAVES:
          if (tmpNode.m_inBsdf === thisBsdf) {
            backHits++;
          }
          break;
        case PathRayType.REFLECTS:
          break;
        default:
          VsdkError.error("CPathNode::GetMatchingNode", "Wrong ray type in path");
      }

      matchedNode = tmpNode;
      tmpNode = tmpNode.previous();
    }

    return backHits === 0 ? matchedNode : null;
  }

  public getPreviousBsdf(): PhongBidirectionalScatteringDistributionFunction | null {
    if ((this.m_hit.getFlags() & RayHitFlag.BACK) === 0) {
      VsdkError.error("CPathNode::getPreviousBsdf", "Last node not a back hit");
      return this.m_inBsdf;
    }

    const patch = this.m_hit.getPatch();
    const patchBsdf = patch !== null && patch.material !== null ? patch.material.getBsdf() : null;
    if (patchBsdf !== this.m_inBsdf) {
      VsdkError.warning("CPathNode::GetPreviousBtdf", "Last back hit has wrong bsdf");
    }

    const matchedNode = this.GetMatchingNode();

    if (matchedNode === null) {
      VsdkError.warning("CPathNode::GetPreviousBtdf", "No corresponding entering ray");
      return this.m_inBsdf;
    }

    return matchedNode.m_inBsdf;
  }

  public assignBsdfAndNormal(): void {
    const patch = this.m_hit.getPatch();
    if (patch === null || patch.material === null) {
      return;
    }

    this.m_normal.copy(this.m_hit.getNormal());

    this.m_useBsdf = patch.material.getBsdf();

    if ((this.m_hit.getFlags() & RayHitFlag.FRONT) !== 0) {
      this.m_outBsdf = this.m_useBsdf;
    }
    else {
      this.m_outBsdf = this.getPreviousBsdf();
    }
  }

  public ends(): boolean {
    return this.m_rayType === PathRayType.STOPS || this.m_rayType === PathRayType.ENVIRONMENT;
  }
}
