import { String as JavaString } from "../../../../../java/lang/String";
import { Logger as VsdkLogger } from "../../../common/logging/Logger";
import { Jacobian } from "../../../common/linealAlgebra/Jacobian";
import { Numeric } from "../../../common/linealAlgebra/Numeric";
import { Ray } from "../../../common/linealAlgebra/Ray";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { Vector3Df } from "../../../common/linealAlgebra/Vector3Df";
import { Statistics } from "../../../common/statistics/Statistics";
import { Material } from "../../../material/Material";
import type { FaceColorTable } from "../../../scene/FaceColorTable";
import { BoundingBox } from "../../../skin/AxisAlignedBoundingBox";
import { PlanarFace } from "../element/PlanarFace";
import type { RayExclusionSet } from "./RayExclusionSet";
import { RayHitFlag } from "./RayHitFlag";
import { RayHit } from "./RayHit";
import { Vertex } from "./Vertex";

// Single precision rounding of the source's float stores and operations
const f = Math.fround;

export class Patch {
  public static readonly MAXIMUM_VERTICES_PER_PATCH = 4;
  public static readonly MAX_EXCLUDED_PATCHES = 4;

  // A static counter which is increased every time a Patch is created in
  // order to make a unique Patch id
  private static patchId = 1;

  // Geometry of the face: positions, plane, area, parametrization; immutable
  private readonly face: PlanarFace;
  private readonly normal: Vector3D; // face.getNormal(), in the type RPK's callers use
  private readonly patchMidPoint: Vector3D; // face.getMidPoint(), in the type RPK's callers use
  private faceIndex: number; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
  private readonly jacobian: Jacobian | null; // face's Jacobian coefficients, for irregular quadrilaterals only

  public id: number; // Identification number for debugging, ID rendering
  public twin: Patch | null; // Twin face (for double-sided surfaces)
  public vertex: Array<Vertex | null>; // The vertices
  public numberOfVertices: number; // Number of vertices: 3 or 4
  public material: Material | null;

  public static getNextId(): number {
    return Patch.patchId;
  }

  public static setNextId(id: number): void {
    Patch.patchId = id;
  }

  private static toFace(v: Vector3D): Vector3Df {
    return new Vector3Df(v.x, v.y, v.z);
  }

  private static fromFace(v: Vector3Df): Vector3D {
    return new Vector3D(v.x(), v.y(), v.z());
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

  /**
  Point IN Triangle: barycentric parametrisation, in single precision
  */
  private static pointInTriangle(
    v0: Vector3D,
    v1: Vector3D,
    v2: Vector3D,
    u: number,
    v: number,
    p: Vector3D
  ): void {
    p.x = f(f(v0.x + f(u * f(v1.x - v0.x))) + f(v * f(v2.x - v0.x)));
    p.y = f(f(v0.y + f(u * f(v1.y - v0.y))) + f(v * f(v2.y - v0.y)));
    p.z = f(f(v0.z + f(u * f(v1.z - v0.z))) + f(v * f(v2.z - v0.z)));
  }

  /**
  Point IN Quadrilateral: bi-linear parametrisation, in single precision
  */
  private static pointInQuadrilateral(
    v0: Vector3D,
    v1: Vector3D,
    v2: Vector3D,
    v3: Vector3D,
    u: number,
    v: number,
    p: Vector3D
  ): void {
    const c = f(u * v);
    const b = f(u - c);
    const d = f(v - c);
    p.x = f(f(f(v0.x + f(b * f(v1.x - v0.x))) + f(c * f(v2.x - v0.x))) + f(d * f(v3.x - v0.x)));
    p.y = f(f(f(v0.y + f(b * f(v1.y - v0.y))) + f(c * f(v2.y - v0.y))) + f(d * f(v3.y - v0.y)));
    p.z = f(f(f(v0.z + f(b * f(v1.z - v0.z))) + f(c * f(v2.z - v0.z))) + f(d * f(v3.z - v0.z)));
  }

  private getInterpolatedNormalAtUv(u: number, v: number): Vector3D {
    const localNormal = new Vector3D();
    const v1 = this.vertex[0]?.normal as Vector3D;
    const v2 = this.vertex[1]?.normal as Vector3D;
    const v3 = this.vertex[2]?.normal as Vector3D;

    switch (this.numberOfVertices) {
      case 3:
        Patch.pointInTriangle(v1, v2, v3, f(u), f(v), localNormal);
        break;
      case 4: {
        const v4 = this.vertex[3]?.normal as Vector3D;
        Patch.pointInQuadrilateral(v1, v2, v3, v4, f(u), f(v), localNormal);
        break;
      }
      default:
        VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "PatchNormalAtUV", JavaString.vformat("Invalid number of vertices %d", [this.numberOfVertices]));
        break;
    }

    localNormal.normalize(Numeric.EPSILON_FLOAT);
    return localNormal;
  }

