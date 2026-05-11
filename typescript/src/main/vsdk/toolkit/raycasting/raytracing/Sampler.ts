import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { BsdfComponent } from "../../material/BsdfComponent";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { PathRayType } from "../common/PathRayType";
import { RayTools } from "../common/RayTools";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";

export abstract class Sampler {
  public static readonly BSDF_ALL_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  protected sampleTransfer(
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    dir: Vector3D,
    pdfDir: number
  ): boolean {
    const ray = new Ray();
    ray.position = thisNode.m_hit.getPoint();
    ray.direction.copy(dir);

    newNode.m_depth = thisNode.m_depth + 1;
    newNode.m_rayType = PathRayType.STOPS;
    const hit = RayTools.findRayIntersection(
      sceneVoxelGrid,
      ray,
      thisNode.m_hit.getPatch(),
      newNode.m_inBsdf,
      newNode.m_hit
    );

    if (hit === null) {
      if (sceneBackground !== null) {
        newNode.m_hit.init(sceneBackground.bkgPatch, null, dir, null);
        newNode.m_inDirT.copy(dir);
        newNode.m_inDirF.set(dir.x, dir.y, dir.z);
        newNode.m_pdfFromPrev = pdfDir;
        newNode.m_G = globalThis.Math.abs(thisNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
        newNode.m_inBsdf = thisNode.m_outBsdf;
        newNode.m_useBsdf = null;
        newNode.m_outBsdf = null;
        newNode.m_rayType = PathRayType.ENVIRONMENT;
        const newFlags = newNode.m_hit.getFlags() | RayHitFlag.FRONT;
        newNode.m_hit.setFlags(newFlags);

        return true;
      }
      return false;
    }

    if ((hit.getFlags() & RayHitFlag.BACK) !== 0) {
      const normal = new Vector3D();
      normal.scaledCopy(-1.0, newNode.m_hit.getNormal());
      newNode.m_hit.setNormal(normal);
    }

    newNode.m_inDirT.copy(ray.direction);
    newNode.m_inDirF.scaledCopy(-1.0, newNode.m_inDirT);

    if (newNode.m_hit.getNormal().dotProduct(newNode.m_inDirF) < 0.0) {
      return false;
    }

    const tmpVec = new Vector3D();

    const cosA = globalThis.Math.abs(thisNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
    const cosB = globalThis.Math.abs(newNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
    tmpVec.subtraction(newNode.m_hit.getPoint(), thisNode.m_hit.getPoint());
    const dist2 = tmpVec.norm2();

    if (dist2 < Numeric.EPSILON) {
      return false;
    }

    newNode.m_G = cosA * cosB / dist2;

    newNode.m_pdfFromPrev = pdfDir * cosB / dist2;

    return true;
  }

  public abstract sample(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    prevNode: SimpleRaytracingPathNode | null,
    thisNode: SimpleRaytracingPathNode | null,
    newNode: SimpleRaytracingPathNode,
    x1: number,
    x2: number,
    doRR?: boolean,
    flags?: number
  ): boolean;

  public abstract evalPDF(
    camera: Camera,
    thisNode: SimpleRaytracingPathNode,
    newNode: SimpleRaytracingPathNode,
    flags?: number,
    probabilityDensityFunction?: number[] | null,
    probabilityDensityFunctionRR?: number[] | null
  ): number;
}
