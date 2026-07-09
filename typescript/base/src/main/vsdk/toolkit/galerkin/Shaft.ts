import { Numeric } from "../common/linealAlgebra/Numeric";
import { Ray } from "../common/linealAlgebra/Ray";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { RayHitFlag } from "../environment/geometry/elements/RayHitFlag";
import { Polygon } from "../scene/Polygon";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { BoundingBoxCoordinateIndex } from "../skin/BoundingBoxCoordinateIndex";
import { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { GeometryClassId } from "../skin/GeometryClassId";
import { Patch } from "../environment/geometry/elements/Patch";
import { PatchSet } from "../environment/geometry/elements/PatchSet";
import { RayHit } from "../environment/geometry/elements/RayHit";
import { ShaftCullStrategy } from "./ShaftCullStrategy";
import { ShaftPlane } from "./ShaftPlane";
import { ShaftPlanePosition } from "./ShaftPlanePosition";

export class Shaft {
  private static createPatchSetWithPool(patches: Patch[] | null): PatchSet {
    const p = new PatchSet(patches);
    p.setMemoryPoolManaged(true);
    return p;
  }

  private static readonly MAX_SKIP_ELEMENTS = 2;
  private static readonly SHAFT_MAX_PLANES = 16;
  private static readonly MIN_MAX_DIMENSIONS = 6;
  private static readonly NONE = -1;

  private referenceItem1: BoundingBox | null;
  private referenceItem2: BoundingBox | null;
  private readonly extentBoundingBox: BoundingBox;
  private readonly planeSet: ShaftPlane[];
  private numberOfPlanesInSet: number;

  private readonly patchIdsToOmit: number[];
  private numberOfGeometriesToOmit: number;
  private readonly geometryIdsToAvoidOpening: number[];
  private numberOfGeometriesToAvoidOpen: number;

  private readonly center1: Vector3D;
  private readonly center2: Vector3D;
  private cut: boolean;

  public constructor() {
    this.referenceItem1 = null;
    this.referenceItem2 = null;
    this.extentBoundingBox = new BoundingBox();
    this.planeSet = new Array<ShaftPlane>(Shaft.SHAFT_MAX_PLANES);
    for (let i = 0; i < this.planeSet.length; i++) {
      this.planeSet[i] = new ShaftPlane();
    }
    this.numberOfPlanesInSet = 0;
    this.patchIdsToOmit = new Array<number>(Shaft.MAX_SKIP_ELEMENTS).fill(0);
    this.geometryIdsToAvoidOpening = new Array<number>(Shaft.MAX_SKIP_ELEMENTS).fill(0);
    this.numberOfGeometriesToOmit = 0;
    this.numberOfGeometriesToAvoidOpen = 0;
    this.center1 = new Vector3D();
    this.center2 = new Vector3D();
    this.cut = false;
  }

  private static fma(a: number, b: number, c: number): number {
    return a * b + c;
  }

  public static freeCandidateList(candidateList: Geometry[] | null): void {
    if (candidateList === null) {
      return;
    }

    for (let i = candidateList.length - 1; i >= 0; i--) {
      const geometry = candidateList[i];
      if (geometry !== null && geometry !== undefined && geometry.shaftCullGeometry) {
        Geometry.destroy(geometry);
      }
    }
    candidateList.length = 0;
  }

  public isCut(): boolean {
    return this.cut;
  }

  public setShaftOmit(patch: Patch | null): void {
    if (patch === null || this.numberOfGeometriesToOmit >= Shaft.MAX_SKIP_ELEMENTS) {
      return;
    }
    this.patchIdsToOmit[this.numberOfGeometriesToOmit++] = patch.id;
  }

  public setShaftDontOpen(geometry: Geometry | null): void {
    if (geometry === null || this.numberOfGeometriesToAvoidOpen >= Shaft.MAX_SKIP_ELEMENTS) {
      return;
    }
    this.geometryIdsToAvoidOpening[this.numberOfGeometriesToAvoidOpen++] = geometry.id;
  }

  public constructFromBoundingBoxes(boundingBox1: BoundingBox | null, boundingBox2: BoundingBox | null): void {
    this.numberOfGeometriesToOmit = 0;
    this.numberOfGeometriesToAvoidOpen = 0;
    this.cut = false;

    this.referenceItem1 = boundingBox1;
    this.referenceItem2 = boundingBox2;
    if (this.referenceItem1 === null || this.referenceItem2 === null) {
      this.numberOfPlanesInSet = 0;
      return;
    }

    this.center1.copy((this.referenceItem1 as BoundingBox).center());
    this.center2.copy((this.referenceItem2 as BoundingBox).center());

    const hasMinMax1 = new Array<boolean>(Shaft.MIN_MAX_DIMENSIONS).fill(false);
    const hasMinMax2 = new Array<boolean>(Shaft.MIN_MAX_DIMENSIONS).fill(false);

    this.extentBoundingBox.setAsUnion(this.referenceItem1, this.referenceItem2);
    this.referenceItem1.computeContributionFlags(this.referenceItem2, hasMinMax1, hasMinMax2);

    let localPlaneIndex = 0;
    for (let i = 0; i < Shaft.MIN_MAX_DIMENSIONS; i++) {
      if (!hasMinMax1[i]) {
        continue;
      }

      for (let j = 0; j < Shaft.MIN_MAX_DIMENSIONS; j++) {
        const a = i % 3;
        const b = j % 3;

        if (!hasMinMax2[j] || a === b) {
          continue;
        }

        const u1 = this.referenceItem1.valueAt(i);
        const v1 = this.referenceItem1.valueAt(j);
        const u2 = this.referenceItem2.valueAt(i);
        const v2 = this.referenceItem2.valueAt(j);

        let du: number;
        let dv: number;

        if (
          (i <= BoundingBoxCoordinateIndex.MIN_Z && j <= BoundingBoxCoordinateIndex.MIN_Z)
          || (i >= BoundingBoxCoordinateIndex.MAX_X && j >= BoundingBoxCoordinateIndex.MAX_X)
        ) {
          du = v2 - v1;
          dv = u1 - u2;
        }
        else {
          du = v1 - v2;
          dv = u2 - u1;
        }

      const localPlane = this.planeSet[localPlaneIndex]!;
        localPlane.n[a] = du;
        localPlane.n[b] = dv;
        localPlane.n[3 - a - b] = 0.0;
        const dExpr = -(du * u1 + dv * v1);
        let dResolved = dExpr;
        if (Math.abs(dResolved) <= 1.0e-10) {
          const dFma = -Shaft.fma(dv, v1, du * u1);
          if (dFma !== 0.0 && Math.abs(dFma) <= 1.0e-5) {
            dResolved = dFma;
          }
        }
        localPlane.d = dResolved;

        localPlane.coordinateOffset[0] = (localPlane.n[0] ?? 0.0) > 0.0 ? BoundingBoxCoordinateIndex.MIN_X : BoundingBoxCoordinateIndex.MAX_X;
        localPlane.coordinateOffset[1] = (localPlane.n[1] ?? 0.0) > 0.0 ? BoundingBoxCoordinateIndex.MIN_Y : BoundingBoxCoordinateIndex.MAX_Y;
        localPlane.coordinateOffset[2] = (localPlane.n[2] ?? 0.0) > 0.0 ? BoundingBoxCoordinateIndex.MIN_Z : BoundingBoxCoordinateIndex.MAX_Z;

        localPlaneIndex++;
      }
    }
    this.numberOfPlanesInSet = localPlaneIndex;
  }

  private static testPolygonWithRespectToPlane(polygon: Polygon, normal: Vector3D, d: number): ShaftPlanePosition {
    let out = false;
    let inside = false;

    for (let i = 0; i < polygon.numberOfVertices; i++) {
      const vtx = polygon.vertex[i];
      if (vtx === undefined) {
        continue;
      }
      const e = normal.dotProduct(vtx) + d;
      const tolerance = Math.abs(d) * Numeric.EPSILON + vtx.tolerance(Numeric.EPSILON_FLOAT);
      out = out || (e > tolerance);
      inside = inside || (e < -tolerance);
      if (out && inside) {
        return ShaftPlanePosition.OVERLAP;
      }
    }

    if (out) {
      return ShaftPlanePosition.OUTSIDE;
    }
    return inside ? ShaftPlanePosition.INSIDE : ShaftPlanePosition.COPLANAR;
  }

  private static verifyPolygonWithRespectToPlane(
    polygon: Polygon,
    normal: Vector3D,
    d: number,
    side: ShaftPlanePosition,
  ): boolean {
    let out = false;
    let inside = false;

    for (let i = 0; i < polygon.numberOfVertices; i++) {
      const vtx = polygon.vertex[i];
      if (vtx === undefined) {
        continue;
      }
      const e = normal.dotProduct(vtx) + d;
      const tolerance = Math.abs(d) * Numeric.EPSILON + vtx.tolerance(Numeric.EPSILON_FLOAT);
      out = out || e > tolerance;
      if (out && side === ShaftPlanePosition.INSIDE) {
        return false;
      }
      inside = inside || e < -tolerance;
      if (inside && side === ShaftPlanePosition.OUTSIDE) {
        return false;
      }
    }

    if (inside) {
      if (side === ShaftPlanePosition.INSIDE) {
        return true;
      }
    }
    else if (out) {
      if (side === ShaftPlanePosition.OUTSIDE) {
        return true;
      }
    }
    return false;
  }

  private static testPointWithRespectToPlane(p: Vector3D, normal: Vector3D, d: number): ShaftPlanePosition {
    const tolerance = Math.abs(d * Numeric.EPSILON) + p.tolerance(Numeric.EPSILON_FLOAT);
    const e = normal.dotProduct(p) + d;
    if (e < -tolerance) {
      return ShaftPlanePosition.INSIDE;
    }
    if (e > +tolerance) {
      return ShaftPlanePosition.OUTSIDE;
    }
    return ShaftPlanePosition.COPLANAR;
  }

  private static compareShaftPlanes(plane1: ShaftPlane, plane2: ShaftPlane): number {
    if ((plane1.n[0] ?? 0.0) < (plane2.n[0] ?? 0.0) - Numeric.EPSILON) {
      return -1;
    }
    else if ((plane1.n[0] ?? 0.0) > (plane2.n[0] ?? 0.0) + Numeric.EPSILON) {
      return +1;
    }

    if ((plane1.n[1] ?? 0.0) < (plane2.n[1] ?? 0.0) - Numeric.EPSILON) {
      return -1;
    }
    else if ((plane1.n[1] ?? 0.0) > (plane2.n[1] ?? 0.0) + Numeric.EPSILON) {
      return +1;
    }

    if ((plane1.n[2] ?? 0.0) < (plane2.n[2] ?? 0.0) - Numeric.EPSILON) {
      return -1;
    }
    else if ((plane1.n[2] ?? 0.0) > (plane2.n[2] ?? 0.0) + Numeric.EPSILON) {
      return +1;
    }

    const tolerance = Math.abs(Math.max(plane1.d, plane2.d) * Numeric.EPSILON);
    if (plane1.d < plane2.d - tolerance) {
      return -1;
    }
    else if (plane1.d > plane2.d + tolerance) {
      return +1;
    }
    return 0;
  }

  private uniqueShaftPlane(parameterPlane: ShaftPlane): boolean {
    for (let i = 0; this.planeSet[i] !== parameterPlane; i++) {
      if (Shaft.compareShaftPlanes(this.planeSet[i]!, parameterPlane) === 0) {
        return false;
      }
    }
    return true;
  }

  private static fillInPlane(plane: ShaftPlane, nx: number, ny: number, nz: number, d: number): void {
    plane.n[0] = nx;
    plane.n[1] = ny;
    plane.n[2] = nz;
    plane.d = d;

    plane.coordinateOffset[0] = plane.n[0] > 0.0 ? BoundingBoxCoordinateIndex.MIN_X : BoundingBoxCoordinateIndex.MAX_X;
    plane.coordinateOffset[1] = plane.n[1] > 0.0 ? BoundingBoxCoordinateIndex.MIN_Y : BoundingBoxCoordinateIndex.MAX_Y;
    plane.coordinateOffset[2] = plane.n[2] > 0.0 ? BoundingBoxCoordinateIndex.MIN_Z : BoundingBoxCoordinateIndex.MAX_Z;
  }

  private constructPolygonToPolygonPlanes(polygon1: Polygon, polygon2: Polygon): void {
    const normal = new Vector3D();
    let localPlaneIndex = this.numberOfPlanesInSet;
    let maxPlanesPerEdge: number;

    normal.copy(polygon1.normal);
    switch (Shaft.testPolygonWithRespectToPlane(polygon2, normal, polygon1.planeConstant)) {
      case ShaftPlanePosition.INSIDE:
        Shaft.fillInPlane(this.planeSet[localPlaneIndex]!, polygon1.normal.x, polygon1.normal.y, polygon1.normal.z, polygon1.planeConstant);
        if (this.uniqueShaftPlane(this.planeSet[localPlaneIndex]!)) {
          localPlaneIndex++;
        }
        maxPlanesPerEdge = 1;
        break;
      case ShaftPlanePosition.OUTSIDE:
        Shaft.fillInPlane(this.planeSet[localPlaneIndex]!, -polygon1.normal.x, -polygon1.normal.y, -polygon1.normal.z, -polygon1.planeConstant);
        if (this.uniqueShaftPlane(this.planeSet[localPlaneIndex]!)) {
          localPlaneIndex++;
        }
        maxPlanesPerEdge = 1;
        break;
      case ShaftPlanePosition.OVERLAP:
        maxPlanesPerEdge = 2;
        break;
      default:
        return;
    }

    for (let i = 0; i < polygon1.numberOfVertices; i++) {
      const current = polygon1.vertex[i];
      const next = polygon1.vertex[(i + 1) % polygon1.numberOfVertices];
      if (current === undefined || next === undefined) {
        continue;
      }
      let planesFoundForEdge = 0;

      for (let j = 0; j < polygon2.numberOfVertices && planesFoundForEdge < maxPlanesPerEdge; j++) {
        const other = polygon2.vertex[j];
        if (other === undefined) {
          continue;
        }

        normal.tripleCrossProduct(current, next, other);
        const localNorm = normal.norm();
        if (localNorm < Numeric.EPSILON) {
          continue;
        }
        normal.inverseScaledCopy(localNorm, normal, Numeric.EPSILON_FLOAT);
        const dNaive = -normal.dotProduct(current);
        const dExtended = -((normal.x * current.x) + (normal.y * current.y) + (normal.z * current.z));
        const dFma = -Shaft.fma(normal.z, current.z, normal.x * current.x + normal.y * current.y);
        let d = dNaive;
        if (Math.abs(d) <= 1.0e-10) {
          if (dFma !== 0.0 && Math.abs(dFma) <= 1.0e-5) {
            d = dFma;
          }
          else if (dExtended !== 0.0 && Math.abs(dExtended) <= 1.0e-5) {
            d = Math.abs(dExtended);
          }
        }

        let side = Shaft.testPointWithRespectToPlane(
          polygon1.vertex[(i + 2) % polygon1.numberOfVertices] ?? current,
          normal,
          d,
        );
        for (let k = (i + 3) % polygon1.numberOfVertices; k !== i; k = (k + 1) % polygon1.numberOfVertices) {
          const nSide = Shaft.testPointWithRespectToPlane(polygon1.vertex[k] ?? current, normal, d);
          if (side === ShaftPlanePosition.COPLANAR) {
            side = nSide;
          }
          else if (nSide !== ShaftPlanePosition.COPLANAR && side !== nSide) {
            side = ShaftPlanePosition.OVERLAP;
          }
        }
        if (side !== ShaftPlanePosition.INSIDE && side !== ShaftPlanePosition.OUTSIDE) {
          continue;
        }

        if (Shaft.verifyPolygonWithRespectToPlane(polygon2, normal, d, side)) {
          if (side === ShaftPlanePosition.INSIDE) {
            Shaft.fillInPlane(this.planeSet[localPlaneIndex]!, normal.x, normal.y, normal.z, d);
          }
          else {
            Shaft.fillInPlane(this.planeSet[localPlaneIndex]!, -normal.x, -normal.y, -normal.z, -d);
          }
          if (this.uniqueShaftPlane(this.planeSet[localPlaneIndex]!)) {
            localPlaneIndex++;
          }
          planesFoundForEdge++;
        }
      }
    }

    this.numberOfPlanesInSet = localPlaneIndex;
  }

  public constructFromPolygonToPolygon(polygon1: Polygon, polygon2: Polygon): void {
    this.referenceItem1 = null;
    this.referenceItem2 = null;

    this.extentBoundingBox.copyFrom(polygon1.bounds);
    this.extentBoundingBox.enlarge(polygon2.bounds);

    this.patchIdsToOmit[0] = Shaft.NONE;
    this.patchIdsToOmit[1] = Shaft.NONE;
    this.geometryIdsToAvoidOpening[0] = Shaft.NONE;
    this.geometryIdsToAvoidOpening[1] = Shaft.NONE;
    this.numberOfGeometriesToOmit = 0;
    this.numberOfGeometriesToAvoidOpen = 0;
    this.cut = false;

    const poly1v0 = polygon1.vertex[0];
    if (poly1v0 === undefined) {
      return;
    }
    this.center1.copy(poly1v0);
    for (let i = 1; i < polygon1.numberOfVertices; i++) {
      const v = polygon1.vertex[i];
      if (v !== undefined) {
        this.center1.addition(this.center1, v);
      }
    }
    this.center1.inverseScaledCopy(polygon1.numberOfVertices, this.center1, Numeric.EPSILON_FLOAT);

    const poly2v0 = polygon2.vertex[0];
    if (poly2v0 === undefined) {
      return;
    }
    this.center2.copy(poly2v0);
    for (let i = 1; i < polygon2.numberOfVertices; i++) {
      const v = polygon2.vertex[i];
      if (v !== undefined) {
        this.center2.addition(this.center2, v);
      }
    }
    this.center2.inverseScaledCopy(polygon2.numberOfVertices, this.center2, Numeric.EPSILON_FLOAT);

    this.numberOfPlanesInSet = 0;
    this.constructPolygonToPolygonPlanes(polygon1, polygon2);
    this.constructPolygonToPolygonPlanes(polygon2, polygon1);
  }

  private static evaluatePlane(plane: ShaftPlane, x: number, y: number, z: number): number {
    return Shaft.fma(plane.n[1] ?? 0.0, y, Shaft.fma(plane.n[0] ?? 0.0, x, Shaft.fma(plane.n[2] ?? 0.0, z, plane.d)));
  }

  private boundingBoxTest(parameterBoundingBox: BoundingBox): ShaftPlanePosition {
    if (parameterBoundingBox.disjointToOtherBoundingBox(this.extentBoundingBox)) {
      return ShaftPlanePosition.OUTSIDE;
    }

    for (let i = 0; i < this.numberOfPlanesInSet; i++) {
      const localPlane = this.planeSet[i]!;
      const e = Shaft.evaluatePlane(
        localPlane,
        parameterBoundingBox.valueAt(localPlane.coordinateOffset[0] ?? BoundingBoxCoordinateIndex.MIN_X),
        parameterBoundingBox.valueAt(localPlane.coordinateOffset[1] ?? BoundingBoxCoordinateIndex.MIN_Y),
        parameterBoundingBox.valueAt(localPlane.coordinateOffset[2] ?? BoundingBoxCoordinateIndex.MIN_Z),
      );
      if (e > -Math.abs(localPlane.d * Numeric.EPSILON)) {
        return ShaftPlanePosition.OUTSIDE;
      }
    }

    if (
      (this.referenceItem1 !== null && !parameterBoundingBox.disjointToOtherBoundingBox(this.referenceItem1))
      || (this.referenceItem2 !== null && !parameterBoundingBox.disjointToOtherBoundingBox(this.referenceItem2))
    ) {
      return ShaftPlanePosition.OVERLAP;
    }

    for (let i = 0; i < this.numberOfPlanesInSet; i++) {
      const localPlane = this.planeSet[i]!;
      const e = Shaft.evaluatePlane(
        localPlane,
        parameterBoundingBox.valueAt(((localPlane.coordinateOffset[0] ?? BoundingBoxCoordinateIndex.MIN_X) + 3) % 6),
        parameterBoundingBox.valueAt(((localPlane.coordinateOffset[1] ?? BoundingBoxCoordinateIndex.MIN_Y) + 3) % 6),
        parameterBoundingBox.valueAt(((localPlane.coordinateOffset[2] ?? BoundingBoxCoordinateIndex.MIN_Z) + 3) % 6),
      );
      if (e > Math.abs(localPlane.d * Numeric.EPSILON)) {
        return ShaftPlanePosition.OVERLAP;
      }
    }

    return ShaftPlanePosition.INSIDE;
  }

  // Scratch storage reused across shaftPatchTest calls (single threaded
  // rendering core, no recursion; the C++ port keeps these on the stack).
  // Every slot that is read is written first on each call.
  private static readonly shaftPatchTestScratchInAll = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(0);
  private static readonly shaftPatchTestScratchTMin = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(0.0);
  private static readonly shaftPatchTestScratchTMax = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(0.0);
  private static readonly shaftPatchTestScratchPTol = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(0.0);
  private static readonly shaftPatchTestScratchE = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(0.0);
  private static readonly shaftPatchTestScratchSide =
    new Array<ShaftPlanePosition>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(ShaftPlanePosition.COPLANAR);
  private static readonly shaftPatchTestScratchRay = new Ray();
  private static readonly shaftPatchTestScratchHitStore = new RayHit();
  private static readonly shaftPatchTestScratchDistance = [0.0];

  private shaftPatchTest(patch: Patch): ShaftPlanePosition {
    const inAll = Shaft.shaftPatchTestScratchInAll;
    const tMin = Shaft.shaftPatchTestScratchTMin;
    const tMax = Shaft.shaftPatchTestScratchTMax;
    const pTol = Shaft.shaftPatchTestScratchPTol;
    const ray = Shaft.shaftPatchTestScratchRay;
    const hitStore = Shaft.shaftPatchTestScratchHitStore;

    let someOut = false;
    for (let j = 0; j < patch.numberOfVertices; j++) {
      inAll[j] = 1;
      tMin[j] = 0.0;
      tMax[j] = 1.0;
      pTol[j] = patch.vertex[j]!.point.tolerance(Numeric.EPSILON_FLOAT);
    }

    for (let i = 0; i < this.numberOfPlanesInSet; i++) {
      const localPlane = this.planeSet[i]!;
      const e = Shaft.shaftPatchTestScratchE;
      const side = Shaft.shaftPatchTestScratchSide;
      let inside = false;
      let out = false;

      for (let j = 0; j < patch.numberOfVertices; j++) {
        const v = patch.vertex[j]!.point;
        e[j] = Shaft.evaluatePlane(localPlane, v.x, v.y, v.z);
        const tolerance = Math.abs(localPlane.d) * Numeric.EPSILON + (pTol[j] ?? 0.0);
        side[j] = ShaftPlanePosition.COPLANAR;
        if ((e[j] ?? 0.0) > tolerance) {
          side[j] = ShaftPlanePosition.OUTSIDE;
          out = true;
        }
        else if ((e[j] ?? 0.0) < -tolerance) {
          side[j] = ShaftPlanePosition.INSIDE;
          inside = true;
        }
        if (side[j] !== ShaftPlanePosition.INSIDE) {
          inAll[j] = 0;
        }
      }

      if (!inside) {
        return ShaftPlanePosition.OUTSIDE;
      }

      if (out) {
        someOut = true;

        for (let j = 0; j < patch.numberOfVertices; j++) {
          const k = (j + 1) % patch.numberOfVertices;
          if ((side[j] ?? ShaftPlanePosition.COPLANAR) !== (side[k] ?? ShaftPlanePosition.COPLANAR)) {
            if ((side[k] ?? ShaftPlanePosition.COPLANAR) === ShaftPlanePosition.OUTSIDE) {
              if ((side[j] ?? ShaftPlanePosition.COPLANAR) === ShaftPlanePosition.INSIDE) {
                if ((tMax[j] ?? 0.0) > (tMin[j] ?? 0.0)) {
                  const t = (e[j] ?? 0.0) / ((e[j] ?? 0.0) - (e[k] ?? 0.0));
                  if (t < (tMax[j] ?? 0.0)) {
                    tMax[j] = t;
                  }
                }
              }
              else {
                tMax[j] = -Numeric.EPSILON;
              }
            }
            else if ((side[j] ?? ShaftPlanePosition.COPLANAR) === ShaftPlanePosition.OUTSIDE) {
              if ((side[k] ?? ShaftPlanePosition.COPLANAR) === ShaftPlanePosition.INSIDE) {
                if ((tMin[j] ?? 0.0) < (tMax[j] ?? 0.0)) {
                  const t = (e[j] ?? 0.0) / ((e[j] ?? 0.0) - (e[k] ?? 0.0));
                  if (t > (tMin[j] ?? 0.0)) {
                    tMin[j] = t;
                  }
                }
              }
              else {
                tMin[j] = 1.0 + Numeric.EPSILON;
              }
            }
          }
          else if ((side[j] ?? ShaftPlanePosition.COPLANAR) === ShaftPlanePosition.OUTSIDE) {
            tMax[j] = -Numeric.EPSILON;
          }
        }
      }
    }

    if (this.referenceItem1 !== null || this.referenceItem2 !== null) {
      return ShaftPlanePosition.OVERLAP;
    }

    if (!someOut) {
      return ShaftPlanePosition.INSIDE;
    }

    for (let j = 0; j < patch.numberOfVertices; j++) {
      if (inAll[j] !== 0) {
        return ShaftPlanePosition.OVERLAP;
      }
    }

    for (let j = 0; j < patch.numberOfVertices; j++) {
      if ((tMin[j] ?? 0.0) + Numeric.EPSILON < (tMax[j] ?? 0.0) - Numeric.EPSILON) {
        return ShaftPlanePosition.OVERLAP;
      }
    }

    ray.position.copy(this.center1);
    ray.direction.subtraction(this.center2, this.center1);
    const dist = Shaft.shaftPatchTestScratchDistance;
    dist[0] = 1.0 - Numeric.EPSILON_FLOAT;
    if (
      patch.intersect(
        ray,
        Numeric.EPSILON_FLOAT,
        dist,
        RayHitFlag.FRONT | RayHitFlag.BACK,
        hitStore,
      ) !== null
    ) {
      this.cut = true;
      return ShaftPlanePosition.OVERLAP;
    }

    return ShaftPlanePosition.OUTSIDE;
  }

  private patchIsOnOmitSet(id: number): boolean {
    for (let i = 0; i < this.numberOfGeometriesToOmit && i < Shaft.MAX_SKIP_ELEMENTS; i++) {
      if (this.patchIdsToOmit[i] === id) {
        return true;
      }
    }
    return false;
  }

  private closedGeometry(geometry: Geometry): boolean {
    for (let i = 0; i < this.numberOfGeometriesToAvoidOpen && i < Shaft.MAX_SKIP_ELEMENTS; i++) {
      if (this.geometryIdsToAvoidOpening[i] === geometry.id) {
        return true;
      }
    }
    return false;
  }

  private cullPatches(patchList: Patch[] | null, culledPatchList: Patch[]): void {
    culledPatchList.length = 0;
    for (let i = 0; patchList !== null && i < patchList.length && !this.cut; i++) {
      const patch = patchList[i];
      if (patch === undefined || patch.omit !== 0 || this.patchIsOnOmitSet(patch.id)) {
        continue;
      }

      if (patch.boundingBox === null) {
        patch.computeBoundingBox();
      }

      const boundingBoxSide = this.boundingBoxTest(patch.boundingBox as BoundingBox);
      if (
        boundingBoxSide !== ShaftPlanePosition.OUTSIDE
        && (boundingBoxSide === ShaftPlanePosition.INSIDE || this.shaftPatchTest(patch) !== ShaftPlanePosition.OUTSIDE)
      ) {
        culledPatchList.push(patch);
      }
    }
  }

  private static keep(geometry: Geometry | null, candidateList: Geometry[] | null): void {
    if (geometry === null || candidateList === null || geometry.omit) {
      return;
    }

    if (geometry.shaftCullGeometry && geometry.className === GeometryClassId.PATCH_SET) {
      const oldPatchSet = geometry as PatchSet;
      const newGeometry = Shaft.createPatchSetWithPool(oldPatchSet.getPatchList());
      newGeometry.shaftCullGeometry = true;
      newGeometry.isDuplicate = true;
      candidateList.push(newGeometry);
    }
    else {
      candidateList.push(geometry);
    }
  }

  private shaftCullOpen(geometry: Geometry | null, candidateList: Geometry[] | null, strategy: ShaftCullStrategy): void {
    if (geometry === null || candidateList === null || geometry.omit) {
      return;
    }

    if (geometry.isCompound()) {
      const compound = geometry as Compound;
      this.doCulling(compound.children, candidateList, strategy);
    }
    else {
      const geometryPatchesList = Geometry.patchListReference(geometry);
      const culledPatches: Patch[] = [];
      this.cullPatches(geometryPatchesList, culledPatches);

      if (culledPatches.length > 0) {
        const newGeometry = Shaft.createPatchSetWithPool(culledPatches);
        newGeometry.shaftCullGeometry = true;
        newGeometry.isDuplicate = false;
        candidateList.push(newGeometry);
      }
      culledPatches.length = 0;
    }
  }

  public cullGeometry(geometry: Geometry | null, candidateList: Geometry[] | null, strategy: ShaftCullStrategy): void {
    if (geometry === null || candidateList === null) {
      return;
    }

    if (geometry.className === GeometryClassId.PATCH_SET) {
      // The C++ port probes the omit set here through an invalid reinterpret_cast of the
      // Geometry as a Patch, reading an arbitrary value that in practice never matches an
      // omitted patch id, so PatchSet geometries are effectively never culled by that test.
      // Probing with a real patch id (e.g. a hardcoded 1) wrongly discards every PatchSet
      // occluder for interactions whose receiver or source is that patch, causing light
      // leaks (see corridor scene, floor patch 1). Only the omit flag is honored.
      if (geometry.omit) {
        return;
      }
    }

    const side = geometry.bounded ? this.boundingBoxTest(geometry.boundingBox) : ShaftPlanePosition.OVERLAP;
    switch (side) {
      case ShaftPlanePosition.INSIDE:
        if (strategy === ShaftCullStrategy.ALWAYS_OPEN && !this.closedGeometry(geometry)) {
          this.shaftCullOpen(geometry, candidateList, strategy);
        }
        else {
          Shaft.keep(geometry, candidateList);
        }
        break;
      case ShaftPlanePosition.OVERLAP:
        if (this.closedGeometry(geometry)) {
          Shaft.keep(geometry, candidateList);
        }
        else {
          this.shaftCullOpen(geometry, candidateList, strategy);
        }
        break;
      default:
        break;
    }
  }

  public doCulling(world: Geometry[] | null, candidateList: Geometry[] | null, strategy: ShaftCullStrategy): void {
    if (candidateList === null) {
      return;
    }
    for (let i = 0; world !== null && i < world.length && !this.cut; i++) {
      this.cullGeometry(world[i] ?? null, candidateList, strategy);
    }
  }
}
