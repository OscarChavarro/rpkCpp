import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "../common/linealAlgebra/Vector2Dd";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import type { Material } from "./Material";

/**
Immutable shading input data used by material evaluation.
*/
export class ShadingContext {
  private readonly point: Vector3D;
  private readonly geometricNormal: Vector3D;
  private readonly shadingNormal: Vector3D;
  private readonly texCoord: Vector3D;
  private readonly uv: Vector2Dd;
  private readonly shadingFrame: CoordinateSystem;
  private readonly material: Material | null;
  private readonly flags: number;

  public constructor(
    inPoint: Vector3D,
    inGeometricNormal: Vector3D,
    inShadingNormal: Vector3D,
    inTexCoord: Vector3D,
    inUv: Vector2Dd,
    inShadingFrame: CoordinateSystem,
    inMaterial: Material | null,
    inFlags: number
  ) {
    this.point = new Vector3D(inPoint.x, inPoint.y, inPoint.z);
    this.geometricNormal = new Vector3D(inGeometricNormal.x, inGeometricNormal.y, inGeometricNormal.z);
    this.shadingNormal = new Vector3D(inShadingNormal.x, inShadingNormal.y, inShadingNormal.z);
    this.texCoord = new Vector3D(inTexCoord.x, inTexCoord.y, inTexCoord.z);
    this.uv = new Vector2Dd();
    this.uv.u = inUv.u;
    this.uv.v = inUv.v;
    this.shadingFrame = new CoordinateSystem();
    this.shadingFrame.setX(inShadingFrame.getX());
    this.shadingFrame.setY(inShadingFrame.getY());
    this.shadingFrame.setZ(inShadingFrame.getZ());
    this.material = inMaterial;
    this.flags = inFlags;
  }

  public getPoint(): Vector3D { return this.point; }
  public getGeometricNormal(): Vector3D { return this.geometricNormal; }
  public getShadingNormal(): Vector3D { return this.shadingNormal; }
  public getTexCoord(): Vector3D { return this.texCoord; }
  public getUv(): Vector2Dd { return this.uv; }
  public getShadingFrame(): CoordinateSystem { return this.shadingFrame; }
  public getMaterial(): Material | null { return this.material; }
  public getFlags(): number { return this.flags; }
  public hasFlag(mask: number): boolean { return (this.flags & mask) === mask; }
}
