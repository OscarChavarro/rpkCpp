import { OutputStream } from "vitral/dist/java/io/OutputStream";
import { String as JavaString } from "vitral/dist/java/lang/String";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Matrix4x4f } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Matrix4x4f";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Camera } from "../../scene/Camera";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class VrmlWriter {
  private static readonly MAXIMUM_CAMERA_STACK = 20;
  private static readonly RPK_HOME = "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/";
  private static readonly cameraStack: Camera[] = (() => {
    const stack = new Array<Camera>(VrmlWriter.MAXIMUM_CAMERA_STACK);
    for (let i = 0; i < stack.length; i++) {
      stack[i] = new Camera();
    }
    return stack;
  })();

  private constructor() {
  }

  private static formatToString(format: string | null, ...argumentsList: unknown[]): string {
    if (format === null) {
      return "";
    }
    return JavaString.vformat(format, argumentsList);
  }

  private static writeFormatted(outputStream: OutputStream | null, format: string | null, ...argumentsList: unknown[]): void {
    if (outputStream === null || format === null) {
      return;
    }

    let text = "";
    try {
      text = VrmlWriter.formatToString(format, ...argumentsList);
    }
    catch (_ignored) {
      text = "";
    }

    if (text.length <= 0) {
      return;
    }

    const bytes = Buffer.from(text, "utf8");
    try {
      outputStream.writeBytes(bytes, 0, bytes.length);
    }
    catch (_ignored) {
    }
  }

  private static nextSavedCamera(previous: Camera | null): Camera | null {
    if (previous === null) {
      return null;
    }

    let index = -1;
    for (let i = 0; i < VrmlWriter.cameraStack.length; i++) {
      if (VrmlWriter.cameraStack[i] === previous) {
        index = i;
        break;
      }
    }

    if (index <= 0) {
      return null;
    }
    return VrmlWriter.cameraStack[index - 1] ?? null;
  }

  /**
  @param modelRotationAxis one-element holder (C++ `Vector3Df *`)
  @param modelRotationAngle one-element holder (C++ `float *`)
  */
  private static transformModel(camera: Camera, modelRotationAxis: Vector3Df[], modelRotationAngle: number[]): Matrix4x4f {
    let upAxis = new Vector3Df();
    upAxis = new Vector3Df(0.0, 1.0, 0.0);
    const cosA = camera.upDirection.dotProduct(upAxis);
    if (cosA < 1.0 - VSDK.EPSILON) {
      modelRotationAngle[0] = Math.fround(globalThis.Math.acos(cosA));
      modelRotationAxis[0] = camera.upDirection.crossProduct(upAxis);
      modelRotationAxis[0] = modelRotationAxis[0].normalized(Numeric.EPSILON_FLOAT);
      return new Matrix4x4f().axisRotation(modelRotationAngle[0], modelRotationAxis[0]);
    }

    modelRotationAxis[0] = new Vector3Df(0.0, 1.0, 0.0);
    modelRotationAngle[0] = 0.0;
    return new Matrix4x4f();
  }

  /**
  Recovers the rotation axis and angle from the given rotation matrix.
  There is no check whether the transform really is a rotation.
  @param angle one-element holder (C++ `float *`)
  @param axis one-element holder (C++ `Vector3Df *`)
  */
  private static recoverRotationParameters(rotation: Matrix4x4f, angle: number[], axis: Vector3Df[]): void {
    const fr = Math.fround;
    const c = fr(fr(fr(fr(rotation.get(0, 0) + rotation.get(1, 1)) + rotation.get(2, 2)) - 1.0) * 0.5);
    if (c > 1.0 - VSDK.EPSILON) {
      angle[0] = 0.0;
      axis[0] = new Vector3Df(0.0, 0.0, 1.0);
    }
    else if (c < -1.0 + VSDK.EPSILON) {
      angle[0] = fr(globalThis.Math.PI);
      const x = fr(globalThis.Math.sqrt(fr(fr(rotation.get(0, 0) + 1.0) * 0.5)));
      let y = fr(globalThis.Math.sqrt(fr(fr(rotation.get(1, 1) + 1.0) * 0.5)));
      let z = fr(globalThis.Math.sqrt(fr(fr(rotation.get(2, 2) + 1.0) * 0.5)));

      // Assume x positive, determine sign of y and z
      if (rotation.get(1, 0) < 0.0) {
        y = -y;
      }
      if (rotation.get(2, 0) < 0.0) {
        z = -z;
      }
      axis[0] = new Vector3Df(x, y, z);
    }
    else {
      angle[0] = fr(globalThis.Math.acos(c));
      const s = fr(globalThis.Math.sqrt(fr(1.0 - fr(c * c))));
      const r = fr(1.0 / fr(2.0 * s));
      axis[0] = new Vector3Df(
        fr(fr(rotation.get(2, 1) - rotation.get(1, 2)) * r),
        fr(fr(rotation.get(0, 2) - rotation.get(2, 0)) * r),
        fr(fr(rotation.get(1, 0) - rotation.get(0, 1)) * r));
    }
  }

  private static writeViewPoint(
    outputStream: OutputStream | null,
    modelTransform: Matrix4x4f,
    camera: Camera,
    viewPointName: string
  ): void {
    let X = new Vector3Df();
    let Y = new Vector3Df();
    let Z = new Vector3Df();
    const viewRotationAxisHolder = [new Vector3Df()];
    const viewRotationAngle = [0.0];

    X = camera.X.multiply(1.0);
    Y = camera.Y.multiply(-1.0);
    Z = camera.Z.multiply(-1.0);

    // Apply model transform (its rotation part)
    const modelRotation = modelTransform.withoutTranslation();
    X = modelRotation.multiply(X);
    Y = modelRotation.multiply(Y);
    Z = modelRotation.multiply(Z);

    const orientation = [
      [X.x(), Y.x(), Z.x(), 0.0],
      [X.y(), Y.y(), Z.y(), 0.0],
      [X.z(), Y.z(), Z.z(), 0.0],
      [0.0, 0.0, 0.0, 1.0]
    ];
    VrmlWriter.recoverRotationParameters(new Matrix4x4f(orientation), viewRotationAngle, viewRotationAxisHolder);
    const viewRotationAxis = viewRotationAxisHolder[0]!;

    const eyePosition = modelRotation.multiply(camera.eyePosition);

    VrmlWriter.writeFormatted(
      outputStream,
      "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
      eyePosition.x(),
      eyePosition.y(),
      eyePosition.z(),
      viewRotationAxis.x(),
      viewRotationAxis.y(),
      viewRotationAxis.z(),
      viewRotationAngle[0],
      2.0 * camera.fieldOfVision * globalThis.Math.PI / 180.0,
      viewPointName
    );
  }

  private static writeViewPoints(camera: Camera, outputStream: OutputStream | null, modelTransform: Matrix4x4f): void {
    let localCamera: Camera | null = null;
    let count = 1;
    VrmlWriter.writeViewPoint(outputStream, modelTransform, camera, "ViewPoint 1");
    while ((localCamera = VrmlWriter.nextSavedCamera(localCamera)) !== null) {
      count++;
      const viewPointName = `ViewPoint ${count}`;
      VrmlWriter.writeViewPoint(outputStream, modelTransform, localCamera, viewPointName);
    }
  }

  public static writeHeader(camera: Camera, outputStream: OutputStream | null, renderOptions: RendererConfiguration): void {
    const modelRotationAxisHolder = [new Vector3Df()];
    const modelRotationAngle = [0.0];

    VrmlWriter.writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");
    VrmlWriter.writeFormatted(
      outputStream,
      "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
      "Some nice model",
      VrmlWriter.RPK_HOME
    );
    VrmlWriter.writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    const modelTransform = VrmlWriter.transformModel(camera, modelRotationAxisHolder, modelRotationAngle);
    const modelRotationAxis = modelRotationAxisHolder[0]!;
    VrmlWriter.writeViewPoints(camera, outputStream, modelTransform);

    VrmlWriter.writeFormatted(
      outputStream,
      "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
      modelRotationAxis.x(),
      modelRotationAxis.y(),
      modelRotationAxis.z(),
      modelRotationAngle[0]
    );
    VrmlWriter.writeFormatted(outputStream, "\tsolid %s\n", renderOptions.backfaceCulling ? "TRUE" : "FALSE");
  }

  public static writeTrailer(outputStream: OutputStream | null): void {
    VrmlWriter.writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
  }
}
