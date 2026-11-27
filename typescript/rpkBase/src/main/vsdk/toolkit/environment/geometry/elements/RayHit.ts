import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { CoordinateSystem } from "../../../common/linealAlgebra/CoordinateSystem";
import { Vector2Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Dd";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Material } from "../../../material/Material";
import { PhongBidirectionalScatteringDistributionFunction } from "../../../material/PhongBidirectionalScatteringDistributionFunction";
import { PhongEmittanceDistributionFunction } from "../../../material/PhongEmittanceDistributionFunction";
import { ShadingContext } from "../../../material/ShadingContext";
import { ShadingContextFlag } from "../../../material/ShadingContextFlag";
import { RayHitFlag } from "./RayHitFlag";
import type { Patch } from "./Patch";

export class RayHit {
  // Immutable constants shared by every hit (TypeScript only: they spare an
  // allocation per initialised hit)
  private static readonly ZERO_VECTOR = new Vector3Dd(0.0, 0.0, 0.0);
  private static readonly UNIT_Z_VECTOR = new Vector3Dd(0.0, 0.0, 1.0);

  private point: Vector3Dd;
  private patch: Patch | null;
  private texCoord: Vector3Dd;
  private geometricNormal: Vector3Dd;
  private material: Material | null;
  private shadingFrame: CoordinateSystem;
  private uv: Vector2Dd;
  private flags: number;

  public constructor() {
    this.point = new Vector3Dd();
    this.patch = null;
    this.texCoord = new Vector3Dd();
    this.geometricNormal = new Vector3Dd();
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
      inUv.x = this.uv.x;
      inUv.y = this.uv.y;
      return true;
    }

    if (((this.flags & RayHitFlag.PATCH) !== 0) && ((this.flags & RayHitFlag.POINT) !== 0) && this.patch !== null) {
      const u = [0.0];
      const v = [0.0];
      this.patch.uv(this.point, u, v);
      this.uv.x = u[0] ?? 0.0;
      this.uv.y = v[0] ?? 0.0;
      inUv.x = this.uv.x;
      inUv.y = this.uv.y;
      this.flags |= RayHitFlag.UV;
      return true;
    }

    return false;
  }

  /**
  @param inX, inY, inZ one-element holders (C++ `Vector3Dd *`), or null
  */
  private pointShadingFrame(inX: Vector3Dd[] | null, inY: Vector3Dd[] | null, inZ: Vector3Dd[] | null): boolean {
    let success = false;

    if (!this.hitInitialised()) {
      VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "pointShadingFrame", "uninitialised hit structure");
      return false;
    }

    if (this.material !== null && this.material.getBsdf() !== null) {
      const context = new ShadingContext(
        this.getPoint(),
        this.getGeometricNormal(),
        this.shadingFrame.getZ(),
        this.texCoord,
        this.uv,
        this.shadingFrame,
        this.material,
        0
      );
      success = PhongBidirectionalScatteringDistributionFunction.bsdfShadingFrame(context, inX, inY, inZ);
    }

    if (!success && this.material !== null && this.material.getEdf() !== null) {
      const context = new ShadingContext(
        this.getPoint(),
        this.getGeometricNormal(),
        this.shadingFrame.getZ(),
        this.texCoord,
        this.uv,
        this.shadingFrame,
        this.material,
        0
      );
      success = PhongEmittanceDistributionFunction.edfShadingFrame(context, inX, inY, inZ);
    }

    if (!success && this.computeUv(this.uv) && this.patch !== null) {
      const x = inX !== null ? inX : [new Vector3Dd()];
      const y = inY !== null ? inY : [new Vector3Dd()];
      const z = inZ !== null ? inZ : [new Vector3Dd()];
      this.patch.interpolatedFrameAtUv(this.uv.x, this.uv.y, x, y, z);
      success = true;
    }

    return success;
  }

  public init(
    inPatch: Patch | null,
    inPoint: Vector3Dd | null,
    inGeometryNormal: Vector3Dd | null,
    inMaterial: Material | null
  ): boolean {
    this.flags = 0;
    this.patch = inPatch;
    if (inPatch !== null) {
      this.flags |= RayHitFlag.PATCH;
    }

    if (inPoint !== null) {
      this.point = inPoint;
      this.flags |= RayHitFlag.POINT;
    }

    if (inGeometryNormal !== null) {
      this.geometricNormal = inGeometryNormal;
      this.flags |= RayHitFlag.GEOMETRIC_NORMAL;
    }

    this.material = inMaterial;
    this.flags |= RayHitFlag.MATERIAL;
    this.flags |= RayHitFlag.DISTANCE;

    const localNormal = RayHit.ZERO_VECTOR;
    this.texCoord = localNormal;
    this.shadingFrame.setX(localNormal);
    this.shadingFrame.setY(localNormal);
    this.shadingFrame.setZ(localNormal);
    this.uv.x = 0.0;
    this.uv.y = 0.0;

    return this.hitInitialised();
  }

  /**
  @param outTexCoord one-element holder (C++ `Vector3Dd *`)
  */
  public getTexCoord(outTexCoord: Vector3Dd[]): boolean {
    if ((this.flags & RayHitFlag.TEXTURE_COORDINATE) !== 0) {
      outTexCoord[0] = this.texCoord;
      return true;
    }

    if (!this.computeUv(this.uv)) {
      return false;
    }

    if ((this.flags & RayHitFlag.PATCH) !== 0 && this.patch !== null) {
      this.texCoord = this.patch.textureCoordAtUv(this.uv.x, this.uv.y);
      outTexCoord[0] = this.texCoord;
      this.flags |= RayHitFlag.TEXTURE_COORDINATE;
      return true;
    }

    return false;
  }

  /**
  @param inNormal one-element holder (C++ `Vector3Dd *`)
  */
  public shadingNormal(inNormal: Vector3Dd[]): boolean {
    if (((this.flags & RayHitFlag.SHADING_FRAME) !== 0) || ((this.flags & RayHitFlag.NORMAL) !== 0)) {
      inNormal[0] = this.shadingFrame.getZ();
      return true;
    }

    const localNormal = [this.shadingFrame.getZ()];
    if (!this.pointShadingFrame(null, null, localNormal)) {
      return false;
    }

    this.flags |= RayHitFlag.NORMAL;
    this.shadingFrame.setZ(localNormal[0]!);
    inNormal[0] = this.shadingFrame.getZ();
    return true;
  }

  public shadingContext(ok: boolean[] | null = null): ShadingContext {
    const normalHolder = [RayHit.ZERO_VECTOR];
    const texCoordHolder = [RayHit.ZERO_VECTOR];
    let localFlags = 0;
    let localOk = true;

    if (!this.shadingNormal(normalHolder)) {
      localOk = false;
      normalHolder[0] = RayHit.UNIT_Z_VECTOR;
    }
    else {
      localFlags |= ShadingContextFlag.SHCTX_NORMAL;
    }

    if (this.getTexCoord(texCoordHolder)) {
      localFlags |= ShadingContextFlag.SHCTX_TEXTURE_COORDINATE;
    }
    else {
      texCoordHolder[0] = RayHit.ZERO_VECTOR;
    }
    const normal = normalHolder[0]!;
    const texCoord = texCoordHolder[0]!;

    if (ok !== null && ok.length > 0) {
      ok[0] = localOk;
    }

    return new ShadingContext(
      this.point,
      this.geometricNormal,
      normal,
      texCoord,
      this.uv,
      this.shadingFrame,
      this.material,
      localFlags
    );
  }

  public setShadingFrame(frame: CoordinateSystem): boolean;
  public setShadingFrame(inX: Vector3Dd, inY: Vector3Dd, inZ: Vector3Dd): void;
  public setShadingFrame(
    frameOrX: CoordinateSystem | Vector3Dd,
    inY?: Vector3Dd,
    inZ?: Vector3Dd
  ): boolean | void {
    if (frameOrX instanceof CoordinateSystem) {
      const frame = frameOrX;
      if ((this.flags & RayHitFlag.SHADING_FRAME) !== 0) {
        frame.setX(this.shadingFrame.getX());
        frame.setY(this.shadingFrame.getY());
        frame.setZ(this.shadingFrame.getZ());
        return true;
      }

      const shadingX = [this.shadingFrame.getX()];
      const shadingY = [this.shadingFrame.getY()];
      const shadingZ = [this.shadingFrame.getZ()];

      if (!this.pointShadingFrame(shadingX, shadingY, shadingZ)) {
        return false;
      }

      this.shadingFrame.setX(shadingX[0]!);
      this.shadingFrame.setY(shadingY[0]!);
      this.shadingFrame.setZ(shadingZ[0]!);
      this.flags |= RayHitFlag.SHADING_FRAME | RayHitFlag.NORMAL;

      frame.setX(this.shadingFrame.getX());
      frame.setY(this.shadingFrame.getY());
      frame.setZ(this.shadingFrame.getZ());
      return true;
    }

    this.shadingFrame.setX(frameOrX);
    this.shadingFrame.setY(inY as Vector3Dd);
    this.shadingFrame.setZ(inZ as Vector3Dd);
    return;
  }

  public getPatch(): Patch | null {
    return this.patch;
  }

  public setPatch(inPatch: Patch | null): void {
    this.patch = inPatch;
  }

  public getPoint(): Vector3Dd {
    return this.point;
  }

  public setPoint(position: Vector3Dd): void {
    this.point = position;
  }

  public setGeometricNormal(inNormal: Vector3Dd): void {
    this.geometricNormal = inNormal;
  }

  public setMaterial(inMaterial: Material | null): void {
    this.material = inMaterial;
  }

  public getUv(): Vector2Dd {
    return this.uv;
  }

  public setUv(inUv: Vector2Dd): void;
  public setUv(inU: number, inV: number): void;
  /**
  Sets the (u,v) parameters of the hit and marks them as known. Everything
  derived from (u,v) and cached in the record, the texture coordinate and the
  interpolated shading normal and frame, is invalidated, so that a record
  reused for several samples on one patch evaluates each sample at its own
  (u,v) and not at the first one.
  */
  public setUv(inUvOrU: Vector2Dd | number, inV?: number): void {
    if (inUvOrU instanceof Vector2Dd) {
      this.setUv(inUvOrU.x, inUvOrU.y);
      return;
    }

    this.uv.x = inUvOrU;
    this.uv.y = inV as number;
    this.flags = (this.flags | RayHitFlag.UV)
      & ~(RayHitFlag.TEXTURE_COORDINATE | RayHitFlag.NORMAL | RayHitFlag.SHADING_FRAME);
  }

  public getFlags(): number {
    return this.flags;
  }

  public setFlags(inFlags: number): void {
    this.flags = inFlags;
  }

  public getNormal(): Vector3Dd {
    return this.shadingFrame.getZ();
  }

  public setNormal(n: Vector3Dd): void {
    this.shadingFrame.setZ(n);
  }

  public getShadingFrame(): CoordinateSystem {
    return this.shadingFrame;
  }

  public getMaterial(): Material | null {
    return this.material;
  }

  public getGeometricNormal(): Vector3Dd {
    return this.geometricNormal;
  }

  /**
  Plain member-wise copy, equivalent to the C++ port's structure assignment
  (*hitStore = hit). Fields not marked as filled in by the flags are copied
  as-is and remain meaningless, just as in the C++ version; readers must
  check the flags (or use the getTexCoord/shadingNormal accessors, which
  compute missing data on demand).
  */
  public copyFrom(other: RayHit | null): void {
    if (other === null) {
      return;
    }

    this.point = other.point;
    this.patch = other.patch;
    this.texCoord = other.texCoord;
    this.geometricNormal = other.geometricNormal;
    this.material = other.material;
    this.shadingFrame.setX(other.shadingFrame.getX());
    this.shadingFrame.setY(other.shadingFrame.getY());
    this.shadingFrame.setZ(other.shadingFrame.getZ());
    this.uv.x = other.uv.x;
    this.uv.y = other.uv.y;
    this.flags = other.flags;
  }
}
