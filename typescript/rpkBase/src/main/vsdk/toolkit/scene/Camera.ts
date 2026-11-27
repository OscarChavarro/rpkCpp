import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Matrix4x4d } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Matrix4x4d";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Plane } from "./Plane";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Double } from "vitral/dist/java/lang/Double";

export class Camera {
  public static readonly NUMBER_OF_VIEW_PLANES = 4;

  public eyePosition: Vector3Dd;
  public lookPosition: Vector3Dd;
  public upDirection: Vector3Dd;
  public viewDistance: number;
  public fieldOfVision: number;
  public horizontalFov: number;
  public verticalFov: number;
  public near: number;
  public far: number;
  public xSize: number;
  public ySize: number;
  public X: Vector3Dd;
  public Y: Vector3Dd;
  public Z: Vector3Dd;
  public background: ColorRgbMutable;
  public changed: number;
  public pixelWidth: number;
  public pixelHeight: number;
  public pixelWidthTangent: number;
  public pixelHeightTangent: number;
  public viewPlanes: Plane[];

  public constructor() {
    this.eyePosition = new Vector3Dd();
    this.lookPosition = new Vector3Dd();
    this.upDirection = new Vector3Dd();
    this.viewDistance = 0.0;
    this.fieldOfVision = 0.0;
    this.horizontalFov = 0.0;
    this.verticalFov = 0.0;
    this.near = 0.0;
    this.far = 0.0;
    this.xSize = 0;
    this.ySize = 0;
    this.X = new Vector3Dd();
    this.Y = new Vector3Dd();
    this.Z = new Vector3Dd();
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
    const minimum = new Vector3Dd(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
    const maximum = new Vector3Dd(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());
    let d = new Vector3Dd();

    this.far = -Double.MAX_VALUE;
    this.near = Double.MAX_VALUE;
    for (let i = 0; i <= 1; i++) {
      for (let j = 0; j <= 1; j++) {
        for (let k = 0; k <= 1; k++) {
          d = new Vector3Dd(i !== 0 ? maximum.x() : minimum.x(), j !== 0 ? maximum.y() : minimum.y(), k !== 0 ? maximum.z() : minimum.z());

          d = d.subtract(this.eyePosition);
          const z = d.dotProduct(this.Z);

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
    this.far = this.far + (0.02 * this.far);
    this.near = this.near - (0.02 * this.near);
    if (this.far < VSDK.EPSILON) {
      this.far = this.viewDistance;
    }
    if (this.near < VSDK.EPSILON) {
      this.near = this.viewDistance / 100.0;
    }
  }

  private computeClippingPlanes(): void {
    const x = this.pixelWidthTangent * this.viewDistance;
    const y = this.pixelHeightTangent * this.viewDistance;
    const vScreen = new Array<Vector3Dd>(4);
    for (let i = 0; i < 4; i++) {
      vScreen[i] = new Vector3Dd();
    }

    vScreen[0]! = this.lookPosition.add(this.X.multiply(x)).add(this.Y.multiply(-y));
    vScreen[1]! = this.lookPosition.add(this.X.multiply(x)).add(this.Y.multiply(y));
    vScreen[2]! = this.lookPosition.add(this.X.multiply(-x)).add(this.Y.multiply(y));
    vScreen[3]! = this.lookPosition.add(this.X.multiply(-x)).add(this.Y.multiply(-y));

    for (let i = 0; i < 4; i++) {
      this.viewPlanes[i]!.normal = vScreen[i]!.subtract(this.eyePosition).crossProduct(vScreen[(i + 1) % 4]!.subtract(this.eyePosition));
      this.viewPlanes[i]!.normal = this.viewPlanes[i]!.normal.normalized(Numeric.EPSILON);
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
    this.Z = this.Z.multiplyByInverse(this.viewDistance, Numeric.EPSILON);

    this.X = this.Z.crossProduct(this.upDirection);
    const n = this.X.length();
    if (n < VSDK.EPSILON) {
      Logger.reportMessage(this, Logger.ERROR, "SetCamera", "up-direction and viewing direction coincide");
      return null;
    }
    this.X = this.X.multiplyByInverse(n, Numeric.EPSILON);

    this.Y = this.Z.crossProduct(this.X);
    this.Y = this.Y.normalized(Numeric.EPSILON);

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

    this.near = Numeric.EPSILON;
    this.far = 2.0 * this.viewDistance;

    this.pixelWidthTangent = globalThis.Math.tan((this.horizontalFov * globalThis.Math.PI) / 180.0);
    this.pixelHeightTangent = globalThis.Math.tan((this.verticalFov * globalThis.Math.PI) / 180.0);

    this.pixelWidth = (2.0 * this.pixelWidthTangent) / this.xSize;
    this.pixelHeight = (2.0 * this.pixelHeightTangent) / this.ySize;

    this.computeClippingPlanes();
    return this;
  }

  public set(
    inEyePosition: Vector3Dd,
    inLoopPosition: Vector3Dd,
    inUpDirection: Vector3Dd,
    inFieldOfVision: number,
    inXSize: number,
    inYSize: number,
    inBackground: ColorRgbMutable
  ): void {
    this.eyePosition = new Vector3Dd(inEyePosition.x(), inEyePosition.y(), inEyePosition.z());
    this.lookPosition = new Vector3Dd(inLoopPosition.x(), inLoopPosition.y(), inLoopPosition.z());
    this.upDirection = new Vector3Dd(inUpDirection.x(), inUpDirection.y(), inUpDirection.z());
    this.fieldOfVision = inFieldOfVision;
    this.xSize = inXSize;
    this.ySize = inYSize;
    this.background = new ColorRgbMutable(inBackground.r, inBackground.g, inBackground.b);
    this.changed = 1;
    this.complete();
  }

  public setEyePosition(x: number, y: number, z: number): void {
    let newEyePosition = new Vector3Dd();
    newEyePosition = new Vector3Dd(x, y, z);
    this.set(newEyePosition, this.lookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setLookPosition(x: number, y: number, z: number): void {
    let newLookPosition = new Vector3Dd();
    newLookPosition = new Vector3Dd(x, y, z);
    this.set(this.eyePosition, newLookPosition, this.upDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setUpDirection(x: number, y: number, z: number): void {
    let newUpDirection = new Vector3Dd();
    newUpDirection = new Vector3Dd(x, y, z);
    this.set(this.eyePosition, this.lookPosition, newUpDirection, this.fieldOfVision, this.xSize, this.ySize, this.background);
  }

  public setFieldOfView(fieldOfView: number): void {
    this.set(this.eyePosition, this.lookPosition, this.upDirection, fieldOfView, this.xSize, this.ySize, this.background);
  }

  public static transformBoundingBox(
    sourceBoundingBox: BoundingBox,
    transform: Matrix4x4d,
    transformedBoundingBox: BoundingBox | null
  ): void {
    if (transformedBoundingBox === null) {
      return;
    }

    const corners = new Array<Vector3Dd>(8);
    for (let i = 0; i < 8; i++) {
      corners[i] = new Vector3Dd();
    }
    sourceBoundingBox.corners(corners);

    transformedBoundingBox.copyFrom(new BoundingBox());
    for (let i = 0; i < 8; i++) {
      // Rotation part only (the 3x3 block), as RenderPark always did
      corners[i] = transform.withoutTranslation().multiply(corners[i]!);
      transformedBoundingBox.enlargeToIncludePoint(corners[i]!);
    }

    const xDelta = transformedBoundingBox.dx() * Numeric.EPSILON;
    const yDelta = transformedBoundingBox.dy() * Numeric.EPSILON;
    const zDelta = transformedBoundingBox.dz() * Numeric.EPSILON;
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
  public static lookAtMatrix(eye: Vector3Dd, centre: Vector3Dd, up: Vector3Dd): Matrix4x4d {
    const Z = eye.subtract(centre).normalized(Numeric.EPSILON); // Z positions towards viewer
    const X = up.crossProduct(Z).normalized(Numeric.EPSILON); // X positions right
    const Y = Z.crossProduct(X); // Y positions up
    const orientation = [
      [X.x(), X.y(), X.z(), 0.0],
      [Y.x(), Y.y(), Y.z(), 0.0],
      [Z.x(), Z.y(), Z.z(), 0.0],
      [0.0, 0.0, 0.0, 1.0]
    ];
    // Translate the eye to the origin, then orient
    return new Matrix4x4d(orientation).multiply(new Matrix4x4d().translation(eye.multiply(-1.0)));
  }

  /**
  Perspective projection like gluPerspective, computed in double precision
  */
  public static perspectiveMatrix(fieldOfViewInRadians: number, aspect: number, near: number, far: number): Matrix4x4d {
    const fieldOfView = fieldOfViewInRadians;
    const aspectRatio = aspect;
    const nearPlane = near;
    const farPlane = far;
    const f = 1.0 / Math.tan(fieldOfView / 2.0);
    const projection = [
      [f / aspectRatio, 0.0, 0.0, 0.0],
      [0.0, f, 0.0, 0.0],
      [0.0, 0.0, (nearPlane + farPlane) / (nearPlane - farPlane), ((2 * farPlane) * nearPlane) / (nearPlane - farPlane)],
      [0.0, 0.0, -1.0, 0.0]
    ];
    return new Matrix4x4d(projection);
  }

  public static projectionMatrixFromBoundingBox(boundingBox: BoundingBox): Matrix4x4d {
    return new Matrix4x4d().orthogonalProjection(
      boundingBox.minX(),
      boundingBox.maxX(),
      boundingBox.minY(),
      boundingBox.maxY(),
      -boundingBox.maxZ(),
      -boundingBox.minZ()
    );
  }
}
