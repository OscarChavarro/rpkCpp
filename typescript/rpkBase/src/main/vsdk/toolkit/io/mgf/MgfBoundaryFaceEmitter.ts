import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import type { PolyhedralBoundedSolid } from "vitral/dist/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid";
import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import type { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ReaderContext } from "../context/ReaderContext";
import { MgfEntityControl } from "./MgfEntityControl";
import type { MgfSurfaceNormalModel } from "./MgfSurfaceNormalModel";
import { MgfTessellationMath } from "./MgfTessellationMath";

/**
Turns the faces of a VITRAL boundary representation, built in the local
frame of a primitive, into MGF faces: for each face, its vertices are
defined (`v`, `p` and, if any, `n` entities, with the values written as the
MGF tessellators always did) and a `f` entity references them. So the faces
go through the same path as the faces of the file: the current transform,
the current material and its sidedness (two sided materials get twin
patches), and the grouping of the primitive in one surface.

Every face gets its own vertices, as with the classic tessellation, where
each `v` entity redefined in it gave a new vertex.

The local frame is given by an origin and three orthonormal axes u, v, w
(right handed), all in MGF coordinates: the local point (x, y, z) is
origin + x * u + y * v + z * w. A reversed emitter turns the surfaces
inside out (MGF primitives with negative radii): it reverses the order of
the vertices of the faces and the vertex normals.
*/
export class MgfBoundaryFaceEmitter {
  private readonly origin: Vector3Dd;
  private readonly u: Vector3Dd;
  private readonly v: Vector3Dd;
  private readonly w: Vector3Dd;
  private readonly reversed: boolean;

  public constructor(origin: Vector3Dd, u: Vector3Dd, v: Vector3Dd, w: Vector3Dd, reversed: boolean) {
    this.origin = origin;
    this.u = u;
    this.v = v;
    this.w = w;
    this.reversed = reversed;
  }

  public toMgfPoint(localPoint: Vector3Dd): Vector3Dd {
    const origin = this.origin;
    const u = this.u;
    const v = this.v;
    const w = this.w;
    return new Vector3Dd(
      origin.x() + localPoint.x() * u.x() + localPoint.y() * v.x() + localPoint.z() * w.x(),
      origin.y() + localPoint.x() * u.y() + localPoint.y() * v.y() + localPoint.z() * w.y(),
      origin.z() + localPoint.x() * u.z() + localPoint.y() * v.z() + localPoint.z() * w.z());
  }

  public toMgfDirection(localDirection: Vector3Dd): Vector3Dd {
    const u = this.u;
    const v = this.v;
    const w = this.w;
    return new Vector3Dd(
      localDirection.x() * u.x() + localDirection.y() * v.x() + localDirection.z() * w.x(),
      localDirection.x() * u.y() + localDirection.y() * v.y() + localDirection.z() * w.y(),
      localDirection.x() * u.z() + localDirection.y() * v.z() + localDirection.z() * w.z());
  }

