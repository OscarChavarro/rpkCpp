import { OutputStream } from "../../../../java/io/OutputStream";
import { String as JavaString } from "../../../../java/lang/String";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Matrix4x4 } from "../../common/linealAlgebra/Matrix4x4";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Camera } from "../../scene/Camera";

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
      outputStream.write(bytes, 0, bytes.length);
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

  private static transformModel(camera: Camera, modelRotationAxis: Vector3D, modelRotationAngle: number[]): Matrix4x4 {
    const upAxis = new Vector3D();
    upAxis.set(0.0, 1.0, 0.0);
    const cosA = camera.upDirection.dotProduct(upAxis);
    if (cosA < 1.0 - Numeric.EPSILON) {
      modelRotationAngle[0] = globalThis.Math.acos(cosA);
      modelRotationAxis.crossProduct(camera.upDirection, upAxis);
      modelRotationAxis.normalize(Numeric.EPSILON_FLOAT);
      return Matrix4x4.createRotationMatrix(modelRotationAngle[0], modelRotationAxis);
    }

    modelRotationAxis.set(0.0, 1.0, 0.0);
    modelRotationAngle[0] = 0.0;
    const identity = new Matrix4x4();
    return identity;
  }

  private static writeViewPoint(
    outputStream: OutputStream | null,
    modelTransform: Matrix4x4,
    camera: Camera,
    viewPointName: string
  ): void {
    const X = new Vector3D();
    const Y = new Vector3D();
    const Z = new Vector3D();
    const viewRotationAxis = new Vector3D();
    const eyePosition = new Vector3D();
    const viewRotationAngle = [0.0];

    X.scaledCopy(1.0, camera.X);
    Y.scaledCopy(-1.0, camera.Y);
    Z.scaledCopy(-1.0, camera.Z);

    modelTransform.transformPoint3D(X, X);
    modelTransform.transformPoint3D(Y, Y);
    modelTransform.transformPoint3D(Z, Z);

    const identity = new Matrix4x4();
    const viewTransform = identity;
    viewTransform.set3X3Matrix(
      X.x, Y.x, Z.x,
      X.y, Y.y, Z.y,
      X.z, Y.z, Z.z
    );
    viewTransform.recoverRotationParameters(viewRotationAngle, viewRotationAxis);

    modelTransform.transformPoint3D(camera.eyePosition, eyePosition);

    VrmlWriter.writeFormatted(
      outputStream,
      "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
      eyePosition.x,
      eyePosition.y,
      eyePosition.z,
      viewRotationAxis.x,
      viewRotationAxis.y,
      viewRotationAxis.z,
      viewRotationAngle[0],
      2.0 * camera.fieldOfVision * globalThis.Math.PI / 180.0,
      viewPointName
    );
  }

  private static writeViewPoints(camera: Camera, outputStream: OutputStream | null, modelTransform: Matrix4x4): void {
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
    const modelRotationAxis = new Vector3D();
    const modelRotationAngle = [0.0];

    VrmlWriter.writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");
    VrmlWriter.writeFormatted(
      outputStream,
      "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
      "Some nice model",
      VrmlWriter.RPK_HOME
    );
    VrmlWriter.writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    const modelTransform = VrmlWriter.transformModel(camera, modelRotationAxis, modelRotationAngle);
    VrmlWriter.writeViewPoints(camera, outputStream, modelTransform);

    VrmlWriter.writeFormatted(
      outputStream,
      "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
      modelRotationAxis.x,
      modelRotationAxis.y,
      modelRotationAxis.z,
      modelRotationAngle[0]
    );
    VrmlWriter.writeFormatted(outputStream, "\tsolid %s\n", renderOptions.backfaceCulling ? "TRUE" : "FALSE");
  }

  public static writeTrailer(outputStream: OutputStream | null): void {
    VrmlWriter.writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
  }
}
