import { ColorRgb } from "../common/color/ColorRgb";
import { Error as VsdkError } from "../common/Error";
import { RenderOptions } from "../common/RenderOptions";
import { Matrix2x2 } from "../common/linealAlgebra/Matrix2x2";
import { Vector2D } from "../common/linealAlgebra/Vector2D";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../material/BsdfComponent";
import { XxdfComponentFlag } from "../material/XxdfComponentFlag";
import { PatchVisitor } from "../numericalAnalysis/PatchVisitor";
import { QuadCubatureRule } from "../numericalAnalysis/QuadCubatureRule";
import { TriangleCubatureRule } from "../numericalAnalysis/TriangleCubatureRule";
import { Polygon } from "../scene/Polygon";
import { BoundingBox } from "../skin/BoundingBox";
import { Element } from "../skin/Element";
import { ElementFlags } from "../skin/ElementFlags";
import { ElementTypes } from "../skin/ElementTypes";
import { Geometry } from "../skin/Geometry";
import { Patch } from "../skin/Patch";
import { GalerkinBasis } from "./GalerkinBasis";
import { GalerkinBasisType } from "./GalerkinBasisType";
import { GalerkinElementRenderMode } from "./GalerkinElementRenderMode";
import { GalerkinIterationMethod } from "./GalerkinIterationMethod";
import { GalerkinState } from "./GalerkinState";

export class GalerkinElement extends Element {
  private static numberOfElements = 0;
  private static numberOfClusters = 0;

  public patch: Patch | null;
  public geometry: Geometry | null;
  public potential: number;
  public receivedPotential: number;
  public unShotPotential: number;
  public directPotential: number;
  public interactions: unknown[];
  public minimumArea: number;
  public blockerSize: number;
  public numberOfPatches: number;
  public scratchVisibilityUsageCounter: number;
  public childNumber: number;
  public basisSize: number;
  public basisUsed: number;
  public galerkinState: GalerkinState;

  private static readonly quadToParentTransformMatrix = GalerkinElement.buildQuadTransforms();
  private static readonly triangleToParentTransformMatrix = GalerkinElement.buildTriangleTransforms();

  private static buildQuadTransforms(): Matrix2x2[] {
    const t = new Array<Matrix2x2>(4);
    for (let i = 0; i < 4; i++) {
      t[i] = new Matrix2x2();
      t[i].m[0][0] = 0.5;
      t[i].m[0][1] = 0.0;
      t[i].m[1][0] = 0.0;
      t[i].m[1][1] = 0.5;
    }
    t[0].t[0] = 0.0;
    t[0].t[1] = 0.0;
    t[1].t[0] = 0.5;
    t[1].t[1] = 0.0;
    t[2].t[0] = 0.0;
    t[2].t[1] = 0.5;
    t[3].t[0] = 0.5;
    t[3].t[1] = 0.5;
    return t;
  }

  private static buildTriangleTransforms(): Matrix2x2[] {
    const t = new Array<Matrix2x2>(4);
    for (let i = 0; i < 4; i++) {
      t[i] = new Matrix2x2();
    }

    t[0].m[0][0] = 0.5;
    t[0].m[0][1] = 0.0;
    t[0].m[1][0] = 0.0;
    t[0].m[1][1] = 0.5;
    t[0].t[0] = 0.0;
    t[0].t[1] = 0.0;

    t[1].m[0][0] = 0.5;
    t[1].m[0][1] = 0.0;
    t[1].m[1][0] = 0.0;
    t[1].m[1][1] = 0.5;
    t[1].t[0] = 0.5;
    t[1].t[1] = 0.0;

    t[2].m[0][0] = 0.5;
    t[2].m[0][1] = 0.0;
    t[2].m[1][0] = 0.0;
    t[2].m[1][1] = 0.5;
    t[2].t[0] = 0.0;
    t[2].t[1] = 0.5;

    t[3].m[0][0] = -0.5;
    t[3].m[0][1] = 0.0;
    t[3].m[1][0] = 0.0;
    t[3].m[1][1] = -0.5;
    t[3].t[0] = 0.5;
    t[3].t[1] = 0.5;

    return t;
  }

