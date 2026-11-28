import { CoordinateSystem } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Dd";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import type { Material } from "./Material";

/**
Immutable shading input data used by material evaluation.
*/
export class ShadingContext {
  private readonly point: Vector3Dd;
  private readonly geometricNormal: Vector3Dd;
  private readonly shadingNormal: Vector3Dd;
  private readonly texCoord: Vector3Dd;
  private readonly uv: Vector2Dd;
  private readonly shadingFrame: CoordinateSystem;
  private readonly material: Material | null;
  private readonly flags: number;

  public constructor(
    inPoint: Vector3Dd,
    inGeometricNormal: Vector3Dd,
    inShadingNormal: Vector3Dd,
    inTexCoord: Vector3Dd,
    inUv: Vector2Dd,
    inShadingFrame: CoordinateSystem,
    inMaterial: Material | null,
    inFlags: number
  ) {
    this.point = new Vector3Dd(inPoint.x(), inPoint.y(), inPoint.z());
    this.geometricNormal = new Vector3Dd(inGeometricNormal.x(), inGeometricNormal.y(), inGeometricNormal.z());
    this.shadingNormal = new Vector3Dd(inShadingNormal.x(), inShadingNormal.y(), inShadingNormal.z());
    this.texCoord = new Vector3Dd(inTexCoord.x(), inTexCoord.y(), inTexCoord.z());
    this.uv = new Vector2Dd();
    this.uv.x = inUv.x;
    this.uv.y = inUv.y;
    this.shadingFrame = new CoordinateSystem();
    this.shadingFrame.setXAxis(inShadingFrame.getXAxis());
    this.shadingFrame.setYAxis(inShadingFrame.getYAxis());
    this.shadingFrame.setZAxis(inShadingFrame.getZAxis());
    this.material = inMaterial;
    this.flags = inFlags;
  }

  public getPoint(): Vector3Dd { return this.point; }
  public getGeometricNormal(): Vector3Dd { return this.geometricNormal; }
  public getShadingNormal(): Vector3Dd { return this.shadingNormal; }
  public getTexCoord(): Vector3Dd { return this.texCoord; }
  public getUv(): Vector2Dd { return this.uv; }
  public getShadingFrame(): CoordinateSystem { return this.shadingFrame; }
  public getMaterial(): Material | null { return this.material; }
  public getFlags(): number { return this.flags; }
  public hasFlag(mask: number): boolean { return (this.flags & mask) === mask; }
}