  /**
  Emits a face.
  @param numberOfVertices number of vertices of the face
  @param localPoints its vertices in the local frame, counterclockwise seen
  from the outside of the (not reversed) surface
  @param mgfNormals vertex normals in MGF coordinates (oriented as the not
  reversed surface; a null vector for none), or null for no normals
  @return the MGF error code
  */
  public emitFace(
    numberOfVertices: number,
    localPoints: Vector3Dd[],
    mgfNormals: Vector3Dd[] | null,
    context: ParseRuntimeContext): number {
    const p = [[""], [""], [""]];
    const n = [[""], [""], [""]];
    const vertexEntity: string[] = [context.entityNames[EntityTypeContext.VERTEX]!, "", "="];
    const pointEntity: string[] = [context.entityNames[EntityTypeContext.MGF_POINT]!, "", "", ""];
    const normalEntity: string[] = [context.entityNames[EntityTypeContext.MGF_NORMAL]!, "", "", ""];
    const faceEntity: string[] = [];

    if (numberOfVertices < 3 || numberOfVertices > ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2) {
      return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    faceEntity.push(context.entityNames[EntityTypeContext.FACE]!);
    for (let i = 0; i < numberOfVertices; i++) {
      // The vertices of a reversed face are taken backwards
      const k = this.reversed ? numberOfVertices - 1 - i : i;
      const name = `_bv${i}`;
      vertexEntity[1] = name;
      let errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, vertexEntity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }

      const point = this.toMgfPoint(localPoints[k]!);
      MgfTessellationMath.formatFloat(p[0]!, MgfTessellationMath.MGF_PV_SIZE, point.x());
      MgfTessellationMath.formatFloat(p[1]!, MgfTessellationMath.MGF_PV_SIZE, point.y());
      MgfTessellationMath.formatFloat(p[2]!, MgfTessellationMath.MGF_PV_SIZE, point.z());
      pointEntity[1] = p[0]![0]!;
      pointEntity[2] = p[1]![0]!;
      pointEntity[3] = p[2]![0]!;
      errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, pointEntity, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }

      if (mgfNormals !== null && !MgfTessellationMath.isNull(mgfNormals[k]!, 0.0)) {
        const sign = this.reversed ? -1.0 : 1.0;
        const normal = mgfNormals[k]!;
        MgfTessellationMath.formatFloat(n[0]!, MgfTessellationMath.MGF_PV_SIZE, sign * normal.x());
        MgfTessellationMath.formatFloat(n[1]!, MgfTessellationMath.MGF_PV_SIZE, sign * normal.y());
        MgfTessellationMath.formatFloat(n[2]!, MgfTessellationMath.MGF_PV_SIZE, sign * normal.z());
        normalEntity[1] = n[0]![0]!;
        normalEntity[2] = n[1]![0]!;
        normalEntity[3] = n[2]![0]!;
        errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, normalEntity, context);
        if (errorCode !== ParseErrorContext.MGF_OK) {
          return errorCode;
        }
      }
      faceEntity.push(name);
    }
    return MgfEntityControl.mgfHandle(EntityTypeContext.FACE, numberOfVertices + 1, faceEntity, context);
  }

  /**
  Emits the faces of `solid` selected by `model`, with the vertex normals
  it gives.
  @return the MGF error code
  */
  public emitSolid(solid: PolyhedralBoundedSolid, model: MgfSurfaceNormalModel, context: ParseRuntimeContext): number {
    const points: Vector3Dd[] = [];
    const normals: Vector3Dd[] = [];

    for (let i = 0; i < solid.getPolygonsList().size(); i++) {
      const numberOfVertices = MgfBoundaryFaceEmitter.faceVertices(
        solid, i, points, ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2);
      if (numberOfVertices < 3) {
        return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
      }

      let centroid = new Vector3Dd(0.0, 0.0, 0.0);
      for (let k = 0; k < numberOfVertices; k++) {
        centroid = centroid.add(points[k]!);
      }
      centroid = centroid.multiply(1.0 / numberOfVertices);
      if (!model.selectsFace(centroid, MgfBoundaryFaceEmitter.polygonNormal(numberOfVertices, points))) {
        continue;
      }

      let faceNormals: Vector3Dd[] | null = null;
      if (model.hasVertexNormals()) {
        for (let k = 0; k < numberOfVertices; k++) {
          normals[k] = this.toMgfDirection(model.vertexNormal(points[k]!));
        }
        faceNormals = normals;
      }
      const errorCode = this.emitFace(numberOfVertices, points, faceNormals, context);
      if (errorCode !== ParseErrorContext.MGF_OK) {
        return errorCode;
      }
    }
    return ParseErrorContext.MGF_OK;
  }

  /**
  Collects the vertices of the outer loop of face number `faceIndex` of
  `solid` (in the list of faces), in their order.
  @return the number of vertices (at most `maximumVertices`), or -1 if the
  face has holes or more vertices
  */
  public static faceVertices(
    solid: PolyhedralBoundedSolid,
    faceIndex: number,
    localPoints: Vector3Dd[],
    maximumVertices: number): number {
    const face = solid.getPolygonsList().get(faceIndex)!;
    if (face.boundariesList.size() !== 1) {
      return -1;
    }
    const start = face.boundariesList.get(0)!.boundaryStartHalfEdge!;
    let he = start;
    let count = 0;
    do {
      if (count >= maximumVertices) {
        return -1;
      }
      localPoints[count++] = he.startingVertex.position;
      he = he.next()!;
    } while (he !== start);
    return count;
  }

  /**
  @return the normal (Newell's method, not normalized) of a polygon
  */
  public static polygonNormal(numberOfVertices: number, points: Vector3Dd[]): Vector3Dd {
    let x = 0.0;
    let y = 0.0;
    let z = 0.0;
    for (let i = 0; i < numberOfVertices; i++) {
      const a = points[i]!;
      const b = points[(i + 1) % numberOfVertices]!;
      x += (a.y() - b.y()) * (a.z() + b.z());
      y += (a.z() - b.z()) * (a.x() + b.x());
      z += (a.x() - b.x()) * (a.y() + b.y());
    }
    return new Vector3Dd(x, y, z);
  }
}
