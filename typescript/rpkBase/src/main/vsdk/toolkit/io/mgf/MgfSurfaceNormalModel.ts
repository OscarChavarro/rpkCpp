import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";

export enum MgfSurfaceNormalModelKind {
  /** Closed planar solid: every face, no vertex normals */
  FLAT_SOLID,
  /** Lamina on the plane z = 0: the faces looking to +Z, no normals */
  FLAT_LAMINA,
  /** Sphere centered at the origin: every face, radial normals */
  SPHERE,
  /**
  Lamina of a cone (or cylinder) along +Z, from radius `bottomRadius` at
  z = 0 to radius `topRadius` at z = `height`: the faces looking away from
  the axis
  */
  CONE_LAMINA,
  /** Torus around the Z axis, of major radius `majorRadius` */
  TORUS,
}

/**
Vertex normals of the patches of a MGF primitive, in the local frame of the
VITRAL primitive that tessellates it, and the choice of the faces of its
boundary representation that become patches.

The normals are the ones the classic MGF tessellation gave (the one of the
MGF parser library, where spheres and tori were expanded into cones whose
end normals were "warped"): the exact normal of the curved surface at each
vertex for spheres and tori, the normal of the cone for cones and cylinders
(constant along each generatrix, and the axis at the apex), and none for
planar primitives.

Open primitives (cones, cylinders and rings) are exported by VITRAL as
laminas: two coincident sheets of faces with opposite orientations. Only
the faces of the sheet oriented as the MGF surface become patches.
*/
export class MgfSurfaceNormalModel {
  private readonly kind: MgfSurfaceNormalModelKind;
  private readonly bottomRadius: number;
  private readonly topRadius: number;
  private readonly height: number;
  private readonly majorRadius: number;

  private constructor(
    kind: MgfSurfaceNormalModelKind,
    bottomRadius: number,
    topRadius: number,
    height: number,
    majorRadius: number) {
    this.kind = kind;
    this.bottomRadius = bottomRadius;
    this.topRadius = topRadius;
    this.height = height;
    this.majorRadius = majorRadius;
  }

  public static flatSolid(): MgfSurfaceNormalModel {
    return new MgfSurfaceNormalModel(MgfSurfaceNormalModelKind.FLAT_SOLID, 0.0, 0.0, 0.0, 0.0);
  }

  public static flatLamina(): MgfSurfaceNormalModel {
    return new MgfSurfaceNormalModel(MgfSurfaceNormalModelKind.FLAT_LAMINA, 0.0, 0.0, 0.0, 0.0);
  }

  public static sphere(): MgfSurfaceNormalModel {
    return new MgfSurfaceNormalModel(MgfSurfaceNormalModelKind.SPHERE, 0.0, 0.0, 0.0, 0.0);
  }

  public static coneLamina(bottomRadius: number, topRadius: number, height: number): MgfSurfaceNormalModel {
    return new MgfSurfaceNormalModel(MgfSurfaceNormalModelKind.CONE_LAMINA, bottomRadius, topRadius, height, 0.0);
  }

  public static torus(majorRadius: number): MgfSurfaceNormalModel {
    return new MgfSurfaceNormalModel(MgfSurfaceNormalModelKind.TORUS, 0.0, 0.0, 0.0, majorRadius);
  }

  public getKind(): MgfSurfaceNormalModelKind {
    return this.kind;
  }

  /**
  @return true if the patches get vertex normals
  */
  public hasVertexNormals(): boolean {
    return this.kind === MgfSurfaceNormalModelKind.SPHERE ||
      this.kind === MgfSurfaceNormalModelKind.CONE_LAMINA ||
      this.kind === MgfSurfaceNormalModelKind.TORUS;
  }

  /**
  @param localPoint a vertex of the tessellation, in the local frame
  @return the (outwards, not necessarily unit) normal of the surface at the
  vertex, in the local frame
  */
  public vertexNormal(localPoint: Vector3Dd): Vector3Dd {
    const radial = Math.sqrt(localPoint.x() * localPoint.x() + localPoint.y() * localPoint.y());

    switch (this.kind) {
      case MgfSurfaceNormalModelKind.SPHERE:
        return localPoint;
      case MgfSurfaceNormalModelKind.CONE_LAMINA:
        if (radial <= VSDK.EPSILON * (this.bottomRadius + this.topRadius)) {
          // The apex: the classic tessellation gives it the axis
          return new Vector3Dd(0.0, 0.0, 1.0);
        }
        // Radial direction, raised by the slope of the generatrix
        return new Vector3Dd(
          localPoint.x() / radial,
          localPoint.y() / radial,
          (this.bottomRadius - this.topRadius) / this.height);
      case MgfSurfaceNormalModelKind.TORUS:
        if (radial <= 0.0) {
          return new Vector3Dd(0.0, 0.0, 0.0);
        }
        // From the closest point of the core circle
        return new Vector3Dd(
          localPoint.x() - this.majorRadius * localPoint.x() / radial,
          localPoint.y() - this.majorRadius * localPoint.y() / radial,
          localPoint.z());
      default:
        return new Vector3Dd(0.0, 0.0, 0.0);
    }
  }

  /**
  @param localCentroid centroid of a face of the boundary representation
  @param localFaceNormal normal of the face (following its loop)
  @return true if the face becomes a patch
  */
  public selectsFace(localCentroid: Vector3Dd, localFaceNormal: Vector3Dd): boolean {
    switch (this.kind) {
      case MgfSurfaceNormalModelKind.FLAT_LAMINA:
        return localFaceNormal.z() > 0.0;
      case MgfSurfaceNormalModelKind.CONE_LAMINA:
        return localFaceNormal.dotProduct(this.vertexNormal(localCentroid)) > 0.0;
      default:
        return true;
    }
  }
}
