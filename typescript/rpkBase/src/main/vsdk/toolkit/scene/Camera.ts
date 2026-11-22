import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Matrix4x4f } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Matrix4x4f";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Plane } from "./Plane";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Float } from "vitral/dist/java/lang/Float";

export class Camera {
  public static readonly NUMBER_OF_VIEW_PLANES = 4;

  public eyePosition: Vector3Df;
  public lookPosition: Vector3Df;
  public upDirection: Vector3Df;
  public viewDistance: number;
  public fieldOfVision: number;
  public horizontalFov: number;
  public verticalFov: number;
  public near: number;
  public far: number;
  public xSize: number;
  public ySize: number;
  public X: Vector3Df;
  public Y: Vector3Df;
  public Z: Vector3Df;
  public background: ColorRgbMutable;
  public changed: number;
  public pixelWidth: number;
  public pixelHeight: number;
  public pixelWidthTangent: number;
  public pixelHeightTangent: number;
  public viewPlanes: Plane[];

  public constructor() {
    this.eyePosition = new Vector3Df();
    this.lookPosition = new Vector3Df();
    this.upDirection = new Vector3Df();
    this.viewDistance = 0.0;
    this.fieldOfVision = 0.0;
    this.horizontalFov = 0.0;
    this.verticalFov = 0.0;
    this.near = 0.0;
    this.far = 0.0;
    this.xSize = 0;
    this.ySize = 0;
    this.X = new Vector3Df();
    this.Y = new Vector3Df();
    this.Z = new Vector3Df();
    this.background = new ColorRgbMutable();
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

  /**
  Sets the near and far clipping distances from the scene bounds projected onto
  the viewing direction (Z axis), with 2% extra distance on both sides and
  fallbacks when the range is not in front of the eye. Every software and
  hardware renderer needs these before rendering, including the ID renders of
  the importance computations, which run before any image is drawn.
  */
  public fitClippingPlanes(sceneBounds: BoundingBox): void {
    const f = Math.fround;
    const minimum = new Vector3Df(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
    const maximum = new Vector3Df(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());
    let d = new Vector3Df();

    this.far = f(-Float.MAX_VALUE);
    this.near = f(Float.MAX_VALUE);
    for (let i = 0; i <= 1; i++) {
      for (let j = 0; j <= 1; j++) {
        for (let k = 0; k <= 1; k++) {
          d = new Vector3Df(i !== 0 ? maximum.x() : minimum.x(), j !== 0 ? maximum.y() : minimum.y(), k !== 0 ? maximum.z() : minimum.z());

          d = d.subtract(this.eyePosition);
          const z = f(d.dotProduct(this.Z));

          if (z > this.far) {
            this.far = z;
          }
          if (z < this.near) {
            this.near = z;
          }
        }
      }
    }

    // Take 2% extra distance for near as well as far clipping plane
    this.far = f(this.far + f(f(0.02) * this.far));
    this.near = f(this.near - f(f(0.02) * this.near));
    if (this.far < VSDK.EPSILON) {
      this.far = this.viewDistance;
    }
    if (this.near < VSDK.EPSILON) {
      this.near = f(this.viewDistance / 100.0);
    }
  }

  private computeClippingPlanes(): void {
    const x = this.pixelWidthTangent * this.viewDistance;
    const y = this.pixelHeightTangent * this.viewDistance;
    const vScreen = new Array<Vector3Df>(4);
    for (let i = 0; i < 4; i++) {
      vScreen[i] = new Vector3Df();
    }

    vScreen[0]! = this.lookPosition.add(this.X.multiply(x)).add(this.Y.multiply(-y));
    vScreen[1]! = this.lookPosition.add(this.X.multiply(x)).add(this.Y.multiply(y));
    vScreen[2]! = this.lookPosition.add(this.X.multiply(-x)).add(this.Y.multiply(y));
    vScreen[3]! = this.lookPosition.add(this.X.multiply(-x)).add(this.Y.multiply(-y));

    for (let i = 0; i < 4; i++) {
      this.viewPlanes[i]!.normal = vScreen[i]!.subtract(this.eyePosition).crossProduct(vScreen[(i + 1) % 4]!.subtract(this.eyePosition));
      this.viewPlanes[i]!.normal = this.viewPlanes[i]!.normal.normalized(Numeric.EPSILON_FLOAT);
      this.viewPlanes[i]!.d = -this.viewPlanes[i]!.normal.dotProduct(this.eyePosition);
    }
  }

  public complete(): Camera | null {
    this.Z = this.lookPosition.subtract(this.eyePosition);

    this.viewDistance = this.Z.length();
    if (this.viewDistance < VSDK.EPSILON) {
      Logger.reportMessage(this, Logger.ERROR, "SetCamera", "eye point and look-point coincide");
      return null;
    }
    this.Z = this.Z.multiplyByInverse(this.viewDistance, Numeric.EPSILON_FLOAT);

    this.X = this.Z.crossProduct(this.upDirection);
    const n = this.X.length();
    if (n < VSDK.EPSILON) {
      Logger.reportMessage(this, Logger.ERROR, "SetCamera", "up-direction and viewing direction coincide");
      return null;
    }
    this.X = this.X.multiplyByInverse(n, Numeric.EPSILON_FLOAT);

    this.Y = this.Z.crossProduct(this.X);
    this.Y = this.Y.normalized(Numeric.EPSILON_FLOAT);

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
    inEyePosition: Vector3Df,
    inLoopPosition: Vector3Df,
    inUpDirection: Vector3Df,
    inFieldOfVision: number,
    inXSize: number,
    inYSize: number,
    inBackground: ColorRgbMutable
  ): void {
    this.eyePosition = new Vector3Df(inEyePosition.x(), inEyePosition.y(), inEyePosition.z());
    this.lookPosition = new Vector3Df(inLoopPosition.x(), inLoopPosition.y(), inLoopPosition.z());
    this.upDirection = new Vector3Df(inUpDirection.x(), inUpDirection.y(), inUpDirection.z());
    this.fieldOfVision = inFieldOfVision;
    this.xSize = inXSize;
    this.ySize = inYSize;
    this.background = new ColorRgbMutable(inBackground.r, inBackground.g, inBackground.b);
    this.changed = 1;
    this.complete();
  }

  public setEyePosition(x: number, y: number, z: number): void {
    let newEyePosition = new Vector3Df();
    newEyePosition = new Vector3Df(x, y, z);
    this.set(newEyePosition, this.lookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setLookPosition(x: number, y: number, z: number): void {
    let newLookPosition = new Vector3Df();
    newLookPosition = new Vector3Df(x, y, z);
    this.set(this.eyePosition, newLookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setUpDirection(x: number, y: number, z: number): void {
    let newUpDirection = new Vector3Df();
    newUpDirection = new Vector3Df(x, y, z);
    this.set(this.eyePosition, this.lookPosition, newUpDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setFieldOfView(fieldOfView: number): void {
    this.set(this.eyePosition, this.lookPosition, this.upDirection, fieldOfView, this.xSize, this.ySize, this.background);
  }

  public static transformBoundingBox(
    sourceBoundingBox: BoundingBox,
    transform: Matrix4x4f,
    transformedBoundingBox: BoundingBox | null
  ): void {
    if (transformedBoundingBox === null) {
      return;
    }

    const corners = new Array<Vector3Df>(8);
    for (let i = 0; i < 8; i++) {
      corners[i] = new Vector3Df();
    }
    sourceBoundingBox.corners(corners);

    transformedBoundingBox.copyFrom(new BoundingBox());
    for (let i = 0; i < 8; i++) {
      // Rotation part only (the 3x3 block), as RenderPark always did
      corners[i] = transform.withoutTranslation().multiply(corners[i]!);
      transformedBoundingBox.enlargeToIncludePoint(corners[i]!);
    }

    const xDelta = transformedBoundingBox.dx() * Numeric.EPSILON_FLOAT;
    const yDelta = transformedBoundingBox.dy() * Numeric.EPSILON_FLOAT;
    const zDelta = transformedBoundingBox.dz() * Numeric.EPSILON_FLOAT;
    let minPoint = transformedBoundingBox.minPoint();
    let maxPoint = transformedBoundingBox.maxPoint();
    minPoint = minPoint.withX(minPoint.x() - xDelta);
    minPoint = minPoint.withY(minPoint.y() - yDelta);
    minPoint = minPoint.withZ(minPoint.z() - zDelta);
    maxPoint = maxPoint.withX(maxPoint.x() + xDelta);
    maxPoint = maxPoint.withY(maxPoint.y() + yDelta);
    maxPoint = maxPoint.withZ(maxPoint.z() + zDelta);

    const expandedBoundingBox = new BoundingBox();
    expandedBoundingBox.enlargeToIncludePoint(minPoint);
    expandedBoundingBox.enlargeToIncludePoint(maxPoint);
    transformedBoundingBox.copyFrom(expandedBoundingBox);
  }

  /**
  Transforms the eye point to the origin and rotates so that the centre lies
  on the negative Z axis and the up direction on the positive Y axis (Y up,
  X right, Z towards the viewer)
  */
  public static lookAtMatrix(eye: Vector3Df, centre: Vector3Df, up: Vector3Df): Matrix4x4f {
    const Z = eye.subtract(centre).normalized(Numeric.EPSILON_FLOAT); // Z positions towards viewer
    const X = up.crossProduct(Z).normalized(Numeric.EPSILON_FLOAT); // X positions right
    const Y = Z.crossProduct(X); // Y positions up
    const orientation = [
      [X.x(), X.y(), X.z(), 0.0],
      [Y.x(), Y.y(), Y.z(), 0.0],
      [Z.x(), Z.y(), Z.z(), 0.0],
      [0.0, 0.0, 0.0, 1.0]
    ];
    // Translate the eye to the origin, then orient
    return new Matrix4x4f(orientation).multiply(new Matrix4x4f().translation(eye.multiply(-1.0)));
  }

  /**
  Perspective projection like gluPerspective, computed in single precision
  */
  public static perspectiveMatrix(fieldOfViewInRadians: number, aspect: number, near: number, far: number): Matrix4x4f {
    const fr = Math.fround;
    const fieldOfView = fr(fieldOfViewInRadians);
    const aspectRatio = fr(aspect);
    const nearPlane = fr(near);
    const farPlane = fr(far);
    const f = fr(1.0 / fr(Math.tan(fr(fieldOfView / 2.0))));
    const projection = [
      [fr(f / aspectRatio), 0.0, 0.0, 0.0],
      [0.0, f, 0.0, 0.0],
      [0.0, 0.0, fr(fr(nearPlane + farPlane) / fr(nearPlane - farPlane)), fr(fr(fr(2 * farPlane) * nearPlane) / fr(nearPlane - farPlane))],
      [0.0, 0.0, -1.0, 0.0]
    ];
    return new Matrix4x4f(projection);
  }

  public static projectionMatrixFromBoundingBox(boundingBox: BoundingBox): Matrix4x4f {
    return new Matrix4x4f().orthogonalProjection(
      boundingBox.minX(),
      boundingBox.maxX(),
      boundingBox.minY(),
      boundingBox.maxY(),
      -boundingBox.maxZ(),
      -boundingBox.minZ()
    );
  }
}