  public constructor(inGalerkinState: GalerkinState);
  public constructor(inPatch: Patch, inGalerkinState: GalerkinState);
  public constructor(inGeometry: Geometry, inGalerkinState: GalerkinState);
  public constructor(first: GalerkinState | Patch | Geometry, inGalerkinState?: GalerkinState) {
    super();
    const localGalerkinState = inGalerkinState !== undefined ? inGalerkinState : (first as GalerkinState);

    GalerkinElement.numberOfElements++;
    this.id = GalerkinElement.numberOfElements;
    this.className = ElementTypes.ELEMENT_GALERKIN;
    this.patch = null;
    this.geometry = null;
    this.potential = 0.0;
    this.receivedPotential = 0.0;
    this.unShotPotential = 0.0;
    this.directPotential = 0.0;
    this.interactions = [];
    this.minimumArea = 0.0;
    this.blockerSize = 0.0;
    this.numberOfPatches = 0;
    this.scratchVisibilityUsageCounter = 0;
    this.childNumber = 0;
    this.basisSize = 0;
    this.basisUsed = 0;
    this.galerkinState = localGalerkinState;

    if (inGalerkinState === undefined) {
      return;
    }

    if (first instanceof Patch) {
      const inPatch = first;
      this.patch = inPatch;
      this.geometry = null;
      this.flags &= ~ElementFlags.IS_CLUSTER_MASK;
      this.area = inPatch === null ? 0.0 : inPatch.area;
      this.minimumArea = this.area;
      this.blockerSize = 2.0 * Math.sqrt(this.area / Math.PI);
      this.numberOfPatches = inPatch === null ? 0 : 1;
      this.directPotential = inPatch === null ? 0.0 : inPatch.directPotential;

      if (inPatch !== null) {
        this.Rd = PatchVisitor.averageNormalAlbedo(inPatch, BsdfComponent.BRDF_DIFFUSE_COMPONENT);
        if (inPatch.material !== null && inPatch.material.getEdf() !== null) {
          this.flags |= ElementFlags.IS_LIGHT_SOURCE_MASK;
          this.Ed = PatchVisitor.averageEmittance(inPatch, XxdfComponentFlag.DIFFUSE_COMPONENT);
          this.Ed.scaleInverse(Math.PI, this.Ed);
        }
        inPatch.radianceData = this;
      }

      this.reAllocCoefficients();
      return;
    }

    const inGeometry = first as Geometry;
    this.geometry = inGeometry;
    this.patch = null;
    this.flags |= ElementFlags.IS_CLUSTER_MASK;
    this.Rd.setMonochrome(1.0);
    this.reAllocCoefficients();
    GalerkinElement.numberOfClusters++;
  }

  public static getNumberOfElements(): number {
    return GalerkinElement.numberOfElements;
  }

  public static getNumberOfClusters(): number {
    return GalerkinElement.numberOfClusters;
  }

  public static getNumberOfSurfaceElements(): number {
    return GalerkinElement.numberOfElements - GalerkinElement.numberOfClusters;
  }

  public static fromPatch(patch: Patch | null): GalerkinElement | null {
    if (patch === null || patch.radianceData === null || !(patch.radianceData instanceof GalerkinElement)) {
      return null;
    }
    return patch.radianceData as GalerkinElement;
  }

  public static initializeBasis(): void {
    GalerkinBasis.computeRegularFilterCoefficients(
      GalerkinBasis.mutableBasisForVertexCount(4),
      GalerkinElement.quadToParentTransformMatrix,
      QuadCubatureRule.degree8QuadrilateralRule(),
    );
    GalerkinBasis.computeRegularFilterCoefficients(
      GalerkinBasis.mutableBasisForVertexCount(3),
      GalerkinElement.triangleToParentTransformMatrix,
      TriangleCubatureRule.degree8Rule(),
    );
  }

  public static renderMode(renderOptions: RenderOptions | null): number {
    if (renderOptions === null) {
      return GalerkinElementRenderMode.FLAT;
    }

    let renderCode = 0;
    if (renderOptions.drawOutlines) {
      renderCode |= GalerkinElementRenderMode.OUTLINE;
    }
    if (renderOptions.smoothShading) {
      renderCode |= GalerkinElementRenderMode.GOURAUD;
    }
    else {
      renderCode |= GalerkinElementRenderMode.FLAT;
    }

    return renderCode;
  }

