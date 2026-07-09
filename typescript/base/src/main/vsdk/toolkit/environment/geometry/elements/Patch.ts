import { ColorRgb } from "../../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../../common/logging/Logger";
import { CoordinateAxis } from "../../../common/linealAlgebra/CoordinateAxis";
import { Jacobian } from "../../../common/linealAlgebra/Jacobian";
import { Numeric } from "../../../common/linealAlgebra/Numeric";
import { Ray } from "../../../common/linealAlgebra/Ray";
import { Vector2Dd } from "../../../common/linealAlgebra/Vector2Dd";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../../common/statistics/Statistics";
import { Material } from "../../../material/Material";
import { RayHitFlag } from "./RayHitFlag";
import type { Element } from "./Element";
import { BoundingBox } from "../../../skin/AxisAlignedBoundingBox";
import { RayHit } from "./RayHit";
import { Vertex } from "./Vertex";

export class Patch {
  public static readonly MAXIMUM_VERTICES_PER_PATCH = 4;
  public static readonly PATCH_VISIBILITY = 0x01;
  public static readonly MAX_EXCLUDED_PATCHES = 4;
  private static readonly TOLERANCE = 1e-5;

  private static patchId = 1;
  private static readonly excludedPatches: Array<Patch | null> = [null, null, null, null];

  private flags: number;

  public id: number;
  public twin: Patch | null;
  public vertex: Array<Vertex | null>;
  public numberOfVertices: number;
  public boundingBox: BoundingBox | null;
  public normal: Vector3D;
  public planeConstant: number;
  public tolerance: number;
  public area: number;
  public midPoint: Vector3D;
  public jacobian: Jacobian | null;
  public directPotential: number;
  public index: CoordinateAxis;
  public omit: number;
  public color: ColorRgb;
  public radianceData: Element | null;
  public material: Material | null;

  public static getNextId(): number {
    return Patch.patchId;
  }

  public static setNextId(id: number): void {
    Patch.patchId = id;
  }

  private isExcluded(): boolean {
    for (let i = 0; i < Patch.MAX_EXCLUDED_PATCHES; i++) {
      if (Patch.excludedPatches[i] === this) {
        return true;
      }
    }
    return false;
  }

  private allVerticesHaveANormal(): boolean {
    let i = 0;
    for (; i < this.numberOfVertices; i++) {
      if (this.vertex[i]?.normal === null) {
        break;
      }
    }
    return i >= this.numberOfVertices;
  }

  private static fma(a: number, b: number, c: number): number {
    return a * b + c;
  }

  private static pointInTriangle(
    v0: Vector3D,
    v1: Vector3D,
    v2: Vector3D,
    u: number,
    v: number,
    p: Vector3D
  ): void {
    p.x = Patch.fma(v, (v2.x - v0.x), Patch.fma(u, (v1.x - v0.x), v0.x));
    p.y = Patch.fma(v, (v2.y - v0.y), Patch.fma(u, (v1.y - v0.y), v0.y));
    p.z = Patch.fma(v, (v2.z - v0.z), Patch.fma(u, (v1.z - v0.z), v0.z));
  }

  private static pointInQuadrilateral(
    v0: Vector3D,
    v1: Vector3D,
    v2: Vector3D,
    v3: Vector3D,
    u: number,
    v: number,
    p: Vector3D
  ): void {
    const c = u * v;
    const b = u - c;
    const d = v - c;
    p.x = Patch.fma(d, (v3.x - v0.x), Patch.fma(c, (v2.x - v0.x), Patch.fma(b, (v1.x - v0.x), v0.x)));
    p.y = Patch.fma(d, (v3.y - v0.y), Patch.fma(c, (v2.y - v0.y), Patch.fma(b, (v1.y - v0.y), v0.y)));
    p.z = Patch.fma(d, (v3.z - v0.z), Patch.fma(c, (v2.z - v0.z), Patch.fma(b, (v1.z - v0.z), v0.z)));
  }

  private getInterpolatedNormalAtUv(u: number, v: number): Vector3D {
    const localNormal = new Vector3D();
    const v1 = this.vertex[0]?.normal as Vector3D;
    const v2 = this.vertex[1]?.normal as Vector3D;
    const v3 = this.vertex[2]?.normal as Vector3D;

    switch (this.numberOfVertices) {
      case 3:
        Patch.pointInTriangle(v1, v2, v3, u, v, localNormal);
        break;
      case 4: {
        const v4 = this.vertex[3]?.normal as Vector3D;
        Patch.pointInQuadrilateral(v1, v2, v3, v4, u, v, localNormal);
        break;
      }
      default:
        VsdkLogger.fatal(-1, "PatchNormalAtUV", "Invalid number of vertices %d", this.numberOfVertices);
        break;
    }

    localNormal.normalize(Numeric.EPSILON_FLOAT);
    return localNormal;
  }

