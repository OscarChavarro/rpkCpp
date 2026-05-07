import { Error as VsdkError } from "../common/Error";
import { CoordinateSystem } from "../common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "../common/linealAlgebra/Vector2Dd";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Material } from "../material/Material";
import { PhongBidirectionalScatteringDistributionFunction } from "../material/PhongBidirectionalScatteringDistributionFunction";
import { PhongEmittanceDistributionFunction } from "../material/PhongEmittanceDistributionFunction";
import { RayHitFlag } from "../skin/RayHitFlag";
import type { Patch } from "./Patch";

export class RayHit {
  private point: Vector3D;
  private patch: Patch | null;
  private texCoord: Vector3D;
  private geometricNormal: Vector3D;
  private material: Material | null;
  private shadingFrame: CoordinateSystem;
  private uv: Vector2Dd;
  private flags: number;

  public constructor() {
    this.point = new Vector3D();
    this.patch = null;
    this.texCoord = new Vector3D();
    this.geometricNormal = new Vector3D();
    this.material = null;
    this.shadingFrame = new CoordinateSystem();
    this.uv = new Vector2Dd();
    this.flags = 0;
  }

  private hitInitialised(): boolean {
    return (((this.flags & RayHitFlag.PATCH) !== 0) || ((this.flags & RayHitFlag.GEOMETRY) !== 0))
      && ((this.flags & RayHitFlag.POINT) !== 0)
      && ((this.flags & RayHitFlag.GEOMETRIC_NORMAL) !== 0)
      && ((this.flags & RayHitFlag.MATERIAL) !== 0)
      && ((this.flags & RayHitFlag.DISTANCE) !== 0);
  }

  private computeUv(inUv: Vector2Dd): boolean {
    if ((this.flags & RayHitFlag.UV) !== 0) {
      inUv.u = this.uv.u;
      inUv.v = this.uv.v;
      return true;
    }

    if (((this.flags & RayHitFlag.PATCH) !== 0) && ((this.flags & RayHitFlag.POINT) !== 0) && this.patch !== null) {
      const u = [0.0];
      const v = [0.0];
      this.patch.uv(this.point, u, v);
      this.uv.u = u[0];
      this.uv.v = v[0];
      inUv.u = this.uv.u;
      inUv.v = this.uv.v;
      this.flags |= RayHitFlag.UV;
      return true;
    }

    return false;
  }

  private pointShadingFrame(inX: Vector3D | null, inY: Vector3D | null, inZ: Vector3D | null): boolean {
    let success = false;

    if (!this.hitInitialised()) {
      VsdkError.warning("pointShadingFrame", "uninitialised hit structure");
      return false;
    }

    if (this.material !== null && this.material.getBsdf() !== null) {
      success = PhongBidirectionalScatteringDistributionFunction.bsdfShadingFrame(this, inX, inY, inZ);
    }

    if (!success && this.material !== null && this.material.getEdf() !== null) {
      success = PhongEmittanceDistributionFunction.edfShadingFrame(this, inX, inY, inZ);
    }

    if (!success && this.computeUv(this.uv) && this.patch !== null) {
      const x = inX !== null ? inX : new Vector3D();
      const y = inY !== null ? inY : new Vector3D();
      const z = inZ !== null ? inZ : new Vector3D();
      this.patch.interpolatedFrameAtUv(this.uv.u, this.uv.v, x, y, z);

      if (inX !== null) {
        inX.copy(x);
      }
      if (inY !== null) {
        inY.copy(y);
      }
      if (inZ !== null) {
        inZ.copy(z);
      }
      success = true;
    }

    return success;
  }

  public init(
    inPatch: Patch | null,
    inPoint: Vector3D | null,
    inGeometryNormal: Vector3D | null,
    inMaterial: Material | null
  ): boolean {
    this.flags = 0;
    this.patch = inPatch;
    if (inPatch !== null) {
      this.flags |= RayHitFlag.PATCH;
    }

    if (inPoint !== null) {
      this.point.copy(inPoint);
      this.flags |= RayHitFlag.POINT;
    }

    if (inGeometryNormal !== null) {
      this.geometricNormal.copy(inGeometryNormal);
      this.flags |= RayHitFlag.GEOMETRIC_NORMAL;
    }

    this.material = inMaterial;
    this.flags |= RayHitFlag.MATERIAL;
    this.flags |= RayHitFlag.DISTANCE;

    const localNormal = new Vector3D();
    localNormal.set(0.0, 0.0, 0.0);
    this.texCoord.copy(localNormal);
    this.shadingFrame.setX(localNormal);
    this.shadingFrame.setY(localNormal);
    this.shadingFrame.setZ(localNormal);
    this.uv.u = 0.0;
    this.uv.v = 0.0;

    return this.hitInitialised();
  }