  public regularSubDivide(): void {
    if (this.isCluster()) {
      return;
    }
    if (this.regularSubElements !== null || this.patch === null) {
      return;
    }

    const children: Array<Element | null> = new Array<Element | null>(4).fill(null);
    for (let i = 0; i < 4; i++) {
      const child = new GalerkinElement(this.galerkinState);
      child.patch = this.patch;
      child.parent = this;
      child.transformToParent = (this.patch.numberOfVertices === 3)
        ? GalerkinElement.triangleToParentTransformMatrix[i]
        : GalerkinElement.quadToParentTransformMatrix[i];
      child.area = 0.25 * this.area;
      child.blockerSize = 2.0 * Math.sqrt(child.area / Math.PI);
      child.childNumber = i;
      child.reAllocCoefficients();
      GalerkinBasis.push(this, this.radiance, child, child.radiance);
      child.potential = this.potential;
      child.directPotential = this.directPotential;
      if (this.galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        GalerkinBasis.push(this, this.unShotRadiance, child, child.unShotRadiance);
        child.unShotPotential = this.unShotPotential;
      }
      child.flags |= (this.flags & ElementFlags.IS_LIGHT_SOURCE_MASK);
      child.Rd = this.Rd;
      child.Ed = this.Ed;
      children[i] = child;
    }
    this.regularSubElements = children;
  }

  private regularSubElementAtPoint(u: number[], v: number[]): GalerkinElement {
    if (this.isCluster() || this.regularSubElements === null || this.patch === null || u === null || v === null) {
      return this;
    }

    let childElement: Element | null;
    const uu = u[0];
    const vv = v[0];
    switch (this.patch.numberOfVertices) {
      case 3:
        if (uu + vv <= 0.5) {
          childElement = this.regularSubElements[0];
          u[0] = uu * 2.0;
          v[0] = vv * 2.0;
        }
        else if (uu > 0.5) {
          childElement = this.regularSubElements[1];
          u[0] = (uu - 0.5) * 2.0;
          v[0] = vv * 2.0;
        }
        else if (vv > 0.5) {
          childElement = this.regularSubElements[2];
          u[0] = uu * 2.0;
          v[0] = (vv - 0.5) * 2.0;
        }
        else {
          childElement = this.regularSubElements[3];
          u[0] = (0.5 - uu) * 2.0;
          v[0] = (0.5 - vv) * 2.0;
        }
        break;
      case 4:
        if (vv <= 0.5) {
          if (uu < 0.5) {
            childElement = this.regularSubElements[0];
            u[0] = uu * 2.0;
          }
          else {
            childElement = this.regularSubElements[1];
            u[0] = (uu - 0.5) * 2.0;
          }
          v[0] = vv * 2.0;
        }
        else {
          if (uu < 0.5) {
            childElement = this.regularSubElements[2];
            u[0] = uu * 2.0;
          }
          else {
            childElement = this.regularSubElements[3];
            u[0] = (uu - 0.5) * 2.0;
          }
          v[0] = (vv - 0.5) * 2.0;
        }
        break;
      default:
        return this;
    }

    if (childElement instanceof GalerkinElement) {
      return childElement;
    }
    return this;
  }

  public regularLeafAtPoint(u: number[], v: number[]): GalerkinElement {
    let leaf: GalerkinElement = this;
    while (leaf.regularSubElements !== null) {
      leaf = leaf.regularSubElementAtPoint(u, v);
    }
    return leaf;
  }

  public vertices(p: Vector3D[] | null): number {
    if (p === null) {
      return 0;
    }

    if (this.isCluster()) {
      const boundingBox = new BoundingBox();
      this.bounds(boundingBox);
      boundingBox.corners(p);
      return 8;
    }

    if (this.patch === null) {
      return 0;
    }

    const topTrans = new Matrix2x2();
    const uv = new Vector2D();
    if (this.transformToParent !== null) {
      this.topTransform(topTrans);
    }

    uv.x = 0.0;
    uv.y = 0.0;
    if (this.transformToParent !== null) {
      topTrans.transformPoint2D(uv, uv);
    }
    this.patch.uniformPoint(uv.x, uv.y, p[0]);

    uv.x = 1.0;
    uv.y = 0.0;
    if (this.transformToParent !== null) {
      topTrans.transformPoint2D(uv, uv);
    }
    this.patch.uniformPoint(uv.x, uv.y, p[1]);

    if (this.patch.numberOfVertices === 4) {
      uv.x = 1.0;
      uv.y = 1.0;
      if (this.transformToParent !== null) {
        topTrans.transformPoint2D(uv, uv);
      }
      this.patch.uniformPoint(uv.x, uv.y, p[2]);

      uv.x = 0.0;
      uv.y = 1.0;
      if (this.transformToParent !== null) {
        topTrans.transformPoint2D(uv, uv);
      }
      this.patch.uniformPoint(uv.x, uv.y, p[3]);
    }
    else {
      uv.x = 0.0;
      uv.y = 1.0;
      if (this.transformToParent !== null) {
        topTrans.transformPoint2D(uv, uv);
      }
      this.patch.uniformPoint(uv.x, uv.y, p[2]);
      if (p.length > 3 && p[3] !== null) {
        p[3].set(0.0, 0.0, 0.0);
      }
    }

    return this.patch.numberOfVertices;
  }