  private static solveQuadraticUnitInterval(A: number, B: number, C: number, x: number[]): boolean {
    let D = B * B - 4.0 * A * C;
    let x1: number;
    let x2: number;

    if (A < Patch.TOLERANCE && A > -Patch.TOLERANCE) {
      x1 = -1.0;
      x2 = -C / B;
    }
    else {
      if (D < -Patch.TOLERANCE * Patch.TOLERANCE) {
        x[0] = -B / (2.0 * A);
        VsdkLogger.error(
          null,
          "Bi-linear->Uniform mapping has negative discriminant D = %g. Taking 0 as discriminant and %g as solution.",
          D,
          x[0]
        );
        return false;
      }

      D = D > Patch.TOLERANCE * Patch.TOLERANCE ? globalThis.Math.sqrt(D) : 0.0;
      A = 1.0 / (2.0 * A);
      x1 = (-B + D) * A;
      x2 = (-B - D) * A;

      if (x1 > -Patch.TOLERANCE && x1 < 1.0 + Patch.TOLERANCE) {
        x[0] = x1;
        if (x2 > -Patch.TOLERANCE && x2 < 1.0 + Patch.TOLERANCE) {
          return false;
        }
        return true;
      }
    }

    if (x2 > -Patch.TOLERANCE && x2 < 1.0 + Patch.TOLERANCE) {
      x[0] = x2;
      return true;
    }

    let d: number;
    if (x1 > 1.0) {
      d = x1 - 1.0;
    }
    else {
      d = -x1;
    }
    x[0] = x1;
    if (x2 > 1.0) {
      if (x2 - 1.0 < d) {
        x[0] = x2;
      }
    }
    else if (0.0 - x2 < d) {
      x[0] = x2;
    }

    if (x[0] < 0.0) {
      x[0] = 0.0;
    }
    if (x[0] > 1.0) {
      x[0] = 1.0;
    }
    return false;
  }

  private connectVertex(paramVertex: Vertex | null): void {
    if (paramVertex === null || paramVertex.patches === null) {
      return;
    }
    paramVertex.patches.push(this);
  }

  private connectVertices(): void {
    for (let i = 0; i < this.numberOfVertices; i++) {
      this.connectVertex(this.vertex[i]!);
    }
  }

  private computeRandomWalkRadiosityArea(): number {
    let p1: Vector3D;
    let p2: Vector3D;
    let p3: Vector3D;
    let p4: Vector3D;
    const d1 = new Vector3D();
    const d2 = new Vector3D();
    const d3 = new Vector3D();
    const d4 = new Vector3D();
    const cp1 = new Vector3D();
    const cp2 = new Vector3D();
    const cp3 = new Vector3D();
    let a: number;
    let b: number;
    let c: number;

    switch (this.numberOfVertices) {
      case 3:
        this.jacobian = null;

        p1 = this.vertex[0]!.point;
        p2 = this.vertex[1]!.point;
        p3 = this.vertex[2]!.point;
        d1.subtraction(p2, p1);
        d2.subtraction(p3, p2);
        cp1.crossProduct(d1, d2);
        this.area = 0.5 * cp1.norm();
        break;
      case 4:
        p1 = this.vertex[0]!.point;
        p2 = this.vertex[1]!.point;
        p3 = this.vertex[2]!.point;
        p4 = this.vertex[3]!.point;
        d1.subtraction(p2, p1);
        d2.subtraction(p3, p2);
        d3.subtraction(p3, p4);
        d4.subtraction(p4, p1);
        cp1.crossProduct(d1, d4);
        cp2.crossProduct(d1, d3);
        cp3.crossProduct(d2, d4);
        a = cp1.dotProduct(this.normal);
        b = cp2.dotProduct(this.normal);
        c = cp3.dotProduct(this.normal);

        this.area = a + 0.5 * (b + c);
        if (this.area < 0.0) {
          b = -b;
          c = -c;
          this.area = -this.area;
        }

        if (globalThis.Math.abs(b) / this.area < Numeric.EPSILON && globalThis.Math.abs(c) / this.area < Numeric.EPSILON) {
          this.jacobian = null;
        }
        else {
          this.jacobian = new Jacobian(a, b, c);
        }
        break;
      default:
        VsdkLogger.fatal(2, "computeRandomWalkRadiosityArea", "Can only handle triangular and quadrilateral patches.");
        this.jacobian = null;
        this.area = 0.0;
        break;
    }

    if (this.area < Numeric.EPSILON * Numeric.EPSILON) {
      process.stderr.write(`Warning: very small patch id ${this.id} area = ${this.area}\n`);
    }

    return this.area;
  }

