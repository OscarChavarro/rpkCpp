import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Dd";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import type { Material } from "./Material";

/**
Immutable shading input data used by material evaluation.
*/
export class ShadingContext {
  private readonly point: Vector3Df;
  private readonly geometricNormal: Vector3Df;
  private readonly shadingNormal: Vector3Df;
  private readonly texCoord: Vector3Df;
  private readonly uv: Vector2Dd;
  private readonly shadingFrame: CoordinateSystem;
  private readonly material: Material | null;
  private readonly flags: number;

  public constructor(
    inPoint: Vector3Df,
    inGeometricNormal: Vector3Df,
    inShadingNormal: Vector3Df,
    inTexCoord: Vector3Df,
    inUv: Vector2Dd,
    inShadingFrame: CoordinateSystem,
    inMaterial: Material | null,
    inFlags: number
  ) {
    this.point = new Vector3Df(inPoint.x(), inPoint.y(), inPoint.z());
    this.geometricNormal = new Vector3Df(inGeometricNormal.x(), inGeometricNormal.y(), inGeometricNormal.z());
    this.shadingNormal = new Vector3Df(inShadingNormal.x(), inShadingNormal.y(), inShadingNormal.z());
    this.texCoord = new Vector3Df(inTexCoord.x(), inTexCoord.y(), inTexCoord.z());
    this.uv = new Vector2Dd();
    this.uv.x = inUv.x;
    this.uv.y = inUv.y;
    this.shadingFrame = new CoordinateSystem();
    this.shadingFrame.setX(inShadingFrame.getX());
    this.shadingFrame.setY(inShadingFrame.getY());
    this.shadingFrame.setZ(inShadingFrame.getZ());
    this.material = inMaterial;
    this.flags = inFlags;
  }

  public getPoint(): Vector3Df { return this.point; }
  public getGeometricNormal(): Vector3Df { return this.geometricNormal; }
  public getShadingNormal(): Vector3Df { return this.shadingNormal; }
  public getTexCoord(): Vector3Df { return this.texCoord; }
  public getUv(): Vector2Dd { return this.uv; }
  public getShadingFrame(): CoordinateSystem { return this.shadingFrame; }
  public getMaterial(): Material | null { return this.material; }
  public getFlags(): number { return this.flags; }
  public hasFlag(mask: number): boolean { return (this.flags & mask) === mask; }
}