  /**
  Adds the patch to the list of patches that share the vertex
  */
  private connectVertex(paramVertex: Vertex | null): void {
    if (paramVertex === null || paramVertex.patches === null) {
      return;
    }
    paramVertex.patches.push(this);
  }

  /**
  Adds the patch to the list of patches sharing each vertex
  */
  private connectVertices(): void {
    for (let i = 0; i < this.numberOfVertices; i++) {
      this.connectVertex(this.vertex[i]!);
    }
  }

  /**
  Return true if patch is virtual
  */
  public hasZeroVertices(): boolean {
    return this.numberOfVertices === 0;
  }

  /**
  Creates a patch structure for a patch with given vertices
  */
  public constructor(inNumberOfVertices: number, v1: Vertex | null, v2: Vertex | null, v3: Vertex | null, v4: Vertex | null) {
    this.faceIndex = -1;
    this.id = 0;
    this.twin = null;
    this.numberOfVertices = inNumberOfVertices;
    this.vertex = [v1, v2, v3, v4];
    this.material = null;
    this.jacobian = null;

    if (v1 === null || v2 === null || v3 === null || (inNumberOfVertices === 4 && v4 === null)) {
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "Patch::Patch", "Null vertex!");
      process.exit(1);
    }

    // It's sad but it's true
    if (inNumberOfVertices !== 3 && inNumberOfVertices !== 4) {
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "Patch::Patch", "Can only handle quadrilateral or triangular patches");
      process.exit(2);
    }

    Statistics.instance().reader.numberOfElements++;
    this.twin = null;
    this.id = Patch.patchId;
    Patch.patchId++;

    this.material = null;

    // Face geometry: normal, area and Jacobian, midpoint, plane, dominant axis
    const positions: Vector3Df[] = [
      Patch.toFace(v1!.point),
      Patch.toFace(v2!.point),
      Patch.toFace(v3!.point),
      inNumberOfVertices === 4 ? Patch.toFace(v4!.point) : new Vector3Df()
    ];
    this.face = new PlanarFace(inNumberOfVertices, positions);
    if (!this.face.isValid()) {
      VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "patchNormal", JavaString.vformat("degenerate patch (id %d)", [this.id]));
      Statistics.instance().reader.numberOfElements--;
      VsdkLogger.reportMessage(this, VsdkLogger.ERROR, "Patch::Patch", "Error computing patch normal");
      process.exit(3);
    }
    this.normal = Patch.fromFace(this.face.getNormal());
    this.patchMidPoint = Patch.fromFace(this.face.getMidPoint());
    if (this.face.hasJacobian()) {
      this.jacobian = new Jacobian(this.face.getJacobianA(), this.face.getJacobianB(), this.face.getJacobianC());
    }
    if (this.face.getArea() < Numeric.EPSILON * Numeric.EPSILON) {
      process.stderr.write(JavaString.vformat("Warning: very small patch id %d area = %g\n", [this.id, this.face.getArea()]));
    }

    // Tell the vertices that there's a new Patch using them
    this.connectVertices();
  }

  public destroy(): void {
  }

  public getArea(): number {
    return this.face.getArea();
  }

  public getFace(): PlanarFace {
    return this.face;
  }

  /** 0: x, 1: y, 2: z */
  public getDominantAxisIndex(): number {
    return this.face.getDominantAxis();
  }

  public getFaceIndex(): number {
    return this.faceIndex;
  }

  public setFaceIndex(newFaceIndex: number): void {
    this.faceIndex = newFaceIndex;
  }

  public getJacobian(): Jacobian | null {
    return this.jacobian;
  }

  public getNormal(): Vector3D {
    return this.normal;
  }

  public getPlaneConstant(): number {
    return this.face.getPlaneConstant();
  }

  public getTolerance(): number {
    return this.face.getTolerance();
  }

  public midPoint(): Vector3D {
    return this.patchMidPoint;
  }

  /**
  Computes the patch's bounding box from its vertices, in vertex order. The
  vertices do not move after construction, so every call gives the same box;
  runtime queries read the scene's FaceBoundsTable, built once from this
  */
  public computeBounds(bounds: BoundingBox): void {
    bounds.copyFrom(new BoundingBox());
    for (let i = 0; i < this.numberOfVertices; i++) {
      bounds.enlargeToIncludePoint(this.vertex[i]!.point);
    }
  }

  /**
  Computes interpolated (= shading) normal at the point with given parameters
  on the patch
  */
  private interpolatedNormalAtUv(u: number, v: number): Vector3D {
    if (!this.allVerticesHaveANormal()) {
      return this.normal;
    }
    return this.getInterpolatedNormalAtUv(u, v);
  }

  /**
  Computes shading frame at the given uv on the patch.
  Computes 'a' interpolated (shading) frame at the uv or point with
  given parameters on the patch. The frame is consistent over the
  complete patch if the shading normals in the vertices do not differ
  too much from the geometric normal. The Z axis is the interpolated
  normal The X is determined by Z and the projection of the patch by
  the dominant axis (patch.getDominantAxisIndex())
  */
  public interpolatedFrameAtUv(u: number, v: number, X: Vector3D | null, Y: Vector3D | null, Z: Vector3D): void {
    Z.copy(this.interpolatedNormalAtUv(u, v));

    if (X !== null && Y !== null) {
      const zz = globalThis.Math.sqrt(f(1.0 - f(Z.z * Z.z)));
      if (zz < Numeric.EPSILON) {
        X.set(1.0, 0.0, 0.0);
      }
      else {
        X.set(f(Z.y / zz), f(-Z.x / zz), 0.0);
      }
      Y.crossProduct(Z, X);
    }
  }

  /**
  Returns texture coordinates determined from vertex texture coordinates and
  given u and v bi-linear of barycentric coordinates on the patch
  */
  public textureCoordAtUv(u: number, v: number): Vector3D {
    const texCoord = new Vector3D();
    texCoord.set(0.0, 0.0, 0.0);

    const t0 = this.vertex[0]?.textureCoordinates ?? null;
    const t1 = this.vertex[1]?.textureCoordinates ?? null;
    const t2 = this.vertex[2]?.textureCoordinates ?? null;

    switch (this.numberOfVertices) {
      case 3:
        if (t0 === null || t1 === null || t2 === null) {
          texCoord.set(f(u), f(v), 0.0);
        }
        else {
          Patch.pointInTriangle(t0, t1, t2, f(u), f(v), texCoord);
        }
        break;
      case 4: {
        const t3 = this.vertex[3]?.textureCoordinates ?? null;
        if (t0 === null || t1 === null || t2 === null || t3 === null) {
          texCoord.set(f(u), f(v), 0.0);
        }
        else {
          Patch.pointInQuadrilateral(t0, t1, t2, t3, f(u), f(v), texCoord);
        }
        break;
      }
      default:
        VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "textureCoordAtUv", JavaString.vformat("Invalid nr of vertices %d", [this.numberOfVertices]));
        break;
    }
    return texCoord;
  }

  // Reused records, mirroring the local stack variables of the C++ port.
  // Results are copied into the caller's hitStore before returning, so the
  // scratch objects never escape this method.
  private static readonly intersectScratchHit = new RayHit();
  private static readonly intersectScratchFaceHit = new PlanarFace.RayHit();
  private static readonly intersectScratchPosition = new Vector3D();
  private static readonly intersectScratchU = [0.0];
  private static readonly intersectScratchV = [0.0];

  /**
  Ray-patch intersection test, for computing form factors, creating ray-cast
  images ... Returns null if the Ray doesn't hit the patch. Fills in the
  'hit' record if there is a new hit and returns it.
  Fills in the distance to the patch in maximumDistance[0] if the patch
  is hit. Intersections closer than minimumDistance or further than
  maximumDistance[0] are ignored. The hitFlags determine what information to
  return about an intersection and whether front/back facing patches are to
  be considered. Both distances are single precision, as in the source.
  */
  public intersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null,
    exclusions: RayExclusionSet
  ): RayHit | null {
    const hit = Patch.intersectScratchHit;
    hit.setFlags(0);
    hit.setPatch(null);
    hit.setMaterial(null);

    if (exclusions.excludesPatch(this) || exclusions.excludesFace(this.getFaceIndex())) {
      return null;
    }

    // Side, plane hit, hit point and (u, v) in one call into the face
    const faceHit = Patch.intersectScratchFaceHit;
    const inside = this.face.rayHit(
      Patch.toFace(ray.position),
      Patch.toFace(ray.direction),
      (hitFlags & RayHitFlag.FRONT) !== 0,
      (hitFlags & RayHitFlag.BACK) !== 0,
      f(minimumDistance),
      f(maximumDistance[0]!),
      faceHit);
    if (inside) {
      const distance = faceHit.getDistance();
      const position = Patch.intersectScratchPosition;
      position.set(faceHit.getPointX(), faceHit.getPointY(), faceHit.getPointZ());
      hit.setPoint(position);
      hit.setUv(faceHit.getU(), faceHit.getV());
      hit.setFlags(hit.getFlags()
        | (faceHit.getSide() === PlanarFace.Side.BACK ? RayHitFlag.BACK : RayHitFlag.FRONT)
        | RayHitFlag.UV);
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
        position.copy(hit.getPoint());
        const u = Patch.intersectScratchU;
        const v = Patch.intersectScratchV;
        hit.getPatch()!.uv(position, u, v);
        hit.setUv(u[0]!, v[0]!);
        hit.setPoint(position);
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

  /**
  Given the parameter (u,v) of a point on the patch, this routine
  computes the 3D space coordinates of the same point, using barycentric
  mapping for a triangle and bi-linear mapping for a quadrilateral.
  u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
  (1-u) and (1-v) are used instead
  */
  public pointBarycentricMapping(u: number, v: number, point: Vector3D): Vector3D | null {
    if (this.hasZeroVertices()) {
      return null;
    }
    point.copy(Patch.fromFace(this.face.pointAt(u, v)));
    return point;
  }

  /**
  Like above, except that always a uniform mapping is used (one that
  preserves area, with this mapping you'll have more positions in "stretched"
  regions of an irregular quadrilateral, irregular quadrilaterals are the
  only once for which this routine will yield other positions than the above
  routine)
  */
  public uniformPoint(u: number, v: number, point: Vector3D): Vector3D | null {
    if (this.hasZeroVertices()) {
      return null;
    }
    point.copy(Patch.fromFace(this.face.uniformPointAt(u, v)));
    return point;
  }

  /**
  Converts bi-linear coordinates into uniform (area preserving) coordinates,
  for an irregular quadrilateral (see PlanarFace.biLinearToUniform)
  */
  public biLinearToUniform(u: number[], v: number[]): void {
    this.face.biLinearToUniform(u, v);
  }

  /**
  Computes (u, v) parameters of the point on the patch (barycentric or bi-linear
  parametrisation). Returns true if the point is inside the patch and false if
  not.
  WARNING: The (u, v) coordinates are correctly computed only for positions inside
  the patch. For positions outside, they can be garbage!
  */
  public uv(point: Vector3D, u: number[], v: number[]): boolean {
    return this.face.parameters(Patch.toFace(point), u, v);
  }

  /**
  Computes a vertex color for the vertices of the patch
  */
  public computeVertexColors(faceColors: FaceColorTable | null): void {
    for (let i = 0; i < this.numberOfVertices; i++) {
      this.vertex[i]!.computeColor(faceColors);
    }
  }

  /**
  Returns true if the two patches can "see" each other: P and Q see each
  other if at least a part of P is in front of Q and vice versa
  */
  public facing(other: Patch): boolean {
    return this.face.facing(other.face);
  }

  /**
  Like 'uv', but returns uniform coordinates (inverse of uniformPoint())
  */
  public uniformUv(point: Vector3D, u: number[], v: number[]): boolean {
    return this.face.uniformParameters(Patch.toFace(point), u, v);
  }
}