  private computeMidpoint(p: Vector3D): void {
    p.set(0.0, 0.0, 0.0);
    for (let i = 0; i < this.numberOfVertices; i++) {
      p.addition(p, this.vertex[i]!.point);
    }
    p.inverseScaledCopy(this.numberOfVertices, p, Numeric.EPSILON_FLOAT);
  }

  private computeTolerance(): number {
    let localTolerance = 0.0;
    for (let i = 0; i < this.numberOfVertices; i++) {
      const p = this.vertex[i]!.point;
      const e = globalThis.Math.abs(this.normal.dotProduct(p) + this.planeConstant) + p.tolerance(Numeric.EPSILON_FLOAT);
      if (e > localTolerance) {
        localTolerance = e;
      }
    }
    return localTolerance;
  }

  // Scratch storage reused across calls on the hot ray intersection paths.
  // The C++ port keeps the equivalent variables on the stack; the rendering
  // core is single threaded, so sharing them here is safe and avoids
  // billions of short-lived heap allocations per scene.
  private static readonly triangleUvScratchP0 = new Vector2Dd();
  private static readonly triangleUvScratchP1 = new Vector2Dd();
  private static readonly triangleUvScratchP2 = new Vector2Dd();

  private triangleUv(point: Vector3D, uv: Vector2Dd): boolean {
    let u0 = 0.0;
    let v0 = 0.0;
    let alpha = 0.0;
    let beta = 0.0;
    const p0 = Patch.triangleUvScratchP0;
    const p1 = Patch.triangleUvScratchP1;
    const p2 = Patch.triangleUvScratchP2;

    let vertexIndex = 0;
    switch (this.index) {
      case CoordinateAxis.X:
        u0 = this.vertex[vertexIndex]!.point.y;
        v0 = this.vertex[vertexIndex]!.point.z;
        Vector2Dd.set(p0, point.y - u0, point.z - v0);
        vertexIndex++;
        Vector2Dd.set(p1, this.vertex[vertexIndex]!.point.y - u0, this.vertex[vertexIndex]!.point.z - v0);
        vertexIndex++;
        Vector2Dd.set(p2, this.vertex[vertexIndex]!.point.y - u0, this.vertex[vertexIndex]!.point.z - v0);
        break;
      case CoordinateAxis.Y:
        u0 = this.vertex[vertexIndex]!.point.x;
        v0 = this.vertex[vertexIndex]!.point.z;
        Vector2Dd.set(p0, point.x - u0, point.z - v0);
        vertexIndex++;
        Vector2Dd.set(p1, this.vertex[vertexIndex]!.point.x - u0, this.vertex[vertexIndex]!.point.z - v0);
        vertexIndex++;
        Vector2Dd.set(p2, this.vertex[vertexIndex]!.point.x - u0, this.vertex[vertexIndex]!.point.z - v0);
        break;
      case CoordinateAxis.Z:
        u0 = this.vertex[vertexIndex]!.point.x;
        v0 = this.vertex[vertexIndex]!.point.y;
        Vector2Dd.set(p0, point.x - u0, point.y - v0);
        vertexIndex++;
        Vector2Dd.set(p1, this.vertex[vertexIndex]!.point.x - u0, this.vertex[vertexIndex]!.point.y - v0);
        vertexIndex++;
        Vector2Dd.set(p2, this.vertex[vertexIndex]!.point.x - u0, this.vertex[vertexIndex]!.point.y - v0);
        break;
      default:
        break;
    }

    if (p1.u < -Numeric.EPSILON || p1.u > Numeric.EPSILON) {
      beta = (p0.v * p1.u - p0.u * p1.v) / (p2.v * p1.u - p2.u * p1.v);
      if (beta >= 0.0 && beta <= 1.0) {
        alpha = (p0.u - beta * p2.u) / p1.u;
      }
      else {
        return false;
      }
    }
    else {
      beta = p0.u / p2.u;
      if (beta >= 0.0 && beta <= 1.0) {
        alpha = (p0.v - beta * p2.v) / p1.v;
      }
      else {
        return false;
      }
    }

    uv.u = alpha;
    uv.v = beta;
    return !(alpha < 0.0 || (alpha + beta) > 1.0);
  }