  public getTexCoord(outTexCoord: Vector3D): boolean {
    if ((this.flags & RayHitFlag.TEXTURE_COORDINATE) !== 0) {
      outTexCoord.copy(this.texCoord);
      return true;
    }

    if (!this.computeUv(this.uv)) {
      return false;
    }

    if ((this.flags & RayHitFlag.PATCH) !== 0 && this.patch !== null) {
      this.texCoord = this.patch.textureCoordAtUv(this.uv.u, this.uv.v);
      outTexCoord.copy(this.texCoord);
      this.flags |= RayHitFlag.TEXTURE_COORDINATE;
      return true;
    }

    return false;
  }

  public shadingNormal(inNormal: Vector3D): boolean {
    if (((this.flags & RayHitFlag.SHADING_FRAME) !== 0) || ((this.flags & RayHitFlag.NORMAL) !== 0)) {
      inNormal.copy(this.shadingFrame.getZ());
      return true;
    }

    const localNormal = this.shadingFrame.getZ();
    if (!this.pointShadingFrame(null, null, localNormal)) {
      return false;
    }

    this.flags |= RayHitFlag.NORMAL;
    this.shadingFrame.setZ(localNormal);
    inNormal.copy(this.shadingFrame.getZ());
    return true;
  }

  public setShadingFrame(frame: CoordinateSystem): boolean;
  public setShadingFrame(inX: Vector3D, inY: Vector3D, inZ: Vector3D): void;
  public setShadingFrame(
    frameOrX: CoordinateSystem | Vector3D,
    inY?: Vector3D,
    inZ?: Vector3D
  ): boolean | void {
    if (frameOrX instanceof CoordinateSystem) {
      const frame = frameOrX;
      if ((this.flags & RayHitFlag.SHADING_FRAME) !== 0) {
        frame.setX(this.shadingFrame.getX());
        frame.setY(this.shadingFrame.getY());
        frame.setZ(this.shadingFrame.getZ());
        return true;
      }

      const shadingX = this.shadingFrame.getX();
      const shadingY = this.shadingFrame.getY();
      const shadingZ = this.shadingFrame.getZ();

      if (!this.pointShadingFrame(shadingX, shadingY, shadingZ)) {
        return false;
      }

      this.shadingFrame.setX(shadingX);
      this.shadingFrame.setY(shadingY);
      this.shadingFrame.setZ(shadingZ);
      this.flags |= RayHitFlag.SHADING_FRAME | RayHitFlag.NORMAL;

      frame.setX(this.shadingFrame.getX());
      frame.setY(this.shadingFrame.getY());
      frame.setZ(this.shadingFrame.getZ());
      return true;
    }

    this.shadingFrame.setX(frameOrX);
    this.shadingFrame.setY(inY as Vector3D);
    this.shadingFrame.setZ(inZ as Vector3D);
    return;
  }

  public getPatch(): Patch | null {
    return this.patch;
  }

  public setPatch(inPatch: Patch | null): void {
    this.patch = inPatch;
  }

  public getPoint(): Vector3D {
    return this.point;
  }

  public setPoint(position: Vector3D): void {
    this.point.copy(position);
  }

  public setGeometricNormal(inNormal: Vector3D): void {
    this.geometricNormal.copy(inNormal);
  }

  public setMaterial(inMaterial: Material | null): void {
    this.material = inMaterial;
  }

  public getUv(): Vector2Dd {
    return this.uv;
  }

  public setUv(inUv: Vector2Dd): void;
  public setUv(inU: number, inV: number): void;
  public setUv(inUvOrU: Vector2Dd | number, inV?: number): void {
    if (inUvOrU instanceof Vector2Dd) {
      this.uv.u = inUvOrU.u;
      this.uv.v = inUvOrU.v;
      return;
    }

    this.uv.u = inUvOrU;
    this.uv.v = inV as number;
  }

  public getFlags(): number {
    return this.flags;
  }

  public setFlags(inFlags: number): void {
    this.flags = inFlags;
  }

  public getNormal(): Vector3D {
    return this.shadingFrame.getZ();
  }

  public setNormal(n: Vector3D): void {
    this.shadingFrame.setZ(n);
  }

  public getShadingFrame(): CoordinateSystem {
    return this.shadingFrame;
  }

  public getMaterial(): Material | null {
    return this.material;
  }

  public getGeometricNormal(): Vector3D {
    return this.geometricNormal;
  }

  public copyFrom(other: RayHit | null): void {
    if (other === null) {
      return;
    }

    this.setPoint(other.getPoint());
    this.setPatch(other.getPatch());
    this.setGeometricNormal(other.getGeometricNormal());
    this.setMaterial(other.getMaterial());
    this.setUv(other.getUv());
    this.setFlags(other.getFlags());

    const frame = other.getShadingFrame();
    this.setShadingFrame(frame.getX(), frame.getY(), frame.getZ());
    const localTex = new Vector3D();
    if (other.getTexCoord(localTex)) {
      this.texCoord = localTex;
    }
  }
}
