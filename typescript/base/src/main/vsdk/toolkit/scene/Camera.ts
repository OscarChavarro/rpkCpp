import { ColorRgb } from "../common/color/ColorRgb";
import { Logger } from "../common/logging/Logger";
import { Matrix4x4 } from "../common/linealAlgebra/Matrix4x4";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Plane } from "./Plane";

export class Camera {
  public static readonly NUMBER_OF_VIEW_PLANES = 4;

  public eyePosition: Vector3D;
  public lookPosition: Vector3D;
  public upDirection: Vector3D;
  public viewDistance: number;
  public fieldOfVision: number;
  public horizontalFov: number;
  public verticalFov: number;
  public near: number;
  public far: number;
  public xSize: number;
  public ySize: number;
  public X: Vector3D;
  public Y: Vector3D;
  public Z: Vector3D;
  public background: ColorRgb;
  public changed: number;
  public pixelWidth: number;
  public pixelHeight: number;
  public pixelWidthTangent: number;
  public pixelHeightTangent: number;
  public viewPlanes: Plane[];

  public constructor() {
    this.eyePosition = new Vector3D();
    this.lookPosition = new Vector3D();
    this.upDirection = new Vector3D();
    this.viewDistance = 0.0;
    this.fieldOfVision = 0.0;
    this.horizontalFov = 0.0;
    this.verticalFov = 0.0;
    this.near = 0.0;
    this.far = 0.0;
    this.xSize = 0;
    this.ySize = 0;
    this.X = new Vector3D();
    this.Y = new Vector3D();
    this.Z = new Vector3D();
    this.background = new ColorRgb();
    this.changed = 0;
    this.pixelWidth = 0.0;
    this.pixelHeight = 0.0;
    this.pixelWidthTangent = 0.0;
    this.pixelHeightTangent = 0.0;
    this.viewPlanes = new Array<Plane>(Camera.NUMBER_OF_VIEW_PLANES);
    for (let i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++) {
      this.viewPlanes[i] = new Plane();
    }
  }

  private computeClippingPlanes(): void {
    const x = this.pixelWidthTangent * this.viewDistance;
    const y = this.pixelHeightTangent * this.viewDistance;
    const vScreen = new Array<Vector3D>(4);
    for (let i = 0; i < 4; i++) {
      vScreen[i] = new Vector3D();
    }

    vScreen[0]!.combine3(this.lookPosition, x, this.X, -y, this.Y);
    vScreen[1]!.combine3(this.lookPosition, x, this.X, y, this.Y);
    vScreen[2]!.combine3(this.lookPosition, -x, this.X, y, this.Y);
    vScreen[3]!.combine3(this.lookPosition, -x, this.X, -y, this.Y);

    for (let i = 0; i < 4; i++) {
      this.viewPlanes[i]!.normal.tripleCrossProduct(vScreen[(i + 1) % 4]!, this.eyePosition, vScreen[i]!);
      this.viewPlanes[i]!.normal.normalize(Numeric.EPSILON_FLOAT);
      this.viewPlanes[i]!.d = -this.viewPlanes[i]!.normal.dotProduct(this.eyePosition);
    }
  }

  public complete(): Camera | null {
    this.Z.subtraction(this.lookPosition, this.eyePosition);

    this.viewDistance = this.Z.norm();
    if (this.viewDistance < Numeric.EPSILON) {
      Logger.error("SetCamera", "eye point and look-point coincide");
      return null;
    }
    this.Z.inverseScaledCopy(this.viewDistance, this.Z, Numeric.EPSILON_FLOAT);

    this.X.crossProduct(this.Z, this.upDirection);
    const n = this.X.norm();
    if (n < Numeric.EPSILON) {
      Logger.error("SetCamera", "up-direction and viewing direction coincide");
      return null;
    }
    this.X.inverseScaledCopy(n, this.X, Numeric.EPSILON_FLOAT);

    this.Y.crossProduct(this.Z, this.X);
    this.Y.normalize(Numeric.EPSILON_FLOAT);

    if (this.xSize < this.ySize) {
      this.horizontalFov = this.fieldOfVision;
      this.verticalFov = globalThis.Math.atan(
        globalThis.Math.tan((this.fieldOfVision * globalThis.Math.PI) / 180.0) * this.ySize / this.xSize
      ) * 180.0 / globalThis.Math.PI;
    }
    else {
      this.verticalFov = this.fieldOfVision;
      this.horizontalFov = globalThis.Math.atan(
        globalThis.Math.tan((this.fieldOfVision * globalThis.Math.PI) / 180.0) * this.xSize / this.ySize
      ) * 180.0 / globalThis.Math.PI;
    }

    this.near = Numeric.EPSILON_FLOAT;
    this.far = 2.0 * this.viewDistance;

    this.pixelWidthTangent = globalThis.Math.tan((this.horizontalFov * globalThis.Math.PI) / 180.0);
    this.pixelHeightTangent = globalThis.Math.tan((this.verticalFov * globalThis.Math.PI) / 180.0);

    this.pixelWidth = (2.0 * this.pixelWidthTangent) / this.xSize;
    this.pixelHeight = (2.0 * this.pixelHeightTangent) / this.ySize;

    this.computeClippingPlanes();
    return this;
  }