  private static readonly quadUvScratchA = new Vector2Dd();
  private static readonly quadUvScratchB = new Vector2Dd();
  private static readonly quadUvScratchC = new Vector2Dd();
  private static readonly quadUvScratchD = new Vector2Dd();
  private static readonly quadUvScratchM = new Vector2Dd();
  private static readonly quadUvScratchAB = new Vector2Dd();
  private static readonly quadUvScratchBC = new Vector2Dd();
  private static readonly quadUvScratchCD = new Vector2Dd();
  private static readonly quadUvScratchAD = new Vector2Dd();
  private static readonly quadUvScratchAM = new Vector2Dd();
  private static readonly quadUvScratchAE = new Vector2Dd();
  private static readonly quadUvScratchVector = new Vector2Dd();

  private static quadUv(patch: Patch, point: Vector3D, uv: Vector2Dd): boolean {
    const A = Patch.quadUvScratchA;
    const B = Patch.quadUvScratchB;
    const C = Patch.quadUvScratchC;
    const D = Patch.quadUvScratchD;
    const M = Patch.quadUvScratchM;
    const AB = Patch.quadUvScratchAB;
    const BC = Patch.quadUvScratchBC;
    const CD = Patch.quadUvScratchCD;
    const AD = Patch.quadUvScratchAD;
    const AM = Patch.quadUvScratchAM;
    const AE = Patch.quadUvScratchAE;
    let u = -1.0;
    let v = -1.0;
    const vector = Patch.quadUvScratchVector;
    let isInside = false;

    let vertexIndex = 0;
    switch (patch.index) {
      case CoordinateAxis.X:
        Vector2Dd.set(A, patch.vertex[vertexIndex]!.point.y, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(B, patch.vertex[vertexIndex]!.point.y, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(C, patch.vertex[vertexIndex]!.point.y, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(D, patch.vertex[vertexIndex]!.point.y, patch.vertex[vertexIndex]!.point.z);
        Vector2Dd.set(M, point.y, point.z);
        break;
      case CoordinateAxis.Y:
        Vector2Dd.set(A, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(B, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(C, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.z);
        vertexIndex++;
        Vector2Dd.set(D, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.z);
        Vector2Dd.set(M, point.x, point.z);
        break;
      case CoordinateAxis.Z:
        Vector2Dd.set(A, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.y);
        vertexIndex++;
        Vector2Dd.set(B, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.y);
        vertexIndex++;
        Vector2Dd.set(C, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.y);
        vertexIndex++;
        Vector2Dd.set(D, patch.vertex[vertexIndex]!.point.x, patch.vertex[vertexIndex]!.point.y);
        Vector2Dd.set(M, point.x, point.y);
        break;
      default:
        break;
    }

    Vector2Dd.subtract(B, A, AB);
    Vector2Dd.subtract(C, B, BC);
    Vector2Dd.subtract(D, C, CD);
    Vector2Dd.subtract(D, A, AD);
    Vector2Dd.add(CD, AB, AE);
    Vector2Dd.negate(AE);
    Vector2Dd.subtract(M, A, AM);

    let a: number;
    let b: number;
    let c: number;

    if (globalThis.Math.abs(Vector2Dd.determinant(AB, CD)) < Numeric.EPSILON) {
      Vector2Dd.subtract(AB, CD, vector);
      v = Vector2Dd.determinant(AM, vector) / Vector2Dd.determinant(AD, vector);
      if (v >= 0.0 && v <= 1.0) {
        b = Vector2Dd.determinant(AB, AD) - Vector2Dd.determinant(AM, AE);
        c = Vector2Dd.determinant(AM, AD);
        u = globalThis.Math.abs(b) < Numeric.EPSILON ? -1.0 : c / b;
        isInside = (u >= 0.0 && u <= 1.0);
      }
    }
    else if (globalThis.Math.abs(Vector2Dd.determinant(BC, AD)) < Numeric.EPSILON) {
      Vector2Dd.add(AD, BC, vector);
      u = Vector2Dd.determinant(AM, vector) / Vector2Dd.determinant(AB, vector);
      if (u >= 0.0 && u <= 1.0) {
        b = Vector2Dd.determinant(AD, AB) - Vector2Dd.determinant(AM, AE);
        c = Vector2Dd.determinant(AM, AB);
        v = globalThis.Math.abs(b) < Numeric.EPSILON ? -1.0 : c / b;
        isInside = (v >= 0.0 && v <= 1.0);
      }
    }
    else {
      a = Vector2Dd.determinant(AB, AE);
      c = -Vector2Dd.determinant(AM, AD);
      b = Vector2Dd.determinant(AB, AD) - Vector2Dd.determinant(AM, AE);
      a = -0.5 / a;
      b *= a;
      c *= (a + a);
      let sqrtDelta = b * b + c;
      if (sqrtDelta >= 0.0) {
        sqrtDelta = globalThis.Math.sqrt(sqrtDelta);
        u = b - sqrtDelta;
        if (u < 0.0 || u > 1.0) {
          u = b + sqrtDelta;
        }
        if (u >= 0.0 && u <= 1.0) {
          v = AD.u + u * AE.u;
          if (globalThis.Math.abs(v) < Numeric.EPSILON) {
            v = (AM.v - u * AB.v) / (AD.v + u * AE.v);
          }
          else {
            v = (AM.u - u * AB.u) / v;
          }
          isInside = (v >= 0.0 && v <= 1.0);
        }
      }
      else {
        u = -1.0;
        v = -1.0;
      }
    }

    uv.u = Patch.clipToUnitInterval(u);
    uv.v = Patch.clipToUnitInterval(v);
    return isInside;
  }

  private static readonly hitInPatchScratchUv = new Vector2Dd();

  private hitInPatch(hit: RayHit, patch: Patch): boolean {
    const newFlags = hit.getFlags() | RayHitFlag.UV;
    hit.setFlags(newFlags);
    const position = hit.getPoint();
    const uv = Patch.hitInPatchScratchUv;
    const result = (patch.numberOfVertices === 3)
      ? this.triangleUv(position, uv)
      : Patch.quadUv(patch, position, uv);
    hit.setUv(uv);
    return result;
  }

  private static patchNormal(patch: Patch, normal: Vector3D): Vector3D | null {
    const current = new Vector3D();

    normal.set(0.0, 0.0, 0.0);
    current.subtraction(patch.vertex[patch.numberOfVertices - 1]!.point, patch.vertex[0]!.point);
    for (let i = 0; i < patch.numberOfVertices; i++) {
      const previous = new Vector3D(current.x, current.y, current.z);
      current.subtraction(patch.vertex[i]!.point, patch.vertex[0]!.point);
      normal.x += (previous.y - current.y) * (previous.z + current.z);
      normal.y += (previous.z - current.z) * (previous.x + current.x);
      normal.z += (previous.x - current.x) * (previous.y + current.y);
    }

    const localNorm = normal.norm();
    if (localNorm < Numeric.EPSILON) {
      VsdkLogger.warning("patchNormal", "degenerate patch (id %d)", patch.id);
      return null;
    }
    normal.inverseScaledCopy(localNorm, normal, Numeric.EPSILON_FLOAT);
    return normal;
  }

  public hasZeroVertices(): boolean {
    return this.numberOfVertices === 0;
  }

  public constructor(inNumberOfVertices: number, v1: Vertex | null, v2: Vertex | null, v3: Vertex | null, v4: Vertex | null) {
    this.flags = 0;
    this.id = 0;
    this.twin = null;
    this.vertex = new Array<Vertex | null>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(null);
    this.numberOfVertices = 0;
    this.boundingBox = null;
    this.normal = new Vector3D();
    this.planeConstant = 0.0;
    this.tolerance = 0.0;
    this.area = 0.0;
    this.midPoint = new Vector3D();
    this.jacobian = null;
    this.directPotential = 0.0;
    this.index = CoordinateAxis.Z;
    this.omit = 0;
    this.color = new ColorRgb();
    this.radianceData = null;
    this.material = null;

    if (v1 === null || v2 === null || v3 === null || (inNumberOfVertices === 4 && v4 === null)) {
      VsdkLogger.error("Patch::Patch", "Null vertex");
      process.exit(1);
    }

    if (inNumberOfVertices !== 3 && inNumberOfVertices !== 4) {
      VsdkLogger.error("Patch::Patch", "Can only handle quadrilateral or triangular patches");
      process.exit(2);
    }

    Statistics.instance().reader.numberOfElements++;
    this.twin = null;
    this.id = Patch.patchId;
    Patch.patchId++;

    this.material = null;
    for (let i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
      this.vertex[i] = null;
    }

    this.numberOfVertices = inNumberOfVertices;
    this.vertex[0] = v1;
    this.vertex[1] = v2;
    this.vertex[2] = v3;
    this.vertex[3] = v4;

    this.boundingBox = null;
    this.normal = new Vector3D();

    if (Patch.patchNormal(this, this.normal) === null) {
      Statistics.instance().reader.numberOfElements--;
      VsdkLogger.error("Patch::Patch", "Error computing patch normal");
      process.exit(3);
    }

    this.area = this.computeRandomWalkRadiosityArea();
    this.midPoint = new Vector3D();
    this.computeMidpoint(this.midPoint);

    this.planeConstant = -this.normal.dotProduct(this.midPoint);
    this.tolerance = this.computeTolerance();
    this.index = this.normal.dominantCoordinate();
    this.connectVertices();

    this.directPotential = 0.0;
    this.color = new ColorRgb();
    this.color.set(0.0, 0.0, 0.0);

    this.omit = 0;
    this.flags = 0;
    this.radianceData = null;
  }

  public destroy(): void {
    this.jacobian = null;
    this.boundingBox = null;
    this.radianceData = null;
  }

  public computeAndGetBoundingBox(bounds: BoundingBox): void {
    this.computeBoundingBox();
    bounds.copyFrom(this.boundingBox as BoundingBox);
  }

  public computeBoundingBox(): void {
    if (this.boundingBox === null) {
      this.boundingBox = new BoundingBox();
      for (let i = 0; i < this.numberOfVertices; i++) {
        this.boundingBox.enlargeToIncludePoint(this.vertex[i]!.point);
      }
    }
  }

  private static dontIntersectBase(n: number, p0: Patch | null, p1: Patch | null, p2: Patch | null, p3: Patch | null): void {
    if (n < 0 || n > Patch.MAX_EXCLUDED_PATCHES) {
      VsdkLogger.fatal(
        -1,
        "Patch::dontIntersectBase",
        "Invalid number of excluded patches %d (maximum is %d)",
        n,
        Patch.MAX_EXCLUDED_PATCHES
      );
      return;
    }

    const localPatches = [p0, p1, p2, p3];
    let i = 0;
    for (; i < n; i++) {
      Patch.excludedPatches[i] = localPatches[i] ?? null;
    }
    for (; i < Patch.MAX_EXCLUDED_PATCHES; i++) {
      Patch.excludedPatches[i] = null;
    }
  }

  public static dontIntersect0(): void {
    Patch.dontIntersectBase(0, null, null, null, null);
  }

  public static dontIntersect2(p0: Patch | null, p1: Patch | null): void {
    Patch.dontIntersectBase(2, p0, p1, null, null);
  }

  public static dontIntersect3(p0: Patch | null, p1: Patch | null, p2: Patch | null): void {
    Patch.dontIntersectBase(3, p0, p1, p2, null);
  }

  public static dontIntersect4(p0: Patch | null, p1: Patch | null, p2: Patch | null, p3: Patch | null): void {
    Patch.dontIntersectBase(4, p0, p1, p2, p3);
  }

  private interpolatedNormalAtUv(u: number, v: number): Vector3D {
    if (!this.allVerticesHaveANormal()) {
      return this.normal;
    }
    return this.getInterpolatedNormalAtUv(u, v);
  }

  public interpolatedFrameAtUv(u: number, v: number, X: Vector3D | null, Y: Vector3D | null, Z: Vector3D): void {
    Z.copy(this.interpolatedNormalAtUv(u, v));

    if (X !== null && Y !== null) {
      const zz = globalThis.Math.sqrt(1.0 - Z.z * Z.z);
      if (zz < Numeric.EPSILON) {
        X.set(1.0, 0.0, 0.0);
      }
      else {
        X.set(Z.y / zz, -Z.x / zz, 0.0);
      }
      Y.crossProduct(Z, X);
    }
  }

  public textureCoordAtUv(u: number, v: number): Vector3D {
    const texCoord = new Vector3D();
    texCoord.set(0.0, 0.0, 0.0);

    const t0 = this.vertex[0]?.textureCoordinates ?? null;
    const t1 = this.vertex[1]?.textureCoordinates ?? null;
    const t2 = this.vertex[2]?.textureCoordinates ?? null;

    switch (this.numberOfVertices) {
      case 3:
        if (t0 === null || t1 === null || t2 === null) {
          texCoord.set(u, v, 0.0);
        }
        else {
          Patch.pointInTriangle(t0, t1, t2, u, v, texCoord);
        }
        break;
      case 4: {
        const t3 = this.vertex[3]?.textureCoordinates ?? null;
        if (t0 === null || t1 === null || t2 === null || t3 === null) {
          texCoord.set(u, v, 0.0);
        }
        else {
          Patch.pointInQuadrilateral(t0, t1, t2, t3, u, v, texCoord);
        }
        break;
      }
      default:
        VsdkLogger.fatal(-1, "textureCoordAtUv", "Invalid nr of vertices %d", this.numberOfVertices);
        break;
    }
    return texCoord;
  }

  // Reused hit record, mirroring the local stack variable of the C++ port.
  // Results are copied into the caller's hitStore before returning, so the
  // scratch object never escapes this method.
  private static readonly intersectScratchHit = new RayHit();
  private static readonly intersectScratchU = [0.0];
  private static readonly intersectScratchV = [0.0];

  public intersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null
  ): RayHit | null {
    const hit = Patch.intersectScratchHit;
    hit.setFlags(0);
    hit.setPatch(null);
    hit.setMaterial(null);

    if (this.isExcluded()) {
      return null;
    }

    let distance = this.normal.dotProduct(ray.direction);
    if (distance > Numeric.EPSILON) {
      if ((hitFlags & RayHitFlag.BACK) === 0) {
        return null;
      }
      const newFlags = hit.getFlags() | RayHitFlag.BACK;
      hit.setFlags(newFlags);
    }
    else if (distance < -Numeric.EPSILON) {
      if ((hitFlags & RayHitFlag.FRONT) === 0) {
        return null;
      }
      const newFlags = hit.getFlags() | RayHitFlag.FRONT;
      hit.setFlags(newFlags);
    }
    else {
      return null;
    }

    distance = -(this.normal.dotProduct(ray.position) + this.planeConstant) / distance;

    if (distance > maximumDistance[0]! || distance < minimumDistance) {
      return null;
    }

    const position = hit.getPoint();
    position.sumScaled(ray.position, distance, ray.direction);

    if (this.hitInPatch(hit, this)) {
      hit.setPatch(this);
      hit.setMaterial(this.material);
      hit.setGeometricNormal(this.normal);
      let newFlags = hit.getFlags()
        | RayHitFlag.PATCH
        | RayHitFlag.POINT
        | RayHitFlag.MATERIAL
        | RayHitFlag.GEOMETRIC_NORMAL
        | RayHitFlag.DISTANCE;
      hit.setFlags(newFlags);

      if ((hitFlags & RayHitFlag.UV) !== 0 && (hit.getFlags() & RayHitFlag.UV) === 0) {
        const localPosition = hit.getPoint();
        const u = Patch.intersectScratchU;
        const v = Patch.intersectScratchV;
        u[0] = 0.0;
        v[0] = 0.0;
        hit.getPatch()!.uv(localPosition, u, v);
        hit.setUv(u[0]!, v[0]!);
        hit.setPoint(localPosition);
        newFlags = hit.getFlags() & RayHitFlag.UV;
        hit.setFlags(newFlags);
      }

      maximumDistance[0] = distance;

      if (hitStore !== null) {
        hitStore.copyFrom(hit);
        return hitStore;
      }
      return hit;
    }

    return null;
  }

  public biLinearToUniform(u: number[], v: number[]): void {
    const a = this.jacobian!.A;
    const b = this.jacobian!.B;
    const c = this.jacobian!.C;
    u[0]! = ((a + 0.5 * c) + 0.5 * b * u[0]!) * u[0]! / this.area;
    v[0]! = ((a + 0.5 * b) + 0.5 * c * v[0]!) * v[0]! / this.area;
  }

  private uniformToBiLinear(u: number[], v: number[]): void {
    const a = this.jacobian!.A;
    const b = this.jacobian!.B;
    const c = this.jacobian!.C;

    let A = 0.5 * b / this.area;
    let B = (a + 0.5 * c) / this.area;
    let C = -u[0]!;
    Patch.solveQuadraticUnitInterval(A, B, C, u);

    A = 0.5 * c / this.area;
    B = (a + 0.5 * b) / this.area;
    C = -v[0]!;
    Patch.solveQuadraticUnitInterval(A, B, C, v);
  }

  public pointBarycentricMapping(u: number, v: number, point: Vector3D): Vector3D | null {
    if (this.hasZeroVertices()) {
      return null;
    }

    const v1 = this.vertex[0]!.point;
    const v2 = this.vertex[1]!.point;
    const v3 = this.vertex[2]!.point;

    if (this.numberOfVertices === 3) {
      if (u + v > 1.0) {
        u = 1.0 - u;
        v = 1.0 - v;
      }
      Patch.pointInTriangle(v1, v2, v3, u, v, point);
    }
    else if (this.numberOfVertices === 4) {
      const v4 = this.vertex[3]!.point;
      Patch.pointInQuadrilateral(v1, v2, v3, v4, u, v, point);
    }
    else {
      VsdkLogger.fatal(4, "pointBarycentricMapping", "Can only handle triangular or quadrilateral patches");
    }

    return point;
  }

  public uniformPoint(u: number, v: number, point: Vector3D): Vector3D | null {
    if (this.jacobian !== null) {
      const uu = [u];
      const vv = [v];
      this.uniformToBiLinear(uu, vv);
      u = uu[0]!;
      v = vv[0]!;
    }
    return this.pointBarycentricMapping(u, v, point);
  }

  public uv(point: Vector3D, u: number[], v: number[]): boolean {
    const localUv = new Vector2Dd();
    let inside = false;

    switch (this.numberOfVertices) {
      case 3:
        inside = this.triangleUv(point, localUv);
        break;
      case 4:
        inside = Patch.quadUv(this, point, localUv);
        break;
      default:
        VsdkLogger.fatal(3, "uv", "Can only handle triangular or quadrilateral patches");
        break;
    }

    u[0] = localUv.u;
    v[0] = localUv.v;
    return inside;
  }

  public computeVertexColors(): void {
    for (let i = 0; i < this.numberOfVertices; i++) {
      this.vertex[i]!.computeColor();
    }
  }

  private isAtLeastPartlyInFront(other: Patch): boolean {
    for (let i = 0; i < this.numberOfVertices; i++) {
      const vp = this.vertex[i]!.point;
      const ep = other.normal.dotProduct(vp) + other.planeConstant;
      const localTolerance = other.tolerance + vp.tolerance(Numeric.EPSILON_FLOAT);
      if (ep > localTolerance) {
        return true;
      }
    }
    return false;
  }

  public facing(other: Patch): boolean {
    return this.isAtLeastPartlyInFront(other) && other.isAtLeastPartlyInFront(this);
  }

  public static clipToUnitInterval(x: number): number {
    if (x < Numeric.EPSILON) {
      return Numeric.EPSILON;
    }
    return x > (1.0 - Numeric.EPSILON) ? 1.0 - Numeric.EPSILON : x;
  }

  public setVisible(): void {
    this.flags |= Patch.PATCH_VISIBILITY;
  }

  public setInvisible(): void {
    this.flags &= ~Patch.PATCH_VISIBILITY;
  }

  public setFlags(newFlags: number): void {
    this.flags = newFlags;
  }

  public getFlags(): number {
    return this.flags;
  }

  public isVisible(): boolean {
    return (this.flags & Patch.PATCH_VISIBILITY) !== 0;
  }

  public uniformUv(point: Vector3D, u: number[], v: number[]): boolean {
    const inside = this.uv(point, u, v);
    if (this.jacobian !== null) {
      this.biLinearToUniform(u, v);
    }
    return inside;
  }

  private getNumberOfSamples(): number {
    let numberOfSamples = 1;
    if (this.material !== null && this.material.getBsdf() !== null && this.material.getBsdf().splitBsdfIsTextured()) {
      const t0 = this.vertex[0]?.textureCoordinates ?? null;
      const t1 = this.vertex[1]?.textureCoordinates ?? null;
      const t2 = this.vertex[2]?.textureCoordinates ?? null;
      const t3 = this.numberOfVertices === 3 ? t0 : (this.vertex[3]?.textureCoordinates ?? null);

      const sameTex = t0 === t1 && t0 === t2 && t0 === t3 && t0 !== null;
      numberOfSamples = sameTex ? 1 : 100;
    }
    return numberOfSamples;
  }
}
