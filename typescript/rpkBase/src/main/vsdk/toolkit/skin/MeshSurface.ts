import { Ray } from "vitral/dist/vsdk/toolkit/environment/geometry/element/Ray";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Statistics } from "../common/statistics/Statistics";
import { Material } from "../material/Material";
import { Geometry } from "./Geometry";
import { GeometryClassId } from "./GeometryClassId";
import { MaterialColorFlags } from "../material/MaterialColorFlags";
import type { Patch } from "../environment/geometry/elements/Patch";
import type { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import type { RayHit } from "../environment/geometry/elements/RayHit";
import { Vertex } from "../environment/geometry/elements/Vertex";

export class MeshSurface extends Geometry {
  private static nextSurfaceId = 0;
  public static colorFlags = MaterialColorFlags.NO_COLORS;

  public meshId: number;
  public objectName: string | null;
  public vertices: Vertex[] | null;
  public positions: Vector3Dd[] | null;
  public normals: Vector3Dd[] | null;
  public faces: Patch[] | null;
  public material: Material;

  public constructor(
    inObjectName: string,
    inMaterial: Material,
    inPoints: Vector3Dd[] | null,
    inNormals: Vector3Dd[] | null,
    texCoords: Vector3Dd[] | null,
    inVertices: Vertex[] | null,
    inFaces: Patch[] | null,
    inFlags: number
  ) {
    super();

    Statistics.instance().reader.numberOfSurfaces++;

    this.id = Geometry.nextGeometryId;
    Geometry.nextGeometryId++;
    this.objectName = inObjectName;
    this.meshId = MeshSurface.nextSurfaceId++;
    this.className = GeometryClassId.SURFACE_MESH;
    this.isDuplicate = false;

    this.material = inMaterial;
    this.positions = inPoints;
    this.normals = inNormals;
    this.vertices = inVertices;
    this.faces = inFaces;

    MeshSurface.colorFlags = inFlags;

    if (MeshSurface.colorFlags === MaterialColorFlags.VERTEX_COLORS) {
      for (let i = 0; this.vertices !== null && i < this.vertices.length; i++) {
        MeshSurface.normalizeVertexColor(this.vertices[i]!);
      }
    }

    if (MeshSurface.colorFlags !== MaterialColorFlags.VERTEX_COLORS) {
      for (let i = 0; this.vertices !== null && i < this.vertices.length; i++) {
        this.vertices[i]!.computeColor(null); // No face colors exist while loading
      }
    }

    MeshSurface.colorFlags = MaterialColorFlags.NO_COLORS;

    Geometry.patchListBounds(this.faces, this.boundingBox);
    this.boundingBox.enlargeTinyBit();
    this.bounded = true;
    this.shaftCullGeometry = false;
    this.radianceData = null;
    this.itemCount = 0;

    void texCoords;
  }

  public override destroy(): void {
    this.objectName = null;

    if (this.positions !== null) {
      this.positions.length = 0;
      this.positions = null;
    }

    if (this.normals !== null) {
      this.normals.length = 0;
      this.normals = null;
    }

    if (this.vertices !== null) {
      for (let i = 0; i < this.vertices.length; i++) {
        this.vertices[i]!.destroy();
      }
      this.vertices.length = 0;
      this.vertices = null;
    }

    if (this.faces !== null) {
      for (let i = 0; i < this.faces.length; i++) {
        this.faces[i]!.destroy();
      }
      this.faces.length = 0;
      this.faces = null;
    }

    super.destroy();
  }

  private static normalizeVertexColor(vertex: Vertex): void {
    let numberOfPatches = 0;
    if (vertex.patches !== null) {
      numberOfPatches = vertex.patches.length;
    }

    if (numberOfPatches > 0) {
      vertex.color.r /= numberOfPatches;
      vertex.color.g /= numberOfPatches;
      vertex.color.b /= numberOfPatches;
    }
  }

  public override discretizationIntersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null,
    exclusions: RayExclusionSet
  ): RayHit | null {
    return Geometry.patchListIntersect(this.faces, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
  }
}