  public bounds(boundingBox: BoundingBox | null): BoundingBox | null {
    if (boundingBox === null) {
      return null;
    }

    if (this.isCluster() && this.geometry !== null) {
      boundingBox.copyFrom(this.geometry.boundingBox);
      return boundingBox;
    }

    const p = [new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()];
    const numberOfVertices = this.vertices(p);
    for (let i = 0; i < numberOfVertices; i++) {
      boundingBox.enlargeToIncludePoint(p[i]);
    }
    return boundingBox;
  }

  public midPoint(): Vector3D {
    if (this.isCluster()) {
      return this.geometry !== null ? this.geometry.boundingBox.center() : new Vector3D();
    }

    const p = [new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D()];
    const numberOfVertices = this.vertices(p);
    const c = new Vector3D();
    for (let i = 0; i < numberOfVertices; i++) {
      c.addition(c, p[i]);
    }
    if (numberOfVertices > 0) {
      c.scaledCopy(1.0 / numberOfVertices, c);
    }
    return c;
  }

  public initPolygon(polygon: Polygon | null): void {
    if (polygon === null) {
      return;
    }
    if (this.isCluster()) {
      VsdkError.fatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
      return;
    }
    if (this.patch === null) {
      return;
    }

    polygon.normal.set(this.patch.normal.x, this.patch.normal.y, this.patch.normal.z);
    polygon.planeConstant = this.patch.planeConstant;
    polygon.numberOfVertices = this.vertices(polygon.vertex);
    polygon.bounds = new BoundingBox();
    for (let i = 0; i < polygon.numberOfVertices; i++) {
      polygon.bounds.enlargeToIncludePoint(polygon.vertex[i]);
    }
  }

  public reAllocCoefficients(): void {
    let localBasisSize: number;

    if (this.isCluster()) {
      localBasisSize = 1;
    }
    else {
      switch (this.galerkinState.basisType) {
        case GalerkinBasisType.GALERKIN_CONSTANT:
          localBasisSize = 1;
          break;
        case GalerkinBasisType.GALERKIN_LINEAR:
          localBasisSize = 3;
          break;
        case GalerkinBasisType.GALERKIN_QUADRATIC:
          localBasisSize = 6;
          break;
        case GalerkinBasisType.GALERKIN_CUBIC:
          localBasisSize = 10;
          break;
        default:
          localBasisSize = 1;
          break;
      }
    }

    const defaultRadiance = new Array<ColorRgb>(localBasisSize);
    const defaultReceivedRadiance = new Array<ColorRgb>(localBasisSize);
    const defaultUnShotRadiance = new Array<ColorRgb>(localBasisSize);
    for (let i = 0; i < localBasisSize; i++) {
      defaultRadiance[i] = new ColorRgb();
      defaultReceivedRadiance[i] = new ColorRgb();
      defaultUnShotRadiance[i] = new ColorRgb();
    }
    ColorRgb.arrayClear(defaultRadiance, localBasisSize);
    ColorRgb.arrayClear(defaultReceivedRadiance, localBasisSize);
    ColorRgb.arrayClear(defaultUnShotRadiance, localBasisSize);

    if (this.radiance !== null) {
      ColorRgb.arrayCopy(defaultRadiance, this.radiance, Math.min(this.basisSize, localBasisSize));
    }
    if (this.receivedRadiance !== null) {
      ColorRgb.arrayCopy(defaultReceivedRadiance, this.receivedRadiance, Math.min(this.basisSize, localBasisSize));
    }
    if (this.unShotRadiance !== null) {
      ColorRgb.arrayCopy(defaultUnShotRadiance, this.unShotRadiance, Math.min(this.basisSize, localBasisSize));
    }

    this.radiance = defaultRadiance;
    this.receivedRadiance = defaultReceivedRadiance;
    this.unShotRadiance = defaultUnShotRadiance;
    this.basisSize = localBasisSize;
    this.basisUsed = localBasisSize;
  }

  public getPatch(): Patch | null {
    return this.patch;
  }

  public setPatch(inPatch: Patch): void {
    this.patch = inPatch;
  }
}