  public set(
    inEyePosition: Vector3D,
    inLoopPosition: Vector3D,
    inUpDirection: Vector3D,
    inFieldOfVision: number,
    inXSize: number,
    inYSize: number,
    inBackground: ColorRgb
  ): void {
    this.eyePosition = new Vector3D(inEyePosition.x, inEyePosition.y, inEyePosition.z);
    this.lookPosition = new Vector3D(inLoopPosition.x, inLoopPosition.y, inLoopPosition.z);
    this.upDirection = new Vector3D(inUpDirection.x, inUpDirection.y, inUpDirection.z);
    this.fieldOfVision = inFieldOfVision;
    this.xSize = inXSize;
    this.ySize = inYSize;
    this.background = new ColorRgb(inBackground.r, inBackground.g, inBackground.b);
    this.changed = 1;
    this.complete();
  }

  public setEyePosition(x: number, y: number, z: number): void {
    const newEyePosition = new Vector3D();
    newEyePosition.set(x, y, z);
    this.set(newEyePosition, this.lookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setLookPosition(x: number, y: number, z: number): void {
    const newLookPosition = new Vector3D();
    newLookPosition.set(x, y, z);
    this.set(this.eyePosition, newLookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setUpDirection(x: number, y: number, z: number): void {
    const newUpDirection = new Vector3D();
    newUpDirection.set(x, y, z);
    this.set(this.eyePosition, this.lookPosition, newUpDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setFieldOfView(fieldOfView: number): void {
    this.set(this.eyePosition, this.lookPosition, this.upDirection, fieldOfView, this.xSize, this.ySize, this.background);
  }

  public static transformBoundingBox(
    sourceBoundingBox: BoundingBox,
    transform: Matrix4x4,
    transformedBoundingBox: BoundingBox | null
  ): void {
    if (transformedBoundingBox === null) {
      return;
    }

    const corners = new Array<Vector3D>(8);
    for (let i = 0; i < 8; i++) {
      corners[i] = new Vector3D();
    }
    sourceBoundingBox.corners(corners);

    transformedBoundingBox.copyFrom(new BoundingBox());
    for (let i = 0; i < 8; i++) {
      transform.transformPoint3D(corners[i]!, corners[i]!);
      transformedBoundingBox.enlargeToIncludePoint(corners[i]!);
    }

    const xDelta = transformedBoundingBox.dx() * Numeric.EPSILON_FLOAT;
    const yDelta = transformedBoundingBox.dy() * Numeric.EPSILON_FLOAT;
    const zDelta = transformedBoundingBox.dz() * Numeric.EPSILON_FLOAT;
    const minPoint = transformedBoundingBox.minPoint();
    const maxPoint = transformedBoundingBox.maxPoint();
    minPoint.x -= xDelta;
    minPoint.y -= yDelta;
    minPoint.z -= zDelta;
    maxPoint.x += xDelta;
    maxPoint.y += yDelta;
    maxPoint.z += zDelta;

    const expandedBoundingBox = new BoundingBox();
    expandedBoundingBox.enlargeToIncludePoint(minPoint);
    expandedBoundingBox.enlargeToIncludePoint(maxPoint);
    transformedBoundingBox.copyFrom(expandedBoundingBox);
  }

  public static projectionMatrixFromBoundingBox(boundingBox: BoundingBox): Matrix4x4 {
    return Matrix4x4.createOrthogonalViewMatrix(
      boundingBox.minX(),
      boundingBox.maxX(),
      boundingBox.minY(),
      boundingBox.maxY(),
      -boundingBox.maxZ(),
      -boundingBox.minZ()
    );
  }
}
